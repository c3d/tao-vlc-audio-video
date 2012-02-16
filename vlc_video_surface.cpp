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

#include "vlc_video_surface.h"
#include "base.h"  // IFTRACE()
#include <vlc/libvlc_events.h>
#include <vlc/libvlc_media_list.h>
#ifdef Q_OS_WIN32
#include <malloc.h>
#endif
#include <QStringList>
#include <QVector>
#ifdef Q_OS_WIN32
#include "vlc_audio_video.h"
#include <QProcess>
#endif

libvlc_instance_t *         VlcVideoSurface::vlc = NULL;
VlcVideoSurface::VlcCleanup VlcVideoSurface::cleanup;


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


VlcVideoSurface::VlcVideoSurface(unsigned int w, unsigned int h)
// ----------------------------------------------------------------------------
//   Initialize a VLC media player to render a video
// ----------------------------------------------------------------------------
    : w(w), h(h), player(NULL), media(NULL), updated(false), textureId(0),
      state(VS_STOPPED), pevm(NULL), mevm(NULL), needResolution(true),
      descriptionMode((w == 0 || h == 0)),
      GLcontext(QGLContext::currentContext()), loopMode(false)
{
    // If w == 0 or h == 0, we start by a query phase (play in 'description'
    // mode). Then when we know the media has at least one stream, we ask for
    // the video resolution and update w and h accordingly (to default values
    // if we can't determine the native resolution)

    IFTRACE(video)
    {
        debug() << "Creating media player\n";
        debug() << "Description mode is "
                << (char*)(descriptionMode ? "true" : "false") << "\n";
    }
    player = libvlc_media_player_new(vlcInstance());
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
    genTexture();
}


VlcVideoSurface::~VlcVideoSurface()
// ----------------------------------------------------------------------------
//   Delete video player
// ----------------------------------------------------------------------------
{
    setState(VS_STOPPED);

    IFTRACE(video)
        debug() << "Deleting media player, media and texture\n";

    if (player)
    {
        libvlc_media_player_stop(player);
        libvlc_media_player_release(player);
    }
    if (media)
        libvlc_media_release(media);
    glDeleteTextures(1, &textureId);
}


void VlcVideoSurface::pause()
// ----------------------------------------------------------------------------
//   Pause playback
// ----------------------------------------------------------------------------
{
    if (state == VS_STOPPED || state == VS_PAUSED || state == VS_ERROR)
        return;
    libvlc_media_player_set_pause(player, true);
    setState(VS_PAUSED);
}


void VlcVideoSurface::play()
// ----------------------------------------------------------------------------
//   Resume playback
// ----------------------------------------------------------------------------
{
    if (state != VS_PAUSED)
        return;
    libvlc_media_player_set_pause(player, false);
}


void VlcVideoSurface::stop()
// ----------------------------------------------------------------------------
//   Stop playback
// ----------------------------------------------------------------------------
{
    if (state == VS_STOPPED || state == VS_ERROR)
        return;
    libvlc_media_release(media);
    libvlc_media_player_stop(player);
    media = NULL;
    setState(VS_STOPPED);
}


void VlcVideoSurface::play(const QString &name)
// ----------------------------------------------------------------------------
//   Play a file or URL. No-op if 'name' is already playing. "" to stop.
// ----------------------------------------------------------------------------
{
    if (name != mediaName)
    {
        IFTRACE2(fileload, video)
        {
            std::string neww = +name;
            if (neww == "")
                neww = "\"\"";
            debug() << "Play: " << neww << "\n";
        }

        mediaName = name;
        stop();
        if (name != "")
        {
            // Open file or URL
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
                return;
            }

            if (descriptionMode)
            {
                w = h = 1;
                if (isVlc1_1())
                    libvlc_media_add_option(media, "sout=#description:dummy");
            }
            startPlayback();
        }
    }
}


