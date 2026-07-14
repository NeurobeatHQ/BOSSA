///////////////////////////////////////////////////////////////////////////////
// BOSSA
//
// Copyright (c) 2018, ShumaTech
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////

#ifndef _NULLFLASH_H
#define _NULLFLASH_H

#include <stdint.h>

#include "Flash.h"

class NullFlash : public Flash
{
public:
    NullFlash(
        Samba& samba,
        const std::string& name,
        uint32_t pages,
        uint32_t size,
        uint32_t user,
        uint32_t stack);

    virtual ~NullFlash();

    void eraseAll(uint32_t offset);
    void eraseAuto(bool enable);
};

#endif // _NULLFLASH_H
