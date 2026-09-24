///////////////////////////////////////////////////////////////////////////////
// BOSSA — ESP-IDF adapter (native ESP-IDF only; the Arduino build skips this).
//
// Ready-made glue for driving BOSSA from an ESP-IDF app: a SerialPort backed by
// an ESP-IDF UART, a default FlasherObserver that logs through ESP_LOG, and a
// one-call bossa_flash_file() convenience. The CALLER still owns the two
// board-specific steps: strapping the target into its SAM-BA/DFU bootloader
// before the call, and resetting it to run afterward.
///////////////////////////////////////////////////////////////////////////////
#pragma once

#if defined(ESP_PLATFORM) && !defined(ARDUINO)

#include <cstdarg>
#include <cstdio>
#include <utility>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Samba.h"
#include "Device.h"
#include "Flasher.h"

// SerialPort backed by an ESP-IDF UART. isUsb() returns true because the slimmed
// BOSSA fork keeps only the SAM-BA binary path (Samba::connect() gates on it),
// even though the wire is a plain UART; connect() calls open(bps) to set baud.
class EspUartSerialPort : public SerialPort {
public:
    EspUartSerialPort(uart_port_t uart, const std::string &name = "esp-uart")
        : SerialPort(name), _uart(uart), _timeoutMs(1000) {}

    bool open(int baud = 115200) override {
        uart_set_baudrate(_uart, baud);
        uart_flush_input(_uart);
        return true;
    }
    void close() override {}
    bool isUsb() override { return true; }

    // Read up to `size` bytes, returning a partial count once the link goes quiet
    // for the configured timeout — mirrors the Posix select() reference semantics.
    int read(uint8_t *data, int size) override {
        int got = 0;
        while (got < size) {
            int n = uart_read_bytes(_uart, data + got, size - got, pdMS_TO_TICKS(_timeoutMs));
            if (n < 0) return -1;
            if (n == 0) break;
            got += n;
        }
        return got;
    }
    int write(const uint8_t *data, int size) override {
        return uart_write_bytes(_uart, data, size);
    }
    bool timeout(int millisecs) override { _timeoutMs = millisecs; return true; }
    void flush() override { uart_wait_tx_done(_uart, pdMS_TO_TICKS(_timeoutMs > 0 ? _timeoutMs : 1000)); }

private:
    uart_port_t _uart;
    int _timeoutMs;
};

// Default observer: forwards status text and 20%-stepped progress to ESP_LOG.
class EspLogObserver : public FlasherObserver {
public:
    explicit EspLogObserver(const char *tag = "bossa") : _tag(tag) {}

    void onStatus(const char *message, ...) override {
        va_list ap;
        va_start(ap, message);
        char buf[128];
        vsnprintf(buf, sizeof(buf), message, ap);
        va_end(ap);
        ESP_LOGI(_tag, "%s", buf);
    }
    void onProgress(int num, int div) override {
        if (div <= 0) return;
        int pct = num * 100 / div;
        int bucket = pct / 20;                 // log on 20%-boundary crossings
        if (bucket != _lastBucket) {
            _lastBucket = bucket;
            ESP_LOGI(_tag, "  %d%% (%d/%d pages)", pct, num, div);
        }
    }
private:
    const char *_tag;
    int _lastBucket = -1;
};

// Connect over `uart` at `baud`, identify the flash, then erase + write the
// binary at `path`. Returns true only if every step succeeded. The caller owns
// strapping the target into its bootloader first and resetting it afterward.
inline bool bossa_flash_file(uart_port_t uart, int baud, const char *path, FlasherObserver &obs)
{
    Samba samba;
    samba.setDebug(false);
    SerialPort::Ptr port(new EspUartSerialPort(uart));
    if (!samba.connect(std::move(port), baud)) {          // moves port in, calls open(baud)
        ESP_LOGE("bossa", "SAM-BA connect failed (bootloader up? baud/wiring?)");
        return false;
    }
    Device device(samba);
    device.create();
    Device::FlashPtr &flash = device.getFlash();
    if (samba.failed() || flash.get() == nullptr) {
        ESP_LOGE("bossa", "no supported flash (chip-id mismatch?)");
        return false;
    }
    ESP_LOGI("bossa", "flash: %s", flash->name().c_str());

    Flasher flasher(samba, device, obs);
    flasher.erase(0);
    vTaskDelay(pdMS_TO_TICKS(100));
    flasher.write(path);
    return !samba.failed();
}

#endif // ESP_PLATFORM && !ARDUINO
