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
#include <string>
#include <exception>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "Flasher.h"

using namespace std;

void
Flasher::erase(uint32_t foffset)
{
    _observer.onStatus("Erase flash\n");
    _flash->eraseAll(foffset);
    _flash->eraseAuto(false);
}

void
Flasher::write(const char* filename, uint32_t foffset)
{
    FILE* infile;
    uint32_t pageSize = _flash->pageSize();
    uint32_t numPages;
    long fsize;
    size_t fbytes;

    if (foffset % pageSize != 0 || foffset >= _flash->totalSize())
        throw FlashOffsetError();

    // Only the extended write-buffer transfer is supported; the
    // page-by-page applet fallback was removed.
    if (!_samba.canWriteBuffer())
        throw FlashOffsetError();

    infile = fopen(filename, "rb");
    if (!infile)
        throw FileOpenError(errno);

    try
    {
        if (fseek(infile, 0, SEEK_END) != 0 || (fsize = ftell(infile)) < 0)
            throw FileIoError(errno);

        rewind(infile);

        numPages = (fsize + pageSize - 1) / pageSize;
        if (numPages > _flash->numPages())
            throw FileSizeError();

        _observer.onStatus("Write %ld bytes to flash (%u pages)\n", fsize, numPages);

        uint32_t offset = 0;
        uint32_t bufferSize = _samba.writeBufferSize();
        uint8_t buffer[bufferSize];

        while ((fbytes = fread(buffer, 1, bufferSize, infile)) > 0)
        {
            _observer.onProgress(offset / pageSize, numPages);

            if (fbytes < bufferSize)
            {
                memset(buffer + fbytes, 0, bufferSize - fbytes);
                fbytes = (fbytes + pageSize - 1) / pageSize * pageSize;
            }

            _flash->loadBuffer(buffer, fbytes);
            _flash->writeBuffer(foffset + offset, fbytes);
            offset += fbytes;
        }
    }
    catch(...)
    {
        fclose(infile);
        throw;
    }

    fclose(infile);
    _observer.onProgress(numPages, numPages);
}
