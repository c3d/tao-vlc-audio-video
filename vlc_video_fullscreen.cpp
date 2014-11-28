// ****************************************************************************
//  vlc_video_fullscreen.cpp                                       Tao project
// ****************************************************************************
//
//   File Description:
//
//    Play audio and/or video using libvlc. Video is displayed in a new widget.
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

#include "vlc_audio_video.h"
#include "vlc_video_fullscreen.h"
#include "base.h"  // IFTRACE()
#include <QWidget>
#include <QKeyEvent>
#include <QPainter>
#include <vlc/libvlc_media_player.h>



class VideoWidget : public QWidget
// ----------------------------------------------------------------------------
//   Container for the video. Escape key stops playback and closes the widget.
// ----------------------------------------------------------------------------
{
public:
    VideoWidget(VlcVideoFullscreen *vobj)
        : QWidget(), vobj(vobj), painted(false), closing(false)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setAttribute(Qt::WA_NoSystemBackground);
        QPalette p(palette());
        p.setColor(QPalette::Background, Qt::black);
        setPalette(p);
    }

protected:
    virtual void keyPressEvent(QKeyEvent *e)
    {
        if (e->key() == Qt::Key_Escape)
            close();
        else
            QWidget::keyPressEvent(e);
    }

    virtual void closeEvent(QCloseEvent *event)
    {
        closing = true;
        Q_ASSERT(vobj);
        vobj->stop();
        VlcAudioVideo::tao->removeWidget(this);
        event->accept();
    }

    virtual void paintEvent(QPaintEvent *e)
    {
        if (!painted)
        {
            QPainter painter(this);
            painter.fillRect(rect(), palette().background());
            painted = true;
        }
        QWidget::paintEvent(e);
    }

protected:
    VlcVideoFullscreen *vobj;
    bool                painted;

public:
    bool                closing;
};



VlcVideoFullscreen::VlcVideoFullscreen(QString mediaNameAndOptions,
                                       unsigned int, unsigned int,
                                       float, float)
// ----------------------------------------------------------------------------
//   Initialize a VLC media player to render into a dedicated widget
// ----------------------------------------------------------------------------
    : VlcVideoBase(mediaNameAndOptions), videoWidget(0)
{
}


VlcVideoFullscreen::~VlcVideoFullscreen()
// ----------------------------------------------------------------------------
//   Delete video player
// ----------------------------------------------------------------------------
{
    stop();
}


void VlcVideoFullscreen::stop()
// ----------------------------------------------------------------------------
//   Stop playback and close video widget
// ----------------------------------------------------------------------------
{
    VlcVideoBase::stop();
    if (videoWidget && !videoWidget->closing)
        videoWidget->close();
}


void VlcVideoFullscreen::startPlayback()
// ----------------------------------------------------------------------------
//   Create output widget and start playback
// ----------------------------------------------------------------------------
{
    if (!videoWidget)
    {
        IFTRACE(video)
            debug() << "Creating video widget\n";
        videoWidget = new VideoWidget(this);

        IFTRACE(video)
            debug() << "Setting video widget as the main Tao display\n";
        VlcAudioVideo::tao->addWidget(videoWidget);
        VlcAudioVideo::tao->setCurrentWidget(videoWidget);

        // Connect the player to the window
#if   defined (Q_OS_MACX)
        libvlc_media_player_set_nsobject(player, (void*)videoWidget->winId());
#elif defined (Q_OS_UNIX)
        libvlc_media_player_set_xwindow(player, videoWidget->winId());
#elif defined (Q_OS_WIN)
        libvlc_media_player_set_hwnd(player, (void *) videoWidget->winId());
#else
#error "Don't know how to play video into a widget."
#endif
    }

    // Play!
    VlcVideoBase::startPlayback();
}


void VlcVideoFullscreen::exec()
// ----------------------------------------------------------------------------
//   Run state machine in main thread
// ----------------------------------------------------------------------------
{
    VlcVideoBase::exec();

    if (state == VS_PLAY_ENDED)
        stop();
}


std::ostream & VlcVideoFullscreen::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[VlcVideoFullscreen " << (void*)this << "] ";
    return std::cerr;
}