void VlcVideoSurface::setState(State state)
// ----------------------------------------------------------------------------
//   Set FSM state
// ----------------------------------------------------------------------------
{
    IFTRACE(video)
        debug() << "New state: " << stateName(state) << "\n";
    this->state = state;
}


void VlcVideoSurface::getMediaInfo()
// ----------------------------------------------------------------------------
//   Get video resolution
// ----------------------------------------------------------------------------
{
    // FIXME timeout
//    needResolution = false;
    libvlc_media_track_info_t * ti = NULL;
    int streams = libvlc_media_get_tracks_info(media, &ti);
    if (!streams)
    {
        IFTRACE(video)
            debug() << "No media stream detected\n";
        return;
    }
    needResolution = false;
    bool has_audio = false, has_video = false;
    unsigned w = 0, h = 0;
    for (int i = 0; i < streams; i++)
    {
        libvlc_media_track_info_t info = ti[i];
        switch (info.i_type)
        {
        case libvlc_track_video:
            if (w == 0 && h == 0)
            {
                // Read resolution of first video stream found
                w = info.u.video.i_width;
                h = info.u.video.i_height;
            }
            has_video = true;
            break;
        case libvlc_track_audio:
            has_audio = true;
            break;
        default:
            break;
        }
    }

    if (has_audio || has_video)
    {
        IFTRACE(video)
        {
            const char * vid = has_video ? "" : "no ";
            debug() << "Media has " << vid << "video\n";
            const char * aud = has_audio ? "" : "no ";
            debug() << "Media has " << aud << "audio\n";
            if (has_video)
                debug() << "Detected video resolution is "
                        << w << "x" << h << "\n";
        }
        if (has_video)
        {
            if (w == 0 || h == 0)
            {
                int nw = 1280, nh = 720;
                debug() << "WARNING: Invalid video resolution "
                        << w << "x" << h
                        << ". Using defaults: "
                        << nw << "x" << nh
                        << ".\n";
                w = nw;
                h = nh;
            }
        }
    }
    else
    {
        IFTRACE(video)
            debug() << "Found no audio and no video\n";
        setState(VS_ERROR);
    }
    if (descriptionMode)
    {
        // We have determined a video resolution. Restart playback with the
        // new values.
        IFTRACE(video)
            debug() << "Restarting playback with new resolution\n";

        needResolution = false;
        stop();
        // TODO factor
        if (mediaName.contains("://"))
            media = libvlc_media_new_location(vlc, mediaName.toUtf8().constData());
        else
            media = libvlc_media_new_path(vlc, mediaName.toUtf8().constData());
        this->w = w;
        this->h = h;
        startPlayback();
        IFTRACE(video)
            debug() << "Leaving description mode\n";
        descriptionMode = false;

    }
    free(ti);
}


void VlcVideoSurface::getMediaSubItems()
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

   setState(VS_SUBITEM_READY);
}


void VlcVideoSurface::playerPlaying(const struct libvlc_event_t *, void *obj)
// ----------------------------------------------------------------------------
//   Change state to notify that media started playing and info may be read
// ----------------------------------------------------------------------------
{
    VlcVideoSurface *v = (VlcVideoSurface *)obj;
    v->setState(VS_PLAY_STARTED);
}


void VlcVideoSurface::playerEndReached(const struct libvlc_event_t *, void *obj)
// ----------------------------------------------------------------------------
//   Change state when player reaches end of media
// ----------------------------------------------------------------------------
{
    VlcVideoSurface *v = (VlcVideoSurface *)obj;
    switch (v->state)
    {
    case VS_PLAYING:
    case VS_PLAY_STARTED:
        v->setState(VS_PLAY_ENDED);
        break;
    case VS_WAITING_FOR_SUBITEMS:
        v->setState(VS_ALL_SUBITEMS_RECEIVED);
        break;
    default:
        break;
    }
}


