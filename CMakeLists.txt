#
# Manta - Structural Variant and Indel Caller
# Copyright (c) 2013-2016 Illumina, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#

################################################################################
##
## Top level cmake file
##
## author Come Raczy
##
################################################################################

if (WIN32)
    cmake_minimum_required(VERSION 3.1.0)
else ()
    cmake_minimum_required(VERSION 2.8.0)
endif ()

message (STATUS "==== Initializing project cmake configuration ====")


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
enable_testing()


# paths:
set(THIS_REDIST_DIR "${CMAKE_SOURCE_DIR}/redist")
set(THIS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(THIS_MODULE_DIR "${THIS_SOURCE_DIR}/cmake")
set(THIS_SOURCE_QC_DIR "${THIS_SOURCE_DIR}/srcqc")

set (THIS_GLOBALS_CMAKE "${THIS_MODULE_DIR}/globals.cmake")
set (THIS_MACROS_CMAKE "${THIS_MODULE_DIR}/macros.cmake")

# add custom functions:
include ("${THIS_MACROS_CMAKE}")

#
# if true, build with very strict error checking (disabled in src release tarballs)
#
set(DEVELOPER_MODE true)

#
# set WIN specific cache values
#   - WIN user potentially interacts with cache via cmake-gui
#   - Linux user interacts via the configure shell script
#
if (WIN32)
    set(IS_MSVC_ANALYZE FALSE CACHE BOOL
        "Turn on Visual Studio /analyze option")
endif ()

#
# setup configuration types, and pick set a build type from this set
#
# To translate the CMAKE-ese here:
# The plural of "BUILD_TYPE" is not "BUILD_TYPES", it is "CONFIGURATION_TYPES"
#
set(CMAKE_CONFIGURATION_TYPES_TMP Debug Release RelWithDebInfo)
if (NOT WIN32)
    set(CMAKE_CONFIGURATION_TYPES_TMP ${CMAKE_CONFIGURATION_TYPES_TMP} ASan)
endif ()
set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES_TMP} CACHE STRING "" FORCE)

join(CMAKE_CONFIGURATION_TYPES ", " CMAKE_CONFIGURATION_TYPES_STRING)

if (NOT CMAKE_BUILD_TYPE)
    set(DEFAULT_CMAKE_BUILD_TYPE Release)
    set(CMAKE_BUILD_TYPE ${DEFAULT_CMAKE_BUILD_TYPE} CACHE STRING
        "Choose the type of build, options are: {${CMAKE_CONFIGURATION_TYPES_STRING}} (default: ${DEFAULT_CMAKE_BUILD_TYPE})")
endif()

# check that a valid build type has been selected:
list(FIND CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} TINDEX)
if(TINDEX EQUAL -1)
    message (FATAL_ERROR "Selected build type, '${CMAKE_BUILD_TYPE}', is not in the set of available types: {${CMAKE_CONFIGURATION_TYPES_STRING}}")
endif()

message (STATUS "BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

#
# other cache variables
#

if (NOT CMAKE_PARALLEL)
    # set to true if win build needs to rebuild boost, right now on win32 all we do is unpack boost for headers:
    set (IS_WIN_BOOST_BUILD FALSE)


    if (WIN32 AND IS_WIN_BOOST_BUILD)
        set(CMAKE_PARALLEL "0" CACHE STRING
            "Choose the number of cores used by cmake during configuration (no default)")

        # give windows/cmake-gui user the chance to set this before proceeding, otherwise
        # we force a long wait for boost to compile
        message (WARNING "\n*** Must set CMAKE_PARALLEL before continuing configuration ***\n")
        return()
    else ()
        set (CMAKE_PARALLEL "1")
    endif ()
endif ()

message (STATUS "CMAKE_PARALLEL: ${CMAKE_PARALLEL}")

#
# setup project/version
#

set(THIS_PROJECT_NAME "manta")

project (${THIS_PROJECT_NAME})

# find interpreters
find_package(PythonInterp)
if (NOT PYTHONINTERP_FOUND)
    message (WARNING "No python interpreter found, disabling optional python build and installation components. Installed workflow requires python interpreter to run")
endif()

set (THIS_ARCH ${CMAKE_SYSTEM_PROCESSOR})
if (NOT WIN32)
    if (CMAKE_SIZEOF_VOID_P MATCHES 8)
        set (THIS_ARCH "x86_64")
    else ()
        set (THIS_ARCH "x86")
    endif ()
endif ()
message (STATUS "TARGET_ARCHITECTURE: " ${THIS_ARCH} )


# Create package versioning target - version derived from git describe except for
# release tarballs. Version file is treated as always out of date.
set(THIS_BUILDTIME_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/buildTimeConfigInfo.txt")
set(THIS_BUILDTIME_CONFIG_TARGET "${THIS_PROJECT_NAME}_buildtime_config")
add_custom_target(
    ${THIS_BUILDTIME_CONFIG_TARGET}
    ALL
    COMMAND ${CMAKE_COMMAND}
    -D REDIST_DIR="${THIS_REDIST_DIR}"
    -D CONFIG_FILE="${THIS_BUILDTIME_CONFIG_FILE}"
    -D SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    -P "${THIS_MODULE_DIR}/getBuildTimeConfigInfo.cmake"
    COMMENT "Updating buildtime config info")

# Installation directories
if    (NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "/usr/local")
endif ()

# check that the installation prefix is acceptable
function(check_prefix)
    string(SUBSTRING "${CMAKE_INSTALL_PREFIX}" 0 1 slash)
    string(COMPARE NOTEQUAL "/" "${slash}" isslash)
    if    (isslash)
        install(CODE "
            string(LENGTH \"\$ENV{DESTDIR}\" ddlen)
            if (ddlen)
                message (FATAL_ERROR \"Installation prefix must begin with '/' if DESTDIR is set.\")
            endif ()
        ")
    endif ()
endfunction()

check_prefix()

# paths for binary components -- we don't currently change this from the non-binary components
set(THIS_EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(THIS_BINDIR "${THIS_EXEC_PREFIX}/bin")
set(THIS_LIBDIR "${THIS_EXEC_PREFIX}/lib")
set(THIS_LIBEXECDIR "${THIS_EXEC_PREFIX}/libexec")

# paths w/o architecture dependency
set(THIS_PYTHON_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib/python")
set(THIS_DATADIR "${CMAKE_INSTALL_PREFIX}/share")
set(THIS_DOCDIR "${CMAKE_INSTALL_PREFIX}/doc")
set(THIS_DEMODIR "${THIS_DATADIR}/demo")

message (STATUS "install prefix: ${CMAKE_INSTALL_PREFIX}")

set(THIS_OPT "${THIS_PROJECT_NAME}_opt")
add_custom_target(${THIS_OPT} ALL)

# redist includes
include ("${THIS_MODULE_DIR}/boost.cmake")

if (NOT WIN32)
    # global source QC -- don't allow non-ascii chars in source files:
    set(THIS_SOURCE_CHECK "${THIS_PROJECT_NAME}_source_check")
    add_custom_target(${THIS_SOURCE_CHECK}
        ALL
        COMMAND bash ${THIS_SOURCE_QC_DIR}/check_for_nonascii_source.bash
        )

    # force this to run early:
    add_dependencies(${THIS_OPT} ${THIS_SOURCE_CHECK})
endif ()

add_subdirectory (redist)
add_subdirectory (src)
