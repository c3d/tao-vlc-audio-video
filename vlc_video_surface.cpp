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
#include <string.h>
#ifdef Q_OS_WIN32
#include <malloc.h>
#endif
#include <QVector>
#ifdef Q_OS_WIN32
#include "vlc_audio_video.h"
#include <QProcess>
#endif

libvlc_instance_t *         VlcVideoSurface::vlc = NULL;
QStringList                 VlcVideoSurface::userOptions;
bool                        VlcVideoSurface::initFailed = false;
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
      state(VS_STOPPED), pevm(NULL), mevm(NULL),
      videoAvailable(false),
      GLcontext(QGLContext::currentContext()), loopMode(false)
{
    if (initFailed)
        return;

    if (!vlcInstance())
    {
        IFTRACE(video)
            debug() << "VLC initialization failed\n";
        lastError = "Failed to initialize libVLC.";
        return;
    }

    IFTRACE(video)
        debug() << "Creating media player\n";

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
    if (textureId)
        glDeleteTextures(1, &textureId);
    foreach (char *opt, mediaOptions)
        free(opt);
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
//   Resume playback after pause or stop
// ----------------------------------------------------------------------------
{
    if (state == VS_PAUSED)
        libvlc_media_player_set_pause(player, false);
    else if (state == VS_STOPPED)
        libvlc_media_player_play(player);
}


void VlcVideoSurface::stop()
// ----------------------------------------------------------------------------
//   Stop playback
// ----------------------------------------------------------------------------
{
    if (state == VS_STOPPED || state == VS_ERROR)
        return;
    libvlc_media_player_stop(player);
    setState(VS_STOPPED);
    videoAvailable = false;
}


QString VlcVideoSurface::stripOptions(QString &name)
// ----------------------------------------------------------------------------
//   Strip and return <options> when name is "<path or URL>##<options>"
// ----------------------------------------------------------------------------
{
    QString opt;
    int pos = name.indexOf("##");
    if (pos > 0)
    {
        opt = name.mid(pos + 2);
        name = name.left(pos);
    }
    return opt;
}


void VlcVideoSurface::addMediaOptions()
// ----------------------------------------------------------------------------
//   Add mediaOptions to current media
// ----------------------------------------------------------------------------
{
    foreach (char *opt, mediaOptions)
    {
        IFTRACE(video)
            debug() << "Adding media option: '" << opt << "'\n";
        libvlc_media_add_option(media, opt);
    }
}


libvlc_media_t *VlcVideoSurface::newMediaFromPathOrUrl(QString name)
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


void VlcVideoSurface::play(const QString &name)
// ----------------------------------------------------------------------------
//   Play a file or URL. No-op if 'name' is already playing. "" to stop.
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return;

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
        libvlc_media_release(media);
        media = NULL;
        if (name != "")
        {
            // Split file path/URL and options
            QString name2 = name;
            QString opts = stripOptions(name2);

            // Open file or URL
            media = newMediaFromPathOrUrl(name2);
            if (!media)
                return;

            // Save options
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

            addMediaOptions();
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
    v->setState(VS_PLAYING);
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
    libvlc_video_set_callbacks(player, lockFrame, NULL, displayFrame, this);
    libvlc_video_set_format_callbacks(player, videoFormat, NULL);
    libvlc_media_player_set_media(player, media);
    libvlc_media_player_play(player);
}


void VlcVideoSurface::mute(bool mute)
// ----------------------------------------------------------------------------
//   Mute/unmute
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return;
    libvlc_audio_set_mute(player, mute);
}


float VlcVideoSurface::volume()
// ----------------------------------------------------------------------------
//   Return current volume level (0.0 <= volume <= 1.0)
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return 0.0;
    return libvlc_audio_get_volume(player) * 0.01;
}


float VlcVideoSurface::position()
// ----------------------------------------------------------------------------
//   Return current position
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return 0.0;
    return libvlc_media_player_get_position(player);
}


