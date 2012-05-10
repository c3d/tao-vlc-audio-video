// ****************************************************************************
//  vlc_video_surface.cpp                                          Tao project
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

#include "vlc_audio_video.h"
#include "vlc_video_surface.h"
#include "base.h"  // IFTRACE()
#include <vlc/libvlc_events.h>
#include <vlc/libvlc_media_list.h>
#include <string.h>
#ifdef Q_OS_WIN32
#include <malloc.h>
#endif



VlcVideoSurface::VlcVideoSurface(QString mediaNameAndOptions,
                                 unsigned int w, unsigned int h)
// ----------------------------------------------------------------------------
//   Initialize a VLC media player to render a video into a texture
// ----------------------------------------------------------------------------
    : VlcVideoBase(mediaNameAndOptions),
      w(w), h(h), updated(false), textureId(0),
      videoAvailable(false), GLcontext(QGLContext::currentContext())
{
    genTexture();
}


VlcVideoSurface::~VlcVideoSurface()
// ----------------------------------------------------------------------------
//   Delete video player
// ----------------------------------------------------------------------------
{
    stop();

    IFTRACE(video)
        debug() << "Deleting texture\n";

    if (textureId)
        glDeleteTextures(1, &textureId);
}


void VlcVideoSurface::stop()
// ----------------------------------------------------------------------------
//   Stop playback
// ----------------------------------------------------------------------------
{
    VlcVideoBase::stop();
    videoAvailable = false;
}


void VlcVideoSurface::exec()
// ----------------------------------------------------------------------------
//   Run state machine in main thread
// ----------------------------------------------------------------------------
{
    switch (state)
    {
    case VS_PLAYING:
    case VS_PAUSED:
    case VS_PLAY_ENDED:
        if (videoAvailable)
        {
            mutex.lock();
            if (updated)
            {
                checkGLContext();
                glBindTexture(GL_TEXTURE_2D, textureId);

                GLenum format = GL_RGBA, type = GL_UNSIGNED_BYTE;
#ifdef Q_OS_MACX
                if (image.chroma == UYVY /* mirrored */)
                {
                    format = GL_YCBCR_422_APPLE;
                    type = GL_UNSIGNED_SHORT_8_8_APPLE;
                }
#endif
                glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, format, type,
                             image.ptr);
                updated = false;
            }
            mutex.unlock();
        }
        break;

    default:
        break;
    }

    VlcVideoBase::exec();
}


void VlcVideoSurface::startPlayback()
// ----------------------------------------------------------------------------
//   Bind vmem callbacks to player and start playback
// ----------------------------------------------------------------------------
{
    libvlc_media_player_stop(player);
    libvlc_video_set_callbacks(player, lockFrame, NULL, displayFrame, this);
    libvlc_video_set_format_callbacks(player, videoFormat, NULL);

    VlcVideoBase::startPlayback();
}


GLuint VlcVideoSurface::texture()
// ----------------------------------------------------------------------------
//   Update texture with current frame and return texture ID
// ----------------------------------------------------------------------------
{
    if (!vlc) // CHECK this
        return 0;
    return videoAvailable ? textureId : 0;
}


void VlcVideoSurface::checkGLContext()
// ----------------------------------------------------------------------------
//   Detect change in current GL context
// ----------------------------------------------------------------------------
{
    const QGLContext * current = QGLContext::currentContext();
    if (current != GLcontext)
    {
        IFTRACE(video)
            debug() << "GL context changed\n";
        genTexture();
        GLcontext = current;
    }
}


void VlcVideoSurface::genTexture()
// ----------------------------------------------------------------------------
//   Create GL texture to render to
// ----------------------------------------------------------------------------
{
    glGenTextures(1, &textureId);
    IFTRACE(video)
        debug() << "Will render to texture #" << textureId << "\n";
}




std::ostream & VlcVideoSurface::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[VlcVideoSurface " << (void*)this << "] ";
    return std::cerr;
}