void VlcVideoSurface::playerError(const struct libvlc_event_t *, void *obj)
// ----------------------------------------------------------------------------
//   Save error
// ----------------------------------------------------------------------------
{
    VlcVideoSurface *v = (VlcVideoSurface *)obj;
    const char *err = libvlc_errmsg();
    if (v->lastError != "")
        v->lastError += "\n";
    v->lastError += QString(err);
}


void VlcVideoSurface::mediaSubItemAdded(const struct libvlc_event_t *,
                                        void *obj)
// ----------------------------------------------------------------------------
//   Change state when a media sub-item is found
// ----------------------------------------------------------------------------
{
    VlcVideoSurface *v = (VlcVideoSurface *)obj;
    if (v->state != VS_WAITING_FOR_SUBITEMS)
        v->setState(VS_WAITING_FOR_SUBITEMS);
}


void VlcVideoSurface::startPlayback()
// ----------------------------------------------------------------------------
//   Configure output format and start playback
// ----------------------------------------------------------------------------
{
    mevm = libvlc_media_event_manager(media);
    libvlc_event_attach(mevm, libvlc_MediaSubItemAdded, mediaSubItemAdded, this);

    libvlc_media_player_stop(player);
    libvlc_video_set_format(player, "RV32", w, h, w*4);
    libvlc_video_set_callbacks(player, lockFrame, unlockFrame,
                               displayFrame, this);
    libvlc_media_player_set_media(player, media);
    libvlc_media_player_play(player);
}


void VlcVideoSurface::mute(bool mute)
// ----------------------------------------------------------------------------
//   Mute/unmute
// ----------------------------------------------------------------------------
{
    libvlc_audio_set_mute(player, mute);
}


float VlcVideoSurface::volume()
// ----------------------------------------------------------------------------
//   Return current volume level (0.0 <= volume <= 1.0)
// ----------------------------------------------------------------------------
{
    return libvlc_audio_get_volume(player) * 0.01;
}


float VlcVideoSurface::position()
// ----------------------------------------------------------------------------
//   Return current position
// ----------------------------------------------------------------------------
{
    return libvlc_media_player_get_position(player);
}


float VlcVideoSurface::time()
// ----------------------------------------------------------------------------
//   Return current time in seconds
// ----------------------------------------------------------------------------
{
    return libvlc_media_player_get_time(player) * 0.001;
}


float VlcVideoSurface::length()
// ----------------------------------------------------------------------------
//   Return length for current media
// ----------------------------------------------------------------------------
{
    return libvlc_media_player_get_length(player) * 0.001;
}


float VlcVideoSurface::rate()
// ----------------------------------------------------------------------------
//   Return play rate for current media
// ----------------------------------------------------------------------------
{
    return libvlc_media_player_get_rate(player);
}


bool VlcVideoSurface::playing()
// ----------------------------------------------------------------------------
//   Return true if media is currently playing
// ----------------------------------------------------------------------------
{
    return state == VS_PLAYING && libvlc_media_player_is_playing(player);
}


bool VlcVideoSurface::paused()
// ----------------------------------------------------------------------------
//   Return true if the surface is paused
// ----------------------------------------------------------------------------
{
    return state == VS_PAUSED;
}


bool VlcVideoSurface::done()
// ----------------------------------------------------------------------------
//   Return true if the surface is done playing its contents
// ----------------------------------------------------------------------------
{
    if (state == VS_PLAYING && !libvlc_media_player_is_playing(player))
        setState(VS_PLAY_ENDED);
    return state==VS_PLAY_ENDED || state==VS_ERROR;
}


bool VlcVideoSurface::loop()
// ----------------------------------------------------------------------------
//   Return true if playback restarts automatically when media reaches end
// ----------------------------------------------------------------------------
{
    return loopMode;
}


