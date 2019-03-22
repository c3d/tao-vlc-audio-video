// *****************************************************************************
// vlc_video_surface.cpp                                           Tao3D project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2012-2013, Baptiste Soulisse <baptiste.soulisse@taodyne.com>
// (C) 2011,2013-2015,2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2011-2014, Jérôme Forissier <jerome@taodyne.com>
// (C) 2012-2013, Baptiste Soulisse <baptiste.soulisse@taodyne.com>
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

#include "tao/graphic_state.h"
#include "tao/tao_gl.h"
#include "vlc_audio_video.h"
#include "vlc_video_surface.h"
#include "base.h"  // IFTRACE()
#include <QApplication>  // qApp
#include <QMutexLocker>
#include <vlc/libvlc_events.h>
#include <vlc/libvlc_media_list.h>
#include <string.h>
#ifdef Q_OS_WIN32
#include <malloc.h>
#endif

DLL_PUBLIC Tao::GraphicState * graphic_state = NULL;

// REVISIT: Deprecated, but currently used to set audio delay in PBO case
extern "C"
float libvlc_media_player_get_fps( libvlc_media_player_t *p_mi );


VlcVideoSurface::VlcVideoSurface(QString mediaNameAndOptions,
                                 unsigned int w, unsigned int h,
                                 float wscale, float hscale)
// ----------------------------------------------------------------------------
//   Initialize a VLC media player to render a video into a texture
// ----------------------------------------------------------------------------
    : VlcVideoBase(mediaNameAndOptions),
      w(w), h(h), wscale(wscale), hscale(hscale), vtId(-1), nextVtId(0),
      usePBO(QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_2_1),
      dropFrames(false)
{
    if (getenv("TAO_VLC_NO_PBO"))
        usePBO = false;
    if (!GL.HasBuffers())
        usePBO = false;
    IFTRACE(video)
        debug() << "Will " << (char*)(usePBO ? "" : "not ") << "use PBOs\n";
}


VlcVideoSurface::~VlcVideoSurface()
// ----------------------------------------------------------------------------
//   Delete video player
// ----------------------------------------------------------------------------
{
    stop();

    foreach(VideoTrack *t, videoTracks)
        t->unref();
}


void VlcVideoSurface::stop()
// ----------------------------------------------------------------------------
//   Stop playback
// ----------------------------------------------------------------------------
{
    VlcVideoBase::stop();

    foreach(VideoTrack *t, videoTracks)
        t->stop();

    // videoFormat() callback will be called again if playback is resumed
    vtId = -1;
    nextVtId = 0;
}


// This is actually deprecated now
LIBVLC_API float libvlc_media_player_get_fps( libvlc_media_player_t *p_mi );


