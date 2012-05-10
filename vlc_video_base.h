#ifndef VLC_VIDEO_BASE_H
#define VLC_VIDEO_BASE_H
// ****************************************************************************
//  vlc_video_base.h                                               Tao project
// ****************************************************************************
//
//   File Description:
//
//    Base class for video playback with libVLC.
//
//
//
//
//
//
//
//
// ****************************************************************************
// (C) 2012 Taodyne SAS <contact@taodyne.com>
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

#include <qgl.h>
#include <QString>
#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>
#include <iostream>

class VlcVideoBase
{
public:
    enum State
    {
        VS_STOPPED,
        VS_PLAYING,
        VS_PAUSED,
        VS_ERROR,
        VS_PLAY_ENDED,
        VS_WAITING_FOR_SUBITEMS,
        VS_ALL_SUBITEMS_RECEIVED,
        VS_SUBITEM_READY,
        VS_STARTING
    };

    std::string stateName(State state)
    {
#define ADD_STATE(st) case st: return #st
        switch (state)
        {
        ADD_STATE(VS_STOPPED);
        ADD_STATE(VS_PLAYING);
        ADD_STATE(VS_PAUSED);
        ADD_STATE(VS_ERROR);
        ADD_STATE(VS_PLAY_ENDED);
        ADD_STATE(VS_WAITING_FOR_SUBITEMS);
        ADD_STATE(VS_ALL_SUBITEMS_RECEIVED);
        ADD_STATE(VS_SUBITEM_READY);
        ADD_STATE(VS_STARTING);
        default: return "UNKNOWN";
        }
#undef ADD_STATE
    }

public:
    VlcVideoBase(QString mediaNameAndOptions);
    virtual ~VlcVideoBase();

public:
    void           play();
    void           pause();
    virtual void   stop();
    void           mute(bool mute);
    float          volume();
    float          position();
    float          time();
    float          length();
    float          rate();
    bool           playing();
    bool           paused();
    bool           done();
    bool           loop();
    void           setVolume(float vol);
    void           setPosition(float pos);
    void           setTime(float pos);
    void           setRate(float pos);
    void           setLoop(bool on);
    QString        url ()   { return mediaName; }

public:
    QString                 lastError;

protected:
    QString                 mediaName;
    libvlc_instance_t *     vlc;
    libvlc_media_player_t * player;
    libvlc_media_t *        media;
    State                   state;
    libvlc_event_manager_t *pevm;
    bool                    loopMode;
    QVector<char *>         mediaOptions;

protected:
    void           setState(State state);
    virtual void   startPlayback() = 0;
    std::ostream & debug();
    libvlc_media_t * newMediaFromPathOrUrl(QString name);

protected:

    static void    playerPlaying(const struct libvlc_event_t *, void *obj);
    static void    playerEndReached(const struct libvlc_event_t *, void *obj);
    static void    playerError(const struct libvlc_event_t *, void *obj);
};

#endif // VLC_VIDEO_BASE_H
