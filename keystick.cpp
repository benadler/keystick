#include "devicehid.h"
#include "keyboardjoystick.h"

#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

#include <csignal>
#include <fcntl.h>

#include "libevdev/libevdev.h"

#ifndef __cpp_lib_format
// std::format polyfill using fmtlib
#include <fmt/core.h>
namespace std {
using fmt::format;
}
#else
#include <format>
#endif

DeviceHid *ptrHid = nullptr;

volatile std::sig_atomic_t gSignalStatus;
void signalHandler(int signal) {
    std::cout << "signal handler" << std::endl;
    gSignalStatus = signal;
    delete ptrHid;
    std::cout << "deleted HID" << std::endl;
    _exit(signal);
}

int main(int argc, char **argv) {

    // shutdown/cleanup is pretty.. creative, but it works. THe only thing that matters is that DeviceHid's dtor is
    // called, so the gadget gets removed. Else we'd have to reboot between runs.
    std::signal(SIGINT, signalHandler);

    try {
        // Iterate input devices, check which ones are real keyboards
        std::vector<std::filesystem::directory_entry> keyboards;
        for (const auto &entry : std::filesystem::directory_iterator("/dev/input")) {

            // std::cout << "Checking " << entry.path() << "..." << std::endl;

            if (!entry.is_character_file()) {
                // std::cout << entry.path() << " is not a character file, skipping" << std::endl;
                continue;
            }

            if (!entry.path().filename().string().starts_with("event")) {
                // std::cout << entry.path() << " doesn't start with event, skipping" << std::endl;
                continue;
            }

            // std::cout << "Trying device " << entry.path() << std::endl;

            if (!KeyboardJoystick::isKeyboard(entry.path()))
                continue;

            keyboards.push_back(entry);
        }

        std::cout << "Found " << keyboards.size() << " keyboards!" << std::endl;

        ptrHid = new DeviceHid;
        try {
            ptrHid->initialize("virtual joysticks from keystick", keyboards.size());
        } catch (const std::exception &e) {
            std::cerr << "HID exception: " << e.what() << std::endl;
            delete ptrHid;
            exit(1);
        }

        // We just created a single /dev/hidgX device. Let's find it!
        std::filesystem::directory_entry hidgDevice;
        for (const auto &entry : std::filesystem::directory_iterator("/dev/")) {
            if (entry.path().filename().string().starts_with("hidg")) {
                std::cout << entry.path() << " starts with hidg, using it!" << std::endl;
                hidgDevice = entry;
            }
        }

        int32_t fdHidg = 0;
        if ((fdHidg = open(hidgDevice.path().c_str(), O_RDWR, 0666)) == -1)
            throw std::runtime_error(
                std::format("the given file {} could not be opened. Running as root?", hidgDevice.path().string()));

        std::mutex mutex;
        std::vector<std::thread> threads;
        std::atomic<bool> shutdown = false;
        for (size_t i = 0; i < keyboards.size(); i++) {
            const std::filesystem::directory_entry &keyboard = keyboards.at(i);
            std::cout << "Creating thread to handle keyboard " << i << std::endl;
            threads.emplace_back(
                [&](const std::filesystem::directory_entry &keyboard, const uint8_t deviceIndex) {
                    KeyboardJoystick kj(
                        keyboard.path(), [hidgDevice, fdHidg, &mutex, deviceIndex](std::array<uint8_t, 4> report) {
                            report[0] = deviceIndex + 1; // set the device id in the report
                            const std::lock_guard<std::mutex> lock(mutex);
                            if (write(fdHidg, report.data(), report.size()) != 4)
                                throw std::runtime_error(std::format("write on hidg {} failed: {}",
                                                                     hidgDevice.path().string(), strerror(errno)));
                        });
                    while (!shutdown)
                        kj.process();
                },
                keyboard, uint8_t(i));
        }

        std::cout << "Done, now waiting for processing threads to end..." << std::endl;
        for (auto &thread : threads)
            thread.join();

        delete ptrHid;

    } catch (const std::exception &e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
    }

    return 0;
}