void VlcVideoSurface::exec()
// ----------------------------------------------------------------------------
//   Run state machine in main thread
// ----------------------------------------------------------------------------
{
    switch (state)
    {
    case VS_PLAYING:
        // REVISIT: this cannot make sense when media has multiple streams
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
        updateTexture();
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
    libvlc_video_set_callbacks(player, VideoTrack::lockFrame, NULL,
                               VideoTrack::displayFrame, this);
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
    VlcVideoSurface *s = (VlcVideoSurface *)*opaque;

#ifdef VLC_HAS_TRACK_ID
    int es_id = libvlc_video_format_cb_get_track_id(opaque);
#else
    int es_id = -1;
#endif

    // When there are multiple video streams in the container, this function is
    // called once per stream from multiple threads
    QMutexLocker(&s->mutex);

    IFTRACE(video)
        s->debug() << "Stream " << es_id << " native video format: "
                   << *width << "x" << *height << " chroma "
                   << chroma << "\n";

    int id = s->newTrack(es_id);
    if (id < 0)
        return 0;


    VideoTrack *v = s->videoTracks[id];

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
            s->debug() << "Will use texture size "
                       << v->w << "x" << v->h << "\n";
    }
    else
    {
        *width  = v->w;
        *height = v->h;
        IFTRACE(video)
            s->debug() << "Requesting libVLC scaling to texture size: "
                       << v->w << "x" << v->h << "\n";
    }

    const char * newchroma;

#if defined(Q_OS_MACX)
    bool useUYVY = (getenv("TAO_VLC_RV32") == NULL);
    if (useUYVY)
    {
        newchroma = "UYVY";
        v->image.chroma = VideoTrack::UYVY;
        pitches[0] = pitches[1] = pitches[2] = v->w * 2;
        lines  [0] = lines  [1] = lines  [2] = v->h;
        v->image.size = v->w * v->h * 2;
    }
    else
#endif
    {
        newchroma = "RV32";
        v->image.chroma = VideoTrack::RV32;
        pitches[0] = pitches[1] = pitches[2] = v->w * 4;
        lines  [0] = lines  [1] = lines  [2] = v->h;
        v->image.size = v->w * v->h * 4;
    }

    IFTRACE(video)
        s->debug() << "Requesting " << newchroma << " chroma\n";
    strcpy(chroma, newchroma);

    *opaque = (void *)v;
    return 1;
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


VideoTrack * VlcVideoSurface::currentVideoTrack()
// ----------------------------------------------------------------------------
//   Return container for current video track or NULL
// ----------------------------------------------------------------------------
{
    return vtId >= 0 ? videoTracks[vtId] : NULL;
}


bool VlcVideoSurface::setVideoTrack(int id)
// ----------------------------------------------------------------------------
//   Select the current video track (affects texture(), width(), height())
// ----------------------------------------------------------------------------
{
    if (!videoTracks.contains(id))
        return false;
    vtId = id;
    return true;
}


unsigned VlcVideoSurface::width()
// ----------------------------------------------------------------------------
//   Return width of current video track in pixels
// ----------------------------------------------------------------------------
{
    VideoTrack * t = currentVideoTrack();
    return t ? t->width() : 0;
}


unsigned VlcVideoSurface::height()
// ----------------------------------------------------------------------------
//   Return height of current video track in pixels
// ----------------------------------------------------------------------------
{
    VideoTrack * t = currentVideoTrack();
    return t ? t->height() : 0;
}


GLuint VlcVideoSurface::texture()
// ----------------------------------------------------------------------------
//   Return ID of OpenGL texture into which current video track is output
// ----------------------------------------------------------------------------
{
    VideoTrack * t = currentVideoTrack();
    return t ? t->texture() : 0;
}


void VlcVideoSurface::updateTexture()
// ----------------------------------------------------------------------------
//   Make sure latest picture from current video track is available in texture
// ----------------------------------------------------------------------------
{
    VideoTrack * t = currentVideoTrack();
    if (t)
        t->updateTexture();
}


int VlcVideoSurface::newTrack(int es_id)
// ----------------------------------------------------------------------------
//   Allocate a new VideoTrack object and return its id or -1 on error
// ----------------------------------------------------------------------------
{
    int id = es_id;
    if (id < 0 || videoTracks.contains(id))
        id = nextVtId++;
    videoTracks[id] = new VideoTrack(this, id);
    if (vtId == -1)
    {
        // This is the first track: make it current
        vtId = id;
    }
    return id;
}



// ============================================================================
//
//   VideoTrack
//
// ============================================================================

VideoTrack::VideoTrack(VlcVideoSurface *parent, unsigned id)
// ----------------------------------------------------------------------------
//   Individual video track in a multistream file
// ----------------------------------------------------------------------------
    : parent(parent), id(id),
      w(parent->w), h(parent->h),
      wscale(parent->wscale), hscale(parent->hscale),
      textureId(0),
      updated(false),
      videoAvailable(false), videoAvailableInTexture(false),
      usePBO(parent->usePBO),
      GLcontext(NULL),
      curPBO(0), curPBOPtr(NULL), refs(1), frameTime(-1)
{
    IFTRACE(video)
        debug() << "Creation\n";
    pbo[0] = pbo[1] = 0;
    // Note: initialization of GL resource is left to checkGLContext()
    // because this constructor is usually not called from the main thread
}


VideoTrack::~VideoTrack()
// ----------------------------------------------------------------------------
//    Video track destructor
// ----------------------------------------------------------------------------
{
    IFTRACE(video)
        debug() << "Deleting texture\n";

    if (textureId)
        GL.DeleteTextures(1, &textureId);

    if (usePBO)
    {
        IFTRACE(video)
            debug() << "Deleting PBOs\n";
        GL.DeleteBuffers(2, pbo);
    }

    IFTRACE(video)
        if (!allocatedFrames.empty())
            debug() << "Freeing " << allocatedFrames.size() << " frame(s)\n";
    foreach (void *frame, allocatedFrames)
        freeFrame(frame);
}


std::ostream & VideoTrack::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[VideoTrack " << id << " " << (void*)this << "] ";
    return std::cerr;
}




