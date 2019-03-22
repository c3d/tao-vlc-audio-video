// *****************************************************************************
// vlc_preferences.cpp                                             Tao3D project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2011, Jérôme Forissier <jerome@taodyne.com>
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
