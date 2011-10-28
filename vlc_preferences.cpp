// ****************************************************************************
//  vlc_preferences.cpp                                            Tao project
// ****************************************************************************
//
//   File Description:
//
//    Show preference dialog for the VLCAudioVideo module
//
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

#include "vlc_preferences.h"
#include <vlc/libvlc.h>


VLCPreferences::VLCPreferences(QWidget *parent)
// ----------------------------------------------------------------------------
//   Construct dialog
// ----------------------------------------------------------------------------
    : QMessageBox(parent)
{
    setWindowTitle(tr("VLC Audio Video module Preferences"));
    QString VLCInfo;
    VLCInfo = tr(
                "<h3>VLC information:</h3>"
                "<p>Version: %1</p>"
                "<p>Changeset: %2</p>"
                "<p>Compiler: %3</p>"
                )
            .arg(libvlc_get_version())
            .arg(libvlc_get_changeset())
            .arg(libvlc_get_compiler());
    setInformativeText(VLCInfo);
}