void VlcVideoSurface::setVolume(float vol)
// ----------------------------------------------------------------------------
//   Set volume (0.0 <= vol <= 1.0)
// ----------------------------------------------------------------------------
{
    if (vol < 0) vol = 0;
    if (vol > 1) vol = 1;
    libvlc_audio_set_volume(player, int(vol * 100));
}


void VlcVideoSurface::setPosition(float pos)
// ----------------------------------------------------------------------------
//   Skip to position pos (0.0 <= pos <= 1.0)
// ----------------------------------------------------------------------------
{
    libvlc_media_player_set_position(player, pos);
}


void VlcVideoSurface::setTime(float t)
// ----------------------------------------------------------------------------
//   Skip to the given time
// ----------------------------------------------------------------------------
{
    libvlc_media_player_set_time(player, libvlc_time_t(t * 1000));
}


void VlcVideoSurface::setRate(float rate)
// ----------------------------------------------------------------------------
//   Set play rate for the current media
// ----------------------------------------------------------------------------
{
    libvlc_media_player_set_rate(player, rate);
}


void VlcVideoSurface::setLoop(bool on)
// ----------------------------------------------------------------------------
//   Set loop mode for the current media
// ----------------------------------------------------------------------------
{
    loopMode = on;
}


GLuint VlcVideoSurface::texture()
// ----------------------------------------------------------------------------
//   Update texture with current frame and return texture ID
// ----------------------------------------------------------------------------
{
    GLuint tex = 0;

    switch (state)
    {
    case VS_PLAY_STARTED:
        if (needResolution)
            getMediaInfo();
        break;
    case VS_PLAYING:
    case VS_PAUSED:
    case VS_PLAY_ENDED:
        if (!descriptionMode)
        {
            mutex.lock();
            if (updated)
            {
                checkGLContext();
                glBindTexture(GL_TEXTURE_2D, textureId);
                glTexImage2D(GL_TEXTURE_2D, 0, 3,
                             image.width(), image.height(), 0, GL_RGBA,
                             GL_UNSIGNED_BYTE, image.bits());
                updated = false;
            }
            mutex.unlock();
            tex = textureId;
            if (state == VS_PLAY_ENDED && loopMode)
            {
                IFTRACE(video)
                    debug() << "Loop mode: restarting playback\n";
                startPlayback();
            }
        }
        break;

    case VS_ALL_SUBITEMS_RECEIVED:
        getMediaSubItems();
        if (state == VS_SUBITEM_READY)
            startPlayback();
        break;

    default:
        break;
    }

    return tex;
}


void VlcVideoSurface::checkGLContext()
// ----------------------------------------------------------------------------
//   Detect change in current GL context
// ----------------------------------------------------------------------------
{
    const QGLContext * current = QGLContext::currentContext();
    if (current != GLcontext)
    {
        IFTRACE(video)
            debug() << "GL context changed\n";
        genTexture();
        GLcontext = current;
    }
}


void VlcVideoSurface::genTexture()
// ----------------------------------------------------------------------------
//   Create GL texture to render to
// ----------------------------------------------------------------------------
{
    glGenTextures(1, &textureId);
    IFTRACE(video)
        debug() << "Will render to texture #" << textureId << "\n";
}


