/*
    * ILI9341Display.cpp
    *
    * Implementation file for the ILI9341Display class, which provides an interface to control
    * an ILI9341 TFT display using SPI communication and GPIO for control signals.
    *
    * This class includes methods for initializing the display, drawing pixels, characters,
    * and text, as well as filling rectangles and displaying a gaming dashboard with FPS,
    * CPU, GPU, and RAM usage.
*/
#include "ILI9341Display.h"

#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <chrono>
#include <thread>
#include <algorithm>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include <gpiod.h>

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           (b >> 3);
}


void ILI9341Display::delayMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

gpiod_chip* ILI9341Display::openGpioChip() {
    gpiod_chip* c = gpiod_chip_open("/dev/gpiochip0");

    if (!c) {
        c = gpiod_chip_open("/dev/gpiochip4");
    }

    if (!c) {
        throw std::runtime_error("Failed to open GPIO chip");
    }

    return c;
}

void ILI9341Display::setDc(bool value) {
    gpiod_line_request_set_value(
        gpioRequest,
        dcPin,
        value ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE
    );
}

void ILI9341Display::setReset(bool value) {
    gpiod_line_request_set_value(
        gpioRequest,
        resetPin,
        value ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE
    );
}

void ILI9341Display::spiWrite(const uint8_t* data, size_t length) {
    if (write(spiFd, data, length) != static_cast<ssize_t>(length)) {
        throw std::runtime_error("SPI write failed");
    }
}

void ILI9341Display::sendCommand(uint8_t command) {
    setDc(false);
    spiWrite(&command, 1);
}

void ILI9341Display::sendData(uint8_t data) {
    setDc(true);
    spiWrite(&data, 1);
}

void ILI9341Display::sendData(const std::vector<uint8_t>& data) {
    setDc(true);
    spiWrite(data.data(), data.size());
}

void ILI9341Display::hardwareReset() {
    setReset(true);
    delayMs(50);
    setReset(false);
    delayMs(50);
    setReset(true);
    delayMs(150);
}

void ILI9341Display::setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    sendCommand(0x2A); // Column address set
    sendData({
        static_cast<uint8_t>(x0 >> 8),
        static_cast<uint8_t>(x0 & 0xFF),
        static_cast<uint8_t>(x1 >> 8),
        static_cast<uint8_t>(x1 & 0xFF)
    });

    sendCommand(0x2B); // Page address set
    sendData({
        static_cast<uint8_t>(y0 >> 8),
        static_cast<uint8_t>(y0 & 0xFF),
        static_cast<uint8_t>(y1 >> 8),
        static_cast<uint8_t>(y1 & 0xFF)
    });

    sendCommand(0x2C); // Memory write
}

std::array<uint8_t, 5> ILI9341Display::glyph(char c) {
    switch (c) {
        case '0': return {0x3E,0x51,0x49,0x45,0x3E};
        case '1': return {0x00,0x42,0x7F,0x40,0x00};
        case '2': return {0x42,0x61,0x51,0x49,0x46};
        case '3': return {0x21,0x41,0x45,0x4B,0x31};
        case '4': return {0x18,0x14,0x12,0x7F,0x10};
        case '5': return {0x27,0x45,0x45,0x45,0x39};
        case '6': return {0x3C,0x4A,0x49,0x49,0x30};
        case '7': return {0x01,0x71,0x09,0x05,0x03};
        case '8': return {0x36,0x49,0x49,0x49,0x36};
        case '9': return {0x06,0x49,0x49,0x29,0x1E};

        case 'A': return {0x7E,0x11,0x11,0x11,0x7E};
        case 'B': return {0x7F,0x49,0x49,0x49,0x36};
        case 'C': return {0x3E,0x41,0x41,0x41,0x22};
        case 'D': return {0x7F,0x41,0x41,0x22,0x1C};
        case 'E': return {0x7F,0x49,0x49,0x49,0x41};
        case 'F': return {0x7F,0x09,0x09,0x09,0x01};
        case 'G': return {0x3E,0x41,0x49,0x49,0x7A};
        case 'H': return {0x7F,0x08,0x08,0x08,0x7F};
        case 'I': return {0x00,0x41,0x7F,0x41,0x00};
        case 'J': return {0x20,0x40,0x41,0x3F,0x01};
        case 'K': return {0x7F,0x08,0x14,0x22,0x41};
        case 'L': return {0x7F,0x40,0x40,0x40,0x40};
        case 'M': return {0x7F,0x02,0x0C,0x02,0x7F};
        case 'N': return {0x7F,0x04,0x08,0x10,0x7F};
        case 'O': return {0x3E,0x41,0x41,0x41,0x3E};
        case 'P': return {0x7F,0x09,0x09,0x09,0x06};
        case 'Q': return {0x3E,0x41,0x51,0x21,0x5E};
        case 'R': return {0x7F,0x09,0x19,0x29,0x46};
        case 'S': return {0x46,0x49,0x49,0x49,0x31};
        case 'T': return {0x01,0x01,0x7F,0x01,0x01};
        case 'U': return {0x3F,0x40,0x40,0x40,0x3F};
        case 'V': return {0x1F,0x20,0x40,0x20,0x1F};
        case 'W': return {0x7F,0x20,0x18,0x20,0x7F};
        case 'X': return {0x63,0x14,0x08,0x14,0x63};
        case 'Y': return {0x07,0x08,0x70,0x08,0x07};
        case 'Z': return {0x61,0x51,0x49,0x45,0x43};

        case ':': return {0x00,0x36,0x36,0x00,0x00};
        case '%': return {0x23,0x13,0x08,0x64,0x62};
        case '-': return {0x08,0x08,0x08,0x08,0x08};
        case ' ': return {0x00,0x00,0x00,0x00,0x00};
        default:  return {0x00,0x00,0x00,0x00,0x00};
    }
}

