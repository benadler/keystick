#pragma once

#include <filesystem>
#include <vector>

struct usbg_state;
struct usbg_gadget;
struct usbg_config;
struct usbg_function;

// Creates a USB gadget with multiple functions (hidg devices, each of which associated to a joystick seen by windows).
// Requires https://github.com/linux-usb-gadgets/libusbgx/
// With a stock kernel, the number of joysticks is limted to 4, see https://unix.stackexchange.com/questions/553957.

class DeviceHid {
    usbg_state *mUsbGadgetState = nullptr;
    usbg_gadget *mUsbGadget = nullptr;
    usbg_config *mUsbConfig = nullptr;
    // We create numberOfDevices of these, one per joystick
    std::vector<usbg_function *> mUsbFunctions;

  public:
    DeviceHid() = default;

    // may throw (e.g. for insufficient kerle support or permissions)
    void initialize(const std::string& name, const size_t numberOfDevices);

    // Will clean up/remove the gadget
    ~DeviceHid();
};
