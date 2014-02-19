// ****************************************************************************
//  vlc_video_surface.cpp                                          Tao project
// ****************************************************************************
//
//   File Description:
//
//    Play audio and/or video using libvlc. Video is available as an OpenGL
//    texture.
//
//
//
//
//
//
//
// ****************************************************************************
// (C) 2011 Taodyne SAS <contact@taodyne.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
// ****************************************************************************

#include "vlc_audio_video.h"
#include "vlc_video_base.h"
#include "base.h"  // IFTRACE()
#include <vlc/libvlc_events.h>
#include <vlc/libvlc_media_list.h>
#include <string.h>
#include <QMutexLocker>
#include <QVector>
#include <QTime>
#ifdef Q_OS_WIN32
#include <QProcess>
#endif



#ifdef Q_OS_WIN32
inline QString operator +(std::string s)
// ----------------------------------------------------------------------------
//   Convert std::string to QString
// ----------------------------------------------------------------------------
{
    return QString::fromUtf8(s.data(), s.length());
}
#endif

inline std::string operator +(QString s)
// ----------------------------------------------------------------------------
//   Convert QString to std::string
// ----------------------------------------------------------------------------
{
    return std::string(s.toUtf8().constData());
}


VlcVideoBase::VlcVideoBase(QString mediaNameAndOptions)
// ----------------------------------------------------------------------------
//   Initialize a VLC media player to render a video
// ----------------------------------------------------------------------------
    : lastTime(-1.0), lastRate(1.0), frameTime(0),
      offline(false),
      vlc(VlcAudioVideo::vlcInstance()), player(NULL), media(NULL),
      state(VS_STOPPED), mevm(NULL), pevm(NULL), loopMode(false)
{
    if (!vlc)
    {
        IFTRACE(video)
            debug() << "VLC initialization failed\n";
        lastError = "Failed to initialize libVLC.";
        return;
    }

    IFTRACE(video)
        debug() << "Creating media player to play "
                << +mediaNameAndOptions << "\n";

    player = libvlc_media_player_new(vlc);
    pevm = libvlc_media_player_event_manager(player);
    libvlc_event_attach(pevm,
                        libvlc_MediaPlayerEncounteredError, playerError,
                        this);
    libvlc_event_attach(pevm,
                        libvlc_MediaPlayerPlaying, playerPlaying,
                        this);
    libvlc_event_attach(pevm,
                        libvlc_MediaPlayerEndReached, playerEndReached,
                        this);

    // Save path/URL and options
    this->mediaName = mediaNameAndOptions;
    QString opts = VlcAudioVideo::stripOptions(this->mediaName);
    QStringList options;
    if (!opts.isEmpty())
    {
        options = opts.split("##");
        if (options.length() == 1)
            options = opts.split(" ");
        foreach (QString opt, options)
        {
            if (opt.isEmpty())
                continue;
            char *o = strdup((+opt).c_str());
            mediaOptions.append(o);
        }
    }

}


VlcVideoBase::~VlcVideoBase()
// ----------------------------------------------------------------------------
//   Delete video player
// ----------------------------------------------------------------------------
{
    IFTRACE(video)
        debug() << "Deleting media player and media\n";

    if (player)
    {
        AsyncSetVolume::discard(player);
        libvlc_media_player_stop(player);
        libvlc_media_player_release(player);
    }
    if (media)
        libvlc_media_release(media);
    foreach (char *opt, mediaOptions)
        free(opt);
}


void VlcVideoBase::pause()
// ----------------------------------------------------------------------------
//   Pause playback
// ----------------------------------------------------------------------------
{
    if (state == VS_STOPPED || state == VS_PAUSED || state == VS_ERROR)
        return;
    libvlc_media_player_set_pause(player, true);
    setState(VS_PAUSED);
}


void VlcVideoBase::stop()
// ----------------------------------------------------------------------------
//   Stop playback
// ----------------------------------------------------------------------------
{
    if (state == VS_STOPPED || state == VS_ERROR)
        return;
    libvlc_media_player_stop(player);
    setState(VS_STOPPED);
}


