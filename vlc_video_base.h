#ifndef VLC_VIDEO_BASE_H
#define VLC_VIDEO_BASE_H
// *****************************************************************************
// vlc_video_base.h                                                Tao3D project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2011,2013-2014,2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2011-2013, Jérôme Forissier <jerome@taodyne.com>
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

#include "tao/tao_gl.h"
#include <QMap>
#include <QMutex>
#include <QString>
#include <QThread>
#include <QWaitCondition>
#include <QVector>
#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_events.h>
#include <vlc/libvlc_media_player.h>
#include <iostream>

struct VlcVideoBase
// ----------------------------------------------------------------------------
//   Base class for VLC audio and video streams
// ----------------------------------------------------------------------------
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
    void           next_frame();
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
    virtual void   exec();
    double         updateTime(double frameTime);

public:
    QString                 lastError;
    double                  lastTime;
    double                  lastRate;
    double                  frameTime;
    double                  fps;         // -1: not tested, 0: unknown
    bool                    offline;

protected:
    QString                 mediaName;
    libvlc_instance_t *     vlc;
    libvlc_media_player_t * player;
    libvlc_media_t *        media;
    State                   state;
    libvlc_event_manager_t *mevm;
    libvlc_event_manager_t *pevm;
    bool                    loopMode;
    QVector<char *>         mediaOptions;

protected:
    void           setState(State state);
    std::ostream & debug();
    void             getMediaSubItems();
    libvlc_media_t * newMediaFromPathOrUrl(QString name);

protected:
    virtual void   startPlayback();

protected:
    static void    playerPlaying(const struct libvlc_event_t *, void *obj);
    static void    playerEndReached(const struct libvlc_event_t *, void *obj);
    static void    playerError(const struct libvlc_event_t *, void *obj);
    static void    mediaSubItemAdded(const struct libvlc_event_t *, void *obj);
};



struct AsyncSetVolume : public QThread
// ----------------------------------------------------------------------------
//  Singleton for non-blocking (threaded) calls to libvlc_audio_set_volume
// ----------------------------------------------------------------------------
{
    AsyncSetVolume() : done(false) {}
    virtual ~AsyncSetVolume() {}

public:
    static void libvlc_audio_set_volume(libvlc_media_player_t *player,
                                        int volume)
    {
        instance()->setVolume(player, volume);
    }
    static void discard(libvlc_media_player_t *player)
    {
        instance()->discardPending(player);
    }
    static void stop()
    {
        AsyncSetVolume *& inst = AsyncSetVolume::inst;
        if (!inst)
            return;
        inst->stopAndWait();
        delete inst;
        inst = NULL;
    }

protected:
    void                    setVolume(libvlc_media_player_t *p_mi,
                                      int i_volume);
    void                    discardPending(libvlc_media_player_t *player);
    void                    stopAndWait();
    void                    run();

protected:
    static AsyncSetVolume * instance();

protected:
    QList<libvlc_media_player_t *>      pendingPlayers;
    QMap<libvlc_media_player_t *, int>  volumes;
    QMutex                              mutex;
    QWaitCondition                      cond;
    bool                                done;

protected:
    static AsyncSetVolume *             inst;
};


inline double VlcVideoBase::updateTime(double prevTime)
// ----------------------------------------------------------------------------
//   Update time for the current stream
// ----------------------------------------------------------------------------
//   The input prevTime is used in the case of multistream movies.
//   In that case, we don't know which stream is going to update time first.
//   So we take as a reference frames that help us make "forward progress"
{
    if (fps > 0 && prevTime > 0)
    {
        double newTime = prevTime + lastRate/fps;
        double oldestTime = prevTime - lastRate/fps;
        if (frameTime >= oldestTime && frameTime < newTime)
            frameTime = newTime;
    }
    return frameTime;
}

#endif // VLC_VIDEO_BASE_H