unsigned VlcVideoSurface::videoFormat(void **opaque, char *chroma,
                                      unsigned *width, unsigned *height,
                                      unsigned *pitches,
                                      unsigned *lines)
// ----------------------------------------------------------------------------
//   Receive video format info from libVLC
// ----------------------------------------------------------------------------
{
    Q_UNUSED(opaque);

    VlcVideoSurface *v = (VlcVideoSurface *)*opaque;
    IFTRACE(video)
        v->debug() << "Native video format:"
                   << " resolution " << *width << "x" << *height
                   << " chroma " << chroma << "\n";

    if (v->w == 0 && v->h == 0)
    {
        v->w = *width;
        v->h = *height;
        IFTRACE(video)
            v->debug() << "Setting texture size to "
                       << v->w << "x" << v->h << "\n";
    }
    else
    {
        *width  = v->w;
        *height = v->h;
        IFTRACE(video)
            v->debug() << "Requesting libVLC scaling to texture size: "
                       << v->w << "x" << v->h << "\n";
    }

    const char * newchroma;

#if defined(Q_OS_MACX)
    newchroma = "UYVY";
    v->image.chroma = UYVY;
    pitches[0] = pitches[1] = pitches[2] = v->w * 2;
    lines  [0] = lines  [1] = lines  [2] = v->h;
    v->image.size = v->w * v->h * 2;
#else
    newchroma = "RV32";
    v->image.chroma = RV32;
    pitches[0] = pitches[1] = pitches[2] = v->w * 4;
    lines  [0] = lines  [1] = lines  [2] = v->h;
    v->image.chroma = RV32;
    v->image.size = v->w * v->h * 4;
#endif

    IFTRACE(video)
        v->debug() << "Requesting " << newchroma << " chroma\n";
    strcpy(chroma, newchroma);

    return 1;
}


void * VlcVideoSurface::lockFrame(void *obj, void **plane)
// ----------------------------------------------------------------------------
//   Allocate video memory
// ----------------------------------------------------------------------------
{
    VlcVideoSurface *v = (VlcVideoSurface *)obj;
    Q_ASSERT(v->image.size);

 #ifdef Q_OS_WIN32
     *plane = __mingw_aligned_malloc(v->image.size, 32);
     if (!*plane)
         throw std::bad_alloc();
 #else
     if (posix_memalign(plane, 32, v->image.size))
         throw std::bad_alloc();
 #endif

    return *plane;
}


void freeFrame(void *picture)
// ----------------------------------------------------------------------------
//   Release video memory
// ----------------------------------------------------------------------------
{
#ifdef Q_OS_WIN32
    __mingw_aligned_free(picture);
#else
    free(picture);
#endif
}


void VlcVideoSurface::displayFrame(void *obj, void *picture)
// ----------------------------------------------------------------------------
//   Copy frame to texture
// ----------------------------------------------------------------------------
{
    VlcVideoSurface *v = (VlcVideoSurface *)obj;
    Q_ASSERT(v->w && v->h && "Invalid video size");

    if (v->state != VS_PLAYING && v->state != VS_PAUSED &&
        v->state != VS_STOPPED)
        v->setState(VS_PLAYING);

#if defined(Q_OS_MACX)
    if (v->image.chroma == UYVY)
    {
        // Hack: here, image is upside-down. To flip it use a QImage with a
        // 16bpp format.
        QImage image((const uchar *)picture, v->w, v->h, QImage::Format_RGB16);
        QImage converted = image.mirrored();
        freeFrame(picture);
        v->mutex.lock();
        v->image.converted = converted;
        v->image.ptr = v->image.converted.bits();
    }
    else
#endif
    {
        QImage image((const uchar *)picture, v->w, v->h, QImage::Format_RGB32);
        QImage converted = QGLWidget::convertToGLFormat(image);
        freeFrame(picture);
        v->mutex.lock();
        v->image.converted = converted;
        v->image.ptr = v->image.converted.bits();
    }
    v->updated = true;
    v->mutex.unlock();
    v->videoAvailable = true;
}
