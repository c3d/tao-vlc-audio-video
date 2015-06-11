// ****************************************************************************
//  vlc_audio_video.cpp                                            Tao project
// ****************************************************************************
//
//   File Description:
//
//    Audio/video decoding and playback for Tao3D, based on VLC.
//
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

#include "tao/graphic_state.h"
#include "tao/tao_gl.h"
#include "vlc_audio_video.h"
#include "vlc_video_surface.h"
#include <vlc_video_fullscreen.h>
#include "vlc_preferences.h"
#include "action.h"
#include "errors.h"
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QStringList>
#include <QVector>
#include <QTime>
#ifdef Q_OS_WIN32
#include <QProcess>
#endif


inline QString operator +(std::string s)
// ----------------------------------------------------------------------------
//   UTF-8 conversion from std::string to QString
// ----------------------------------------------------------------------------
{
    return QString::fromUtf8(s.data(), s.length());
}


inline std::string operator +(QString s)
// ----------------------------------------------------------------------------
//   UTF-8 conversion from QString to std::string
// ----------------------------------------------------------------------------
{
    return std::string(s.toUtf8().constData());
}


using namespace XL;

const Tao::ModuleApi * VlcAudioVideo::tao = NULL;
VlcAudioVideo::video_map VlcAudioVideo::videos;
#ifdef Q_OS_WIN32
text VlcAudioVideo::modulePath;
#endif
libvlc_instance_t *         VlcAudioVideo::vlc = NULL;
QStringList                 VlcAudioVideo::userOptions;
QStringList                 VlcAudioVideo::lastUserOptions;
bool                        VlcAudioVideo::initFailed = false;
VlcAudioVideo::VlcCleanup   VlcAudioVideo::cleanup;


std::ostream & VlcAudioVideo::sdebug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[VlcAudioVideo] ";
    return std::cerr;
}


VlcVideoBase *VlcAudioVideo::surface(text name)
// ----------------------------------------------------------------------------
//   Return the video surface associated with a given name or NULL
// ----------------------------------------------------------------------------
{
    video_map::iterator found = videos.find(name);
    if (found != videos.end())
        return (*found).second;
    return NULL;
}


QList<VlcVideoBase *> VlcAudioVideo::surfaces(text expr)
// ----------------------------------------------------------------------------
//   Return the videos that match expr ("<name>" or "re:<regexp>")
// ----------------------------------------------------------------------------
{
    QList<VlcVideoBase *> ret;
    QString qexpr(+expr);
    if (qexpr.startsWith("re:"))
    {
        QRegExp re(qexpr.mid(3));
        for (video_map::iterator v = videos.begin(); v != videos.end(); ++v)
        {
            VlcVideoBase *s = (*v).second;
            if (re.indexIn(s->url()) != -1)
                ret.append(s);
        }
    }
    else
    {
        if (VlcVideoBase *s = surface(expr))
            ret.append(s);
    }
    return ret;
}


libvlc_instance_t * VlcAudioVideo::vlcInstance()
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
            argv.append("--verbose=4");
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
        QString cg(+modulePath + "/lib/vlc/vlc-cache-gen.exe");
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

        libvlc_set_user_agent(vlc, "Tao3D (LibVLC)", NULL);

        IFTRACE(video)
        {
            sdebug() << "libLVC version: " << libvlc_get_version() << "\n";
            sdebug() << "libLVC changeset: " << libvlc_get_changeset() << "\n";
            sdebug() << "libLVC compiler: " << libvlc_get_compiler() << "\n";
        }
    }
    return vlc;
}


XL::Name_p VlcAudioVideo::vlc_reset(XL::Tree_p self)
// ----------------------------------------------------------------------------
//   Clear the user options list
// ----------------------------------------------------------------------------
{
    Q_UNUSED(self);
    userOptions.clear();
    return XL::xl_true;
}


XL::Name_p VlcAudioVideo::vlc_arg(XL::Tree_p self, text opt)
// ----------------------------------------------------------------------------
//   Append the given option to the list of user-defined options
// ----------------------------------------------------------------------------
{
    Q_UNUSED(self);
    userOptions.append(+opt);
    return XL::xl_true;
}


XL::Name_p VlcAudioVideo::vlc_init(XL::Tree_p self)
// ----------------------------------------------------------------------------
//   Initialize VLC with the given options
// ----------------------------------------------------------------------------
{
    bool needInit = userOptions != lastUserOptions || (!vlc && !initFailed);
    if (needInit)
    {
        if (vlc)
            deleteVlcInstance();
        lastUserOptions = userOptions;
        libvlc_instance_t *ok = vlcInstance();
        if (!ok)
        {
            Ooops("Failed to initialize VLC library: $1", self);
            return XL::xl_false;
        }
    }
    return XL::xl_true;
}


