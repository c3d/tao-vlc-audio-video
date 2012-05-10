// ****************************************************************************
//  vlc_video_fullscreen.cpp                                       Tao project
// ****************************************************************************
//
//   File Description:
//
//    Play audio and/or video using libvlc. Video is displayed in a new window
//    in fullscreen mode.
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
#include <QApplication>
#include <QDesktopWidget>
#include <QMainWindow>
#include <vlc/libvlc_media_player.h>
#include <string.h>



VlcVideoFullscreen::VlcVideoFullscreen(QString mediaNameAndOptions)
// ----------------------------------------------------------------------------
//   Initialize a VLC media player to render into a fullscreen window
// ----------------------------------------------------------------------------
    : VlcVideoBase(mediaNameAndOptions), videoWindow(0), videoWidget(0)
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
//   Stop playback
// ----------------------------------------------------------------------------
{
    VlcVideoBase::stop();
    if (videoWindow)
        deleteVideoWindow();
}


void VlcVideoFullscreen::startPlayback()
// ----------------------------------------------------------------------------
//   Configure output format and start playback
// ----------------------------------------------------------------------------
{
    if (!videoWindow)
        createVideoWindow();

    libvlc_media_player_stop(player); // CHECK THIS

    // Show video window full-screen
    videoWindow->setWindowState(videoWindow->windowState() |
                                Qt::WindowFullScreen);
    videoWindow->show();


    // Connect the player to the window
#if   defined (Q_OS_MACX)
    libvlc_media_player_set_nsobject(player, (void*)videoWidget->winId());
#elif defined (Q_OS_UNIX)
    libvlc_media_player_set_xwindow(player, videoWidget->winId());
#elif defined (Q_OS_WIN)
    libvlc_media_player_set_hwnd(player, videoWidget->winId());
#else
#error Don't know how to play fullscreen video with VLC.
#endif

    // Play!
    libvlc_media_player_set_media(player, media);
    libvlc_media_player_play(player);
}


bool VlcVideoFullscreen::createVideoWindow()
// ----------------------------------------------------------------------------
//   Create a fullscreen window to display the video (do not show it yet)
// ----------------------------------------------------------------------------
{
    Q_ASSERT(!videoWindow);
    Q_ASSERT(!videoWidget);

    int s = VlcAudioVideo::tao->screenNumber();
    IFTRACE(video)
        debug() << "Creating video window on screen #" << s << "\n";

    QDesktopWidget * desktop = qApp->desktop();
    QRect geom = desktop->availableGeometry(s);
    videoWindow = new QMainWindow;
    videoWidget = new QWidget;
    videoWindow->setCentralWidget(videoWidget);
    videoWindow->setGeometry(geom);

    return true;
}


void VlcVideoFullscreen::deleteVideoWindow()
// ----------------------------------------------------------------------------
//   Delete fullscreen window
// ----------------------------------------------------------------------------
{
    Q_ASSERT(videoWindow);

    IFTRACE(video)
        debug() << "Deleting fullscreen video window\n";

    // Will also delete videoWidget
    delete videoWindow;
    videoWindow = NULL;
    videoWidget = NULL;
}


std::ostream & VlcVideoFullscreen::debug()
// ----------------------------------------------------------------------------
//   Convenience method to log with a common prefix
// ----------------------------------------------------------------------------
{
    std::cerr << "[VlcVideoFullscreen " << (void*)this << "] ";
    return std::cerr;
}
