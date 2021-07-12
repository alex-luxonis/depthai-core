#include <chrono>
#include <string>

#include "depthai/depthai.hpp"

int main(int argc, char** argv) {
    using namespace std;
    using namespace std::chrono;

    dai::DeviceBootloader::Type blType = dai::DeviceBootloader::Type::USB;
    if(argc > 1) {
        if(string(argv[1]) == "usb") {
            blType = dai::DeviceBootloader::Type::USB;
        } else if(string(argv[1]) == "eth") {
            blType = dai::DeviceBootloader::Type::NETWORK;
        } else {
            cout << "Specify either 'usb' or 'eth' bootloader type\n";
            return 0;
        }
    } else {
        cout << "Usage: " << argv[0] << " <usb/eth>\n";
        return 0;
    }

    cout << "Flashing bootloader is a risky operation and can result in soft-bricking your device\n";
    cout << "Type 'yes' and press enter if you understand the risk and want to continue\n";
    string confirmation;
    cin >> confirmation;
    if(confirmation != "yes"){
        cout << "Not flashing the bootloader" << endl;
        return 0;
    }

    bool res = false;
    dai::DeviceInfo info;
    tie(res, info) = dai::DeviceBootloader::getFirstAvailableDevice();

    if(res) {

        dai::DeviceBootloader bl(info);
        auto progress = [](float p) { cout << "Flashing Progress..." << p * 100 << "%" << endl; };

        string message;
        auto t1 = steady_clock::now();
        tie(res, message) = bl.flashBootloader(dai::DeviceBootloader::Memory::FLASH, blType, progress);
        if(res) {
            cout << "Flashing successful. Took " << duration_cast<milliseconds>(steady_clock::now() - t1).count() << "ms" << endl;
        } else {
            cout << "Flashing failed: " << message << endl;
        }

    } else {
        std::cout << "No devices found" << std::endl;
    }

    return 0;
}