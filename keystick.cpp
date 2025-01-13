#include "devicehid.h"
#include "iodevice.h"

#include <iostream>

int main(int argc, char **argv) {

    std::cout << "main function" << std::endl;

    try {
        DeviceHid dh1("gadget one", 16);
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
