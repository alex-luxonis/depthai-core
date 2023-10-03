#include <iostream>

// Includes common necessary includes for development using depthai library
#include "depthai/depthai.hpp"

#include <chrono>
#include "XLink/XLink.h"

int main() {
    using namespace std;

    // Initial can be empty, will then be populated after first run
    // (then the app will be tied to device)
    std::string serialNumber = ""; // 18443010E169C51200

  // Main (re-)connect and stream loop
  while (true) {

    using namespace std::chrono;
    auto connectTime = seconds(35);

    std::tuple<bool, dai::DeviceInfo> reconnect;
    bool cnct;
    duration<float> elapsed;
    bool expectedState = false;
    bool success = false;

    printf("\nAttempting to find device [%s] ", serialNumber.c_str());
    int rep = 0;
    auto startTime = steady_clock::now();
    do {
        printf(".");
        fflush(stdout);
        reconnect = dai::XLinkConnection::getDeviceByMxId(serialNumber);
        elapsed = duration_cast<duration<float>>(steady_clock::now() - startTime);
        cnct = std::get<0>(reconnect);

        if (cnct) {
            auto state = std::get<1>(reconnect).state;
            expectedState = 0
                    || (state == X_LINK_UNBOOTED)
                    || (state == X_LINK_BOOTLOADER)
                 // || (state == X_LINK_FLASH_BOOTED)
                    ;
            if (!expectedState) {
                printf("\nDevice in unexpected state: %s\n", XLinkDeviceStateToStr(state));
                // This sleep isn't required, but getDeviceByMxId may return very quickly for example with X_LINK_BOOTED
                std::this_thread::sleep_for(milliseconds(500));
            }
        }
        success = cnct && expectedState;
        rep++;
    } while (!success && (elapsed < connectTime));

    printf("\nreconnect succeeded? %d. Tries %d, elapsed %.3f s\n", success, rep, elapsed.count());

    if (success) {
        // Save our device ID for future reconnects
        serialNumber = std::get<1>(reconnect).mxid;
    } else {
        // retry
        continue;
    }

    // Create pipeline
    dai::Pipeline pipeline;

    // Define source and output
    auto camRgb = pipeline.create<dai::node::ColorCamera>();
    auto xoutRgb = pipeline.create<dai::node::XLinkOut>();

    xoutRgb->setStreamName("rgb");

    // Properties
    camRgb->setPreviewSize(300, 300);
    camRgb->setBoardSocket(dai::CameraBoardSocket::CAM_A);
    camRgb->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1080_P);
    camRgb->setInterleaved(false);
    camRgb->setColorOrder(dai::ColorCameraProperties::ColorOrder::RGB);

    // Linking
    camRgb->preview.link(xoutRgb->input);

    // Connect to device and start pipeline
    dai::Device device(pipeline, std::get<1>(reconnect), dai::UsbSpeed::SUPER);

    // cout << "Connected cameras: " << device.getConnectedCameraFeatures() << endl;

    // Print USB speed
    cout << "Usb speed: " << device.getUsbSpeed() << endl;

    // Bootloader version
    if(device.getBootloaderVersion()) {
        cout << "Bootloader version: " << device.getBootloaderVersion()->toString() << endl;
    }

    // Device name
    cout << "Device name: " << device.getDeviceName() << endl;

    // Output queue will be used to get the rgb frames from the output defined above
    auto qRgb = device.getOutputQueue("rgb", 4, false);

    auto getTime = seconds(30);
    while(true) {
        bool hasTimedOut = false;
        auto inRgb = qRgb->get<dai::ImgFrame>(getTime, hasTimedOut);
        if (hasTimedOut) {
            printf("Queue receive timeout, will try to reconnect...\n");
            break;
        }

        // Retrieve 'bgr' (opencv format) frame
        cv::imshow("rgb", inRgb->getCvFrame());

        int key = cv::waitKey(1);
        if(key == 'q' || key == 'Q') {
            break;
        }
    }
  }
    return 0;
}
