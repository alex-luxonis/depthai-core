#include <chrono>
#include <iostream>
#include <unordered_map>

// Includes common necessary includes for development using depthai library
#include "depthai/depthai.hpp"

bool previewEnabled = 0;

std::shared_ptr<dai::Pipeline> createPipeline(std::string rgbCamName, bool enablePreview) {
    printf("Creating pipeline for RGB camera: %s\n", rgbCamName.c_str());
    // Start defining a pipeline
    auto pipeline = std::make_shared<dai::Pipeline>();
    // Define a source - color camera
    auto camRgb = pipeline->create<dai::node::ColorCamera>();
    camRgb->setPreviewSize(1024, 768);
    camRgb->setVideoSize(1024, 768);

    camRgb->setFps(15.0);
    camRgb->setColorOrder(dai::ColorCameraProperties::ColorOrder::BGR);

    if(rgbCamName == "IMX296")
        camRgb->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1440X1080);
    else
        camRgb->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1080_P);
    //camRgb->setBoardSocket(dai::CameraBoardSocket::RGB);
    camRgb->setInterleaved(false);

    auto manipRgb = pipeline->create<dai::node::ImageManip>();
    dai::RotatedRect rgbRr = {{camRgb->getPreviewWidth() / 2.0f, camRgb->getPreviewHeight() / 2.0f},  // center
                              {camRgb->getPreviewHeight() * 1.0f, camRgb->getPreviewWidth() * 1.0f},  // size
                              90};                                                                    // angle
    manipRgb->initialConfig.setCropRotatedRect(rgbRr, false);
    manipRgb->initialConfig.setFrameType(dai::ImgFrame::Type::NV12);
    manipRgb->setMaxOutputFrameSize(1024 * 768 * 3);
    manipRgb->inputImage.setBlocking(false);  // true
    manipRgb->inputImage.setQueueSize(1);
    manipRgb->setNumFramesPool(1);

    auto videoEncoder = pipeline->create<dai::node::VideoEncoder>();
    videoEncoder->setDefaultProfilePreset(15, dai::VideoEncoderProperties::Profile::H264_MAIN);
    videoEncoder->setQuality(100);
    videoEncoder->input.setBlocking(true);
    videoEncoder->input.setQueueSize(1);

    auto xoutEnc = pipeline->create<dai::node::XLinkOut>();
    xoutEnc->setStreamName("encoded");

    camRgb->preview.link(manipRgb->inputImage);
    manipRgb->out.link(videoEncoder->input);
    videoEncoder->bitstream.link(xoutEnc->input);

    if(enablePreview) {
        auto xoutRgb = pipeline->create<dai::node::XLinkOut>();
        xoutRgb->setStreamName("preview");

        manipRgb->out.link(xoutRgb->input);
    }

    return pipeline;
}

int main(int argc, char** argv) {
    auto deviceInfoVec = dai::Device::getAllAvailableDevices();
    const auto usbSpeed = dai::UsbSpeed::SUPER;
    auto openVinoVersion = dai::OpenVINO::Version::VERSION_2021_4;

    std::map<std::string, std::shared_ptr<dai::DataOutputQueue>> qRgbMap;
    std::vector<std::shared_ptr<dai::Device>> devices;

    for(auto& deviceInfo : deviceInfoVec) {
        auto device = std::make_shared<dai::Device>(openVinoVersion, deviceInfo, usbSpeed);
        devices.push_back(device);
        std::cout << "===Connected to " << deviceInfo.getMxId() << std::endl;
        auto mxId = device->getMxId();
        auto cameras = device->getCameraSensorNames();
        auto usbSpeed = device->getUsbSpeed();
        auto eepromData = device->readCalibration2().getEepromData();
        std::cout << "   >>> MXID:" << mxId << std::endl;
        std::cout << "   >>> Num of cameras:" << cameras.size() << std::endl;
        std::cout << "   >>> USB speed:" << usbSpeed << std::endl;
        if(eepromData.boardName != "") {
            std::cout << "   >>> Board name:" << eepromData.boardName << std::endl;
        }
        if(eepromData.productName != "") {
            std::cout << "   >>> Product name:" << eepromData.productName << std::endl;
        }
        auto pipeline = createPipeline(cameras[dai::CameraBoardSocket::RGB], previewEnabled);
        device->startPipeline(*pipeline);

        std::string streamName;
        auto qEnc = device->getOutputQueue("encoded", 15, true);
        streamName = "enc-" + eepromData.productName + "-" + mxId;
        qRgbMap.insert({streamName, qEnc});
        if(previewEnabled) {
            auto qRgb = device->getOutputQueue("preview", 4, false);
            streamName = "rgb-" + eepromData.productName + "-" + mxId;
            qRgbMap.insert({streamName, qRgb});
        }
    }

    // pairs of frame count and accumulated size
    std::unordered_map<std::string, std::pair<uint64_t, uint64_t>> total;

    auto tprev = std::chrono::steady_clock::now();
    while(true) {
        for(auto& element : qRgbMap) {
            auto qRgb = element.second;
            auto streamName = element.first;
            auto inRgb = qRgb->tryGet<dai::ImgFrame>();
            if(inRgb != nullptr) {
                if(streamName.rfind("rgb", 0) == 0) {
                    cv::imshow(streamName, inRgb->getCvFrame());
                } else {
                    volatile auto rawFrame = inRgb->getData();
                    // printf("frame from %s, size %lu\n", streamName.c_str(), inRgb->getData().size());
                    total[streamName.c_str()] = std::make_pair(total[streamName.c_str()].first + 1, total[streamName.c_str()].second + inRgb->getData().size());
                }
            }
        }

        // Print frame stats every second
        auto tnow = std::chrono::steady_clock::now();
        using namespace std::chrono_literals;
        if(tnow - tprev >= 1s) {
            tprev = tnow;
            for(const auto& kv : total) {
                printf("%35s: %lu frames, size %lu\n", kv.first.c_str(), kv.second.first, kv.second.second);
            }
        }

        int key = cv::waitKey(1);
        if(key == 'q' || key == 'Q') {
            return 0;
        }
    }
    return 0;
}