void VideoTrack::render_callback(void *arg)
// ----------------------------------------------------------------------------
//   Rendering callback: call the render function for the object
// ----------------------------------------------------------------------------
{
    ((VideoTrack *)arg)->Draw();
}


void VideoTrack::identify_callback(void *arg)
// ----------------------------------------------------------------------------
//   Identify callback: don't do anything
// ----------------------------------------------------------------------------
{
    Q_UNUSED(arg);
}


void VideoTrack::delete_callback(void *arg)
// ----------------------------------------------------------------------------
//   Object is not used anymore by rendering code
// ----------------------------------------------------------------------------
{
    VideoTrack * t = (VideoTrack *)arg;
    t->unref();
}


void VideoTrack::Draw()
// ----------------------------------------------------------------------------
//   Draw video texture
// ----------------------------------------------------------------------------
{
    // Bind Texture
    GL.Enable(GL_TEXTURE_2D);
    GL.BindTexture(GL_TEXTURE_2D, texture());

    // We don't want to use Tao preferences so we
    // have to set texture settings
    GL.TexParameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GL.TexParameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GL.TexParameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL.TexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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
    // Adapted from Qt source code: qglcpp
    // (License: LGPL)

    XL_ASSERT(src.depth() == 32);
    XL_ASSERT(dst.depth() == 32);

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


void VideoTrack::transferPBO()
// ----------------------------------------------------------------------------
//   PBO update and GL texture transfer
// ----------------------------------------------------------------------------
{
    XL_ASSERT(image.ptr);
    XL_ASSERT(image.size);

    checkGLContext();

    bool firstFrame = (curPBOPtr == (GLubyte *)1);

    // Assure we save and restore settings to avoid
    // conflict with Tao GL states
    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

    // Copy and convert at the same time the latest picture into the current
    // PBO
    GL.BindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[curPBO]);
    curPBOPtr = (GLubyte*) GL.MapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if (!curPBOPtr)
    {
        curPBOPtr = (GLubyte*)1;
        glPopClientAttrib();
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
    GL.UnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    // Copy from previous PBO to texture

    if (!firstFrame)
    {
        GL.BindTexture(GL_TEXTURE_2D, textureId);
        GL.BindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[1-curPBO]);
        doGLTexImage2D();
    }

    // Restore saved settings
    glPopClientAttrib();

    curPBO = 1 - curPBO;

    if (parent)
        frameTime = parent->updateTime(frameTime);
}


void VideoTrack::transferNoPBO()
// ----------------------------------------------------------------------------
//   Normal GL texture transfer
// ----------------------------------------------------------------------------
{
    checkGLContext();

    GL.BindTexture(GL_TEXTURE_2D, textureId);
    doGLTexImage2D();

    if (parent)
        frameTime = parent->updateTime(frameTime);
}


void VideoTrack::doGLTexImage2D()
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
            GL.PixelStorei(GL_UNPACK_ALIGNMENT, 2);
        }
    }
#endif
    GL.TexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, format, type,
                 usePBO ? NULL : image.ptr);
    videoAvailableInTexture = true;
}


