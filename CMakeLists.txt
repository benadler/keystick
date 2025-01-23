cmake_minimum_required(VERSION 3.25)

project(keystick
    LANGUAGES CXX
    VERSION 1.0.0
    DESCRIPTION "Emulates a joystick using USB gadget framework for every connected keyboard"
    HOMEPAGE_URL https://github.com/benadler/keystick)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# On something more modern that raspberrypios these libraries would probably have cmake support.
# Trying to maximize compatibility I'll rely on pkgconfig for now.
find_package(PkgConfig REQUIRED)
pkg_search_module(LIBEVDEV REQUIRED libevdev)
pkg_search_module(LIBUSBGX REQUIRED libusbgx)

find_package(fmt REQUIRED)

add_executable(keystick
    keystick.cpp
    keyboardjoystick.cpp
    devicehid.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${LIBEVDEV_INCLUDE_DIRS}
    ${LIBUSBGX_INCLUDE_DIRS} 
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${LIBEVDEV_LIBRARIES}
    ${LIBUSBGX_LIBRARIES}
    fmt::fmt-header-only
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
