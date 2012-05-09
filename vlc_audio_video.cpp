// ****************************************************************************
//  vlc_audio_video.cpp                                            Tao project
// ****************************************************************************
//
//   File Description:
//
//    Audio/video decoding and playback for Tao Presentations, based on VLC.
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

#include "tao/tao_gl.h"
#include "vlc_audio_video.h"
#include "vlc_preferences.h"
#include "action.h"
#include "errors.h"
#include <QFileInfo>
#include <QStringList>


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

const Tao::ModuleApi * VideoSurface::tao = NULL;
VideoSurface::video_map VideoSurface::videos;
#ifdef Q_OS_WIN32
text VideoSurface::modulePath;
#endif


VideoSurface::VideoSurface(text pathOrUrl, unsigned int w, unsigned int h)
// ----------------------------------------------------------------------------
//   Create the video player
// ----------------------------------------------------------------------------
    : VlcVideoSurface(+pathOrUrl, w, h)
{
}


VideoSurface::~VideoSurface()
// ----------------------------------------------------------------------------
//    Stop the player and delete resources
// ----------------------------------------------------------------------------
{
}


std::ostream & VideoSurface::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[AudioVideo] " << (void*)this << " ";
    return std::cerr;
}


struct ParseTextTree : XL::Action
// ----------------------------------------------------------------------------
//   Extract text from a tree of blocks and infixes, into a QStringList
// ----------------------------------------------------------------------------
{
    ParseTextTree(QStringList &list) : list(list) {}

    Tree *DoBlock(Block *what)
    {
        return what->child->Do(this);
    }
    Tree *DoInfix(Infix *what)
    {
        if (Tree * t = what->left->Do(this))
            return t;
        return what->right->Do(this);
    }
    Tree *DoText(Text *what)
    {
        list << +what->value;
        return NULL;
    }
    Tree *Do(Tree *what)
    {
        Q_UNUSED(what); return NULL;
    }

    QStringList &list;
};


XL::Name_p VideoSurface::vlc_init(XL::Tree_p self, XL::Tree_p opts)
// ----------------------------------------------------------------------------
//   Add option to the list of VLC initialization options
// ----------------------------------------------------------------------------
{
    bool ok = (vlc != NULL);

    if (!vlc && !initFailed)
    {
        QStringList options;
        ParseTextTree parse(options);
        opts->Do(parse);
        ok = VlcVideoSurface::vlcInit(options);
        if (!ok)
        {
            QString err = "Failed to initialize libVLC: $1";
            if (options.size())
            {
                err += "\nCheck user-supplied options: ";
                foreach (QString opt, options)
                    err += "'" + opt + "' ";
                err += "\nNOTE: You must CLOSE the document before trying "
                             "new options.";
            }
            Ooops(+err, self);
        }
    }
    return ok ? XL::xl_true : XL::xl_false;
}


XL::Integer_p VideoSurface::movie_texture(XL::Context_p context,
                                          XL::Tree_p self, text name,
                                          XL::Integer_p width,
                                          XL::Integer_p height)
// ----------------------------------------------------------------------------
//   Make a video player texture of given size
// ----------------------------------------------------------------------------
{
    if (name == "")
        return new Integer(0, self->Position());

#ifdef USE_LICENSE
    static bool licensed, tested = false;
    if (!tested)
    {
        licensed = tao->checkImpressOrLicense("VLCAudioVideo 1.031");
        tested = true;
    }

    if (!licensed && !tao->blink(4.5, 0.5, 300.0))
        return new Integer(0, self->Position());
#endif

    // Get or build the current frame if we don't have one
    VideoSurface *surface = VideoSurface::surface(name);
    if (!surface)
    {
        text saveName(name);
        QRegExp re("[a-z]+://");
        QString qn = QString::fromUtf8(name.data(), name.length());
        if (re.indexIn(qn) == -1)
        {
            // Not a URL: resolve file path

            // 1. Remove options, if any
            QString nam = +name;
            QString opt = VlcVideoSurface::stripOptions(nam);
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
            surface = new VideoSurface(name, width->value, height->value);
            videos[saveName] = surface;

            // 5. Ouput error if file does not exist
            if (!inf.isReadable())
            {
                QString err;
                err = QString("File not found or unreadable: $1\n"
                              "File path: %1").arg(+name);
                Ooops(+err, self);
                return new Integer(0, self->Position());
            }
        }
        else
        {
            // name is a URL. Create and keep video player
            surface = new VideoSurface(name, width->value, height->value);
            videos[saveName] = surface;
        }

        surface->play();
    }

    // Bind texture
    GLuint id = surface->texture();
    if (surface->lastError != "")
    {
        XL::Ooops("Cannot play: $1", self);
        QString err = "Media player error: " + surface->lastError;
        XL::Ooops(+err, self);
        QString err2 = "Path or URL: " + surface->url();
        XL::Ooops(+err2, self);
        surface->lastError = "";
        return new Integer(0, self->Position());
    }
    if (id != 0)
        tao->BindTexture2D(id, surface->w, surface->h);

    tao->refreshOn(QEvent::Timer, -1.0);
    return new Integer(id, self->Position());
}


