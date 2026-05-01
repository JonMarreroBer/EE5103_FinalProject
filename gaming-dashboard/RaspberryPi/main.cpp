/*
    * main.cpp
    * This program runs on the Raspberry Pi and receives telemetry data (FPS, CPU usage, GPU usage, RAM usage) from a PC over UDP.
    * It uses the standard C++ library for network communication and data handling.
*/
#include <iostream>
#include <memory>
#include <string>

// Class to represent the telemetry data received from the PC
class TelemetryData {
public:
    int fps;
    int cpuUsage;
    int gpuUsage;
    int ramUsage;

    TelemetryData(int fps, int cpu, int gpu, int ram)
        : fps(fps), cpuUsage(cpu), gpuUsage(gpu), ramUsage(ram) {}
};

// Class to represent the dashboard that displays the telemetry data
class Dashboard {
public:
    Dashboard() {
        std::cout << "Dashboard created\n";
    }

    ~Dashboard() {
        std::cout << "Dashboard destroyed\n";
    }

    void display(const TelemetryData& data) {
        std::cout << "FPS: " << data.fps << "\n";
        std::cout << "CPU: " << data.cpuUsage << "%\n";
        std::cout << "GPU: " << data.gpuUsage << "%\n";
        std::cout << "RAM: " << data.ramUsage << "%\n";
    }
};

// Main function to create the dashboard and display sample telemetry data
// SIMULATED DATA FOR TESTING PURPOSES ONLY
int main() {
    std::unique_ptr<Dashboard> dashboard = std::make_unique<Dashboard>();

    TelemetryData data(144, 42, 76, 61);

    dashboard->display(data);

    return 0;
}