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

#include "vlc_video_base.h"
#include <qgl.h>
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QImage>
#include <QVector>
#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>
#include <iostream>

class VideoTrack;

class VlcVideoSurface : public VlcVideoBase
// ----------------------------------------------------------------------------
//    Make video from a file/URL available as a texture
// ----------------------------------------------------------------------------
{
public:
    VlcVideoSurface(QString mediaNameAndOptions,
                    unsigned int w = 0, unsigned int h = 0,
                    float wscale = -1.0, float hscale = -1.0);
    virtual ~VlcVideoSurface();

public:
    virtual void   stop();
    virtual void   exec();
    VideoTrack *   currentVideoTrack();
    bool           setVideoTrack(int id);

    // Info re. current video track
    GLuint         texture();
    unsigned       width();
    unsigned       height();

    // REVISIT make VlcVideoSurface::setState() public?
    void           _setState(State s) { setState(s); }

protected:
    unsigned                w, h;
    float                   wscale, hscale;
    QMap<int, VideoTrack *> videoTracks; // Tracks by id
    int                     vtId;        // Current track id (-1: none)
    int                     nextVtId;    // Next available id for unumbered tracks
    bool                    usePBO;
    float                   fps;         // -1: not tested, 0: unknown
    bool                    dropFrames;
    QMutex                  mutex;       // make videoFormat() thread-safe


protected:
    virtual void   startPlayback();

    void           startGetMediaInfo();
    void           getMediaSubItems();
    std::ostream & debug();
    void           updateTexture();
    int            newTrack(int es_id);

protected:
    static unsigned videoFormat(void **opaque, char *chroma,
                                unsigned *width, unsigned *height,
                                unsigned *pitches,
                                unsigned *lines);

    static void    playerTimeChanged(const struct libvlc_event_t *, void *obj);

friend class VideoTrack;
};


class VideoTrack
// ----------------------------------------------------------------------------
//    Receive images from 1 video track (video surface can have several tracks)
// ----------------------------------------------------------------------------
{
public:
    VideoTrack(VlcVideoSurface *parent, unsigned id);
    virtual ~VideoTrack();

    void           Draw();

    static void    render_callback(void *arg);
    static void    identify_callback(void *arg);
    static void    delete_callback(void *arg);

public:
    unsigned       width()   { return w; }
    unsigned       height()  { return h; }
    GLuint         texture();
    void           updateTexture();
    void           stop();
    void           ref()     { refs++; }
    void           unref()   { if (--refs == 0) delete this; }

protected:
    enum Chroma { INVALID, RV32, UYVY };

    struct ImageBuf
    {
        ImageBuf() : ptr(NULL), size(0), chroma(INVALID) {}

        void     * ptr;       // No PBO: converted.bits() / PBO: unconverted
        unsigned   size;      // bytes
        Chroma     chroma;
        QImage     converted; // RV32 -> GL_RGBA or flip YUVY
    };

protected:
    VlcVideoSurface       * parent;
    unsigned                id;
    unsigned                w, h;
    float                   wscale, hscale;
    GLuint                  textureId;
    QMutex                  mutex;  // Protect 'image' and 'updated'
    ImageBuf                image;
    QImage                  converted;
    bool                    updated;
    bool                    videoAvailable;          // In ImageBuf
    bool                    videoAvailableInTexture;
    bool                    usePBO;
    const QGLContext      * GLcontext;
    GLuint                  pbo[2];
    int                     curPBO;
    GLubyte               * curPBOPtr;
    QSet<void *>            allocatedFrames;
    unsigned                refs;

protected:
    std::ostream & debug();
    void           checkGLContext();
    void           genTexture();
    void           genPBO();
    void           transferPBO();
    void           transferNoPBO();
    void           doGLTexImage2D();
    void           displayFrameNoPBO(void *picture);
    void           displayFramePBO(void *picture);
    void           freeFrame(void *picture);
    VlcVideoBase::State
                   state() { return parent->state; }
    void           setState(VlcVideoBase::State s) { parent->_setState(s); }
    bool           dropFrames() { return parent->dropFrames; }

protected:
    static void *  lockFrame(void *obj, void **plane);
    static void    displayFrame(void *obj, void *picture);

friend class VlcVideoSurface;
};

#endif // VLC_VIDEO_SURFACE_H
