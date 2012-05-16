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
#include <QPointer>
#include <iostream>

class VideoWidget;


class VlcVideoFullscreen : public VlcVideoBase
// ----------------------------------------------------------------------------
//  Play a fullscreen video
// ----------------------------------------------------------------------------
{
public:
    VlcVideoFullscreen(QString mediaNameAndOptions,
                       unsigned int w = 0, unsigned int h = 0);
    ~VlcVideoFullscreen();

public:
    virtual void   stop();
    virtual void   exec();

protected:
    virtual void   startPlayback();

    std::ostream & debug();

protected:
    QPointer<VideoWidget>  videoWidget;
};

#endif // VLC_VIDEO_FULLSCREEN_H
