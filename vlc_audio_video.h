#ifndef VLC_AUDIO_VIDEO_H
#define VLC_AUDIO_VIDEO_H
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
#include <QObject>
#include "tree.h"
#include "context.h"
#include "tao/module_api.h"
#include "tao/tao_info.h"
#include "vlc_video_surface.h"

struct VideoSurfaceInfo : VlcVideoSurface, Tao::Info
// ----------------------------------------------------------------------------
//    Play audio and/or video using VLCVideoSurface
// ----------------------------------------------------------------------------
{
    typedef VideoSurfaceInfo * data_t;
    VideoSurfaceInfo();
    ~VideoSurfaceInfo();
    virtual void               Delete() { tao->deferredDelete(this); }
    operator                   data_t() { return this; }
    GLuint                     bind(text url);

public:
    // XL interface
    static XL::Integer_p       movie_texture(XL::Context_p context,
                                             XL::Tree_p self, text name);

protected:
    std::ostream &             debug();

public:
    text                       unresolvedName;
    static const
    Tao::ModuleApi *           tao;
};

#endif // VLC_AUDIO_VIDEO_H
