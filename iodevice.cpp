#include "iodevice.h"
#if 0
#include <format>
#include <iostream>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUF_LEN 512

static void print_abs_bits(struct libevdev *dev, int axis) {
    const struct input_absinfo *abs;

    if (!libevdev_has_event_code(dev, EV_ABS, axis))
        return;

    abs = libevdev_get_abs_info(dev, axis);

    printf("	Value	%6d\n", abs->value);
    printf("	Min	%6d\n", abs->minimum);
    printf("	Max	%6d\n", abs->maximum);
    if (abs->fuzz)
        printf("	Fuzz	%6d\n", abs->fuzz);
    if (abs->flat)
        printf("	Flat	%6d\n", abs->flat);
    if (abs->resolution)
        printf("	Resolution	%6d\n", abs->resolution);
}

static void print_code_bits(struct libevdev *dev, unsigned int type, unsigned int max) {
    unsigned int i;
    for (i = 0; i <= max; i++) {
        if (!libevdev_has_event_code(dev, type, i))
            continue;

        printf("    Event code %i (%s)\n", i, libevdev_event_code_get_name(type, i));
        if (type == EV_ABS)
            print_abs_bits(dev, i);
    }
}

static void print_bits(struct libevdev *dev) {
    unsigned int i;
    printf("Supported events:\n");

    for (i = 0; i <= EV_MAX; i++) {
        if (libevdev_has_event_type(dev, i))
            printf("  Event type %d (%s)\n", i, libevdev_event_type_get_name(i));
        switch (i) {
        case EV_KEY:
            print_code_bits(dev, EV_KEY, KEY_MAX);
            break;
        case EV_REL:
            print_code_bits(dev, EV_REL, REL_MAX);
            break;
        case EV_ABS:
            print_code_bits(dev, EV_ABS, ABS_MAX);
            break;
        case EV_LED:
            print_code_bits(dev, EV_LED, LED_MAX);
            break;
        }
    }
}

static void print_props(struct libevdev *dev) {
    unsigned int i;
    printf("Properties:\n");

    for (i = 0; i <= INPUT_PROP_MAX; i++) {
        if (libevdev_has_property(dev, i))
            printf("  Property type %d (%s)\n", i, libevdev_property_get_name(i));
    }
}

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

IoDevice::IoDevice(const std::filesystem::path &file) {
    initKeyboard(file);
    initJoystick(file);
}

IoDevice::initKeyboard(const std::filesystem::path &file) {

    if (!std::filesystem::exists(file))
        throw std::runtime_error(std::format("the given keyboard file {} does not exist", file));

    if (argc < 2)
        goto out;

    int fd = open(file.c_str(), O_RDONLY);
    if (fd < 0)
        throw std::runtime_error(std::format("the given file {} could not be opened. Running as root?", file));

    int rc = libevdev_new_from_fd(fd, &mLibevdev);
    if (rc < 0)
        throw std::runtime_error(std::format("failed to initialize evdev on file {}: {}", file, strerror(-rc)));

    printf("Input device ID: bus %#x vendor %#x product %#x\n", libevdev_get_id_bustype(mLibevdev),
           libevdev_get_id_vendor(mLibevdev), libevdev_get_id_product(mLibevdev));
    printf("Evdev version: %x\n", libevdev_get_driver_version(mLibevdev));
    printf("Input device name: \"%s\"\n", libevdev_get_name(mLibevdev));
    printf("Phys location: %s\n", libevdev_get_phys(mLibevdev));
    printf("Uniq identifier: %s\n", libevdev_get_uniq(mLibevdev));
    print_bits(mLibevdev);
    print_props(mLibevdev);
}

int joystick_fill_report(char report[8], char buf[BUF_LEN], int *hold) {
    char *tok = strtok(buf, " ");
    int mvt = 0;
    int i = 0;

    *hold = 1;

    /* set default hat position: neutral */
    report[3] = 0x04;

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
    }
    return 4;
}

const std::filesystem::path IoDevice::initHid() {

    // inspired by libusbgx's examples/gadget-hid.c
}

void IoDevice::initJoystick(const std::filesystem::path &file) {

    // @file should be e.g. /dev/hidg0, which must already exist!

    if (!std::filesystem::exists(file))
        throw std::runtime_error(std::format("the given joystick hidg device file {} does not exist", file));

    if ((mFdJoystick = open(file.c_str(), O_RDWR, 0666)) == -1)
        throw std::runtime_error(std::format("the given file {} could not be opened. Running as root?", file));

    while (42) {

        FD_ZERO(&mRfds);
        FD_SET(STDIN_FILENO, &mRfds);
        FD_SET(fd, &mRfds);

        const int retval = select(mFdJoystick + 1, &mRfds, NULL, NULL, NULL);
        if (retval == -1 && errno == EINTR)
            continue;

        if (retval < 0)
            throw std::runtime_error(std::format("select on {} failed: {}", file, strerror(errno)));

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
                throw std::runtime_error(std::format("write on {} failed: {}", file, strerror(errno)));

            if (!hold) {
                memset(report, 0x0, sizeof(report));
                if (write(mFdJoystick, report, to_send) != to_send)
                    throw std::runtime_error(std::format("write 0 on {} failed: {}", file, strerror(errno)));
            }
        }
    }
}

void IoDevice::process() {

    int rc = 0;
    do {
        input_event ev;
        rc = libevdev_next_event(mLibevdev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);
        if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            printf("::::::::::::::::::::: dropped ::::::::::::::::::::::\n");
            while (rc == LIBEVDEV_READ_STATUS_SYNC) {
                print_sync_event(&ev);
                rc = libevdev_next_event(mLibevdev, LIBEVDEV_READ_FLAG_SYNC, &ev);
            }
            printf("::::::::::::::::::::: re-synced ::::::::::::::::::::::\n");
        } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS)
            print_event(&ev);
    } while (rc == LIBEVDEV_READ_STATUS_SYNC || rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN);

    if (rc != LIBEVDEV_READ_STATUS_SUCCESS && rc != -EAGAIN)
        fprintf(stderr, "Failed to handle events: %s\n", strerror(-rc));
}

IoDevice::~IoDevice() {
    libevdev_free(mLibevdev);
    close(mFdJoystick);
}
#endif
