
#include <cstdio>
#include <iostream>

#include "utility.hpp"

// Includes common necessary includes for development using depthai library
#include "depthai/depthai.hpp"

int main() {
    using namespace std;
    using namespace std::chrono;

    dai::Device device;

    auto imuVersion = device.getConnectedIMU();
    auto imuFirmwareVersion = device.getIMUFirmwareVersion();
    auto latestImuFirmwareVersion = device.getLatestAvailableIMUFirmwareVersion();
    std::cout << "IMU version: " << imuVersion << " firmware version: " << imuFirmwareVersion
              << " latest available firmware version: " << latestImuFirmwareVersion << std::endl;

    std::cout << "Warning! Flashing IMU firmware can potentially soft brick your device and should be done with caution." << std::endl;
    std::cout << "Do not unplug your device while the IMU firmware is flashing." << std::endl;
    std::cout << "Type 'y' and press enter to proceed, otherwise exits: ";
    std::cin.ignore();
    if(std::cin.get() != 'y') {
        std::cout << "Prompt declined, exiting..." << std::endl;
        return -1;
    }

    auto started = device.startIMUFirmwareUpdate();
    if(!started) {
        std::cout << "Couldn't start IMU firmware update" << std::endl;
        return 1;
    }

    while(true) {
        bool fwUpdatePending;
        float percentage;
        std::tie(fwUpdatePending, percentage) = device.getIMUFirmwareUpdateStatus();
        std::cout << "IMU FW update status: " << std::setprecision(1) << percentage << std::endl;
        if(fwUpdatePending == false && percentage == 100) {
            std::cout << "Firmware update successful!" << std::endl;
            break;
        }
        if(fwUpdatePending == false && percentage != 100) {
            std::cout << "Firmware update failed!" << std::endl;
            break;
        }
        std::this_thread::sleep_for(1s);
    }

    return 0;
}