float VlcVideoSurface::time()
// ----------------------------------------------------------------------------
//   Return current time in seconds
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return 0.0;
    return libvlc_media_player_get_time(player) * 0.001;
}


float VlcVideoSurface::length()
// ----------------------------------------------------------------------------
//   Return length for current media
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return 0.0;
    return libvlc_media_player_get_length(player) * 0.001;
}


float VlcVideoSurface::rate()
// ----------------------------------------------------------------------------
//   Return play rate for current media
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return 1.0;
    return libvlc_media_player_get_rate(player);
}


bool VlcVideoSurface::playing()
// ----------------------------------------------------------------------------
//   Return true if media is currently playing
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return false;
    return state == VS_PLAYING && libvlc_media_player_is_playing(player);
}


bool VlcVideoSurface::paused()
// ----------------------------------------------------------------------------
//   Return true if the surface is paused
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return false;
    return state == VS_PAUSED;
}


bool VlcVideoSurface::done()
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
    if (!vlc)
        return;
    if (vol < 0) vol = 0;
    if (vol > 1) vol = 1;
    libvlc_audio_set_volume(player, int(vol * 100));
}


void VlcVideoSurface::setPosition(float pos)
// ----------------------------------------------------------------------------
//   Skip to position pos (0.0 <= pos <= 1.0)
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return;
    libvlc_media_player_set_position(player, pos);
}


void VlcVideoSurface::setTime(float t)
// ----------------------------------------------------------------------------
//   Skip to the given time
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return;
    libvlc_media_player_set_time(player, libvlc_time_t(t * 1000));
}


