/*
  BossaArduino.h
  Copyright (c) 2023 Arduino SA.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _BOSSAA_H
#define _BOSSAA_H

#include "SerialPort.h"
#include <HardwareSerial.h>

class BossacSerialPort : public SerialPort {
    public:
        BossacSerialPort(const std::string& name, HardwareSerial& serial)
        : SerialPort(name)
        , _serial{&serial}
        {

        }

        bool open(int baud, int data, Parity parity, StopBit stop) {
            _serial->begin(baud);
            return true;
        }

        void close() {

        }

        bool initcmd() {
            return true;
        }

        bool endcmd() {
            return true;
        }

        bool isUsb() {
            return true;
        }

        int read(uint8_t* data, int size) {
            const int n = _serial->readBytes(data,size);
            return n;
        }

        int write(const uint8_t* data, int size) {
            const int n = _serial->write(data,size);
            return n;
        }

        int get() {
            int c = _serial->read();
            return c;
        }

        int put(int c) {
            _serial->write(c);
            _serial->flush();
        }

        bool timeout(int millisecs) {
            _serial->setTimeout(millisecs);
            return true;
        }

        void flush() {
            _serial->flush();
        }

        void setDTR(bool dtr) {

        }

        void setRTS(bool rts) {

        }

    private:
    HardwareSerial* _serial;
};


class BOSSA {
    public:
        static int flash(const char* file_path, HardwareSerial& serial);
        static int flashUnoR4WiFi(const char* file_path, HardwareSerial& serial, int boot, int reset);
};
#endif
