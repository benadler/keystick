cmake_minimum_required(VERSION 3.25)

project(keystick LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_search_module(LIBEVDEV REQUIRED libevdev)
pkg_search_module(LIBUSBGX REQUIRED libusbgx)

# not available on raspbian
#find_package(LibUsbgx REQUIRED)
find_package(fmt REQUIRED)

add_executable(keystick
    keystick.cpp
    iodevice.cpp
    devicehid.cpp)

target_include_directories(
    keystick
    PRIVATE
    ${LIBEVDEV_INCLUDE_DIRS}
    ${LIBUSBGX_INCLUDE_DIRS} 
)

target_link_libraries(
    keystick
    PRIVATE
    ${LIBEVDEV_LIBRARIES}
    ${LIBUSBGX_LIBRARIES}
    fmt::fmt-header-only
)