XL::Integer_p VideoSurface::movie_texture(XL::Context_p context,
                                          XL::Tree_p self, text name)
// ----------------------------------------------------------------------------
//   Make a video player texture]
// ----------------------------------------------------------------------------
{
    return movie_texture(context, self, name,
                         new XL::Integer(0, self->Position()),
                         new XL::Integer(0, self->Position()));
}


XL::Name_p VideoSurface::movie_drop(text name)
// ----------------------------------------------------------------------------
//   Purge the given video surface from memory
// ----------------------------------------------------------------------------
{
    video_map::iterator found = videos.find(name);
    if (found != videos.end())
    {
        VideoSurface *s = (*found).second;
        videos.erase(found);
        delete s;
        return XL::xl_true;
    }
    return XL::xl_false;
}


XL::Name_p VideoSurface::movie_only(text name)
// ----------------------------------------------------------------------------
//   Purge all other surfaces from memory
// ----------------------------------------------------------------------------
{
    video_map::iterator n = videos.begin();
    for (video_map::iterator v = videos.begin(); v != videos.end(); v = n)
    {
        if (name != (*v).first)
        {
            VideoSurface *s = (*v).second;
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


VideoSurface *VideoSurface::surface(text name)
// ----------------------------------------------------------------------------
//   Return the video surface associated with a given name or NULL
// ----------------------------------------------------------------------------
{
    video_map::iterator found = videos.find(name);
    if (found != videos.end())
        return (*found).second;
    return NULL;
}


#define MOVIE_ADAPTER(id)                       \
XL::Name_p VideoSurface::movie_##id(text name)  \
{                                               \
    if (VideoSurface *s = surface(name))        \
    {                                           \
        s->id();                                \
        return XL::xl_true;                     \
    }                                           \
    return XL::xl_false;                        \
}

MOVIE_ADAPTER(play)
MOVIE_ADAPTER(pause)
MOVIE_ADAPTER(stop)

#define MOVIE_FLOAT_ADAPTER(id, ev)                             \
XL::Real_p VideoSurface::movie_##id(XL::Tree_p self, text name) \
{                                                               \
    float result = -1.0;                                        \
    ev;                                                         \
    if (VideoSurface *s = surface(name))                        \
        result = s->id();                                       \
    return new XL::Real(result, self->Position());              \
}

MOVIE_FLOAT_ADAPTER(volume,   )
MOVIE_FLOAT_ADAPTER(position, tao->refreshOn(QEvent::Timer, -1))
MOVIE_FLOAT_ADAPTER(time,     tao->refreshOn(QEvent::Timer, -1))
MOVIE_FLOAT_ADAPTER(length,   )
MOVIE_FLOAT_ADAPTER(rate ,    )

#define MOVIE_BOOL_ADAPTER(id)                  \
XL::Name_p VideoSurface::movie_##id(text name)  \
{                                               \
    tao->refreshOn(QEvent::Timer, -1);          \
    if (VideoSurface *s = surface(name))        \
        if (s->id())                            \
            return XL::xl_true;                 \
    return XL::xl_false;                        \
}

MOVIE_BOOL_ADAPTER(playing)
MOVIE_BOOL_ADAPTER(paused)
MOVIE_BOOL_ADAPTER(done)
MOVIE_BOOL_ADAPTER(loop)

#define MOVIE_FLOAT_SETTER(id, mid)                             \
XL::Name_p VideoSurface::movie_set_##id(text name, float value) \
{                                                               \
    if (VideoSurface *s = surface(name))                        \
    {                                                           \
        s->mid(value);                                          \
        return XL::xl_true;                                     \
    }                                                           \
    return XL::xl_false;                                        \
}


MOVIE_FLOAT_SETTER(volume, setVolume)
MOVIE_FLOAT_SETTER(position, setPosition)
MOVIE_FLOAT_SETTER(time, setTime)
MOVIE_FLOAT_SETTER(rate, setRate)

#define MOVIE_BOOL_SETTER(id, mid)                             \
XL::Name_p VideoSurface::movie_set_##id(text name, bool on) \
{                                                               \
    if (VideoSurface *s = surface(name))                        \
    {                                                           \
        s->mid(on);                                          \
        return XL::xl_true;                                     \
    }                                                           \
    return XL::xl_false;                                        \
}

MOVIE_BOOL_SETTER(loop, setLoop)

XL_DEFINE_TRACES

int module_init(const Tao::ModuleApi *api, const Tao::ModuleInfo *mod)
// ----------------------------------------------------------------------------
//   Initialize the Tao module
// ----------------------------------------------------------------------------
{
    Q_UNUSED(mod);
    glewInit();
    XL_INIT_TRACES();
    VideoSurface::tao = api;
#ifdef Q_OS_WIN32
    VideoSurface::modulePath = mod->path;
#endif
    return 0;
}


int module_exit()
// ----------------------------------------------------------------------------
//   Uninitialize the Tao module
// ----------------------------------------------------------------------------
{
    VideoSurface::movie_only("");
    VlcVideoSurface::deleteVlcInstance();
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
