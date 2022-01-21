#include <iostream>

// Includes common necessary includes for development using depthai library
#include "depthai/depthai.hpp"

// Closer-in minimum depth, disparity range is doubled (from 95 to 190):
static std::atomic<bool> extended_disparity{false};
// Better accuracy for longer distance, fractional disparity 32-levels:
static std::atomic<bool> subpixel{false};
// Better handling for occlusions:
static std::atomic<bool> lr_check{true};

int main() {
    // Create pipeline
    dai::Pipeline pipeline;


    auto monoLeft = pipeline.create<dai::node::ColorCamera>();
    auto monoRight = pipeline.create<dai::node::ColorCamera>();
    auto rgb = pipeline.create<dai::node::ColorCamera>();
    auto depth = pipeline.create<dai::node::StereoDepth>();
    auto xoutDisparity = pipeline.create<dai::node::XLinkOut>();
    auto xoutDepth = pipeline.create<dai::node::XLinkOut>();
    auto xoutRectifiedRight = pipeline.create<dai::node::XLinkOut>();
    auto xoutRectifiedLeft = pipeline.create<dai::node::XLinkOut>();
    auto controlDepthLeft = pipeline.create<dai::node::XLinkIn>();
    auto controlDepthRight = pipeline.create<dai::node::XLinkIn>();
    auto controlColor = pipeline.create<dai::node::XLinkIn>();
    auto xoutRGB = pipeline.create<dai::node::XLinkOut>();

    // Properties
    monoLeft->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1200_P);
    monoLeft->setBoardSocket(dai::CameraBoardSocket::LEFT);
    monoLeft->setIspScale(1280,1920);

    monoRight->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1200_P);
    monoRight->setBoardSocket(dai::CameraBoardSocket::RIGHT);
    monoRight->setIspScale(1280,1920);

    rgb->setBoardSocket(dai::CameraBoardSocket::RGB);
    rgb->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1200_P);
    rgb->setPreviewSize(1280, 720);

    depth->initialConfig.setConfidenceThreshold(254);

    depth->initialConfig.setMedianFilter(dai::MedianFilter::KERNEL_5x5);
    depth->initialConfig.setBilateralFilterSigma(65000);
    //depth->setRectifyEdgeFillColor(0);
    depth->setLeftRightCheck(lr_check);
    depth->setExtendedDisparity(extended_disparity);
    depth->setSubpixel(subpixel);
    depth->setInputResolution(monoLeft->getIspSize());


    xoutRGB->setStreamName("rgb");
    xoutDisparity->setStreamName("disparity");
    xoutRectifiedRight->setStreamName("rectifiedRight");
    xoutRectifiedLeft->setStreamName("rectifiedLeft");
    controlDepthLeft->setStreamName("controlLeft");
    controlDepthRight->setStreamName("controlRight");
    controlColor->setStreamName("controlColor");
    xoutDepth->setStreamName("depth");

    // Linking

    monoLeft->isp.link(depth->left);
    monoRight->isp.link(depth->right);
    depth->depth.link(xoutDepth->input);
    depth->disparity.link(xoutDisparity->input);
    depth->rectifiedRight.link(xoutRectifiedRight->input);
    depth->rectifiedLeft.link(xoutRectifiedLeft->input);
    controlDepthLeft->out.link(monoLeft->inputControl);
    controlDepthRight->out.link(monoRight->inputControl);
    controlColor->out.link(rgb->inputControl);
    rgb->isp.link(xoutRGB->input);

    // Connect to device and start pipeline
    dai::Device device(pipeline);
    dai::CalibrationHandler calibData = device.readCalibration();


    auto qd = device.getOutputQueue("disparity", 2, false);
    auto qdepth = device.getOutputQueue("depth", 2, false);
    auto qr = device.getOutputQueue("rectifiedRight", 2, false);
    auto qrl = device.getOutputQueue("rectifiedLeft", 2, false);
    auto qc = device.getOutputQueue("rgb", 2, false);
    auto controlQueueLeft = device.getInputQueue("controlLeft");
    auto controlQueueRight = device.getInputQueue("controlRight");
    auto controlQueueColor = device.getInputQueue("controlColor");


    while(true) {
        auto inDepth = qdepth->get<dai::ImgFrame>();
        auto frame = inDepth->getFrame();

        // Divide values by 10, converting to cm
        frame.convertTo(frame, CV_8UC1, 0.1);

        cv::imshow("depth", frame);

        int key = cv::waitKey(1);
        if(key == 'q' || key == 'Q') {
            return 0;
        }
    }
    return 0;
}