void VlcVideoBase::next_frame()
// ----------------------------------------------------------------------------
//   Decode next frame of a paused video
// ----------------------------------------------------------------------------
{
    if (state != VS_PAUSED)
        return;
    libvlc_media_player_next_frame(player);
}


libvlc_media_t *VlcVideoBase::newMediaFromPathOrUrl(QString name)
// ----------------------------------------------------------------------------
//   Create media instance from path or URL.
// ----------------------------------------------------------------------------
{
    libvlc_media_t *media = NULL;

    if (name.contains("://"))
        media = libvlc_media_new_location(vlc, name.toUtf8().constData());
    else
        media = libvlc_media_new_path(vlc, name.toUtf8().constData());

    if (!media)
    {
        lastError = "Can't open " + name;
        IFTRACE(video)
            debug() << "Can't open media\n";
        setState(VS_ERROR);
    }

    return media;
}


void VlcVideoBase::play()
// ----------------------------------------------------------------------------
//   Start playback, or resume playback if paused
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return;

    if (state == VS_PAUSED)
    {
        setState(VS_PLAYING); // Or playerPlaying() would pause again
        libvlc_media_player_set_pause(player, false);
        return;
    }
    if (state != VS_STOPPED)
        return;

    IFTRACE2(fileload, video)
        debug() << "Play: " << +mediaName << "\n";

    // Open file or URL
    media = newMediaFromPathOrUrl(mediaName);
    if (!media)
        return;

    // Add options if any
    foreach (char *opt, mediaOptions)
    {
        IFTRACE(video)
            debug() << "Adding media option: '" << opt << "'\n";
        libvlc_media_add_option(media, opt);
    }

    startPlayback();
}


void VlcVideoBase::setState(State state)
// ----------------------------------------------------------------------------
//   Set FSM state
// ----------------------------------------------------------------------------
{
    IFTRACE(video)
        debug() << "New state: " << stateName(state) << "\n";
    this->state = state;
}


void VlcVideoBase::getMediaSubItems()
// ----------------------------------------------------------------------------
//   After playback when media is a playlist: get subitem(s)
// ----------------------------------------------------------------------------
{
    IFTRACE(video)
        debug() << "Getting media subitems\n";
   libvlc_media_list_t *mlist = libvlc_media_subitems(media);
   if (!mlist)
   {
       setState(VS_ERROR);
       return;
   }

   libvlc_media_list_lock(mlist);
   IFTRACE(video)
   {
       int count = libvlc_media_list_count(mlist);
       debug() << count << " subitem(s) found, selecting first one\n";
   }
   libvlc_media_release(media);
   media = libvlc_media_list_item_at_index(mlist, 0);
   libvlc_media_list_unlock(mlist);
   libvlc_media_list_release(mlist);

   libvlc_media_player_stop(player);

   setState(VS_SUBITEM_READY);
}


void VlcVideoBase::startPlayback()
// ----------------------------------------------------------------------------
//   Configure output format and start playback
// ----------------------------------------------------------------------------
{
    mevm = libvlc_media_event_manager(media);
    libvlc_event_attach(mevm, libvlc_MediaSubItemAdded, mediaSubItemAdded, this);

    libvlc_media_player_set_media(player, media);
    libvlc_media_player_play(player);

    setState(VS_STARTING);
}


void VlcVideoBase::exec()
// ----------------------------------------------------------------------------
//   Run state machine in main thread
// ----------------------------------------------------------------------------
{
    switch (state)
    {
    case VS_ALL_SUBITEMS_RECEIVED:
        getMediaSubItems();
        if (state == VS_SUBITEM_READY)
            startPlayback();
        break;

    case VS_PLAY_ENDED:
        if (loopMode)
        {
            IFTRACE(video)
                debug() << "Loop mode: restarting playback\n";
            startPlayback();
        }
        break;

    default:
        break;
    }
}


