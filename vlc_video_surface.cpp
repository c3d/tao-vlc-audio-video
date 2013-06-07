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

#include "tao/tao_gl.h"
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
                                 unsigned int w, unsigned int h,
                                 float wscale, float hscale)
// ----------------------------------------------------------------------------
//   Initialize a VLC media player to render a video into a texture
// ----------------------------------------------------------------------------
    : VlcVideoBase(mediaNameAndOptions),
      w(w), h(h), wscale(wscale), hscale(hscale),
      textureId(0),
      updated(false), 
      videoAvailable(false), videoAvailableInTexture(false),
      usePBO(QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_2_1),
      curPBO(0), curPBOPtr(NULL), fps(-1.0), dropFrames(false)
{
    genTexture();
    if (getenv("TAO_VLC_NO_PBO"))
        usePBO = false;
#ifdef WIN32
    if (glGenBuffers == NULL || glBufferData == NULL || glMapBuffer == NULL ||
        glBindBuffer == NULL)
        usePBO = false;
#endif
    IFTRACE(video)
        debug() << "Will " << (char*)(usePBO ? "" : "not ") << "use PBOs\n";
    pbo[0] = pbo[1] = 0;
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

    if (usePBO)
    {
        IFTRACE(video)
            debug() << "Deleting PBOs\n";
        glDeleteBuffers(2, pbo);
    }

    IFTRACE(video)
        if (!allocatedFrames.empty())
            debug() << "Freeing " << allocatedFrames.size() << " frame(s)\n";
    foreach (void *frame, allocatedFrames)
        freeFrame(frame);
}


void VlcVideoSurface::stop()
// ----------------------------------------------------------------------------
//   Stop playback
// ----------------------------------------------------------------------------
{
    VlcVideoBase::stop();
    videoAvailable = false;
    videoAvailableInTexture = false;
}


#if defined(Q_OS_MACX)
static void verticalFlip16(void *to, const void *from, int w, int h)
// ----------------------------------------------------------------------------
//   Flip 16-bit image vertically
// ----------------------------------------------------------------------------
{
    // Adapted from Qt source code: qimage.cpp
    // (License: LGPL)

    int dy = h-1;
    for (int sy = 0; sy < h; sy++, dy--)
    {
        quint16* ssl = (quint16*)((quint8*)from + sy*w*2);
        quint16* dsl = (quint16*)((quint8*)to + dy*w*2);
        int dx = 0;
        for (int sx = 0; sx < w; sx++, dx++)
            dsl[dx] = ssl[sx];
    }
}
#endif


static void convertToGLFormat(QImage &dst, const QImage &src)
// ----------------------------------------------------------------------------
//   Convert from QImage::Format_RGB32 to GL_RGBA
// ----------------------------------------------------------------------------
{
    // Adapted from Qt source code: qgl.cpp
    // (License: LGPL)

    Q_ASSERT(src.depth() == 32);
    Q_ASSERT(dst.depth() == 32);

    const int width = src.width();
    const int height = src.height();
    const uint *p = (const uint*) src.scanLine(src.height() - 1);
    uint *q = (uint*) dst.scanLine(0);

    if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
    {
        for (int i=0; i < height; ++i)
        {
            const uint *end = p + width;
            while (p < end)
            {
                *q = (*p << 8) | ((*p >> 24) & 0xff);
                p++;
                q++;
            }
            p -= 2 * width;
        }
    }
    else
    {
        for (int i=0; i < height; ++i)
        {
            const uint *end = p + width;
            while (p < end)
            {
                *q = ((*p << 16) & 0xff0000) | ((*p >> 16) & 0xff)
                                             | (*p & 0xff00ff00);
                p++;
                q++;
            }
            p -= 2 * width;
        }
    }
}


void VlcVideoSurface::transferPBO()
// ----------------------------------------------------------------------------
//   PBO update and GL texture transfer
// ----------------------------------------------------------------------------
{
    Q_ASSERT(image.ptr);
    Q_ASSERT(image.size);

    checkGLContext();

    bool firstFrame = (curPBOPtr == (GLubyte *)1);

    // Copy and convert at the same time the latest picture into the current
    // PBO

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[curPBO]);
    curPBOPtr = (GLubyte*) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if (!curPBOPtr)
    {
        curPBOPtr = (GLubyte*)1;
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        return;
    }
#if defined(Q_OS_MACX)
    if (image.chroma == UYVY)
    {
        verticalFlip16(curPBOPtr, image.ptr, w, h);
    }
    else
