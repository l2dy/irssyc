cmake_minimum_required(VERSION 2.8)
project(psycd)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
set(VERSION_PATCH 0)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo." FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

INCLUDE(FindPkgConfig)

pkg_check_modules(GLIB2 glib-2.0 REQUIRED)

set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_C_FLAGS_DEBUG "-ggdb -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-ggdb -O2")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_BINARY_DIR .CMakeFiles)

set(IRSSI_DIR "${CMAKE_SYSTEM_INCLUDE_PATH}/irssi")
include_directories(
  ${IRSSI_DIR} ${IRSSI_DIR}/src ${IRSSI_DIR}/src/core
  ${GLIB2_INCLUDE_DIRS}
)

#include(CheckIncludeFile)
#include(FindDoxygen)
#add_custom_target(doc ${DOXYGEN_EXECUTABLE})
#configure_file(Doxyfile.in Doxyfile @ONLY IMMEDIATE)

add_subdirectory(src)
