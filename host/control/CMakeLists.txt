cmake_minimum_required(VERSION 3.14)

project(avona_control LANGUAGES C)
set(TARGET_NAME avona_control)

set(SDK_PATH $ENV{XCORE_SDK_PATH})

# Disable in-source build.
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

# Maybe move these options into a .cmake file in the host device control
# directory in the SDK?
option(USE_USB "Use USB" ON)
option(USE_RPI_I2C "Use I2C from a Raspberry Pi" OFF)
option(USE_AARDVARK_I2C "Use I2C with an Aardvark" OFF)

# Enable compiler warnings
if (MSVC)
    # Force to always compile with W4
    if (CMAKE_C_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    else ()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
    endif ()
elseif (CMAKE_COMPILER_IS_GNUCC)
    # Update if necessary
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O2")
endif ()

if (USE_RPI_I2C)
    # Define the cross compiler locations
    set(CMAKE_C_COMPILER   /opt/raspberrypi-tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc)
    set(CMAKE_CXX_COMPILER /opt/raspberrypi-tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc)
    # Define the sysroot path for the RaspberryPi distribution in our tools folder 
    set(CMAKE_FIND_ROOT_PATH /opt/raspberrypi-tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/arm-linux-gnueabihf/sysroot/)

    # Use our definitions for compiler tools
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    # Search for libraries and headers in the target directories only
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    add_definitions(-std=c11)
endif ()

# Tell cmake where to find the macros/functions for things like finding libusb
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/.cmake)

# Tell cmake where to install things
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install CACHE PATH "" FORCE)

set(DEVICE_CONTROL_PATH "${SDK_PATH}/modules/rtos/sw_services/device_control")

set(DEVICE_CONTROL_INCLUDES
    "${DEVICE_CONTROL_PATH}/host"
)

set(DEVICE_CONTROL_SOURCES
    "${SDK_PATH}/modules/rtos/sw_services/device_control/host/util.c"
)

set(APP_SOURCES
    "avona_control.c"
)

set(APP_INCLUDES "")
set(LINK_LIBS "")

if (USE_USB)
    list(APPEND DEVICE_CONTROL_SOURCES "${DEVICE_CONTROL_PATH}/host/device_access_usb.c")

    #Find libusb, generate error on failure
    find_package(LibUSB)
    if (NOT ${LibUSB_FOUND})
        message(FATAL_ERROR "LibUSB not found.")
    endif ()
    
    list(APPEND LINK_LIBS ${LibUSB_LIBRARIES})
    list(APPEND APP_INCLUDES ${LibUSB_INCLUDE_DIRS})
endif ()

if (USE_USB)
    message(STATUS "Building for USB")
    add_compile_definitions(USE_USB=1)
elseif (USE_RPI_I2C)
    message(STATUS "Building for RPi I2C")
    add_compile_definitions(USE_I2C=1)
    add_compile_definitions(RPI=1)
elseif (USE_AARDVARK_I2C)
    message(STATUS "Building for Aardvark I2C")
    message(FATAL_ERROR "I2C Aardvark not supported yet")
else()
    message(FATAL_ERROR "No transport chosen")
endif()

add_executable(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} ${LINK_LIBS})

target_sources(${TARGET_NAME} PRIVATE ${APP_SOURCES} ${DEVICE_CONTROL_SOURCES})
target_include_directories(${TARGET_NAME} PRIVATE ${APP_INCLUDES} ${DEVICE_CONTROL_INCLUDES})

#target_compile_options(${TARGET_NAME} PRIVATE "-O2 -Wall")
target_link_options(${TARGET_NAME} PRIVATE "")
