# ******************************************************************************
#  vlc_audio_video.pro                                              Tao project
# ******************************************************************************
# File Description:
# Qt build file for the VLC Audio Video module
# Requires VLC >= 1.2.0
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
  # Default location for VLC SDK
  VLC_TOP=$$PWD/../../../vlc
  macx {
    contains(CONFIG, x86) {
      VLC_TOP=$${VLC_TOP}_x86
    } else {
      VLC_TOP=$${VLC_TOP}_x86_64
    }
    VLC=$$VLC_TOP/VLC.app/Contents/MacOS
  }
  win32:VLC=$$VLC_TOP/sdk
  linux-g++*:VLC=$$VLC_TOP

  !exists($$VLC/include/vlc/libvlc.h) {
    VLC=
    message(VLC was not found in the default location: $$VLC_TOP)
  }
}

!build_pass:isEmpty(VLC) {

  message()
  message("To build the VLCAudioVideo module, I need VLC >= 1.2.0")
  message("If you have the VLC SDK, please set the VLC variable during qmake.")
  message(For instance:)
  message([MacOSX] qmake <options> VLC=/Users/jerome/Desktop/vlc-1.2.0-git/VLC.app/Contents/MacOS)
  message([Windows] qmake <options> VLC=/c/Users/Jerome/Desktop/vlc-1.2.0-git-20111024-0002/sdk)
  message("'VLC' must point to a directory with include/, lib/, plugins/")
  message(You may download binaries from:)
  message(http://nightlies.videolan.org/build/macosx-intel/)
  message(http://nightlies.videolan.org/build/win32/last/)
  message()
  message(*** THE VLCAudioVideo MODULE WILL NOT BE BUILT ***)

} else {

  !exists($$VLC/include/vlc/libvlc.h):error(VLC files not found under $$VLC)

  MODINSTDIR = vlc_audio_video

  include(../modules.pri)

  DEFINES    += GLEW_STATIC
  HEADERS     = vlc_audio_video.h vlc_video_surface.h vlc_preferences.h
  SOURCES     = vlc_audio_video.cpp vlc_video_surface.cpp vlc_preferences.cpp
  !macx:SOURCES += $${TAOTOPSRC}/tao/include/tao/GL/glew.c
  TBL_SOURCES = vlc_audio_video.tbl
  OTHER_FILES = vlc_audio_video.xl vlc_audio_video.tbl traces.tbl

  QT       += core gui opengl

  INCLUDEPATH += $${VLC}/include
  LIBS += -L$${VLC}/lib -lvlc -lvlccore
  macx:LIBS += -framework AppKit

  # Icon: http://en.wikipedia.org/wiki/File:VLC_Icon.svg
  # License: CC BY-SA 3.0
  INSTALLS += thismod_icon

  macx {
    # Install will create <module>/lib/{lib,plugins,share/lua}
    vlc_libs.path = $${MODINSTPATH}/lib
    vlc_libs.files = $${VLC}/lib
    vlc_plugins.path  = $${MODINSTPATH}/lib
    vlc_plugins.files = $${VLC}/plugins
    vlc_lua.path = $${MODINSTPATH}/lib/share
    vlc_lua.files = $${VLC}/share/lua
  }
  linux-g++* {
    # Untested
    vlc_libs.path = $${MODINSTPATH}/lib
    vlc_libs.files = $${VLC}/lib/*
    vlc_plugins.path  = $${MODINSTPATH}/plugins
    vlc_plugins.files = $${VLC}/plugins/*
    vlc_lua.path = $${MODINSTPATH}/lib
    vlc_lua.files = $${VLC}/share
  }
  win32 {
    # Install will create <module>/lib/{plugins,lua}
    vlc_libs.path = $${MODINSTPATH}/lib
    vlc_libs.files = $${VLC}/../*.dll
    vlc_plugins.path  = $${MODINSTPATH}/lib
    vlc_plugins.files = $${VLC}/../plugins
    vlc_lua.path  = $${MODINSTPATH}/lib
    vlc_lua.files = $${VLC}/../lua
  }
  INSTALLS += vlc_libs vlc_plugins vlc_lua
}