void VlcVideoBase::playerPlaying(const struct libvlc_event_t *, void *obj)
// ----------------------------------------------------------------------------
//   Forward 'playing' event to object
// ----------------------------------------------------------------------------
{
    VlcVideoBase *v = (VlcVideoBase *)obj;

    switch (v->state)
    {
    case VS_PAUSED:
        // If pause() was called before the first playerPlaying() notification
        // (#2653 movie immediately followed by movie_pause), the LibVLC call
        // to set_pause was ignored. Do it again.
        libvlc_media_player_set_pause(v->player, true);
        break;
    case VS_PLAYING:
        break;
    default:
        v->setState(VS_PLAYING);
    }
}


void VlcVideoBase::playerEndReached(const struct libvlc_event_t *, void *obj)
// ----------------------------------------------------------------------------
//   Forward 'end reached' event to object
// ----------------------------------------------------------------------------
{
    VlcVideoBase *v = (VlcVideoBase *)obj;
    switch (v->state)
    {
    case VS_PLAYING:
        v->setState(VS_PLAY_ENDED);
        break;
    case VS_WAITING_FOR_SUBITEMS:
        v->setState(VS_ALL_SUBITEMS_RECEIVED);
        break;
    default:
        break;
    }
}


void VlcVideoBase::playerError(const struct libvlc_event_t *, void *obj)
// ----------------------------------------------------------------------------
//   Forward 'error' event to object
// ----------------------------------------------------------------------------
{
    VlcVideoBase *v = (VlcVideoBase *)obj;
    const char *err = libvlc_errmsg();
    if (v->lastError != "")
        v->lastError += "\n";
    v->lastError += QString(err);
}


void VlcVideoBase::mediaSubItemAdded(const struct libvlc_event_t *,
                                     void *obj)
// ----------------------------------------------------------------------------
//   Forward 'media sub-item added' event to object
// ----------------------------------------------------------------------------
{
    VlcVideoBase *v = (VlcVideoBase *)obj;
    if (v->state != VS_WAITING_FOR_SUBITEMS)
        v->setState(VS_WAITING_FOR_SUBITEMS);
}


void VlcVideoBase::mute(bool mute)
// ----------------------------------------------------------------------------
//   Mute/unmute
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return;
    libvlc_audio_set_mute(player, mute);
}


float VlcVideoBase::volume()
// ----------------------------------------------------------------------------
//   Return current volume level (0.0 <= volume <= 1.0)
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return 0.0;
    return libvlc_audio_get_volume(player) * 0.01;
}


float VlcVideoBase::position()
// ----------------------------------------------------------------------------
//   Return current position
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return 0.0;
    return libvlc_media_player_get_position(player);
}


float VlcVideoBase::time()
// ----------------------------------------------------------------------------
//   Return current time in seconds
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return 0.0;
    double vlcTime = libvlc_media_player_get_time(player) * 0.001;

    // If VLC gives us a new time, take that
    if (lastTime != vlcTime)
        lastTime = frameTime = vlcTime;
        
    // Otherwise, compute the time from the sum of frames
    vlcTime = frameTime;
    return vlcTime;
}


float VlcVideoBase::length()
// ----------------------------------------------------------------------------
//   Return length for current media
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return 0.0;
    return libvlc_media_player_get_length(player) * 0.001;
}


float VlcVideoBase::rate()
// ----------------------------------------------------------------------------
//   Return play rate for current media
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return 1.0;
    double vlcRate = libvlc_media_player_get_rate(player);
    if (!offline)
        lastRate = vlcRate;
    return vlcRate;
}


bool VlcVideoBase::playing()
// ----------------------------------------------------------------------------
//   Return true if media is currently playing
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return false;
    return state == VS_PLAYING && libvlc_media_player_is_playing(player);
}


bool VlcVideoBase::paused()
// ----------------------------------------------------------------------------
//   Return true if the surface is paused
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return false;
    return state == VS_PAUSED;
}


bool VlcVideoBase::done()
// ----------------------------------------------------------------------------
//   Return true if the surface is done playing its contents
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return true;
    if (state == VS_PLAYING && !libvlc_media_player_is_playing(player))
        setState(VS_PLAY_ENDED);
    return state==VS_PLAY_ENDED || state==VS_ERROR;
}


