cmake_minimum_required(VERSION 3.31)

project(keystick LANGUAGES CXX)

 set(CMAKE_CXX_STANDARD 23)
 set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_search_module(LIBEVDEV REQUIRED libevdev)

find_package(LibUsbgx REQUIRED)

add_executable(keystick
    keystick.cpp
    iodevice.cpp
    devicehid.cpp)

target_include_directories(
    keystick
    PRIVATE
    ${LIBEVDEV_INCLUDE_DIRS})

target_link_libraries(
    keystick
    PRIVATE
    ${LIBEVDEV_LIBRARIES}
    LibUsbgx::LibUsbgx)
