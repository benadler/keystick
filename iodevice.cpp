#include "iodevice.h"


#include <iostream>
#include <set>

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

#define BUF_LEN 512

void print_event(struct input_event *ev) {
    if (ev->type == EV_SYN)
        printf("Event: time %ld.%06ld, ++++++++++++++++++++ %s +++++++++++++++\n", ev->input_event_sec,
               ev->input_event_usec, libevdev_event_type_get_name(ev->type));
    else
        printf("Event: time %ld.%06ld, type %d (%s), code %d (%s), value %d\n", ev->input_event_sec,
               ev->input_event_usec, ev->type, libevdev_event_type_get_name(ev->type), ev->code,
               libevdev_event_code_get_name(ev->type, ev->code), ev->value);
}

void print_sync_event(struct input_event *ev) {
    printf("SYNC: ");
    print_event(ev);
}

IoDevice::IoDevice(const std::filesystem::path &keyboard, const std::filesystem::path &joystick) :
    mPathKeyboard(keyboard),
    mPathJoystick(joystick)
{
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
        throw std::runtime_error(std::format("the given file {} could not be opened. Running as root?", mPathKeyboard.string()));

    int rc = libevdev_new_from_fd(mFdKeyboard, &mLibevdev);
    if (rc < 0)
        throw std::runtime_error(std::format("failed to initialize evdev on file {}: {}", mPathKeyboard.string(), strerror(-rc)));

    std::cout << mPathKeyboard.filename().string() << " using keyboard fd " << mFdKeyboard << std::endl;
}

int joystick_fill_report(char report[8], char buf[BUF_LEN], int *hold) {
    char *tok = strtok(buf, " ");
    int mvt = 0;
    int i = 0;

    *hold = 1;

    /* set default hat position: neutral */
    report[3] = 0x04;
/*
    while (42) {

        FD_ZERO(&mRfds);
        FD_SET(STDIN_FILENO, &mRfds);
        FD_SET(mFdJoystick, &mRfds);

        const int retval = select(mFdJoystick + 1, &mRfds, NULL, NULL, NULL);
        if (retval == -1 && errno == EINTR)
            continue;

        if (retval < 0)
            throw std::runtime_error(std::format("select on {} failed: {}", mPathJoystick.string(), strerror(errno)));

        int cmd_len;
        char buf[BUF_LEN];
        if (FD_ISSET(mFdJoystick, &mRfds)) {
            cmd_len = read(mFdJoystick, buf, BUF_LEN - 1);
            printf("recv report:");
            for (int i = 0; i < cmd_len; i++)
                printf(" %02x", buf[i]);
            printf("\n");
        }

        if (FD_ISSET(STDIN_FILENO, &mRfds)) {
            char report[8];
            memset(report, 0x0, sizeof(report));
            cmd_len = read(STDIN_FILENO, buf, BUF_LEN - 1);

            if (cmd_len == 0)
                break;

            buf[cmd_len - 1] = '\0';
            int hold = 0;

            memset(report, 0x0, sizeof(report));
            int to_send = joystick_fill_report(report, buf, &hold);

            if (to_send == -1)
                break;

            if (write(mFdJoystick, report, to_send) != to_send)
                throw std::runtime_error(std::format("write on {} failed: {}", mPathJoystick.string(), strerror(errno)));

            if (!hold) {
                memset(report, 0x0, sizeof(report));
                if (write(mFdJoystick, report, to_send) != to_send)
                    throw std::runtime_error(std::format("write 0 on {} failed: {}", mPathJoystick.string(), strerror(errno)));
            }
        }
    }
 * ============================================================================
    for (; tok != NULL; tok = strtok(NULL, " ")) {

        if (strcmp(tok, "--quit") == 0)
            return -1;

        for (i = 0; jmod[i].opt != NULL; i++)
            if (strcmp(tok, jmod[i].opt) == 0) {
                report[3] = (report[3] & 0xF0) | jmod[i].val;
                break;
            }
        if (jmod[i].opt != NULL)
            continue;

        if (!(tok[0] == '-' && tok[1] == '-') && mvt < 3) {
            errno = 0;
            report[mvt++] = (char)strtol(tok, NULL, 0);
            if (errno != 0) {
                fprintf(stderr, "Bad value:'%s'\n", tok);
                report[mvt--] = 0;
            }
            continue;
        }

        fprintf(stderr, "unknown option: %s\n", tok);
    }*/
    return 4;
}

void IoDevice::initJoystick() {

    // @path should be e.g. /dev/hidg0, which must already exist!

    if (!std::filesystem::exists(mPathJoystick))
        throw std::runtime_error(std::format("the given joystick hidg device file {} does not exist", mPathJoystick.string()));

    if ((mFdJoystick = open(mPathJoystick.c_str(), O_RDWR, 0666)) == -1)
        throw std::runtime_error(std::format("the given file {} could not be opened. Running as root?", mPathJoystick.string()));

}

void IoDevice::process() {

    int rc = 0;
    do {
        input_event ev;
        rc = libevdev_next_event(mLibevdev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);
        if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            std::cout << "SYNC" << std::endl;
            while (rc == LIBEVDEV_READ_STATUS_SYNC) {
                print_sync_event(&ev);
                rc = libevdev_next_event(mLibevdev, LIBEVDEV_READ_FLAG_SYNC, &ev);
            }
            printf("::::::::::::::::::::: re-synced ::::::::::::::::::::::\n");
        } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            if(ev.value == 2) // means key is held down, repeated presses. Ignore.
                continue;

            if(ev.type == EV_SYN) // What's a SYN_REPORT?
                continue;

            if(ev.type == EV_MSC) // Something scancode something?
                continue;

            std::cout << mPathKeyboard.filename().string() << " at " << ev.input_event_sec << "." << ev.input_event_usec << ": " << ev.type << " evttype " << libevdev_event_type_get_name(ev.type) << " code " << ev.code << " (" << libevdev_event_code_get_name(ev.type, ev.code) << ") value " <<  ev.value << std::endl;


            static const std::set<uint32_t> keys{KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_A, KEY_S, KEY_D, KEY_F};
            // write joystick!
            if(!keys.contains(ev.code))
                continue;

            std::cout << mPathKeyboard.filename().string() << " sending key " << libevdev_event_code_get_name(ev.type, ev.code) << (ev.value == 0 ? " UP" : " DOWN") << " to " << mPathJoystick.filename().string() << std::endl;
        }

    } while (rc == LIBEVDEV_READ_STATUS_SYNC || rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN);

    if (rc != LIBEVDEV_READ_STATUS_SUCCESS && rc != -EAGAIN)
        fprintf(stderr, "Failed to handle events: %s\n", strerror(-rc));
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
        throw std::runtime_error(std::format("failed to initialize evdev on file {}: {}", path.string(), strerror(-rc)));

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

    for(const uint32_t& key : {KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_A, KEY_S, KEY_D, KEY_F})
        if (!libevdev_has_event_code(libevdev, EV_KEY, key))
            return false;

    if (!libevdev_has_event_type(libevdev, EV_LED))
        return false;

    if (!libevdev_has_event_code(libevdev, EV_LED, LED_CAPSL))
        return false;

    std::cout << path.string() << " is a keyboard!" << std::endl;

    return true;
}
