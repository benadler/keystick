#pragma once

#include "libevdev/libevdev.h"
#include <filesystem>
#include <map>

// Eats a keyboard (I) via evdev, creates a joystick (O) via g_hid
class IoDevice {
    libevdev *mLibevdev = NULL;

    int mFdJoystick = 0, mFdKeyboard = 0;
    fd_set mRfds; // not sure what this is yet

    //char mReport[8];
    struct Switch {
      bool isPressed = false;
      uint32_t value = 0;
    };
      std::map<uint32_t, Switch> mStatus;

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
