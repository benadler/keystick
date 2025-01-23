#pragma once

#include "libevdev/libevdev.h"
#include <array>
#include <filesystem>
#include <functional>
#include <map>

// Handles a physical keyboard through evdev, whenever the emulated joystick's state changes it calls
// mCallback with the appropriate report.
class KeyboardJoystick {

    libevdev *mLibevdev = nullptr;
    int32_t mFdKeyboard = 0;

    // maps from keyboard key's code to its status (key down is true, key up is false)
    std::map<uint32_t, bool> mSwitchStatus;

    // This class will call mCallback with a usb-hidg-joystick report of 4 bytes whenever keys are pressed.
    // Whoever uses this class shoul set this callback and then pass the report on to the USB side.
    std::function<void(std::array<uint8_t, 4>)> mCallback;

    std::filesystem::path mPathKeyboard;

  public:
    KeyboardJoystick(const std::filesystem::path &keyboard, std::function<void(std::array<uint8_t, 4>)> callback);
    ~KeyboardJoystick();

    void process();

    // Returns true only if we estimate the devic specified by @path to be a keyboard.
    // Somewhat fuzzy; run evdev's tools to see how one physical keyboard can spawn many input devices.
    static bool isKeyboard(const std::filesystem::path &path);

  private:
    void initJoystick();
    void initKeyboard();
};
