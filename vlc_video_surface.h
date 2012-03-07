#ifndef VLC_VIDEO_SURFACE_H
#define VLC_VIDEO_SURFACE_H
// ****************************************************************************
//  vlc_video_surface.h                                            Tao project
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

#include <qgl.h>
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QImage>
#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>
#include <iostream>

class VlcVideoSurface
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
        VS_SUBITEM_READY
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
        default: return "UNKNOWN";
        }
#undef ADD_STATE
    }

public:
    VlcVideoSurface(unsigned int w = 0, unsigned int h = 0);
    ~VlcVideoSurface();

public:
    void           play(const QString & name);
    void           play();
    void           pause();
    void           stop();
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
    GLuint         texture();
    QString        url ()   { return mediaName; }

public:
    static bool             vlcInit(QStringList options);
    static void             deleteVlcInstance();
    static QString          stripOptions(QString &name);

public:
    unsigned                w, h;
    QString                 mediaName;
    QString                 lastError;

public:
    static bool             initFailed;

protected:
    libvlc_media_player_t * player;
    libvlc_media_t *        media;
    QMutex                  mutex;  // Protect 'image' and 'updated'
    QImage                  image;
    bool                    updated;
    GLuint                  textureId;
    State                   state;
    libvlc_event_manager_t *pevm;
    libvlc_event_manager_t *mevm;
    bool                    needResolution;  // REVISIT
    bool                    videoAvailable;
    bool                    descriptionMode;
    const QGLContext      * GLcontext;
    bool                    loopMode;
    QVector<char *>         mediaOptions;

protected:
    struct VlcCleanup
    {
        ~VlcCleanup()
        {
            if (VlcVideoSurface::vlc)
                libvlc_release(VlcVideoSurface::vlc);
        }
    };

protected:
    void           setState(State state);
    void           startGetMediaInfo();
    void           getMediaInfo();
    void           startPlayback();
    void           getMediaSubItems();
    std::ostream & debug();
    void           checkGLContext();
    void           genTexture();
    void           addMediaOptions();
    libvlc_media_t * newMediaFromPathOrUrl(QString name);

protected:
    static libvlc_instance_t *  vlcInstance();
    static std::ostream &       sdebug();

    static void *  lockFrame(void *obj, void **plane);
    static void    unlockFrame(void *obj, void *picture, void *const *plane);
    static void    displayFrame(void *obj, void *picture);

    static void    mediaParsed(const struct libvlc_event_t *, void *obj);
    static void    playerPlaying(const struct libvlc_event_t *, void *obj);
    static void    mediaSubItemAdded(const struct libvlc_event_t *, void *obj);
    static void    playerEndReached(const struct libvlc_event_t *, void *obj);
    static void    playerError(const struct libvlc_event_t *, void *obj);

protected:
    static libvlc_instance_t *  vlc;
    static QStringList          userOptions;
    static VlcCleanup           cleanup;
};

#endif // VLC_VIDEO_SURFACE_H
