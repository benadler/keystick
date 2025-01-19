#include "iodevice.h"

#include <iostream>
#include <set>
#include <vector>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef __cpp_lib_format
// std::format polyfill using fmtlib
#include <fmt/core.h>
namespace std {
using fmt::format;
}
#else
#include <format>
#endif

IoDevice::IoDevice(const std::filesystem::path &keyboard, const std::filesystem::path &joystick)
    : mPathKeyboard(keyboard), mPathJoystick(joystick) {
    initKeyboard();
    initJoystick();
}

IoDevice::~IoDevice() {
    libevdev_free(mLibevdev);
    close(mFdJoystick);
    close(mFdKeyboard);
}

void IoDevice::initKeyboard() {

    if (!std::filesystem::exists(mPathKeyboard))
        throw std::runtime_error(std::format("the given keyboard file {} does not exist", mPathKeyboard.string()));

    mFdKeyboard = open(mPathKeyboard.c_str(), O_RDONLY);
    if (mFdKeyboard < 0)
        throw std::runtime_error(
            std::format("the given file {} could not be opened. Running as root?", mPathKeyboard.string()));

    int rc = libevdev_new_from_fd(mFdKeyboard, &mLibevdev);
    if (rc < 0)
        throw std::runtime_error(
            std::format("failed to initialize evdev on file {}: {}", mPathKeyboard.string(), strerror(-rc)));

    std::cout << mPathKeyboard.filename().string() << " using keyboard fd " << mFdKeyboard << std::endl;
}

void IoDevice::initJoystick() {

    // @path should be e.g. /dev/hidg0, which must already exist!

    if (!std::filesystem::exists(mPathJoystick))
        throw std::runtime_error(
            std::format("the given joystick hidg device file {} does not exist", mPathJoystick.string()));

    if ((mFdJoystick = open(mPathJoystick.c_str(), O_RDWR, 0666)) == -1)
        throw std::runtime_error(
            std::format("the given file {} could not be opened. Running as root?", mPathJoystick.string()));
}

void IoDevice::process() {

    int rc = 0;
    do {
        input_event ev;
        rc = libevdev_next_event(mLibevdev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);
        if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            std::cout << "SYNC" << std::endl;
            while (rc == LIBEVDEV_READ_STATUS_SYNC)
                rc = libevdev_next_event(mLibevdev, LIBEVDEV_READ_FLAG_SYNC, &ev);
        } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            if (ev.value == 2) // means key is held down, repeated presses. Ignore.
                continue;

            if (ev.type == EV_SYN) // What's a SYN_REPORT?
                continue;

            if (ev.type == EV_MSC) // Something scancode something?
                continue;

            // std::cout << mPathKeyboard.filename().string() << " at " << ev.input_event_sec << "." <<
            // ev.input_event_usec << ": " << ev.type << " evttype " << libevdev_event_type_get_name(ev.type) << " code
            // " << ev.code << " (" << libevdev_event_code_get_name(ev.type, ev.code) << ") value " <<  ev.value <<
            // std::endl;

            static const std::set<uint32_t> keys{KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_A, KEY_S,
                                                 KEY_D,  KEY_F,    KEY_Z,    KEY_X,     KEY_C, KEY_V};
            // write joystick!
            if (!keys.contains(ev.code))
                continue;

            mSwitchStatus[ev.code] = ev.value > 0 ? true : false;

            // std::cout << mPathKeyboard.filename().string() << " sending key " <<
            // libevdev_event_code_get_name(ev.type, ev.code) << (ev.value == 0 ? " UP" : " DOWN") << " to " <<
            // mPathJoystick.filename().string() << std::endl;

            int8_t mReport[8];
            memset(mReport, 0x0, sizeof(mReport));
            if (mSwitchStatus[KEY_LEFT])
                mReport[0] = -127;
            else if (mSwitchStatus[KEY_RIGHT])
                mReport[0] = 127;
            else
                mReport[0] = 0;

            if (mSwitchStatus[KEY_UP])
                mReport[1] = -127;
            else if (mSwitchStatus[KEY_DOWN])
                mReport[1] = 127;
            else
                mReport[1] = 0;

            static const std::vector<uint32_t> buttons{KEY_F, KEY_D, KEY_S, KEY_A, KEY_V, KEY_C, KEY_X, KEY_Z};
            for (size_t i = 0; i < buttons.size(); i++)
                if (mSwitchStatus[buttons[i]])
                    mReport[2] |= 0x01 << i;
                else
                    mReport[2] &= ~(0x01 << i);

            if (write(mFdJoystick, mReport, 3) != 3)
                throw std::runtime_error(
                    std::format("write on {} failed: {}", mPathJoystick.string(), strerror(errno)));
        }
    } while (rc == LIBEVDEV_READ_STATUS_SYNC || rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN);

    if (rc != LIBEVDEV_READ_STATUS_SUCCESS && rc != -EAGAIN)
        throw std::runtime_error(std::format("failed to handle events: {}", strerror(-rc)));
}

bool IoDevice::isKeyboard(const std::filesystem::path &path) {

    // For now, a device is a keyboard if it matches all of these criteria:
    // - has EV_REP event type
    // - has EV_KEY event type
    // - has EV_KEY event codes KEY_LEFT, KEY_UP, KEY_RIGHT, KEY_DOWN
    // - has EV_LED event type
    // - has EV_LED event code LED_CAPSL

    if (!std::filesystem::exists(path))
        throw std::runtime_error(std::format("the given keyboard file {} does not exist", path.string()));

    const int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        throw std::runtime_error(std::format("the given file {} could not be opened. Running as root?", path.string()));

    libevdev *libevdev = nullptr;

    int rc = libevdev_new_from_fd(fd, &libevdev);
    if (rc < 0)
        throw std::runtime_error(
            std::format("failed to initialize evdev on file {}: {}", path.string(), strerror(-rc)));

    // until std::experimental::scope_exit is available...
    std::shared_ptr<int> x(NULL, [&](int *) {
        // std::cout << path.string() << " freeing resources!" << std::endl;
        libevdev_free(libevdev);
        close(fd);
    });

    if (!libevdev_has_event_type(libevdev, EV_REP))
        return false;

    if (!libevdev_has_event_type(libevdev, EV_KEY))
        return false;

    for (const uint32_t &key : {KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_A, KEY_S, KEY_D, KEY_F})
        if (!libevdev_has_event_code(libevdev, EV_KEY, key))
            return false;

    if (!libevdev_has_event_type(libevdev, EV_LED))
        return false;

    if (!libevdev_has_event_code(libevdev, EV_LED, LED_CAPSL))
        return false;

    std::cout << path.string() << " is a keyboard!" << std::endl;

    return true;
}
