# ******************************************************************************
#  vlc_audio_video.pro                                              Tao project
# ******************************************************************************
# File Description:
# Qt build file for the VLC Audio Video module
# Requires VLC media player >= 1.1
# ******************************************************************************
# (C) 2011 Taodyne SAS <contact@taodyne.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
# ******************************************************************************

isEmpty(VLC) {
  # Default location for the VLC SDK
  macx:VLC=/Applications/VLC.app/Contents/MacOS
  win32:VLC=../../../VLC/sdk
  linux-g++*:VLC=/usr
}


!exists($$VLC/include/vlc/libvlc.h) {
  !build_pass {
    message("$$VLC/include/vlc/libvlc.h not found")
    message()
    message("To build the VLCAudioVideo module, I need the VLC media player >= 1.1.x")

    linux-g++* {
      message(Please install the libvlc-dev package.)
      message(See http://nightlies.videolan.org/)
    } else {
      message("Please install the VLC media player from:")
      message("http://www.videolan.org/vlc/")
      message("If you have extracted the VLC SDK somewhere else than the default")
      message("installation directory, you may set the VLC variable.")
      message(For instance:)
      message([MacOSX] ./configure VLC=/Users/jerome/Desktop/vlc-1.1.12/VLC.app/Contents/MacOS)
      message([Windows] ./configure VLC=/c/Users/Jerome/Desktop/vlc-1.1.11/sdk)
    }
    message()
    message(*** THE VLCAudioVideo MODULE WILL NOT BE BUILT ***)
  }
} else {

  MODINSTDIR = vlc_audio_video

  include(../modules.pri)

  DEFINES    += GLEW_STATIC
  HEADERS     = vlc_audio_video.h vlc_video_surface.h vlc_preferences.h
  SOURCES     = vlc_audio_video.cpp vlc_video_surface.cpp vlc_preferences.cpp
  !macx:SOURCES += $${TAOTOPSRC}/tao/include/tao/GL/glew.c
  TBL_SOURCES = vlc_audio_video.tbl
  OTHER_FILES = vlc_audio_video.xl vlc_audio_video.tbl traces.tbl

  QT       += core gui opengl

  INCLUDEPATH += "$${VLC}/include"
  LIBS += -L"$${VLC}/lib" -lvlc -lvlccore
  macx:LIBS += -framework AppKit

  # Icon: http://en.wikipedia.org/wiki/File:VLC_Icon.svg
  # License: CC BY-SA 3.0
  INSTALLS += thismod_icon

  macx {
    # Install will create <module>/lib/{lib,plugins,share/lua}
    vlc_libs.path = $${MODINSTPATH}/lib
    vlc_libs.files = "$${VLC}/lib"
    vlc_plugins.path  = $${MODINSTPATH}/lib
    vlc_plugins.files = "$${VLC}/plugins"
    vlc_lua.path = $${MODINSTPATH}/lib/share
    vlc_lua.files = "$${VLC}/share/lua"
  }
  linux-g++* {
    # Install will create <module>/lib/vlc/{plugins,lua}
    vlc_libs.path = $${MODINSTPATH}/lib
    vlc_libs.files = "$${VLC}/lib/libvlc*.so.*"
    vlc_plugins.path  = $${MODINSTPATH}/lib/vlc
    vlc_plugins.files = "$${VLC}/lib/vlc/plugins" "$${VLC}/lib/vlc/vlc-cache-gen"
    vlc_lua.path = $${MODINSTPATH}/lib/vlc
    vlc_lua.files = "$${VLC}/lib/vlc/lua"
  }
  win32 {
    # Install will create <module>/lib/{plugins,lua}
    vlc_libs.path = $${MODINSTPATH}/lib
    vlc_libs.files = "$${VLC}/../*.dll" "$${VLC}/../vlc-cache-gen.exe"
    vlc_plugins.path  = $${MODINSTPATH}/lib
    vlc_plugins.files = "$${VLC}/../plugins"
    vlc_lua.path  = $${MODINSTPATH}/lib
    vlc_lua.files = "$${VLC}/../lua"
  }
  INSTALLS += vlc_libs vlc_plugins vlc_lua

  LICENSE_FILES = vlc_audio_video.taokey.notsigned
  exists(../licenses.pri):include(../licenses.pri)
}
