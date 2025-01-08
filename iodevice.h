#pragma once

#include "libevdev/libevdev.h"
#include <filesystem>

// Eats a keyboard (I) via evdev, creates a joystick (O) via g_hid
class IoDevice {
    libevdev *mLibevdev = NULL;

    int mFdJoystick = 0;
    fd_set mRfds; // not sure what this is yet

  public:
    IoDevice(const std::filesystem::path &file);
    ~IoDevice();

    void process();

  private:
    void initJoystick(const std::filesystem::path &file);
    void initKeyboard(const std::filesystem::path &file);

    // creates a new hid device and returns it, for initJoystick() to use
    const std::filesystem::path initHid();
};
