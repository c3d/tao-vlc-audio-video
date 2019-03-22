#ifndef VLC_VIDEO_FULLSCREEN_H
#define VLC_VIDEO_FULLSCREEN_H
// *****************************************************************************
// vlc_video_fullscreen.h                                          Tao3D project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2011-2012, Jérôme Forissier <jerome@taodyne.com>
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
                       unsigned int w = 0, unsigned int h = 0,
                       float wscale = -1.0, float hscale = -1.0);
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
