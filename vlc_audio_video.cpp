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

#include "vlc_audio_video.h"
#include "vlc_preferences.h"
#include "errors.h"

inline QString operator +(std::string s)
{
    return QString::fromUtf8(s.data(), s.length());
}

inline std::string operator +(QString s)
{
    return std::string(s.toUtf8().constData());
}

using namespace XL;

const Tao::ModuleApi * VideoSurfaceInfo::tao = NULL;

VideoSurfaceInfo::VideoSurfaceInfo()
// ----------------------------------------------------------------------------
//   Create the video player
// ----------------------------------------------------------------------------
    : VlcVideoSurface()
{
}


VideoSurfaceInfo::~VideoSurfaceInfo()
// ----------------------------------------------------------------------------
//    Stop the player and delete resources
// ----------------------------------------------------------------------------
{
}


GLuint VideoSurfaceInfo::bind(text url)
// ----------------------------------------------------------------------------
//    Start playback or refresh the surface and bind to the texture
// ----------------------------------------------------------------------------
{
    play(+url);
    return texture();
}


std::ostream & VideoSurfaceInfo::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[AudioVideo] " << (void*)this << " ";
    return std::cerr;
}



XL::Integer_p VideoSurfaceInfo::movie_texture(XL::Context_p context,
                                              XL::Tree_p self, text name)
// ----------------------------------------------------------------------------
//   Make a video player texture
// ----------------------------------------------------------------------------
{
    // Get or build the current frame if we don't have one
    VideoSurfaceInfo *surface = self->GetInfo<VideoSurfaceInfo>();
    if (!surface)
    {
        surface = new VideoSurfaceInfo();
        self->SetInfo<VideoSurfaceInfo> (surface);
    }

    if (name != surface->unresolvedName)
    {
        // Name has not been converted to URL format, or has changed
        surface->unresolvedName = name;
        if (name != "")
        {
            QRegExp re("[a-z]+://");
            QString qn = QString::fromUtf8(name.data(), name.length());
            if (re.indexIn(qn) == -1)
            {
                name = context->ResolvePrefixedPath(name);
                text folder = tao->currentDocumentFolder();
                QString qf = QString::fromUtf8(folder.data(), folder.length());
                QString qn = QString::fromUtf8(name.data(), name.length());
                QFileInfo inf(QDir(qf), qn);
                if (inf.isReadable())
                {
                    name =
#if defined(Q_OS_WIN)
                            "file:///"
#else
                            "file://"
#endif
                            + text(inf.absoluteFilePath().toUtf8().constData());
                }
            }
        }
    }
    else
    {
        // Raw name has not changed, no need to resolve again
        name = +surface->playing;
    }

    // Resize to requested size, and bind texture
    GLuint id = surface->bind(name);
    if (surface->lastError != "")
    {
        XL::Ooops("Cannot play: $1", self);
        QString err = "Media player error: " + surface->lastError;
        XL::Ooops(+err, self);
        QString err2 = "Path or URL: " + surface->playing;
        XL::Ooops(+err2, self);
        surface->lastError = "";
        return new Integer(0, self->Position());
    }
    if (id != 0)
        tao->BindTexture2D(id, surface->w, surface->h);

    tao->refreshOn(QEvent::Timer, -1.0);
    return new Integer(id, self->Position());
}


XL_DEFINE_TRACES

int module_init(const Tao::ModuleApi *api, const Tao::ModuleInfo *mod)
// ----------------------------------------------------------------------------
//   Initialize the Tao module
// ----------------------------------------------------------------------------
{
    Q_UNUSED(mod);
    glewInit();
    XL_INIT_TRACES();
    VideoSurfaceInfo::tao = api;
    return 0;
}


int module_exit()
// ----------------------------------------------------------------------------
//   Uninitialize the Tao module
// ----------------------------------------------------------------------------
{
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