void VlcAudioVideo::deleteVlcInstance()
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


QString VlcAudioVideo::stripOptions(QString &name)
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


template <class T>
T * VlcAudioVideo::getOrCreateVideoObject(XL::Context_p context,
                                          XL::Tree_p self,
                                          text name,
                                          unsigned width,
                                          unsigned height,
                                          float wscale,
                                          float hscale)
// ----------------------------------------------------------------------------
//   Find object derived from VlcVideoBase by name, or create it and start it
// ----------------------------------------------------------------------------
{
#if (TAO_MODULE_API_CURRENT == 30 && TAO_MODULE_API_AGE == 12)
    if (tao->offlineRendering())
    {
        IFTRACE(video)
            sdebug() << "Tao is in 'offline rendering' mode\n";
        return NULL;
    }
#endif

    VlcVideoBase *base = surface(name);
    T *vobj = dynamic_cast<T *>(base);
    if (!vobj)
    {
        if (base)
        {
            // Another type of video object with the same name already exists.
            // Replace it.
            IFTRACE(video)
                sdebug() << "Deleting existing video object "
                            "of unsuitable type for media: " << name << "\n";
            movie_drop(name);
        }

        text saveName(name);
        QRegExp re("[a-z]+://");
        QString qn = QString::fromUtf8(name.data(), name.length());
        if (re.indexIn(qn) == -1)
        {
            // Not a URL: resolve file path

            // 1. Remove options, if any
            QString nam = +name;
            QString opt = stripOptions(nam);
            name = +nam;

            // 2. Resolve path
            name = context->ResolvePrefixedPath(name);
            text folder = tao->currentDocumentFolder();
            QString qf = QString::fromUtf8(folder.data(), folder.length());
            QString qn = QString::fromUtf8(name.data(), name.length());
            QFileInfo inf(QDir(qf), qn);
            name = +QDir::toNativeSeparators(inf.absoluteFilePath());

            // 3. Restore options
            if (!opt.isEmpty())
            {
                name.append("##");
                name.append(+opt);
            }

            // 4. Create and keep video player
            vobj = new T(+name, width, height, wscale, hscale);
            videos[saveName] = (VlcVideoBase *)vobj;

            // 5. Ouput error if file does not exist
            if (!inf.isReadable())
            {
                QString err;
                err = QString("File not found or unreadable: $1\n"
                              "File path: %1").arg(+name);
                Ooops(+err, self);
                return NULL;
            }
        }
        else
        {
            // name is a URL. Create and keep video player
            vobj = new T(+name, width, height);
            videos[saveName] = (VlcVideoBase *)vobj;
        }

        vobj->play();

#if (TAO_MODULE_API_CURRENT > 30 || \
    (TAO_MODULE_API_CURRENT == 30 && TAO_MODULE_API_AGE > 12))
    if (tao->offlineRendering())
    {
        IFTRACE(video)
            sdebug() << "Tao is in 'offline rendering' mode\n";
        double ttime = tao->currentPageTime();
        double itime = vobj->lastTime + vobj->lastRate*(ttime-vobj->lastTime);
        vobj->setTime(itime);
    }
#endif
    }

    return vobj;
}


static bool checkVideoError(XL::Tree_p self, VlcVideoBase *video)
// ----------------------------------------------------------------------------
//   Convenience function to report errors on a video
// ----------------------------------------------------------------------------
{
    if (video->lastError == "")
        return false;

    XL::Ooops("Cannot play: $1", self);
    QString err = "Media player error: " + video->lastError;
    XL::Ooops(+err, self);
    QString err2 = "Path or URL: " + video->url();
    XL::Ooops(+err2, self);
    video->lastError = "";
    return true;
}


XL::Integer_p VlcAudioVideo::movie_texture(XL::Context_p context,
                                          XL::Tree_p self, text name,
                                          XL::Integer_p width,
                                          XL::Integer_p height,
                                          float wscale, float hscale)
