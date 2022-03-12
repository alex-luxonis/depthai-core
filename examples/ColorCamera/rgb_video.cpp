#include <iostream>

// Includes common necessary includes for development using depthai library
#include "depthai/depthai.hpp"

bool enableUVC = 0;

int main() {
    // Create pipeline
    dai::Pipeline pipeline;

    // Define source and output
    auto camRgb = pipeline.create<dai::node::ColorCamera>();
    auto xoutVideo = pipeline.create<dai::node::XLinkOut>();

    xoutVideo->setStreamName("video");

    // Properties
    camRgb->setBoardSocket(dai::CameraBoardSocket::RGB);
//    camRgb->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1080_P);
    camRgb->setResolution(dai::ColorCameraProperties::SensorResolution::THE_4_K);
    camRgb->setIspScale(1, 2);
    camRgb->setVideoSize(1920, 1080);
    camRgb->initialControl.setAntiBandingMode(dai::CameraControl::AntiBandingMode::MAINS_60_HZ);
    camRgb->setFps(38);

    xoutVideo->input.setBlocking(false);
    xoutVideo->input.setQueueSize(1);

    auto uac = pipeline.create<dai::node::UAC>();
    uac->initialConfig.setMicGainTimes(7);
    //uac->initialConfig.setMicGainDecibels(7);

    auto audioIn = pipeline.create<dai::node::XLinkOut>();
    audioIn->setStreamName("mic");
    uac->out.link(audioIn->input);

    auto audioInCfg = pipeline.create<dai::node::XLinkIn>();
    audioInCfg->setStreamName("micCfg");
    audioInCfg->out.link(uac->inputConfig);

    // Linking
    if (enableUVC) {
        auto uvc = pipeline.create<dai::node::UVC>();
        camRgb->video.link(uvc->input);
        //camRgb->video.link(xoutVideo->input); // Could actually keep this as well
        printf(">>> Keep this running, and open a separate UVC viewer\n");
    } else {
        camRgb->video.link(xoutVideo->input);
    }

    // Connect to device and start pipeline
    auto config = dai::Device::Config();
    config.board.uvcEnable = enableUVC;
    dai::Device device(config);
    device.startPipeline(pipeline);

    int qsize = 1;
    bool blocking = false;
    auto video = device.getOutputQueue("video", qsize, blocking);
    auto mic   = device.getOutputQueue("mic", 20, blocking);
    auto micCfg = device.getInputQueue("micCfg");

    using namespace std::chrono;
    auto tprev = steady_clock::now();
    int count = 0;

    float gain = 10;
    while(true) {
        auto videoIn = video->get<dai::ImgFrame>();

        if (1) { // FPS calc
            auto tnow = steady_clock::now();
            count++;
            auto tdiff = duration<double>(tnow - tprev).count();
            if (tdiff >= 1) {
                double fps = count / tdiff;
                printf("FPS: %.3f\n", fps);
                count = 0;
                tprev = tnow;

                // Send every second a gain change command
                dai::AudioInConfig cfg;
                cfg.setMicGainTimes(gain);
                //cfg.setMicGainDecibels(7);
                micCfg->send(cfg);

                gain += 10;
                if (gain > 30) gain = 10;
            }
        }

        auto micPkt = mic->tryGet<dai::ImgFrame>();
        if (micPkt) {
            // real gain (in times) multiplied by 100 (saved as int), like ISO
            printf("Audio, gain: %.2f times\n", micPkt->getSensitivity() / 100.);
        }

        // Get BGR frame from NV12 encoded video frame to show with opencv
        // Visualizing the frame on slower hosts might have overhead
        cv::imshow("video", videoIn->getCvFrame());

        int key = cv::waitKey(1);
        if(key == 'q' || key == 'Q') {
            return 0;
        }
    }
    return 0;
}