#endif
    {
        QImage from((const uchar *)image.ptr, w, h, QImage::Format_RGB32);
        QImage to((uchar *)curPBOPtr, w, h, QImage::Format_RGB32);
        convertToGLFormat(to, from);
    }
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // Copy from previous PBO to texture

    if (!firstFrame)
    {
        glBindTexture(GL_TEXTURE_2D, textureId);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[1-curPBO]);
        doGLTexImage2D();
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        videoAvailableInTexture = true;
    }

    curPBO = 1 - curPBO;
    if (fps > 0)
        frameTime += lastRate / fps;
}


void VlcVideoSurface::transferNoPBO()
// ----------------------------------------------------------------------------
//   Normal GL texture transfer
// ----------------------------------------------------------------------------
{
    checkGLContext();
    glBindTexture(GL_TEXTURE_2D, textureId);
    doGLTexImage2D();
    glBindTexture(GL_TEXTURE_2D, 0);
    videoAvailableInTexture = true;
    if (fps > 0)
        frameTime += lastRate / fps;
}


void VlcVideoSurface::doGLTexImage2D()
// ----------------------------------------------------------------------------
//   GL texture transfer
// ----------------------------------------------------------------------------
{
    GLenum format = GL_RGBA, type = GL_UNSIGNED_BYTE;
#ifdef Q_OS_MACX
    if (image.chroma == UYVY /* mirrored */)
    {
        format = GL_YCBCR_422_APPLE;
        type = GL_UNSIGNED_SHORT_8_8_APPLE; // 2 bytes per pixel
        if (w % 2)
        {
            // Row size in bytes is w * 2, which is not a multiple of 4
            // (the default value for GL_UNPACK_ALIGNEMENT) when w is odd.
            glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
        }
    }
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, format, type,
                 usePBO ? NULL : image.ptr);
}

