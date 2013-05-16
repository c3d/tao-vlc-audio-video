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
#include <vlc/libvlc.h>
#include <map>
#include <QList>
#include <QStringList>


class VlcVideoBase;


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
    static XL::Name_p           vlc_init(XL::Tree_p self,
                                         XL::Tree_p opts);

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
    static bool                 vlcInit(QStringList options);
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
    static QStringList          userOptions;
    static VlcCleanup           cleanup;

public:
    static video_map            videos;
    static const Tao::ModuleApi*tao;
#ifdef Q_OS_WIN32
    static text                 modulePath;
#endif
};

#endif // VLC_AUDIO_VIDEO_H
