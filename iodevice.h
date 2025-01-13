#pragma once

#include "libevdev/libevdev.h"
#include <filesystem>

// Eats a keyboard (I) via evdev, creates a joystick (O) via g_hid
class IoDevice {
    libevdev *mLibevdev = NULL;

    int mFdJoystick = 0, mFdKeyboard = 0;
    fd_set mRfds; // not sure what this is yet

    std::filesystem::path mPathKeyboard, mPathJoystick;

  public:
    IoDevice(const std::filesystem::path &keyboard, const std::filesystem::path &joystick);
    ~IoDevice();

    void process();

    // Returns true only if we estimate the devic specified by @path to be a keyboard.
    // Somewhat fuzzy; run evdev's tools to see how one physical keyboard can spawn many input devices.
    static bool isKeyboard(const std::filesystem::path &path);

  private:
    void initJoystick();
    void initKeyboard();

    // creates a new hid device and returns it, for initJoystick() to use
    //const std::filesystem::path initHid();
};
