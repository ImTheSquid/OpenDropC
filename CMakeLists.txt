cmake_minimum_required(VERSION 3.18)

project(OpenDropC VERSION 0.0.1 DESCRIPTION "Implementation of OpenDrop in C for Linux")

include(CTest)
enable_testing()

add_library(OpenDropC SHARED
    src/browser.c
    src/client.c
    src/server.c
    src/config.c
    src/utils.c
)

set_target_properties(OpenDropC PROPERTIES PUBLIC_HEADER include/browser.h)
target_link_libraries(OpenDropC PRIVATE avahi-common avahi-client curl ssl plist-2.0)

# CLI
add_executable(OpenDropCLI
  src/cli.c
)

target_link_libraries(OpenDropCLI PRIVATE OpenDropC)

# Testing
add_executable(OpenDropCTest
  test/main.c
)

target_link_libraries(OpenDropCTest PRIVATE OpenDropC)

add_test(Browser OpenDropCTest browser)
add_test(Config OpenDropCTest config)
