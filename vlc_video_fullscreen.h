#ifndef VLC_VIDEO_FULLSCREEN_H
#define VLC_VIDEO_FULLSCREEN_H
// ****************************************************************************
//  vlc_video_fullscreen.h                                         Tao project
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

#include "vlc_video_base.h"
//#include <QString>
//#include <QStringList>
//#include <QMutex>
//#include <QImage>
//#include <vlc/libvlc.h>
//#include <vlc/libvlc_media.h>
//#include <vlc/libvlc_media_player.h>
#include <iostream>

QT_BEGIN_NAMESPACE
class QMainWindow;
class QWidget;
QT_END_NAMESPACE


class VlcVideoFullscreen : public VlcVideoBase
// ----------------------------------------------------------------------------
//  Play a fullscreen video
// ----------------------------------------------------------------------------
{
public:
    VlcVideoFullscreen(QString mediaNameAndOptions);
    ~VlcVideoFullscreen();

public:
    virtual void   stop();

protected:
    virtual void   startPlayback();
    std::ostream & debug();
    bool           createVideoWindow();
    void           deleteVideoWindow();

protected:
    QMainWindow *  videoWindow;
    QWidget *      videoWidget;
};

#endif // VLC_VIDEO_FULLSCREEN_H