// ----------------------------------------------------------------------------
//   Make a video player texture of given size (in pixels, or relative)
// ----------------------------------------------------------------------------
{
    if (name == "")
        return new Integer(0, self->Position());

    VlcVideoSurface *surface =
            getOrCreateVideoObject<VlcVideoSurface>(context, self, name,
                                                    width->value,
                                                    height->value,
                                                    wscale, hscale);
    if (!surface)
        return new Integer(0, self->Position());

#if (TAO_MODULE_API_CURRENT > 30 || \
    (TAO_MODULE_API_CURRENT == 30 && TAO_MODULE_API_AGE > 12))
    if (tao->offlineRendering())
    {
        IFTRACE(video)
            sdebug() << "Tao is in 'offline rendering' mode\n";

        if (!surface->offline)
        {
            surface->setVolume(0);
            surface->offline = true;
        }

        double ttime = tao->currentPageTime();
        double itime = surface->lastTime +
                       surface->lastRate * (ttime-surface->lastTime);
        QTime t0;
        const int TIMEOUT = 2000;
        t0.start();
        surface->setTime(itime);
        
        IFTRACE(video)
            sdebug() << "Seeking at " << t0.elapsed()
                     << " movie " << surface->time()
                     << " target " << itime
                     << " texture " << surface->texture()
                     << "\n";

        while (surface->texture() == 0      &&
               surface->lastError == ""     &&
               t0.elapsed() < TIMEOUT)
            surface->exec();
        IFTRACE(video)
            sdebug() << "Texture at " << t0.elapsed()
                     << " stime " << surface->time()
                     << " itime " << itime
                     << " tex " << surface->texture()
                     << "\n";

        while (surface->time() < itime &&
               surface->lastError == ""     &&
               t0.elapsed() < TIMEOUT)
            surface->exec();

        IFTRACE(video)
            sdebug() << "Result  at " << t0.elapsed()
                     << " stime " << surface->time()
                     << " itime " << itime
                     << " tex " << surface->texture()
                     << "\n";
    }
    else
    {
        if (surface->offline)
        {
            surface->setVolume(1);
            surface->offline = false;
        }
    }
#endif

    surface->exec();

    if (checkVideoError(self, surface))
        return new Integer(0, self->Position());

    // Bind texture
    unsigned int w = 0, h = 0;
    GLuint id = surface->texture();
    if (id != 0)
    {
        VideoTrack * t = surface->currentVideoTrack();
        t->ref();
        w = t->width();
        h = t->height();
        tao->AddToLayout2(VideoTrack::render_callback,
                          VideoTrack::identify_callback,
                          t, VideoTrack::delete_callback);
    }
    GL.Enable(GL_TEXTURE_2D);
    GL.BindTexture(GL_TEXTURE_2D, id);
    GL.TextureSize(w, h);

    tao->refreshOn(QEvent::Timer, -1.0);
    return new Integer(id, self->Position());
}


XL::Integer_p VlcAudioVideo::movie_texture(XL::Context_p context,
                                          XL::Tree_p self, text name)
// ----------------------------------------------------------------------------
//   Make a video player texture
// ----------------------------------------------------------------------------
{
    return movie_texture(context, self, name,
                         new XL::Integer(0, self->Position()),
                         new XL::Integer(0, self->Position()),
                         -1.0, -1.0);
}


XL::Integer_p VlcAudioVideo::movie_texture_relative(XL::Context_p context,
                                          XL::Tree_p self, text name,
                                          float wscale,
                                          float hscale)
// ----------------------------------------------------------------------------
//   Make a video player texture of given size (relative to native resolution)
// ----------------------------------------------------------------------------
{
    return movie_texture(context, self, name,
                         new XL::Integer(0, self->Position()),
                         new XL::Integer(0, self->Position()),
                         wscale, hscale);
}


XL::Name_p VlcAudioVideo::movie_fullscreen(XL::Context_p context,
                                           XL::Tree_p self, text name)
// ----------------------------------------------------------------------------
//   Play a movie
// ----------------------------------------------------------------------------
{
    if (name == "")
        return  XL::xl_false;

    VlcVideoFullscreen *video =
            getOrCreateVideoObject<VlcVideoFullscreen>(context, self, name,
                                                       0, 0);
    if (!video)
        return XL::xl_false;

    video->exec();

    if (checkVideoError(self, video))
        return XL::xl_false;

    // Refresh every 500 ms only for reduced CPU usage
    // Video runs at its own pace, so this rate only impacts the state machine
    // in VlcVideobase (for instance, the fullscreen window may close as much
    // as 500 ms after the video has ended).
    // (This is a marginal optimization as long as movie_fullscreen is
    // properly enclosed in a locally block)
    tao->refreshOn(QEvent::Timer, tao->currentTime() + .5);

    return XL::xl_true;
}


XL::Name_p VlcAudioVideo::movie_drop(text name)
// ----------------------------------------------------------------------------
//   Purge the given video surface from memory
// ----------------------------------------------------------------------------
{
    video_map::iterator found = videos.find(name);
    if (found != videos.end())
    {
        VlcVideoBase *s = (*found).second;
        videos.erase(found);
        delete s;
        return XL::xl_true;
    }
    return XL::xl_false;
}


XL::Name_p VlcAudioVideo::movie_only(text name)
// ----------------------------------------------------------------------------
//   Purge all other surfaces from memory
// ----------------------------------------------------------------------------
{
    video_map::iterator n = videos.begin();
    for (video_map::iterator v = videos.begin(); v != videos.end(); v = n)
    {
        if (name != (*v).first)
        {
            VlcVideoBase *s = (*v).second;
            videos.erase(v);
            delete s;
            n = videos.begin();
        }
        else
        {
            n = ++v;
        }
    }
    return XL::xl_false;
}


