#pragma once

#include <filesystem>

struct usbg_state;
struct usbg_gadget;
struct usbg_config;
struct usbg_function;

// Creates a USB gadget, requires https://github.com/linux-usb-gadgets/libusbgx/

class DeviceHid {
    usbg_state *mUsbGadgetState = nullptr;
    usbg_gadget *mUsbGadget = nullptr;
    usbg_config *mUsbConfig = nullptr;
    usbg_function *mUsbGadgetFunction = nullptr;

  public:
    DeviceHid();
    ~DeviceHid();

    // Creates a joystick hid device, like /dev/hidg0.
    // Returns device if successful, throws if not
    const std::filesystem::path create();
};
