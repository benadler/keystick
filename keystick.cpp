#include "devicehid.h"
#include "iodevice.h"

#include <filesystem>
#include <iostream>
#include <vector>
#include <thread>

#include <csignal>

#include "libevdev/libevdev.h"

DeviceHid* ptrHid = nullptr;

volatile std::sig_atomic_t gSignalStatus;
void signalHandler(int signal)
{
    std::cout << "signal handler" << std::endl;
    gSignalStatus = signal;
    delete ptrHid;
    std::cout << "deleted HID" << std::endl;
    _exit(signal);
}

int main(int argc, char **argv) {

    std::signal(SIGINT, signalHandler);

    std::cout << "main function" << std::endl;

    try {
        // Iterate input devices, check which ones are real keyboards
        std::vector<std::filesystem::directory_entry> keyboards;
        for (const auto & entry : std::filesystem::directory_iterator("/dev/input")) {

            // std::cout << "Checking " << entry.path() << "..." << std::endl;

            if(!entry.is_character_file()){
                // std::cout << entry.path() << " is not a character file, skipping" << std::endl;
                continue;
            }

            if(!entry.path().filename().string().starts_with("event")){
                // std::cout << entry.path() << " doesn't start with event, skipping" << std::endl;
                continue;
            }

            // std::cout << "Trying device " << entry.path() << std::endl;

            if(!IoDevice::isKeyboard(entry.path()))
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

        // Now pray that we're seeing as many /dev/hidgX devices as we found keyboards!
        std::vector<std::filesystem::directory_entry> joysticks;
        for (const auto & entry : std::filesystem::directory_iterator("/dev/")) {
            if(entry.path().filename().string().starts_with("hidg")){
                std::cout << entry.path() << " starts with hidg, using it!" << std::endl;
                joysticks.push_back(entry);
            }
        }

        std::vector<std::thread> threads;
        std::atomic<bool> shutdown = false;
        for(size_t i=0; i < keyboards.size(); i++) {
            const std::filesystem::directory_entry& keyboard = keyboards.at(i);
            const std::filesystem::directory_entry& joystick = joysticks.at(i);

            threads.emplace_back(
                [&](const std::filesystem::directory_entry& keyboard, const std::filesystem::directory_entry& joystick) {
                    IoDevice device(keyboard.path(), joystick.path());
                    while(!shutdown) {
                        device.process();
                    }
                },
                keyboard, joystick);
        }

        for (auto& thread : threads) {
            std::cout << "Waiting for processing thread to end!" << std::endl;
            thread.join();
        }

        delete ptrHid;

    } catch (const std::exception &e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
    }

    return 0;
}