libvlc_instance_t * VlcVideoSurface::vlcInstance()
// ----------------------------------------------------------------------------
//   Return/create the VLC instance
// ----------------------------------------------------------------------------
{
    if (!vlc)
    {
        QVector<const char *> argv;
        argv.append("--no-video-title-show");
        IFTRACE(vlc)
        {
            argv.append("--extraintf=logger");
            argv.append("--verbose=2");
        }
        else
        {
            argv.append("-q");
        }

        IFTRACE(video)
        {
            sdebug() << "Initializing VLC instance with parameters:\n";
            for (int i = 0; i < argv.size(); i++)
                sdebug() << "  " << argv[i] << "\n";
        }

#ifdef Q_OS_WIN32
        // #1555 Windows: crash when loading vlc_audio_video module for the
        // first time
        // libvlc_new() takes care of updating plugins.dat if needed, but it
        // seems that it corrupts the current process doing so.
        QString cg(+VideoSurface::modulePath + "/lib/vlc-cache-gen.exe");
        QStringList args("plugins");
        IFTRACE(video)
            sdebug() << "Running: '" << +cg << " " << +args.join(" ")
                     << "'...\n";
        QProcess p;
        p.start(cg, args);
        bool ok = false;
        if (p.waitForStarted() && p.waitForFinished())
            ok = true;
        const char *status = ok ? "done" : "error";
        IFTRACE(video)
            sdebug() << "...vlc-cache-gen " << status << "\n";
#endif

        vlc = libvlc_new(argv.size(), argv.data());

        IFTRACE(video)
        {
            sdebug() << "libLVC version: " << libvlc_get_version() << "\n";
            sdebug() << "libLVC changeset: " << libvlc_get_changeset() << "\n";
            sdebug() << "libLVC compiler: " << libvlc_get_compiler() << "\n";
        }

        (void)isVlc1_1();
    }
    return vlc;
}


bool VlcVideoSurface::isVlc1_1()
// ----------------------------------------------------------------------------
//   Return true if libvlc is version 1.1.x
// ----------------------------------------------------------------------------
{
    static bool is_1_1 = false, done = false;
    if (!done)
    {
        QString version = QString(libvlc_get_version());
        if (version.startsWith("1.1"))
            is_1_1 = true;
        IFTRACE(video)
            sdebug() << "VLC 1.1.x " << (char *)(is_1_1 ? "" : "NOT ")
                     << "detected\n";
        done = true;
    }
    return is_1_1;
}


std::ostream & VlcVideoSurface::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[VlcVideoSurface " << (void*)this << "] ";
    return std::cerr;
}


std::ostream & VlcVideoSurface::sdebug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[VlcVideoSurface] ";
    return std::cerr;
}


void * VlcVideoSurface::lockFrame(void *obj, void **plane)
// ----------------------------------------------------------------------------
//   Allocate video memory
// ----------------------------------------------------------------------------
{
    VlcVideoSurface *v = (VlcVideoSurface *)obj;
    if (v->w == 0 || v->h == 0)
        std::cerr << "FIXME zero size\n";

    size_t size = v->w * v->h * 4;
    if (!size)
        size = 1;
 #ifdef Q_OS_WIN32
     *plane = __mingw_aligned_malloc(size, 32);
     if (!*plane)
         throw std::bad_alloc();
 #else
     if (posix_memalign(plane, 32, size))
         throw std::bad_alloc();
 #endif

    return *plane;
}


void VlcVideoSurface::unlockFrame(void *obj, void *picture, void *const *plane)
// ----------------------------------------------------------------------------
//   Release video memory
// ----------------------------------------------------------------------------
{
    Q_UNUSED(obj);
    Q_UNUSED(plane);

#ifdef Q_OS_WIN32
    __mingw_aligned_free(picture);
#else
    free(picture);
#endif
}


void VlcVideoSurface::displayFrame(void *obj, void *picture)
// ----------------------------------------------------------------------------
//   Copy frame to texture
// ----------------------------------------------------------------------------
{
    VlcVideoSurface *v = (VlcVideoSurface *)obj;
    Q_ASSERT(v->w && v->h && "Invalid video size");

    QImage image((const uchar *)picture, v->w, v->h, QImage::Format_RGB32);
    QImage converted = QGLWidget::convertToGLFormat(image);
    // REVISIT
    if (v->descriptionMode)
    {
        v->setState(VS_PLAY_STARTED);
    }
    else
    {
        if (v->state != VS_PLAYING && v->state != VS_PAUSED)
            v->setState(VS_PLAYING);
    }
    v->mutex.lock();
    v->image = converted;
    v->updated = true;
    v->mutex.unlock();
}