void VlcVideoSurface::exec()
// ----------------------------------------------------------------------------
//   Run state machine in main thread
// ----------------------------------------------------------------------------
{
    switch (state)
    {
    case VS_PLAYING:
        if (usePBO && fps == -1.0)
        {
            fps = libvlc_media_player_get_fps(player);
            if (usePBO)
            {
                if (fps)
                {
                    int64_t delay = 1000000/fps;
                    IFTRACE(video)
                    {
                        debug() << "FPS: " << fps << "\n";
                        debug() << "Compensating for PBO ping-pong delay: "
                                << delay << " us\n";
                    }
                    libvlc_audio_set_delay(player, delay);
                }
                else
                {
                    IFTRACE(video)
                        debug() << "Unknown FPS - "
                                   "won't compensate for PBO ping-pong delay\n";
                }
            }
        }
        // FALL THROUGH
    case VS_PAUSED:
    case VS_PLAY_ENDED:
        // CHECK: videoAvailable should be accessed under lock
        if (videoAvailable)
        {
            mutex.lock();
            if (updated)
            {
                if (usePBO)
                {
                    if (!curPBOPtr && image.size)
                        genPBO();
                    transferPBO();
                }
                else
                {
                    transferNoPBO();
                }
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
    libvlc_video_set_callbacks(player, lockFrame, NULL, displayFrame, this);
    libvlc_video_set_format_callbacks(player, videoFormat, NULL);

    // #3026 workaround VLC bug
    // When start-time is non-zero, VLC shows one or more frames that belong to
    // the beginning of the video before actually showing the frames that are
    // (approximately) at start-time.
    // With this code I could get rid of the unwanted pictures with all the
    // videos I tested.
    foreach(char *opt, mediaOptions)
    {
        QString option(opt);
        if (option.startsWith("start-time="))
        {
            bool ok = false;
            double start = option.mid(option.indexOf("=") + 1).toDouble(&ok);
            if (ok && start != 0.0)
            {
                if (!dropFrames)
                {
                    IFTRACE(video)
                        debug() << "start-time != 0: drop frames until "
                                   "MediaPlayerTimeChanged\n";
                    dropFrames = true;
                    libvlc_event_attach(pevm,
                                        libvlc_MediaPlayerTimeChanged,
                                        playerTimeChanged, this);
                }
            }
        }
    }

    VlcVideoBase::startPlayback();
}


GLuint VlcVideoSurface::texture()
// ----------------------------------------------------------------------------
//   Update texture with current frame and return texture ID
// ----------------------------------------------------------------------------
{
    if (!vlc) // CHECK this
        return 0;
    return videoAvailableInTexture ? textureId : 0;
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
        if (image.size)
            genPBO();
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


void VlcVideoSurface::genPBO()
// ----------------------------------------------------------------------------
//   Create two GL Pixel Buffer Objects for asynchronous transfer to texture
// ----------------------------------------------------------------------------
{
    Q_ASSERT(image.size);

    GLuint t = GL_PIXEL_UNPACK_BUFFER;

    glGenBuffers(2, pbo);
    glBindBuffer(t, pbo[0]);
    glBufferData(t, image.size, NULL, GL_STREAM_DRAW);
    glBindBuffer(t, pbo[1]);
    glBufferData(t, image.size, NULL, GL_STREAM_DRAW);
    curPBOPtr = (GLubyte *)1; // REVISIT?
    curPBO = 1;
    glBindBuffer(t, 0);

    IFTRACE(video)
        debug() << "PBOs and buffers allocated: #"
                << pbo[0] << ", #" << pbo[1] << "\n";
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

    if (v->wscale != -1.0 && v->hscale != 1.0)
    {
        IFTRACE(video)
            v->debug() << "Relative texture size requested: ("
                       << 100 * v->wscale << "%)x("
                       << 100 * v->hscale << "%)\n";
        v->w = *width  * v->wscale;
        v->h = *height * v->hscale;
    }
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
    bool useUYVY = (getenv("TAO_VLC_RV32") == NULL);
    if (useUYVY)
    {
        newchroma = "UYVY";
        v->image.chroma = UYVY;
        pitches[0] = pitches[1] = pitches[2] = v->w * 2;
        lines  [0] = lines  [1] = lines  [2] = v->h;
        v->image.size = v->w * v->h * 2;
    }
    else
#endif
    {
        newchroma = "RV32";
        v->image.chroma = RV32;
        pitches[0] = pitches[1] = pitches[2] = v->w * 4;
        lines  [0] = lines  [1] = lines  [2] = v->h;
        v->image.chroma = RV32;
        v->image.size = v->w * v->h * 4;
    }

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

    v->allocatedFrames.insert(*plane);
    return *plane;
}


void VlcVideoSurface::freeFrame(void *picture)
// ----------------------------------------------------------------------------
//   Release video memory
// ----------------------------------------------------------------------------
{
#ifdef Q_OS_WIN32
    __mingw_aligned_free(picture);
#else
    free(picture);
#endif

    allocatedFrames.remove(picture);
}


void VlcVideoSurface::displayFrame(void *obj, void *picture)
// ----------------------------------------------------------------------------
//   Copy frame to texture
// ----------------------------------------------------------------------------
{
    VlcVideoSurface *v = (VlcVideoSurface *)obj;
    Q_ASSERT(v->w && v->h && "Invalid video size");

    if (v->dropFrames)
    {
        v->freeFrame(picture);
        return;
    }

    if (v->state != VS_PLAYING && v->state != VS_PAUSED &&
        v->state != VS_STOPPED)
        v->setState(VS_PLAYING);

    if (v->usePBO)
        v->displayFramePBO(picture);
    else
        v->displayFrameNoPBO(picture);

}


void VlcVideoSurface::displayFrameNoPBO(void *picture)
{
#if defined(Q_OS_MACX)
    if (image.chroma == UYVY)
    {
        // Hack: here, image is upside-down. To flip it use a QImage with a
        // 16bpp format.
        QImage image((const uchar *)picture, w, h, QImage::Format_RGB16);
        QImage converted = image.mirrored();
        freeFrame(picture);
        mutex.lock();
        this->image.converted = converted;
        this->image.ptr = this->image.converted.bits();
    }
    else
#endif
    {
        QImage image((const uchar *)picture, w, h, QImage::Format_RGB32);
        QImage converted = QGLWidget::convertToGLFormat(image);
        freeFrame(picture);
        mutex.lock();
        this->image.converted = converted;
        this->image.ptr = this->image.converted.bits();
    }
    updated = true;
    mutex.unlock();
    videoAvailable = true;
}


void VlcVideoSurface::displayFramePBO(void *picture)
{
    mutex.lock();
    void * prev = image.ptr;
    image.ptr = picture;
    updated = true;
    videoAvailable = true;
    mutex.unlock();

    freeFrame(prev);
}


void VlcVideoSurface::playerTimeChanged(const libvlc_event_t *, void *obj)
// ----------------------------------------------------------------------------
//   Callback for libvlc_MediaPlayerTimeChanged event
// ----------------------------------------------------------------------------
{
    VlcVideoSurface *v = (VlcVideoSurface *)obj;
    IFTRACE(video)
        v->debug() << "MediaPlayerTimeChanged: stop dropping frames\n";
    v->dropFrames = false;
    libvlc_event_detach(v->pevm,
                        libvlc_MediaPlayerTimeChanged,
                        playerTimeChanged, v);
}