void VideoTrack::updateTexture()
// ----------------------------------------------------------------------------
//   Update the texture in a thread-safe way
// ----------------------------------------------------------------------------
{
    if (videoAvailable)
    {
        mutex.lock();
        if (updated)
        {
            if (usePBO)
                transferPBO();
            else
                transferNoPBO();
            updated = false;
        }
        mutex.unlock();
    }
}


GLuint VideoTrack::texture()
// ----------------------------------------------------------------------------
//   Update texture with current frame and return texture ID
// ----------------------------------------------------------------------------
{
    return videoAvailableInTexture ? textureId : 0;
}


void VideoTrack::checkGLContext()
// ----------------------------------------------------------------------------
//   Detect change in current GL context.
// ----------------------------------------------------------------------------
{
    const QGLContext * current = QGLContext::currentContext();
    if (current != GLcontext)
    {
        IFTRACE(video)
            if (GLcontext)
                debug() << "GL context changed\n";
        genTexture();
        if (usePBO && image.size)
            genPBO();
        GLcontext = current;
    }
}


void VideoTrack::genTexture()
// ----------------------------------------------------------------------------
//   Create GL texture to render to
// ----------------------------------------------------------------------------
{
    GL.GenTextures(1, &textureId);
    IFTRACE(video)
        debug() << "Texture allocated: #" << textureId << "\n";
}


void VideoTrack::genPBO()
// ----------------------------------------------------------------------------
//   Create two GL Pixel Buffer Objects for asynchronous transfer to texture
// ----------------------------------------------------------------------------
{
    XL_ASSERT(image.size);
    GLuint t = GL_PIXEL_UNPACK_BUFFER;

    // Assure we save and restore settings to avoid
    // conflict with Tao GL states
    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
    GL.GenBuffers(2, pbo);
    GL.BindBuffer(t, pbo[0]);
    GL.BufferData(t, image.size, NULL, GL_STREAM_DRAW);
    GL.BindBuffer(t, pbo[1]);
    GL.BufferData(t, image.size, NULL, GL_STREAM_DRAW);
    curPBOPtr = (GLubyte *)1; // REVISIT?
    curPBO = 1;

    // Restore saved settings
    glPopClientAttrib();

    IFTRACE(video)
        debug() << "PBOs and buffers allocated: #"
                << pbo[0] << ", #" << pbo[1] << "\n";
}


void VideoTrack::stop()
{
    videoAvailable = false;
    videoAvailableInTexture = false;
}



void * VideoTrack::lockFrame(void *obj, void **plane)
// ----------------------------------------------------------------------------
//   Allocate video memory
// ----------------------------------------------------------------------------
{
    VideoTrack *v = (VideoTrack *)obj;
    XL_ASSERT(v->image.size);

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


void VideoTrack::displayFrame(void *obj, void *picture)
// ----------------------------------------------------------------------------
//   Copy frame to texture
// ----------------------------------------------------------------------------
{
    VideoTrack *v = (VideoTrack *)obj;
    XL_ASSERT(v->w && v->h && "Invalid video size");

    if (v->dropFrames())
    {
        v->freeFrame(picture);
        return;
    }

    if (v->state() != VlcVideoBase::VS_PLAYING &&
        v->state() != VlcVideoBase::VS_PAUSED &&
        v->state() != VlcVideoBase::VS_STOPPED)
        v->setState(VlcVideoBase::VS_PLAYING);

    if (v->usePBO)
        v->displayFramePBO(picture);
    else
        v->displayFrameNoPBO(picture);

}


void VideoTrack::freeFrame(void *picture)
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


void VideoTrack::displayFrameNoPBO(void *picture)
// ----------------------------------------------------------------------------
//   Prepare image pointer when Pixel Buffer Objects are NOT enabled
// ----------------------------------------------------------------------------
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


void VideoTrack::displayFramePBO(void *picture)
// ----------------------------------------------------------------------------
//   Prepare image pointer when Pixel Buffer Objects are enabled
// ----------------------------------------------------------------------------
{
    mutex.lock();
    void * prev = image.ptr;
    image.ptr = picture;
    updated = true;
    videoAvailable = true;
    mutex.unlock();

    freeFrame(prev);
}
