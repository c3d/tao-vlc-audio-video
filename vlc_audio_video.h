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

#include "tree.h"
#include "context.h"
#include "tao/module_api.h"
#include "tao/tao_info.h"
#include "vlc_video_surface.h"
#include <map>


struct VideoSurface : VlcVideoSurface
// ----------------------------------------------------------------------------
//    Play audio and/or video using VLCVideoSurface
// ----------------------------------------------------------------------------
{
    typedef std::map<text, VideoSurface *>  video_map;
    VideoSurface();
    virtual ~VideoSurface();
    unsigned int                bind(text url);

public:
    // XL interface
    static XL::Integer_p        movie_texture(XL::Context_p context,
                                              XL::Tree_p self,
                                              text name);
    static XL::Name_p           movie_drop(text name);
    static XL::Name_p           movie_only(text name);
    static XL::Name_p           movie_play(text name);
    static XL::Name_p           movie_pause(text name);
    static XL::Name_p           movie_stop(text name);

    static XL::Real_p           movie_volume(XL::Tree_p self, text name);
    static XL::Real_p           movie_position(XL::Tree_p self, text name);
    static XL::Real_p           movie_time(XL::Tree_p self, text name);
    static XL::Real_p           movie_length(XL::Tree_p self, text name);
    static XL::Real_p           movie_rate(XL::Tree_p self, text name);


    static XL::Name_p           movie_playing(text name);
    static XL::Name_p           movie_paused(text name);
    static XL::Name_p           movie_done(text name);

    static XL::Name_p           movie_set_volume(text name, float volume);
    static XL::Name_p           movie_set_position(text name, float position);
    static XL::Name_p           movie_set_time(text name, float position);
    static XL::Name_p           movie_set_rate(text name, float rate);
    
protected:
    std::ostream &              debug();
    static VideoSurface *       surface(text name);

public:
    static video_map            videos;
    static const Tao::ModuleApi*tao;
#ifdef Q_OS_WIN32
    static text                 modulePath;
#endif
};

#endif // VLC_AUDIO_VIDEO_H