void VlcVideoSurface::setRate(float rate)
// ----------------------------------------------------------------------------
//   Set play rate for the current media
// ----------------------------------------------------------------------------
{
    if (!vlc)
        return;
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
    if (!vlc)
        return 0;

    GLuint tex = 0;

    switch (state)
    {
    case VS_PLAYING:
    case VS_PAUSED:
    case VS_PLAY_ENDED:
        if (videoAvailable)
        {
            mutex.lock();
            if (updated)
            {
                checkGLContext();
                glBindTexture(GL_TEXTURE_2D, textureId);

                GLenum format = GL_RGBA, type = GL_UNSIGNED_BYTE;
#ifdef Q_OS_MACX
                if (image.chroma == UYVY /* mirrored */)
                {
                    format = GL_YCBCR_422_APPLE;
                    type = GL_UNSIGNED_SHORT_8_8_APPLE;
                }
#endif
                glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, format, type,
                             image.ptr);
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

        // Tracing options
        IFTRACE(vlc)
        {
            argv.append("--extraintf=logger");
            argv.append("--verbose=2");
        }
        else
        {
            argv.append("-q");
        }

        // User options
        QVector<const char *> user_opts;
        foreach (QString opt, userOptions)
        {
            const char * copt = strdup(opt.toUtf8().constData());
            user_opts.append(copt);
            argv.append(copt);
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
        foreach (const char *opt, user_opts)
            free((void*)opt);
        if (!vlc)
        {
            initFailed = true;
            return NULL;
        }

        IFTRACE(video)
        {
            sdebug() << "libLVC version: " << libvlc_get_version() << "\n";
            sdebug() << "libLVC changeset: " << libvlc_get_changeset() << "\n";
            sdebug() << "libLVC compiler: " << libvlc_get_compiler() << "\n";
        }
    }
    return vlc;
}


bool VlcVideoSurface::vlcInit(QStringList options)
// ----------------------------------------------------------------------------
//   Initialize VLC, possibly with additional options
// ----------------------------------------------------------------------------
{
    userOptions = options;
    return (vlcInstance() != NULL);
}


void VlcVideoSurface::deleteVlcInstance()
// ----------------------------------------------------------------------------
//   Destroy VLC instance and any static stuff, ready for new creation
// ----------------------------------------------------------------------------
{
    initFailed = false;
    userOptions.clear();
    if (!vlc)
        return;
    IFTRACE(video)
        sdebug() << "Deleting VLC instance\n";
    libvlc_release(vlc);
    vlc = NULL;
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


unsigned VlcVideoSurface::videoFormat(void **opaque, char *chroma,
                                      unsigned *width, unsigned *height,
                                      unsigned *pitches,
                                      unsigned *lines)
// ----------------------------------------------------------------------------
//   Receive video format info from libVLC
// ----------------------------------------------------------------------------
{
    Q_UNUSED(opaque);

    VlcVideoSurface *v = (VlcVideoSurface *)*opaque;
    IFTRACE(video)
        v->debug() << "Native video format:"
                   << " resolution " << *width << "x" << *height
                   << " chroma " << chroma << "\n";

    if (v->w == 0 && v->h == 0)
    {
        v->w = *width;
        v->h = *height;
        IFTRACE(video)
            v->debug() << "Setting texture size to "
                       << v->w << "x" << v->h << "\n";
    }
    else
    {
        *width  = v->w;
        *height = v->h;
        IFTRACE(video)
            v->debug() << "Requesting libVLC scaling to texture size: "
                       << v->w << "x" << v->h << "\n";
    }

    const char * newchroma;

#if defined(Q_OS_MACX)
    newchroma = "UYVY";
    v->image.chroma = UYVY;
    pitches[0] = pitches[1] = pitches[2] = v->w * 2;
    lines  [0] = lines  [1] = lines  [2] = v->h;
    v->image.size = v->w * v->h * 2;
#else
    newchroma = "RV32";
    v->image.chroma = RV32;
    pitches[0] = pitches[1] = pitches[2] = v->w * 4;
    lines  [0] = lines  [1] = lines  [2] = v->h;
    v->image.chroma = RV32;
    v->image.size = v->w * v->h * 4;
#endif

    IFTRACE(video)
        v->debug() << "Requesting " << newchroma << " chroma\n";
    strcpy(chroma, newchroma);

    return 1;
}


void * VlcVideoSurface::lockFrame(void *obj, void **plane)
// ----------------------------------------------------------------------------
//   Allocate video memory
// ----------------------------------------------------------------------------
{
    VlcVideoSurface *v = (VlcVideoSurface *)obj;
    Q_ASSERT(v->image.size);

 #ifdef Q_OS_WIN32
     *plane = __mingw_aligned_malloc(v->image.size, 32);
     if (!*plane)
         throw std::bad_alloc();
 #else
     if (posix_memalign(plane, 32, v->image.size))
         throw std::bad_alloc();
 #endif

    return *plane;
}


void freeFrame(void *picture)
// ----------------------------------------------------------------------------
//   Release video memory
// ----------------------------------------------------------------------------
{
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

    if (v->state != VS_PLAYING && v->state != VS_PAUSED &&
        v->state != VS_STOPPED)
        v->setState(VS_PLAYING);

#if defined(Q_OS_MACX)
    if (v->image.chroma == UYVY)
    {
        // Hack: here, image is upside-down. To flip it use a QImage with a
        // 16bpp format.
        QImage image((const uchar *)picture, v->w, v->h, QImage::Format_RGB16);
        QImage converted = image.mirrored();
        freeFrame(picture);
        v->mutex.lock();
        v->image.converted = converted;
        v->image.ptr = v->image.converted.bits();
    }
    else
#endif
    {
        QImage image((const uchar *)picture, v->w, v->h, QImage::Format_RGB32);
        QImage converted = QGLWidget::convertToGLFormat(image);
        freeFrame(picture);
        v->mutex.lock();
        v->image.converted = converted;
        v->image.ptr = v->image.converted.bits();
    }
    v->updated = true;
    v->mutex.unlock();
    v->videoAvailable = true;
}
