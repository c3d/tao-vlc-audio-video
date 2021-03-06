#ifndef VLC_AUDIO_VIDEO_H
#define VLC_AUDIO_VIDEO_H
// *****************************************************************************
// vlc_audio_video.h                                               Tao3D project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2011,2014-2015,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2011-2014, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of Tao3D
//
// Tao3D is free software: you can r redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tao3D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Tao3D, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

#include "tree.h"
#include "context.h"
#include "tao/module_api.h"
#include "tao/tao_info.h"
#include <vlc/libvlc.h>
#include <map>
#include <QList>
#include <QStringList>


struct VlcVideoBase;


struct VlcAudioVideo
// ----------------------------------------------------------------------------
//    Play audio and/or video using VLCVideoSurface
// ----------------------------------------------------------------------------
{
    typedef std::map<text, VlcVideoBase *>  video_map;

public:
    static libvlc_instance_t *  vlcInstance();
    static void                 deleteVlcInstance();
    static QString              stripOptions(QString &name);

public:
    // XL interface
    static XL::Name_p           vlc_reset(XL::Tree_p self);
    static XL::Name_p           vlc_arg (XL::Tree_p self, text opt);
    static XL::Name_p           vlc_init(XL::Tree_p self);

    static XL::Integer_p        movie_texture(XL::Context_p context,
                                              XL::Tree_p self,
                                              text name);
    static XL::Integer_p        movie_texture(XL::Context_p context,
                                              XL::Tree_p self,
                                              text name,
                                              XL::Integer_p width,
                                              XL::Integer_p height,
                                              float wscale,
                                              float hscale);
    static XL::Integer_p        movie_texture_relative(
                                              XL::Context_p context,
                                              XL::Tree_p self,
                                              text name,
                                              float wscale,
                                              float hscale);
    static XL::Name_p           movie_fullscreen(XL::Context_p context,
                                                 XL::Tree_p self,
                                                 text name);
    static XL::Name_p           movie_drop(text name);
    static XL::Name_p           movie_only(text name);
    static XL::Name_p           movie_play(text name);
    static XL::Name_p           movie_pause(text name);
    static XL::Name_p           movie_stop(text name);
    static XL::Name_p           movie_next_frame(text name);

    static XL::Real_p           movie_volume(XL::Tree_p self, text name);
    static XL::Real_p           movie_position(XL::Tree_p self, text name);
    static XL::Real_p           movie_time(XL::Tree_p self, text name);
    static XL::Real_p           movie_length(XL::Tree_p self, text name);
    static XL::Real_p           movie_rate(XL::Tree_p self, text name);


    static XL::Name_p           movie_playing(text name);
    static XL::Name_p           movie_paused(text name);
    static XL::Name_p           movie_done(text name);
    static XL::Name_p           movie_loop(text name);

    static XL::Name_p           movie_set_volume(text name, float volume);
    static XL::Name_p           movie_set_position(text name, float position);
    static XL::Name_p           movie_set_time(text name, float position);
    static XL::Name_p           movie_set_rate(text name, float rate);
    static XL::Name_p           movie_set_loop(text name, bool on);
    static XL::Name_p           movie_set_video_stream(text name, int num);

protected:
    struct VlcCleanup
    {
        ~VlcCleanup()
        {
            if (VlcAudioVideo::vlc)
                libvlc_release(VlcAudioVideo::vlc);
        }
    };

protected:
    static VlcVideoBase *       surface(text name);
    static QList<VlcVideoBase*> surfaces(text name);
    static std::ostream &       sdebug();
#ifdef USE_LICENSE
    static bool                 licenseOk();
#endif

protected:
    template <class T>
    static T *                  getOrCreateVideoObject(XL::Context_p context,
                                                       XL::Tree_p self,
                                                       text name,
                                                       unsigned width,
                                                       unsigned height,
                                                       float wscale = -1.0,
                                                       float hscale = -1.0);

protected:
    static bool                 initFailed;
    static libvlc_instance_t *  vlc;
    static QStringList          userOptions, lastUserOptions;
    static VlcCleanup           cleanup;

public:
    static video_map            videos;
    static const Tao::ModuleApi*tao;
#ifdef Q_OS_WIN32
    static text                 modulePath;
#endif
};

#endif // VLC_AUDIO_VIDEO_H
