cmake_minimum_required(VERSION 3.4)

project(wds CXX)

set(WDS_VERSION_MAJOR 1)
set(WDS_VERSION_MINOR 1)
set(WDS_VERSION_PATCH 0)

enable_testing()

option(WDS_INSTALL_TESTS "Install test programs" off)

include(GNUInstallDirs)

add_subdirectory(data)
add_subdirectory(libwds/rtsp)
add_subdirectory(libwds/rtsp/tests)
add_subdirectory(libwds/common)
add_subdirectory(libwds/source)
add_subdirectory(libwds/sink)
add_subdirectory(libwds)
add_subdirectory(p2p)
add_subdirectory(mirac_network)
add_subdirectory(sink)
add_subdirectory(desktop_source)
