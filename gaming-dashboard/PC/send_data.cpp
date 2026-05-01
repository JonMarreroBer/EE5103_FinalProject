/*
    * send_data.cpp
    * This program runs on the PC and sends telemetry data (FPS, CPU usage, GPU usage, RAM usage) to a Raspberry Pi over UDP.
    * It uses the Windows API to retrieve system performance metrics and the Winsock API for network communication.
    * The SystemMonitor class encapsulates the logic for retrieving CPU, RAM, and NVIDIA GPU usage. 
    * CPU usage is calculated using GetSystemTimes, RAM usage is retrieved using GlobalMemoryStatusEx, 
    * and NVIDIA GPU usage is obtained by executing the nvidia-smi command and parsing its output.
    * The main loop collects the telemetry data every second and sends it to the Raspberry Pi in a comma-separated format.
    * 
    * Command to compile (using g++ with C++17):
    *   g++ -std=c++17 -o send_data send_data.exe -lws2_32
    *   ./send_data.exe
    * 
    * It should print the sent telemetry data to the console for verification.
    * The Raspberry Pi should be set up to receive this data and display it on the dashboard.
*/
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

/*
    * SystemMonitor class to retrieve CPU, RAM, and NVIDIA GPU usage.
    * CPU usage is calculated using GetSystemTimes and comparing idle, kernel, and user times.
    * RAM usage is retrieved using GlobalMemoryStatusEx.
    * NVIDIA GPU usage is obtained by executing the nvidia-smi command and parsing its output.
*/
class SystemMonitor {
    public:
        SystemMonitor(): previousIdleTime(0), previousKernelTime(0), previousUserTime(0), firstRead(true) {}

        // Retrieves the current CPU usage percentage by comparing the idle, kernel, and user times from GetSystemTimes.
        int getCpuUsage() {
            FILETIME idleTime, kernelTime, userTime;

            if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
                return -1;
            }

            ULONGLONG idle = fileTimeToULL(idleTime);
            ULONGLONG kernel = fileTimeToULL(kernelTime);
            ULONGLONG user = fileTimeToULL(userTime);

            if (firstRead) {
                previousIdleTime = idle;
                previousKernelTime = kernel;
                previousUserTime = user;
                firstRead = false;
                return 0;
            }

            ULONGLONG idleDiff = idle - previousIdleTime;
            ULONGLONG kernelDiff = kernel - previousKernelTime;
            ULONGLONG userDiff = user - previousUserTime;

            ULONGLONG totalDiff = kernelDiff + userDiff;

            previousIdleTime = idle;
            previousKernelTime = kernel;
            previousUserTime = user;

            if (totalDiff == 0) {
                return 0;
            }

            double cpuUsage = 100.0 * (1.0 - static_cast<double>(idleDiff) / totalDiff);

            if (cpuUsage < 0) cpuUsage = 0;
            if (cpuUsage > 100) cpuUsage = 100;

            return static_cast<int>(cpuUsage);
        }

        // Retrieves the current RAM usage percentage using GlobalMemoryStatusEx.
        int getRamUsage() {
            MEMORYSTATUSEX memInfo;
            memInfo.dwLength = sizeof(MEMORYSTATUSEX);

            if (!GlobalMemoryStatusEx(&memInfo)) {
                return -1;
            }

            return static_cast<int>(memInfo.dwMemoryLoad);
        }


        // Retrieves the current NVIDIA GPU usage percentage by executing the nvidia-smi command and parsing its output.
        int getNvidiaGpuUsage() {
        FILE* pipe = _popen(
            "nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits",
            "r"
        );

        if (!pipe) {
            return -1;
        }

        char buffer[128];
        std::string result = "";

        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result = buffer;
        }

        _pclose(pipe);

        try {
            return std::stoi(result);
        } catch (...) {
            return -1;
        }
    }

    private:
    // ULONGLONG is a value type that represents an unsigned 64-bit integer. 
    //It is used to store the previous idle, kernel, and user times for CPU usage calculation.
    ULONGLONG previousIdleTime;
    ULONGLONG previousKernelTime;
    ULONGLONG previousUserTime;
    bool firstRead; // Flag to indicate if it's the first time reading CPU times, to initialize previous times without calculating usage.

    // Helper function to convert FILETIME to ULONGLONG
    ULONGLONG fileTimeToULL(const FILETIME& ft) {
        ULARGE_INTEGER ul;
        ul.LowPart = ft.dwLowDateTime;
        ul.HighPart = ft.dwHighDateTime;
        return ul.QuadPart; 
    }

};

int main() {
    const char* PI_IP = "192.168.4.147"; // Replace with your Raspberry Pi's IP address
    const int PORT = 5000; // Port number to send data to

    WSADATA wsaData; // Structure to hold information about the Windows Sockets implementation

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Create a UDP socket

    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in piAddress{}; // Structure to hold the Raspberry Pi's address information
    piAddress.sin_family = AF_INET;
    piAddress.sin_port = htons(PORT);
    piAddress.sin_addr.s_addr = inet_addr(PI_IP);

    if (piAddress.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid Raspberry Pi IP address\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    SystemMonitor monitor; // Create an instance of the SystemMonitor class to retrieve system performance metrics

    std::srand(static_cast<unsigned int>(std::time(nullptr))); // Seed the random number generator for simulating FPS values

    while (true) {
        int cpu = monitor.getCpuUsage();
        int ram = monitor.getRamUsage();
        int gpu = monitor.getNvidiaGpuUsage();

        // Still simulated for now
        int fps = 45 + std::rand() % 121;

        std::string message =
            std::to_string(fps) + "," +
            std::to_string(cpu) + "," +
            std::to_string(gpu) + "," +
            std::to_string(ram);

        int result = sendto(
            sock,
            message.c_str(),
            static_cast<int>(message.size()),
            0,
            reinterpret_cast<sockaddr*>(&piAddress),
            sizeof(piAddress)
        );

        if (result == SOCKET_ERROR) {
            std::cerr << "sendto failed\n";
        } else {
            std::cout << "Sent telemetry -> "
                      << "FPS: " << fps << " simulated"
                      << ", CPU Usage: " << cpu << "% real"
                      << ", GPU Usage: " << gpu << "% real"
                      << ", RAM Usage: " << ram << "% real"
                      << "\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait for 1 second before sending the next telemetry data
    }

    closesocket(sock); // Close the socket before exiting
    WSACleanup(); // Clean up Winsock resources

    return 0;
}