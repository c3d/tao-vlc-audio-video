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
#include <QVector>
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
    : vlc(VlcAudioVideo::vlcInstance()), player(NULL), media(NULL),
      state(VS_STOPPED), pevm(NULL), loopMode(false)
{
    if (!vlc)
    {
        IFTRACE(video)
            debug() << "VLC initialization failed\n";
        lastError = "Failed to initialize libVLC.";
        return;
    }

    IFTRACE(video)
        debug() << "Creating media player to play " << +mediaNameAndOptions << "\n";

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
        options = opts.split(" ");
        foreach (QString opt, options)
        {
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
        libvlc_media_player_set_pause(player, false);
        return;
    }
    if (state != VS_STOPPED)
        return;

    IFTRACE2(fileload, video)
        debug() << "Play: " << +mediaName << "\n";
    setState(VS_STARTING);

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


void VlcVideoBase::playerPlaying(const struct libvlc_event_t *, void *obj)
// ----------------------------------------------------------------------------
//   Change state to notify that media started playing and info may be read
// ----------------------------------------------------------------------------
{
    VlcVideoBase *v = (VlcVideoBase *)obj;
    v->setState(VS_PLAYING);
}


void VlcVideoBase::playerEndReached(const struct libvlc_event_t *, void *obj)
// ----------------------------------------------------------------------------
//   Change state when player reaches end of media
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
//   Save error
// ----------------------------------------------------------------------------
{
    VlcVideoBase *v = (VlcVideoBase *)obj;
    const char *err = libvlc_errmsg();
    if (v->lastError != "")
        v->lastError += "\n";
    v->lastError += QString(err);
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
    return libvlc_media_player_get_time(player) * 0.001;
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
    return libvlc_media_player_get_rate(player);
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
    libvlc_audio_set_volume(player, int(vol * 100));
}


void VlcVideoBase::setPosition(float pos)
// ----------------------------------------------------------------------------
//   Skip to position pos (0.0 <= pos <= 1.0)
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return;
    libvlc_media_player_set_position(player, pos);
}


void VlcVideoBase::setTime(float t)
// ----------------------------------------------------------------------------
//   Skip to the given time
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return;
    libvlc_media_player_set_time(player, libvlc_time_t(t * 1000));
}


void VlcVideoBase::setRate(float rate)
// ----------------------------------------------------------------------------
//   Set play rate for the current media
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return;
    libvlc_media_player_set_rate(player, rate);
}


void VlcVideoBase::setLoop(bool on)
// ----------------------------------------------------------------------------
//   Set loop mode for the current media
// ----------------------------------------------------------------------------
{
    loopMode = on;
}



std::ostream & VlcVideoBase::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[VlcVideoBase " << (void*)this << "] ";
    return std::cerr;
}