#define MOVIE_ADAPTER(id)                       \
XL::Name_p VlcAudioVideo::movie_##id(text name)  \
{                                               \
    bool ok = false;                            \
    foreach (VlcVideoBase *s, surfaces(name))   \
    {                                           \
        s->id();                                \
        ok = true;                              \
    }                                           \
    return ok ? XL::xl_true : XL::xl_false;     \
}

MOVIE_ADAPTER(play)
MOVIE_ADAPTER(pause)
MOVIE_ADAPTER(stop)
MOVIE_ADAPTER(next_frame)

#define MOVIE_FLOAT_ADAPTER(id, ev)                             \
XL::Real_p VlcAudioVideo::movie_##id(XL::Tree_p self, text name) \
{                                                               \
    float result = -1.0;                                        \
    ev;                                                         \
    if (VlcVideoBase *s = surface(name))                        \
        result = s->id();                                       \
    return new XL::Real(result, self->Position());              \
}

MOVIE_FLOAT_ADAPTER(volume,   )
MOVIE_FLOAT_ADAPTER(position, tao->refreshOn(QEvent::Timer, -1))
MOVIE_FLOAT_ADAPTER(time,     tao->refreshOn(QEvent::Timer, -1))
MOVIE_FLOAT_ADAPTER(length,   )
MOVIE_FLOAT_ADAPTER(rate ,    )

#define MOVIE_BOOL_ADAPTER(id)                  \
XL::Name_p VlcAudioVideo::movie_##id(text name) \
{                                               \
    QList<VlcVideoBase *>list = surfaces(name); \
    bool ok = !list.isEmpty();                  \
    tao->refreshOn(QEvent::Timer, -1);          \
    foreach (VlcVideoBase *s, surfaces(name))   \
        ok &= s->id();                          \
    return ok ? XL::xl_true : XL::xl_false;     \
}

MOVIE_BOOL_ADAPTER(playing)
MOVIE_BOOL_ADAPTER(paused)
MOVIE_BOOL_ADAPTER(done)
MOVIE_BOOL_ADAPTER(loop)

#define MOVIE_FLOAT_SETTER(id, mid)                             \
XL::Name_p VlcAudioVideo::movie_set_##id(text name, float value) \
{                                                               \
    bool ok = false;                                            \
    foreach (VlcVideoBase *s, surfaces(name))                   \
    {                                                           \
        s->mid(value);                                          \
        ok = true;                                              \
    }                                                           \
    return ok ? XL::xl_true : XL::xl_false;                     \
}


MOVIE_FLOAT_SETTER(volume, setVolume)
MOVIE_FLOAT_SETTER(position, setPosition)
MOVIE_FLOAT_SETTER(time, setTime)
MOVIE_FLOAT_SETTER(rate, setRate)

#define MOVIE_BOOL_SETTER(id, mid)                              \
XL::Name_p VlcAudioVideo::movie_set_##id(text name, bool on)    \
{                                                               \
    bool ok = false;                                            \
    foreach (VlcVideoBase *s, surfaces(name))                   \
    {                                                           \
        s->mid(on);                                             \
        ok = true;                                              \
    }                                                           \
    return ok ? XL::xl_true : XL::xl_false;                     \
}

MOVIE_BOOL_SETTER(loop, setLoop)

XL::Name_p VlcAudioVideo::movie_set_video_stream(text name, int num)
{
    int st = 0;
    foreach (VlcVideoBase *s, surfaces(name))
    {
        if (st == 0)
            st = 1;
        VlcVideoSurface * sf = dynamic_cast<VlcVideoSurface *>(s);
        if (!sf || !sf->setVideoTrack(num))
            if (st == 1)
                st = 2;
    }
    return (st == 1) ? XL::xl_true : XL::xl_false;
}

XL_DEFINE_TRACES

int module_init(const Tao::ModuleApi *api, const Tao::ModuleInfo *mod)
// ----------------------------------------------------------------------------
//   Initialize the Tao module
// ----------------------------------------------------------------------------
{
    Q_UNUSED(mod);
    XL_INIT_TRACES();
    VlcAudioVideo::tao = api;
#ifdef Q_OS_WIN32
    VlcAudioVideo::modulePath = mod->path;
#endif
    return 0;
}


int module_exit()
// ----------------------------------------------------------------------------
//   Uninitialize the Tao module
// ----------------------------------------------------------------------------
{
    VlcAudioVideo::movie_only("");
    AsyncSetVolume::stop();
    VlcAudioVideo::deleteVlcInstance();
    return 0;
}


int show_preferences()
// ----------------------------------------------------------------------------
//   Show the preference dialog
// ----------------------------------------------------------------------------
{
    VLCPreferences().exec();
    return 0;
}
