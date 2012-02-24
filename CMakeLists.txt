cmake_minimum_required(VERSION 2.8)
project(psycd)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
set(VERSION_PATCH 0)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo."
	FORCE)
ENDIF()

INCLUDE(FindPkgConfig)

pkg_check_modules(GLIB2 glib-2.0 REQUIRED)

set(CMAKE_C_FLAGS " -Wall" CACHE STRING
    "Common C flags")
set(CMAKE_C_FLAGS_DEBUG "-ggdb -O0 -DDEBUG" CACHE STRING
    "C flags for debug build")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-ggdb -O2" CACHE STRING
    "C flags for release with debug info")
set(CMAKE_C_FLAGS_RELEASE "-O2" CACHE STRING
    "C flags for release")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_BINARY_DIR .CMakeFiles)

set(LIBPSYC_PREFIX "/usr" CACHE FILEPATH "libpsyc install prefix")
set(PSYCD_PREFIX "/usr" CACHE FILEPATH "psycd install prefix")
set(IRSSI_INCLUDE_DIR "/usr/include/irssi" CACHE FILEPATH
    "Irssi include directory")
set(IRSSI_MODULES_DIR "/usr/lib/irssi/modules" CACHE FILEPATH
    "Irssi modules directory for installing the module
     (/usr/lib/irssi/modules or ~/.irssi/modules)")

include_directories(
  ${IRSSI_INCLUDE_DIR}/src ${IRSSI_INCLUDE_DIR}/src/core
	${LIBPSYC_PREFIX}/include ${PSYCD_PREFIX}/include
  ${GLIB2_INCLUDE_DIRS}
)

#include(CheckIncludeFile)
#include(FindDoxygen)
#add_custom_target(doc ${DOXYGEN_EXECUTABLE})
#configure_file(Doxyfile.in Doxyfile @ONLY IMMEDIATE)

add_subdirectory(src)
