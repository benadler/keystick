#include "devicehid.h"

#include <iostream>

#include <errno.h>
#include <linux/usb/ch9.h>
#include <stdio.h>
#include <usbg/function/hid.h>
#include <usbg/usbg.h>

#ifndef __cpp_lib_format
  // std::format polyfill using fmtlib
  #include <fmt/core.h>
  namespace std {
  using fmt::format;
  }
#else
  #include <format>
#endif


#define VENDOR 0x1d6b
#define PRODUCT 0x0104

// Use http://eleccelerator.com/usbdescreqparser/ to understand report descriptors
// clang-format off
static uint8_t report_desc[] = {
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x04, // Usage (Joystick)
    0xA1, 0x01, // Collection (Application)
    0x15, 0x81, //   Logical Minimum (-127)
    0x25, 0x7F, //   Logical Maximum (127)
    0x09, 0x01, //   Usage (Pointer)
    0xA1, 0x00, //   Collection (Physical)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x02, //     Report Count (2)
    0x81, 0x02, //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,       //   End Collection
    0x05, 0x09, //   Usage Page (Button)
    0x19, 0x01, //   Usage Minimum (0x01)
    0x29, 0x08, //   Usage Maximum (0x08)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x08, //   Report Count (8)
    0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,       // End Collection
};
// clang-format on
static_assert(sizeof(report_desc) == 42, "sizeof() is incorrect!");

void check(const int32_t error, const std::string& task, const std::string& hint) {
    if (error != USBG_SUCCESS)
        throw std::runtime_error(std::format("error on {}: {} ({})! {}",
                                             task,
                                             usbg_error_name(usbg_error(error)),
                                             usbg_strerror(usbg_error(error)),
                                             hint));
    std::cout << "success " << task << std::endl;
}

void DeviceHid::initialize(const std::string& name, const size_t numberOfDevices) {

    struct usbg_gadget_attrs g_attrs = {
        .bcdUSB = 0x0200,
        // should probably be USB_CLASS_COMM or USB_CLASS_HID
        .bDeviceClass = USB_CLASS_HID, // maybe should be USB_CLASS_MISC
        .bDeviceSubClass = 0x00,       // maybe should be 0x02
        .bDeviceProtocol = 0x00,       // maybe should be 0x01
        .bMaxPacketSize0 = 64, // Max allowed ep0 packet size
        .idVendor = VENDOR,
        .idProduct = PRODUCT,
        .bcdDevice = 0x0001, // Version of device
    };

    struct usbg_gadget_strs g_strs = {
        .manufacturer = (char *)("Foo Inc."), // Manufacturer
        .product = (char *)("Bar Gadget"),    // Product string
        .serial = (char *)("0123456789")      // Serial number
    };

    struct usbg_config_strs c_strs = {.configuration = (char *)("1xHID")};

    struct usbg_f_hid_attrs f_attrs = {
        .protocol = 1,
        .report_desc =
            {
                .desc = reinterpret_cast<char *>(report_desc),
                .len = sizeof(report_desc),
            },
        .report_length = 8,
        .subclass = 0,
    };

    check(usbg_init("/sys/kernel/config", &mUsbGadgetState), "initializing usbg", "Is the kernel module loaded, are you root?");

    // creates /sys/kernel/config/usb_gadget/${name}/
    check(usbg_create_gadget(mUsbGadgetState, name.c_str(), &g_attrs, &g_strs, &mUsbGadget), "creating USB gadget", "");

    check(usbg_create_config(mUsbGadget, 1, "KeyStick", NULL, &c_strs, &mUsbConfig), "creating USB config", "");

    for(size_t i=0; i<numberOfDevices; i++) {
        // creates /sys/kernel/config/usb_gadget/${name}/functions/hid.usb${i}
        mUsbFunctions.push_back(nullptr);
        const std::string funcInstanceName = std::string("usb") + std::to_string(i);
        check(usbg_create_function(mUsbGadget, USBG_F_HID, funcInstanceName.c_str(), &f_attrs, &mUsbFunctions.back()), "creating USB function","");
        const std::string nameConfFuncBinding = std::string("function") + std::to_string(i);
        check(usbg_add_config_function(mUsbConfig, nameConfFuncBinding.c_str(), mUsbFunctions.back()), "adding USB function", "");
    }

    check(usbg_enable_gadget(mUsbGadget, DEFAULT_UDC), "enabling USB gadget", "");

    // Now /sys/kernel/config/usb_gadget/${name}/functions/hid.usb0/dev contains e.g. 236:1,
    // which matches major/minor device number of a /dev/hidgX device file. I guess that's how we can associate this?

    std::cout << "Succesfully enabled gadget " << name << std::endl;
}

DeviceHid::~DeviceHid() {
    if(mUsbGadget)
        check(usbg_disable_gadget(mUsbGadget), "disabling USB gadget", "");

    if(mUsbGadget)
        // Remove gadget with USBG_RM_RECURSE flag to remove also its configurations, functions and strings
        check(usbg_rm_gadget(mUsbGadget, USBG_RM_RECURSE), "removing USB gadget", "");

    if (mUsbGadgetState)
        usbg_cleanup(mUsbGadgetState);
}
