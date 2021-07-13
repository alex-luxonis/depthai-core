#include <iostream>

// Inludes common necessary includes for development using depthai library
#include "depthai/depthai.hpp"
void printSystemInformation(dai::SystemInformation info) {
    printf("Ddr used / total - %.2f / %.2f MiB\n", info.ddrMemoryUsage.used / (1024.0f * 1024.0f), info.ddrMemoryUsage.total / (1024.0f * 1024.0f));
    printf("Cmx used / total - %.2f / %.2f MiB\n", info.cmxMemoryUsage.used / (1024.0f * 1024.0f), info.cmxMemoryUsage.total / (1024.0f * 1024.0f));
    printf("LeonCss heap used / total - %.2f / %.2f MiB\n",
           info.leonCssMemoryUsage.used / (1024.0f * 1024.0f),
           info.leonCssMemoryUsage.total / (1024.0f * 1024.0f));
    printf("LeonMss heap used / total - %.2f / %.2f MiB\n",
           info.leonMssMemoryUsage.used / (1024.0f * 1024.0f),
           info.leonMssMemoryUsage.total / (1024.0f * 1024.0f));
    const auto& t = info.chipTemperature;
    printf("Chip temperature - average: %.2f, css: %.2f, mss: %.2f, upa0: %.2f, upa1: %.2f\n", t.average, t.css, t.mss, t.upa, t.dss);
    printf("Cpu usage - Leon OS: %.2f %%, Leon RT: %.2f %%\n", info.leonCssCpuUsage.average * 100, info.leonMssCpuUsage.average * 100);
    printf("----------------------------------------\n");
}

int main(int argc, char *argv[]) {
    // Create pipeline
    dai::Pipeline pipeline;

    // Define source and output
    auto sysLog = pipeline.create<dai::node::SystemLogger>();
    auto xout = pipeline.create<dai::node::XLinkOut>();

    xout->setStreamName("sysinfo");

    // Properties
    sysLog->setRate(1.0f);  // 1 hz updates

    // Linking
    sysLog->out.link(xout->input);

    // Connect to device and start pipeline
#if 0
    dai::Device device(pipeline);
#else
    bool usb2 = false;
    if (argc > 1 && std::string(argv[1]) == "2") {
        printf("=== Forcing USB2 mode\n");
        usb2 = true;
    }

    auto devs = dai::Device::getAllAvailableDevices();
    int total = 0;
    const char *states[] = {"any-state", "booted", "unbooted", "bootloader"};
    for (const auto& d : devs) {
        total++;
        printf("%d : MXID %s - %s\n", total, d.getMxId().c_str(), states[d.state]);
    }

    int choice = 1;
    if (total > 1) {
        std::cout << "Enter the device index to connect to: ";
        std::cin >> choice;
    }
    printf("Connecting to device %d, MXID %s in %s mode\n", choice,
            devs[choice - 1].getMxId().c_str(),
            usb2 ? "USB2" : "USB3");

    // Connect to device and start pipeline
    dai::Device device(pipeline, devs[choice - 1], usb2);
#endif

    // Output queue will be used to get the system info
    auto qSysInfo = device.getOutputQueue("sysinfo", 4, false);

    while(true) {
        auto sysInfo = qSysInfo->get<dai::SystemInformation>();
        printSystemInformation(*sysInfo);
    }
    return 0;
}
