/*
    * ILI9341Display.h
    *
    * Header file for the ILI9341Display class, which provides an interface to control
    * an ILI9341 TFT display using SPI communication and GPIO for control signals.
    *
    * This class includes methods for initializing the display, drawing pixels, characters,
    * and text, as well as filling rectangles and displaying a gaming dashboard with FPS,
    * CPU, GPU, and RAM usage.
*/
#ifndef ILI9341_DISPLAY_H
#define ILI9341_DISPLAY_H

#include <string>
#include <array>
#include <vector>
#include <cstdint>

#include <gpiod.h>

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b);

class ILI9341Display {
    private:
        int spiFd;
        gpiod_chip* chip;
        gpiod_line_request* gpioRequest;

        const int dcPin;
        const int resetPin;

        int width;
        int height;

        void delayMs(int ms);
        gpiod_chip* openGpioChip();

        void setDc(bool value);
        void setReset(bool value);

        void spiWrite(const uint8_t* data, size_t length);
        void sendCommand(uint8_t command);
        void sendData(uint8_t data);
        void sendData(const std::vector<uint8_t>& data);

        void hardwareReset();
        void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

        std::array<uint8_t, 5> glyph(char c);

    public:
        ILI9341Display(const std::string& spiDevice, int dc, int reset);
        ~ILI9341Display();

        void initialize();

        void fillScreen(uint16_t color);
        void fillRect(int x, int y, int w, int h, uint16_t color);
        void drawPixel(int x, int y, uint16_t color);
        void drawChar(int x, int y, char c, uint16_t color, uint16_t bg, int scale);
        void drawText(int x, int y, const std::string& text, uint16_t color, uint16_t bg, int scale);
        void drawBar(int x, int y, int value, uint16_t color);

        void showDashboard(int fps, int cpu, int gpu, int ram);
    };

#endif