ILI9341Display::ILI9341Display(const std::string& spiDevice, int dc, int reset): spiFd(-1), chip(nullptr), gpioRequest(nullptr), dcPin(dc), resetPin(reset), width(320), height(240) {
    spiFd = open(spiDevice.c_str(), O_RDWR);
    if (spiFd < 0) {
        throw std::runtime_error("Failed to open SPI device");
    }

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = 32000000;

    ioctl(spiFd, SPI_IOC_WR_MODE, &mode);
    ioctl(spiFd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(spiFd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    chip = openGpioChip();

    gpiod_line_settings* settings = gpiod_line_settings_new();
    if (!settings) {
        throw std::runtime_error("Failed to create GPIO line settings");
    }

    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    gpiod_line_config* lineConfig = gpiod_line_config_new();
    if (!lineConfig) {
        gpiod_line_settings_free(settings);
        throw std::runtime_error("Failed to create GPIO line config");
    }

    unsigned int offsets[2] = {
        static_cast<unsigned int>(dcPin),
        static_cast<unsigned int>(resetPin)
    };

    if (gpiod_line_config_add_line_settings(lineConfig, offsets, 2, settings) < 0) {
        gpiod_line_config_free(lineConfig);
        gpiod_line_settings_free(settings);
        throw std::runtime_error("Failed to add GPIO line settings");
    }

    gpiod_request_config* requestConfig = gpiod_request_config_new();
    if (!requestConfig) {
        gpiod_line_config_free(lineConfig);
        gpiod_line_settings_free(settings);
        throw std::runtime_error("Failed to create GPIO request config");
    }

    gpiod_request_config_set_consumer(requestConfig, "tft-display");

    gpioRequest = gpiod_chip_request_lines(chip, requestConfig, lineConfig);

    gpiod_request_config_free(requestConfig);
    gpiod_line_config_free(lineConfig);
    gpiod_line_settings_free(settings);

    if (!gpioRequest) {
        throw std::runtime_error("Failed to request GPIO lines");
    }

    initialize();
}

ILI9341Display::~ILI9341Display() {
if (gpioRequest) {
        gpiod_line_request_release(gpioRequest);
    }

    if (chip) {
        gpiod_chip_close(chip);
    }

    if (spiFd >= 0) {
        close(spiFd);
    }
}

void ILI9341Display::initialize() {
    hardwareReset();

    sendCommand(0x01); // Software reset
    delayMs(150);

    sendCommand(0xEF);
    sendData({0x03, 0x80, 0x02});

    sendCommand(0xCF);
    sendData({0x00, 0xC1, 0x30});

    sendCommand(0xED);
    sendData({0x64, 0x03, 0x12, 0x81});

    sendCommand(0xE8);
    sendData({0x85, 0x00, 0x78});

    sendCommand(0xCB);
    sendData({0x39, 0x2C, 0x00, 0x34, 0x02});

    sendCommand(0xF7);
    sendData(0x20);

    sendCommand(0xEA);
    sendData({0x00, 0x00});

    sendCommand(0xC0); // Power control
    sendData(0x23);

    sendCommand(0xC1); // Power control
    sendData(0x10);

    sendCommand(0xC5); // VCOM control
    sendData({0x3E, 0x28});

    sendCommand(0xC7); // VCOM control
    sendData(0x86);

    sendCommand(0x36); // Memory access control
    sendData(0x28);    // Landscape, RGB/BGR setting

    sendCommand(0x3A); // Pixel format
    sendData(0x55);    // 16-bit color

    sendCommand(0xB1);
    sendData({0x00, 0x18});

    sendCommand(0xB6);
    sendData({0x08, 0x82, 0x27});

    sendCommand(0xF2);
    sendData(0x00);

    sendCommand(0x26);
    sendData(0x01);

    sendCommand(0x11); // Sleep out
    delayMs(150);

    sendCommand(0x29); // Display on
    delayMs(100);
}

void ILI9341Display::fillScreen(uint16_t color) {
    fillRect(0, 0, width, height, color);
}

void ILI9341Display::fillRect(int x, int y, int w, int h, uint16_t color) {
    if (x < 0 || y < 0 || x >= width || y >= height) return;

    if (x + w > width) w = width - x;
    if (y + h > height) h = height - y;

    setAddressWindow(x, y, x + w - 1, y + h - 1);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    std::vector<uint8_t> buffer;
    buffer.reserve(4096);

    int totalPixels = w * h;
    int pixelsSent = 0;

    setDc(true);

    while (pixelsSent < totalPixels) {
        buffer.clear();

        int chunkPixels = std::min(2048, totalPixels - pixelsSent);

        for (int i = 0; i < chunkPixels; ++i) {
            buffer.push_back(hi);
            buffer.push_back(lo);
        }

        spiWrite(buffer.data(), buffer.size());
        pixelsSent += chunkPixels;
    }
}

void ILI9341Display::drawPixel(int x, int y, uint16_t color) {
    if (x < 0 || y < 0 || x >= width || y >= height) return;

    setAddressWindow(x, y, x, y);

    uint8_t data[2] = {
        static_cast<uint8_t>(color >> 8),
        static_cast<uint8_t>(color & 0xFF)
    };

    setDc(true);
    spiWrite(data, 2);
}

void ILI9341Display::drawChar(int x, int y, char c, uint16_t color, uint16_t bg, int scale) {
    auto g = glyph(c);

    for (int col = 0; col < 5; ++col) {
        uint8_t line = g[col];

        for (int row = 0; row < 7; ++row) {
            bool pixelOn = line & (1 << row);

            uint16_t drawColor = pixelOn ? color : bg;

            fillRect(
                x + col * scale,
                y + row * scale,
                scale,
                scale,
                drawColor
            );
        }
    }

    fillRect(x + 5 * scale, y, scale, 7 * scale, bg);
}

void ILI9341Display::drawText(int x, int y, const std::string& text, uint16_t color, uint16_t bg, int scale) {
    int cursorX = x;

    for (char c : text) {
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }

        drawChar(cursorX, y, c, color, bg, scale);
        cursorX += 6 * scale;
    }
}

void ILI9341Display::drawBar(int x, int y, int value, uint16_t color) {
    if (value < 0) value = 0;
    if (value > 100) value = 100;

    int barWidth = 160;
    int barHeight = 18;

    uint16_t white = rgb565(255, 255, 255);
    uint16_t black = rgb565(0, 0, 0);

    fillRect(x, y, barWidth, barHeight, black);

    fillRect(x, y, barWidth, 2, white);
    fillRect(x, y + barHeight - 2, barWidth, 2, white);
    fillRect(x, y, 2, barHeight, white);
    fillRect(x + barWidth - 2, y, 2, barHeight, white);

    int fillWidth = (value * (barWidth - 4)) / 100;
    fillRect(x + 2, y + 2, fillWidth, barHeight - 4, color);
}

void ILI9341Display::showDashboard(int fps, int cpu, int gpu, int ram) {
    const uint16_t BLACK  = rgb565(0, 0, 0);
    const uint16_t WHITE  = rgb565(255, 255, 255);
    const uint16_t GREEN  = rgb565(0, 255, 0);
    const uint16_t YELLOW = rgb565(255, 255, 0);
    const uint16_t RED    = rgb565(255, 0, 0);
    const uint16_t CYAN   = rgb565(0, 255, 255);

    fillScreen(BLACK);

    drawText(20, 15, "GAMING DASHBOARD", CYAN, BLACK, 2);

    uint16_t fpsColor = GREEN;
    if (fps < 60) {
        fpsColor = RED;
    } else if (fps < 100) {
        fpsColor = YELLOW;
    }

    drawText(20, 55, "FPS: " + std::to_string(fps), fpsColor, BLACK, 2);

    uint16_t cpuColor = GREEN;
    if (cpu >= 85) {
        cpuColor = RED;
    } else if (cpu >= 70) {
        cpuColor = YELLOW;
    }

    uint16_t gpuColor = GREEN;
    if (gpu >= 85) {
        gpuColor = RED;
    } else if (gpu >= 70) {
        gpuColor = YELLOW;
    }

    uint16_t ramColor = GREEN;
    if (ram >= 85) {
        ramColor = RED;
    } else if (ram >= 70) {
        ramColor = YELLOW;
    }

    drawText(20, 95, "CPU: " + std::to_string(cpu) + "%", WHITE, BLACK, 2);
    drawBar(130, 95, cpu, cpuColor);

    drawText(20, 130, "GPU: " + std::to_string(gpu) + "%", WHITE, BLACK, 2);
    drawBar(130, 130, gpu, gpuColor);

    drawText(20, 165, "RAM: " + std::to_string(ram) + "%", WHITE, BLACK, 2);
    drawBar(130, 165, ram, ramColor);

    bool warning = false;

    if (cpu >= 85 || gpu >= 85 || ram >= 85) {
        warning = true;
    }

    if (warning) {
        drawText(20, 205, "STATUS: WARNING", RED, BLACK, 2);
    } else {
        drawText(20, 205, "STATUS: NORMAL", GREEN, BLACK, 2);
    }
}
