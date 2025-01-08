#include "devicehid.h"

#include <errno.h>
#include <linux/usb/ch9.h>
#include <stdio.h>
#include <usbg/function/hid.h>
#include <usbg/usbg.h>

#include <format>

#define VENDOR 0x1d6b
#define PRODUCT 0x0104

// Use http://eleccelerator.com/usbdescreqparser/ to understand report descriptors

#if 0
// this is for a keyboard, don't want it
static char report_desc[] = {
    0x05, 0x01, /* USAGE_PAGE (Generic Desktop)	          */
    0x09, 0x06, /* USAGE (Keyboard)                       */
    0xa1, 0x01, /* COLLECTION (Application)               */
    0x05, 0x07, /*   USAGE_PAGE (Keyboard)                */
    0x19, 0xe0, /*   USAGE_MINIMUM (Keyboard LeftControl) */
    0x29, 0xe7, /*   USAGE_MAXIMUM (Keyboard Right GUI)   */
    0x15, 0x00, /*   LOGICAL_MINIMUM (0)                  */
    0x25, 0x01, /*   LOGICAL_MAXIMUM (1)                  */
    0x75, 0x01, /*   REPORT_SIZE (1)                      */
    0x95, 0x08, /*   REPORT_COUNT (8)                     */
    0x81, 0x02, /*   INPUT (Data,Var,Abs)                 */
    0x95, 0x01, /*   REPORT_COUNT (1)                     */
    0x75, 0x08, /*   REPORT_SIZE (8)                      */
    0x81, 0x03, /*   INPUT (Cnst,Var,Abs)                 */
    0x95, 0x05, /*   REPORT_COUNT (5)                     */
    0x75, 0x01, /*   REPORT_SIZE (1)                      */
    0x05, 0x08, /*   USAGE_PAGE (LEDs)                    */
    0x19, 0x01, /*   USAGE_MINIMUM (Num Lock)             */
    0x29, 0x05, /*   USAGE_MAXIMUM (Kana)                 */
    0x91, 0x02, /*   OUTPUT (Data,Var,Abs)                */
    0x95, 0x01, /*   REPORT_COUNT (1)                     */
    0x75, 0x03, /*   REPORT_SIZE (3)                      */
    0x91, 0x03, /*   OUTPUT (Cnst,Var,Abs)                */
    0x95, 0x06, /*   REPORT_COUNT (6)                     */
    0x75, 0x08, /*   REPORT_SIZE (8)                      */
    0x15, 0x00, /*   LOGICAL_MINIMUM (0)                  */
    0x25, 0x65, /*   LOGICAL_MAXIMUM (101)                */
    0x05, 0x07, /*   USAGE_PAGE (Keyboard)                */
    0x19, 0x00, /*   USAGE_MINIMUM (Reserved)             */
    0x29, 0x65, /*   USAGE_MAXIMUM (Keyboard Application) */
    0x81, 0x00, /*   INPUT (Data,Ary,Abs)                 */
    0xc0        /* END_COLLECTION                         */
};
#endif

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
static_assert(sizeof(report_desc) == 42, "sizeof() is incorrect!");

DeviceHid::DeviceHid() {

    struct usbg_gadget_attrs g_attrs = {
        .bcdUSB = 0x0200,
        // should probably be USB_CLASS_COMM or USB_CLASS_HID
        .bDeviceClass = USB_CLASS_HID,
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
        .idVendor = VENDOR,
        .idProduct = PRODUCT,
        .bcdDevice = 0x0001, /* Version of device */
    };

    struct usbg_gadget_strs g_strs = {
        .manufacturer = (char *)("Foo Inc."), /* Manufacturer */
        .product = (char *)("Bar Gadget"),    /* Product string */
        .serial = (char *)("0123456789")      /* Serial number */
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

    int usbg_err;
    usbg_err = usbg_init("/sys/kernel/config", &mUsbGadgetState);
    if (usbg_err != USBG_SUCCESS)
        throw std::runtime_error(std::format("error on usbg init: {} ({}). Maybe need to load gadget kernel module?",
                                             usbg_error_name(usbg_error(usbg_err)),
                                             usbg_strerror(usbg_error(usbg_err))));

    usbg_err = usbg_create_gadget(mUsbGadgetState, "g1", &g_attrs, &g_strs, &mUsbGadget);
    if (usbg_err != USBG_SUCCESS) {
        throw std::runtime_error(std::format("error creating USB gadget: {} {}", usbg_error_name(usbg_error(usbg_err)),
                                             usbg_strerror(usbg_error(usbg_err))));
    }
    usbg_err = usbg_create_function(mUsbGadget, USBG_F_HID, "usb0", &f_attrs, &mUsbGadgetFunction);
    if (usbg_err != USBG_SUCCESS)
        throw std::runtime_error(std::format("error creating USB function: {} {}",
                                             usbg_error_name(usbg_error(usbg_err)),
                                             usbg_strerror(usbg_error(usbg_err))));

    usbg_err = usbg_create_config(mUsbGadget, 1, "The only one", NULL, &c_strs, &mUsbConfig);
    if (usbg_err != USBG_SUCCESS)
        throw std::runtime_error(std::format("error creating USB config: {} {}", usbg_error_name(usbg_error(usbg_err)),
                                             usbg_strerror(usbg_error(usbg_err))));

    usbg_err = usbg_add_config_function(mUsbConfig, "some_name", mUsbGadgetFunction);
    if (usbg_err != USBG_SUCCESS)
        throw std::runtime_error(std::format("error adding USB function: {} {}", usbg_error_name(usbg_error(usbg_err)),
                                             usbg_strerror(usbg_error(usbg_err))));

    usbg_err = usbg_enable_gadget(mUsbGadget, DEFAULT_UDC);
    if (usbg_err != USBG_SUCCESS)
        throw std::runtime_error(std::format("error enabling USB gadget: {} {}", usbg_error_name(usbg_error(usbg_err)),
                                             usbg_strerror(usbg_error(usbg_err))));
}

DeviceHid::~DeviceHid() {
    if (mUsbGadgetState)
        usbg_cleanup(mUsbGadgetState);
}
