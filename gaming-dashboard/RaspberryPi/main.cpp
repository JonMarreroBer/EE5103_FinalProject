/*
    * main.cpp
    * This program runs on the Raspberry Pi and receives telemetry data (FPS, CPU usage, GPU usage, RAM usage) from a PC over UDP.
    * It uses the standard C++ library for network communication and data handling.
*/
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <gpiod.h>

#include "ILI9341Display.h"

// Class to represent the telemetry data received from the PC
class TelemetryData {
    public:
        int fps;
        int cpuUsage;
        int gpuUsage;
        int ramUsage;

        TelemetryData()
                : fps(0), cpuUsage(0), gpuUsage(0), ramUsage(0) {}

        TelemetryData(int fps, int cpu, int gpu, int ram)
            : fps(fps), cpuUsage(cpu), gpuUsage(gpu), ramUsage(ram) {}
};

// Class to represent the telemetry receiver that listens for data from the PC
class TelemetryReceiver {
    public: 
        virtual ~TelemetryReceiver() = default;
        virtual TelemetryData receive() = 0;
};

// Class to implement the UDP receiver that listens for telemetry data on a specified port
class UDPReceiver : public TelemetryReceiver {
    private:
        int socketFd;
        int port;

    public:
        // Constructor to create a UDP socket and bind it to the specified port
        UDPReceiver(int port): socketFd(-1), port(port) {
            
            socketFd = socket(AF_INET, SOCK_DGRAM, 0);

            if (socketFd < 0) {
                throw std::runtime_error("Failed to create socket");
            }

            // Set socket options to allow address reuse
            sockaddr_in serverAddress{};
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_addr.s_addr = INADDR_ANY;
            serverAddress.sin_port = htons(port);

            if (bind(socketFd, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) < 0) {
                close(socketFd);
                throw std::runtime_error("Failed to bind socket");
            }

            std::cout << "Listening for telemetry on UDP port " << port << "...\n";
        }

        // Destructor to close the socket when the receiver is destroyed
        ~UDPReceiver() override {
            if (socketFd >= 0) {
                close(socketFd);
                std::cout << "Socket closed\n";
            }
        }

        // Override the receive method to listen for incoming telemetry data and parse it into a TelemetryData object
        TelemetryData receive() override {
            char buffer[1024];
            std::memset(buffer, 0, sizeof(buffer));

            sockaddr_in clientAddress{};
            socklen_t clientLength = sizeof(clientAddress);

            ssize_t bytesReceived = recvfrom(
                socketFd,
                buffer,
                sizeof(buffer) - 1,
                0,
                reinterpret_cast<sockaddr*>(&clientAddress),
                &clientLength
            );

            if (bytesReceived < 0) {
                throw std::runtime_error("Failed to receive data");
            }

            std::string message(buffer);

            int fps = 0;
            int cpu = 0;
            int gpu = 0;
            int ram = 0;
            char comma1, comma2, comma3;

            std::stringstream ss(message);

            if (!(ss >> fps >> comma1 >> cpu >> comma2 >> gpu >> comma3 >> ram)) {
                std::cerr << "Invalid telemetry message received: " << message << "\n";
                return TelemetryData();
            }

            return TelemetryData(fps, cpu, gpu, ram);
        }
};

// Class to represent the display that shows the telemetry data on the dashboard
class Display {
    public:
        virtual ~Display() = default;
        virtual void show(const TelemetryData& data) = 0;
};

// Class to implement the console display that outputs the telemetry data to the terminal
class ConsoleDisplay : public Display {
    public:
        void show(const TelemetryData& data) override {
            std::cout << "\033[2J\033[H";

            std::cout << "Gaming Dashboard\n";
            std::cout << "----------------\n";
            std::cout << "FPS: " << data.fps << "\n";
            std::cout << "CPU Usage: " << data.cpuUsage << "%\n";
            std::cout << "GPU Usage: " << data.gpuUsage << "%\n";
            std::cout << "RAM Usage: " << data.ramUsage << "%\n";
        }
};


int main() {
    try {
        // Create a UDP receiver to listen for telemetry data on port 5000 and a console display to show the data
        std::unique_ptr<TelemetryReceiver> receiver =  std::make_unique<UDPReceiver>(5000);
        std::unique_ptr<Display> display = std::make_unique<ConsoleDisplay>();

        ILI9341Display tft("/dev/spidev0.0", 25, 24);
        tft.fillScreen(rgb565(0, 0, 0));
        tft.drawText(20, 15, "GAMING DASHBOARD", rgb565(0, 255, 255), rgb565(0, 0, 0), 2);

        while (true) {
            TelemetryData data = receiver->receive();
            tft.showDashboard(data.fps, data.cpuUsage, data.gpuUsage, data.ramUsage);
            display->show(data);
        }
    }
    catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }

    return 0;
}