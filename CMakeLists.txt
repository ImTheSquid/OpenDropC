cmake_minimum_required(VERSION 3.18)

if(NOT DEFINED ENV{VCPKG_ROOT})
  message(FATAL_ERROR "VCPKG required")
endif()


set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

if(NOT EXISTS ${CMAKE_TOOLCHAIN_FILE})
  message(FATAL_ERROR "No toolchain file at ${CMAKE_TOOLCHAIN_FILE}")
endif()

include_directories($ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include)

project(OpenDropC VERSION 0.0.1 DESCRIPTION "Implementation of OpenDrop in C for Linux")

include(CTest)
enable_testing()

add_library(OpenDropC SHARED
    src/browser.c
    src/client.c
    src/server.c
)

set_target_properties(OpenDropC PROPERTIES PUBLIC_HEADER include/browser.h)

target_link_libraries(OpenDropC PRIVATE avahi-common avahi-client)