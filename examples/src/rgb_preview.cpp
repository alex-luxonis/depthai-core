#include <iostream>

// Inludes common necessary includes for development using depthai library
#include "depthai/depthai.hpp"

class MultiCamSystem {
  public:
    std::string deviceName;
    std::shared_ptr<dai::DataOutputQueue> qFrame;

    std::shared_ptr<dai::DataOutputQueue> getFrameQueue() {
        return qFrame;
    }
};

int main() {
    using namespace std;
    // Create pipeline
    dai::Pipeline pipeline;

    // Define source and output
    auto camRgb = pipeline.create<dai::node::ColorCamera>();
    auto xoutRgb = pipeline.create<dai::node::XLinkOut>();

    xoutRgb->setStreamName("frame");

    // Properties
    camRgb->setPreviewSize(300, 300);
    camRgb->setBoardSocket(dai::CameraBoardSocket::RGB);
    camRgb->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1080_P);
    camRgb->setInterleaved(false);
    camRgb->setColorOrder(dai::ColorCameraProperties::ColorOrder::RGB);

    // Linking
    camRgb->preview.link(xoutRgb->input);

    std::vector<dai::DeviceInfo> availableDevices = dai::Device::getAllAvailableDevices();
    std::vector<MultiCamSystem> cams;
    std::vector<dai::Device*> devs; // just to keep references for eventual later cleanup/etc
    for (int i=0; i < availableDevices.size(); i++)
    {
        std::cout << "Connected to: "<< availableDevices[i].getMxId() << std::endl;
        dai::Device *device = new dai::Device(pipeline, availableDevices[i]);
        devs.push_back(device);
        if (1) {
            cout << "  -> Connected cameras: ";
            for(const auto& c : device->getConnectedCameras()) {
                cout << c << " ";
            }
            cout << endl;
            cout << "  -> Usb speed: " << device->getUsbSpeed() << endl;
        }

        MultiCamSystem cam;
        cam.deviceName = availableDevices[i].getMxId();
        cam.qFrame = device->getOutputQueue("frame", 1, false);
        //cam.qClassifier = device.getOutputQueue("nn", 1, false);
        cams.push_back(cam);
    }

    while(true) {
        for(auto& c : cams) {
            auto inRgb = c.getFrameQueue()->get<dai::ImgFrame>();

            // Retrieve 'bgr' (opencv format) frame
            cv::imshow("rgb-" + c.deviceName, inRgb->getCvFrame());
        }

        int key = cv::waitKey(1);
        if(key == 'q' || key == 'Q') {
            break;
        }
    }
    return 0;
}
