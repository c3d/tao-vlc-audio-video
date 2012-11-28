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
#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>
#include <iostream>

class VlcVideoSurface : public VlcVideoBase
{
public:
    VlcVideoSurface(QString mediaNameAndOptions,
                    unsigned int w = 0, unsigned int h = 0,
                    float wscale = -1.0, float hscale = -1.0);
    virtual ~VlcVideoSurface();

public:
    virtual void   stop();
    virtual void   exec();
    GLuint         texture();

public:
    unsigned       w, h;
    float          wscale, hscale;

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
    QMutex                  mutex;  // Protect 'image' and 'updated'
    ImageBuf                image;
    bool                    updated;
    QImage                  converted;
    GLuint                  textureId;
    bool                    videoAvailable;          // In ImageBuf
    bool                    videoAvailableInTexture;
    bool                    descriptionMode;
    const QGLContext      * GLcontext;

    bool                    usePBO;
    GLuint                  pbo[2];
    int                     curPBO;
    GLubyte               * curPBOPtr;
    float                   fps;     // -1: not tested, 0: unknown

protected:
    virtual void   startPlayback();

    void           startGetMediaInfo();
    void           getMediaSubItems();
    std::ostream & debug();
    void           checkGLContext();
    void           genTexture();
    void           genPBO();
    void           transferPBO();
    void           transferNoPBO();
    void           doGLTexImage2D();
    void           displayFrameNoPBO(void *picture);
    void           displayFramePBO(void *picture);

protected:
    static unsigned videoFormat(void **opaque, char *chroma,
                                unsigned *width, unsigned *height,
                                unsigned *pitches,
                                unsigned *lines);
    static void *  lockFrame(void *obj, void **plane);
    static void    displayFrame(void *obj, void *picture);
};

#endif // VLC_VIDEO_SURFACE_H
