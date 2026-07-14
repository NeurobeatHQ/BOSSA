///////////////////////////////////////////////////////////////////////////////
// BOSSA
//
// Copyright (c) 2011-2018, ShumaTech
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the <organization> nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////
#include "Samba.h"

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

using namespace std;

#define TIMEOUT_QUICK   100
#define TIMEOUT_NORMAL  1000
#define TIMEOUT_LONG    5000

Samba::Samba() :
    _canChipErase(false),
    _canWriteBuffer(false),
    _canIdentifyChip(false),
    _debug(false)
{
}

Samba::~Samba()
{
}

bool
Samba::init()
{
    uint8_t cmd[3];

    _port->timeout(TIMEOUT_QUICK);

    // Flush garbage
    uint8_t dummy[1024];
    _port->read(dummy, sizeof(dummy));

    // Set binary mode
    if (_debug)
        printf("Set binary mode\n");
    cmd[0] = 'N';
    cmd[1] = '#';
    _port->write(cmd, 2);
    _port->read(cmd, 2);

    std::string ver;
    try
    {
        ver = version();
    }
    catch(SambaError& err)
    {
        return false;
    }

    std::size_t extIndex = ver.find("[Arduino:");
    if (extIndex != string::npos)
    {
        extIndex += 9;
        while (ver[extIndex] != ']')
        {
            switch (ver[extIndex])
            {
                case 'I': _canIdentifyChip = true; break;
                case 'X': _canChipErase = true; break;
                case 'Y': _canWriteBuffer = true; break;
            }
            extIndex++;
        }
    }

    _port->timeout(TIMEOUT_NORMAL);

    return true;
}

bool
Samba::connect(SerialPort::Ptr port, int bps)
{
    _port = move(port);

    // The XMODEM/RS-232 transfer path was removed; only bootloaders
    // reachable over a USB-style binary link are supported.
    if (_port->isUsb() && _port->open(bps) && init())
    {
        if (_debug)
            printf("Connected at %d baud\n", bps);
        return true;
    }

    disconnect();
    return false;
}

void
Samba::disconnect()
{
    _port->close();
    _port.release();
}

void
Samba::writeWord(uint32_t addr, uint32_t value)
{
    uint8_t cmd[20];

    if (_debug)
        printf("%s(addr=%#x,value=%#x)\n", __FUNCTION__, addr, value);

    snprintf((char*) cmd, sizeof(cmd), "W%08X,%08X#", addr, value);
    if (_port->write(cmd, sizeof(cmd) - 1) != sizeof(cmd) - 1)
        throw SambaError();

    // The SAM firmware has a bug that if the command and binary data
    // are received in the same USB data packet, then the firmware
    // gets confused.  Even though the writes are sperated in the code,
    // USB drivers often do write combining which can put them together
    // in the same USB data packet.  To avoid this, we call the serial
    // port object's flush method before writing the data.
    _port->flush();

    usleep(10000);
}

void
Samba::writeBinary(const uint8_t* buffer, int size)
{
    while (size)
    {
        int written = _port->write(buffer, size);
        if (written <= 0)
            throw SambaError();
        buffer += written;
        size -= written;
    }
    usleep(10000);
}

void
Samba::write(uint32_t addr, const uint8_t* buffer, int size)
{
    uint8_t cmd[20];

    if (_debug)
        printf("%s(addr=%#x,size=%#x)\n", __FUNCTION__, addr, size);

    snprintf((char*) cmd, sizeof(cmd), "S%08X,%08X#", addr, size);
    if (_port->write(cmd, sizeof(cmd) - 1) != sizeof(cmd) - 1)
        throw SambaError();

    // The SAM firmware has a bug that if the command and binary data
    // are received in the same USB data packet, then the firmware
    // gets confused.  Even though the writes are separated in the code,
    // USB drivers often do write combining which can put them together
    // in the same USB data packet.  To avoid this, we call the serial
    // port object's flush method before writing the data.
    _port->flush();
    writeBinary(buffer, size);
}

std::string
Samba::readPrintable()
{
    uint8_t cmd[256];
    char* str;
    int size;
    int pos;

    _port->timeout(TIMEOUT_QUICK);
    size = _port->read(cmd, sizeof(cmd) - 1);
    _port->timeout(TIMEOUT_NORMAL);
    if (size <= 0)
        throw SambaError();

    str = (char*) cmd;
    for (pos = 0; pos < size; pos++)
    {
        if (!isprint(str[pos]))
            break;
    }
    str[pos] = '\0';

    return std::string(str);
}

std::string
Samba::version()
{
    uint8_t cmd[2];

    cmd[0] = 'V';
    cmd[1] = '#';
    _port->write(cmd, 2);

    std::string ver = readPrintable();

    if (_debug)
        printf("%s()=%s\n", __FUNCTION__, ver.c_str());

    return ver;
}

std::string
Samba::identifyChip()
{
    uint8_t cmd[2];

    cmd[0] = 'I';
    cmd[1] = '#';
    _port->write(cmd, 2);

    std::string id = readPrintable();

    if (_debug)
        printf("%s()=%s\n", __FUNCTION__, id.c_str());

    return id;
}

void
Samba::chipErase(uint32_t start_addr)
{
    if (!_canChipErase)
        throw SambaError();

    uint8_t cmd[64];

    if (_debug)
        printf("%s(addr=%#x)\n", __FUNCTION__, start_addr);

    int l = snprintf((char*) cmd, sizeof(cmd), "X%08X#", start_addr);
    if (_port->write(cmd, l) != l)
        throw SambaError();
    _port->timeout(TIMEOUT_LONG);
    _port->read(cmd, 3); // Expects "X\n\r"
    _port->timeout(TIMEOUT_NORMAL);
    if (cmd[0] != 'X')
        throw SambaError();
}

void
Samba::writeBuffer(uint32_t src_addr, uint32_t dst_addr, uint32_t size)
{
    if (!_canWriteBuffer)
        throw SambaError();

    if (size > writeBufferSize())
        throw SambaError();

    if (_debug)
        printf("%s(scr_addr=%#x, dst_addr=%#x, size=%#x)\n", __FUNCTION__, src_addr, dst_addr, size);

    uint8_t cmd[64];
    int l = snprintf((char*) cmd, sizeof(cmd), "Y%08X,0#", src_addr);
    if (_port->write(cmd, l) != l)
        throw SambaError();
    _port->timeout(TIMEOUT_NORMAL);
    cmd[0] = 0;
    _port->read(cmd, 3); // Expects "Y\n\r"
    _port->timeout(TIMEOUT_NORMAL);
    if (cmd[0] != 'Y')
        throw SambaError();

    l = snprintf((char*) cmd, sizeof(cmd), "Y%08X,%08X#", dst_addr, size);
    if (_port->write(cmd, l) != l)
        throw SambaError();
    _port->timeout(TIMEOUT_LONG);
    cmd[0] = 0;
    _port->read(cmd, 3); // Expects "Y\n\r"
    _port->timeout(TIMEOUT_NORMAL);
    if (cmd[0] != 'Y')
        throw SambaError();
}
