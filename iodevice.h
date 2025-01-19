#pragma once

#include "libevdev/libevdev.h"
#include <filesystem>
#include <map>

// Admittedly not a very inspired class name, but, IoDevice eats a keyboard's keypressed via evdev (input), then feeds a joystick via g_hid (output).
class IoDevice {

    libevdev *mLibevdev = nullptr;
    int32_t mFdJoystick = 0, mFdKeyboard = 0;

    // maps from keyboard key's code to its status (key down is true, key up is false)
    std::map<uint32_t, bool> mSwitchStatus;

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
};