bool VlcVideoBase::loop()
// ----------------------------------------------------------------------------
//   Return true if playback restarts automatically when media reaches end
// ----------------------------------------------------------------------------
{
    return loopMode;
}


void VlcVideoBase::setVolume(float vol)
// ----------------------------------------------------------------------------
//   Set volume (0.0 <= vol <= 1.0)
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return;
    if (vol < 0) vol = 0;
    if (vol > 1) vol = 1;
    AsyncSetVolume::libvlc_audio_set_volume(player, int(vol * 100));
}


void VlcVideoBase::setPosition(float pos)
// ----------------------------------------------------------------------------
//   Skip to position pos (0.0 <= pos <= 1.0)
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return;
    libvlc_media_player_set_position(player, pos);
    if (!offline)
        lastTime = libvlc_media_player_get_time(player) * 0.001;
}


void VlcVideoBase::setTime(float t)
// ----------------------------------------------------------------------------
//   Skip to the given time
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return;
    libvlc_media_player_set_time(player, libvlc_time_t(t * 1000));
    if (!offline)
        lastTime = libvlc_media_player_get_time(player) * 0.001;
}


void VlcVideoBase::setRate(float rate)
// ----------------------------------------------------------------------------
//   Set play rate for the current media
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return;
    libvlc_media_player_set_rate(player, rate);
    if (!offline)
        lastRate = rate;
}


void VlcVideoBase::setLoop(bool on)
// ----------------------------------------------------------------------------
//   Set loop mode for the current media
// ----------------------------------------------------------------------------
{
    if (loopMode != on)
    {
        IFTRACE(video)
            debug() << "Setting loop mode: " << on << "\n";
        loopMode = on;
    }
}


std::ostream & VlcVideoBase::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[VlcVideoBase " << (void*)this << "] ";
    return std::cerr;
}



// ============================================================================
//
//   Setting LibVLC volume asynchronously
//
// ============================================================================

AsyncSetVolume * AsyncSetVolume::inst = NULL;


AsyncSetVolume * AsyncSetVolume::instance()
// ----------------------------------------------------------------------------
//   Instance of the singleton
// ----------------------------------------------------------------------------
{
    if (!inst)
    {
        inst = new AsyncSetVolume;
        inst->moveToThread(inst);
        inst->start();
    }
    return inst;
}


void AsyncSetVolume::setVolume(libvlc_media_player_t *player, int volume)
// ----------------------------------------------------------------------------
//   Queue volume change request
// ----------------------------------------------------------------------------
{
    QMutexLocker locker(&mutex);
    if (!pendingPlayers.contains(player))
        pendingPlayers.append(player);
    volumes[player] = volume;
    cond.wakeOne();
}


void AsyncSetVolume::stopAndWait()
// ----------------------------------------------------------------------------
//   Stop the thread and wait for it to terminate
// ----------------------------------------------------------------------------
{
    mutex.lock();
    done = true;
    cond.wakeOne();
    mutex.unlock();
    wait();
}


void AsyncSetVolume::discardPending(libvlc_media_player_t *player)
// ----------------------------------------------------------------------------
//  Remove player from the list of pending requests
// ----------------------------------------------------------------------------
{
    QMutexLocker locker(&mutex);
    if (pendingPlayers.contains(player))
    {
        pendingPlayers.removeOne(player);
        volumes.remove(player);
    }
}


void AsyncSetVolume::run()
// ----------------------------------------------------------------------------
//   Main loop run by the thread
// ----------------------------------------------------------------------------
{
    QMutexLocker locker(&mutex);

    for(;;)
    {
        while (pendingPlayers.isEmpty() && !done)
            cond.wait(&mutex);

        if (done)
            return;

        Q_ASSERT(!pendingPlayers.isEmpty());
        libvlc_media_player_t * player = pendingPlayers.takeFirst();
        Q_ASSERT(!pendingPlayers.contains(player));
        Q_ASSERT(volumes.contains(player));
        int volume = volumes[player];
        volumes.remove(player);

        libvlc_media_player_retain(player);
        mutex.unlock();
        ::libvlc_audio_set_volume(player, volume);
        mutex.lock();
        libvlc_media_player_release(player);
    }
}
