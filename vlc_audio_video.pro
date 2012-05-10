# ******************************************************************************
#  vlc_audio_video.pro                                              Tao project
# ******************************************************************************
# File Description:
# Qt build file for the VLC Audio Video module
# Requires VLC media player >= 2.0
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

HEADER=$$VLC/include/vlc/libvlc_media_player.h
exists($$HEADER) {
  VLC_FOUND=1
  system(bash -c \"grep libvlc_video_set_format_callbacks \\\"$$HEADER\\\" >/dev/null 2>&1 \"):VLC_VERSION_OK=1
} else {
  !build_pass:macx:exists($$VLC/include/libvlc.h) {
      message("*** Broken MacOSX VLC package warning:")
      message("VLC include files found under $$VLC/include/")
      message("They should be under $$VLC/include/vlc/")
      message("Please upgrade VLC or do: \(cd \"$$VLC/include\" ; ln -s . vlc\)")
      message("...and try again.")
  }
}

isEmpty(VLC_FOUND)|isEmpty(VLC_VERSION_OK) {
    !isEmpty(VLC_FOUND):isEmpty(VLC_VERSION_OK):message("*** VLC is too old! \($$VLC\)")
    message()
    message("To build the VLCAudioVideo module, I need the VLC media player >= 2.0")
    linux-g++* {
      message(Please install the libvlc-dev package.)
      message(See http://nightlies.videolan.org/)
    } else {
      message("Please install the VLC media player from:")
      message("http://www.videolan.org/vlc/")
      message("If you have extracted the VLC SDK somewhere else than the default")
      message("installation directory, you may set the VLC variable.")
      message(For instance:)
      message([MacOSX] ./configure VLC=/Users/jerome/Desktop/vlc-2.0.1/VLC.app/Contents/MacOS)
      message([Windows] ./configure VLC=/c/Users/Jerome/Desktop/vlc-2.0.0/sdk)
    }
    message()
    message(*** THE VLCAudioVideo MODULE WILL NOT BE BUILT ***)

} else {

  MODINSTDIR = vlc_audio_video

  include(../modules.pri)

  DEFINES    += GLEW_STATIC
  HEADERS     = vlc_audio_video.h \
                vlc_preferences.h \
                vlc_video_base.h \
                vlc_video_fullscreen.h \
                vlc_video_surface.h
  SOURCES     = vlc_audio_video.cpp \
                vlc_preferences.cpp \
                vlc_video_base.cpp \
                vlc_video_fullscreen.cpp \
                vlc_video_surface.cpp
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

  # Install GPL license text
  copying.files = COPYING
  copying.path  = $${MODINSTPATH}
  QMAKE_TARGETS += copying
  INSTALLS += copying

  macx {
    # Install will create <module>/lib/{lib,plugins,share/lua}
    vlc_libs.path = $${MODINSTPATH}/lib
    vlc_libs.files = "$${VLC}/lib"
    vlc_plugins.path  = $${MODINSTPATH}/lib
    vlc_plugins.files = "$${VLC}/plugins"
    vlc_lua.path = $${MODINSTPATH}/lib/share
    vlc_lua.files = "$${VLC}/share/lua"

    # Bug #1944
    vlc_rm_freetype.commands = rm \"$${MODINSTPATH}/lib/plugins/libfreetype_plugin.dylib\"
    vlc_rm_freetype.depends = install_vlc_plugins
    vlc_rm_freetype.path = $${MODINSTPATH}/lib/plugins
    INSTALLS += vlc_rm_freetype
  }
  win32 {
    # Install will create <module>/lib/{plugins,lua}
    vlc_libs.path = $${MODINSTPATH}/lib
    vlc_libs.files = "$${VLC}/../*.dll" "$${VLC}/../vlc-cache-gen.exe"
    vlc_plugins.path  = $${MODINSTPATH}/lib/plugins
    vlc_plugins.commands = mkdir -p $${MODINSTPATH}/lib/plugins ; cp -R "$${VLC}/../plugins/*" $${MODINSTPATH}/lib/plugins
    vlc_lua.path  = $${MODINSTPATH}/lib/lua
    vlc_lua.commands = mkdir -p $${MODINSTPATH}/lib/lua ; cp -R "$${VLC}/../lua/*" $${MODINSTPATH}/lib/lua
  }
  macx|win32:INSTALLS += vlc_libs vlc_plugins vlc_lua

  LICENSE_FILES = vlc_audio_video.taokey.notsigned
  exists(../licenses.pri):include(../licenses.pri)

  QMAKE_SUBSTITUTES = doc/Doxyfile.in
  DOXYFILE = doc/Doxyfile
  DOXYLANG = en,fr
  include(../modules_doc.pri)
}
