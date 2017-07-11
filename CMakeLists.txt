cmake_minimum_required(VERSION 2.8.12)
PROJECT(enc-aomedia-av1)

################################################################################
# Options
################################################################################
SET(AOMEDIA_AV1_BUILD_DIR "" CACHE PATH "Path to the directory containing the CMake build files.")
SET(AOMEDIA_AV1_INCLUDE_DIR "" CACHE PATH "Include directory for AOMedia1 containing aom.h")

if(NOT TARGET obs-studio)
	SET(OBSSTUDIO_DIR "" CACHE PATH "OBS Studio Source Code Directory")
endif()

# Validate
if(AOMEDIA_AV1_BUILD_DIR STREQUAL "")
	message(FATAL_ERROR "AV1: Build directory not set!")
	return()
elseif(NOT EXISTS "${AOMEDIA_AV1_BUILD_DIR}/RelWithDebInfo/aom.lib")
	message(FATAL_ERROR "AV1: Build directory set, but could not find RelWithDebInfo/aom.lib!")
	return()
elseif(NOT EXISTS "${AOMEDIA_AV1_BUILD_DIR}/Debug/aom.lib")
	message(FATAL_ERROR "AV1: Build directory set, but could not find Debug/aom.lib!")
	return()
endif()

if(AOMEDIA_AV1_INCLUDE_DIR STREQUAL "")
	message(FATAL_ERROR "AV1: Include directory not set!")
	return()
elseif(NOT EXISTS "${AOMEDIA_AV1_INCLUDE_DIR}/aom/aom.h")
	message(FATAL_ERROR "AV1: Include directory set, but could not find aom.h!")
	return()
endif()

if(NOT TARGET obs-studio)
	if(OBSSTUDIO_DIR STREQUAL "")
		message(FATAL_ERROR "OBSSTUDIO_DIR not set!")
		return()
	endif()
	if(NOT EXISTS "${OBSSTUDIO_DIR}/libobs/obs-module.h")
		message(FATAL_ERROR "OBSSTUDIO_DIR invalid!")
		return()
	endif()
endif()

################################################################################
# Code
################################################################################

# Versioning
SET(VERSION_MAJOR 0)
SET(VERSION_MINOR 0)
SET(VERSION_PATCH 0)
configure_file(
	"${PROJECT_SOURCE_DIR}/source/version.h.in"
	"${PROJECT_BINARY_DIR}/source/version.h"
)

# Headers
SET(enc-aomedia-av1_HEADERS
	"${PROJECT_SOURCE_DIR}/source/av1-encoder.h"
	"${PROJECT_SOURCE_DIR}/source/plugin.h"
	"${PROJECT_BINARY_DIR}/source/version.h"
	"${PROJECT_SOURCE_DIR}/source/strings.h"
)

# Sources
SET(enc-aomedia-av1_SOURCES
	"${PROJECT_SOURCE_DIR}/source/av1-encoder.cpp"
	"${PROJECT_SOURCE_DIR}/source/plugin.cpp"
	"${PROJECT_SOURCE_DIR}/source/version.h.in"
)

# Libraries
SET(enc-aomedia-av1_LIBRARIES
	debug ${AOMEDIA_AV1_BUILD_DIR}/Debug/aom.lib
	optimized ${AOMEDIA_AV1_BUILD_DIR}/RelWithDebInfo/aom.lib
)

# Include Directories
INCLUDE_DIRECTORIES(
	"${AOMEDIA_AV1_BUILD_DIR}/dist/include"
	"${AOMEDIA_AV1_BUILD_DIR}"
	"${PROJECT_BINARY_DIR}"
	"${PROJECT_BINARY_DIR}/source"
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/source"
)

################################################################################
# Standalone and OBS Studio Build Data
################################################################################
if(TARGET obs-studio)
	# Directories
	INCLUDE_DIRECTORIES(
		"${CMAKE_SOURCE_DIR}"
	)
	SET(LIBOBS_LIBRARIES libobs)
else()
	# Standlone Specific

	# Find OBS Libraries
	SET(obsPath "${OBSSTUDIO_DIR}")
	INCLUDE("${obsPath}/cmake/external/Findlibobs.cmake")

	# Compiling
	INCLUDE_DIRECTORIES(
		"${OBSSTUDIO_DIR}"
	)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

################################################################################
# Build
################################################################################
ADD_LIBRARY(enc-aomedia-av1 MODULE
	${enc-aomedia-av1_HEADERS}
	${enc-aomedia-av1_SOURCES}
)
TARGET_LINK_LIBRARIES(enc-aomedia-av1
	${LIBOBS_LIBRARIES}
	${enc-aomedia-av1_LIBRARIES}
)

# All Warnings, Extra Warnings, Pedantic
if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
endif()

if(TARGET obs-studio)
	install_obs_plugin_with_data(enc-aomedia-av1 data)
else()
	math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
	add_custom_command(TARGET enc-aomedia-av1 POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${PROJECT_SOURCE_DIR}/data"
		"${PROJECT_SOURCE_DIR}/#Build/data/obs-plugins/enc-aomedia-av1"
	)
	add_custom_command(TARGET enc-aomedia-av1 POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		"$<TARGET_FILE:enc-aomedia-av1>"
		"${PROJECT_SOURCE_DIR}/#Build/obs-plugins/${BITS}bit/$<TARGET_FILE_NAME:enc-aomedia-av1>"
	)
	add_custom_command(TARGET enc-aomedia-av1 POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		"$<TARGET_FILE_DIR:enc-aomedia-av1>/enc-aomedia-av1.pdb"
		"${PROJECT_SOURCE_DIR}/#Build/obs-plugins/${BITS}bit/enc-aomedia-av1.pdb"
	)
endif()
