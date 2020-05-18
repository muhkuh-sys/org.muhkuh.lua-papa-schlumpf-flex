cmake_minimum_required(VERSION 3.5)

PROJECT("papa_schlumpf_flex_plugin")


# Select the LUA version.
INCLUDE(${CMAKE_HOME_DIRECTORY}/cmake/muhkuh_lua.cmake)

# Enable tests (see here for details: http://www.vtk.org/Wiki/CMake/Testing_With_CTest)
ENABLE_TESTING()


# Set some installation folders.
INCLUDE(${CMAKE_HOME_DIRECTORY}/cmake/muhkuh_installation_folders.cmake)

# Detect the platform.
INCLUDE(${CMAKE_HOME_DIRECTORY}/cmake/platform.cmake)

# Swig is required to generate the bindings.
FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

# Add a custom target for the swig runtime for LUA.
INCLUDE(${CMAKE_HOME_DIRECTORY}/cmake/muhkuh_swig_lua_runtime.cmake)

FIND_PACKAGE(libusb REQUIRED)


# Append the libusb defines.
ADD_DEFINITIONS(${LIBUSB_DEFINES})

SET_SOURCE_FILES_PROPERTIES(papa_schlumpf.i PROPERTIES CPLUSPLUS ON)
SET_PROPERTY(SOURCE papa_schlumpf.i PROPERTY SWIG_FLAGS -I${CMAKE_HOME_DIRECTORY})

IF(CMAKE_VERSION VERSION_LESS 3.8.0)
	SWIG_ADD_MODULE(TARGET_papa_schlumpf lua papa_schlumpf.i papa_schlumpf.cpp papa_schlumpf_device.cpp)
ELSE(CMAKE_VERSION VERSION_LESS 3.8.0)
	SWIG_ADD_LIBRARY(TARGET_papa_schlumpf
	                 TYPE MODULE
	                 LANGUAGE LUA
	                 SOURCES papa_schlumpf.i papa_schlumpf.cpp)
ENDIF(CMAKE_VERSION VERSION_LESS 3.8.0)
TARGET_INCLUDE_DIRECTORIES(TARGET_papa_schlumpf
                           PRIVATE ${LUA_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/src/common ${SWIG_RUNTIME_OUTPUT_PATH})
SWIG_LINK_LIBRARIES(TARGET_papa_schlumpf ${LUA_LIBRARIES} libusb::TARGET_libusb)
ADD_DEPENDENCIES(TARGET_papa_schlumpf TARGET_swigluarun)

# Set the name of the output file to "papa_schlumpf".
SET_TARGET_PROPERTIES(TARGET_papa_schlumpf PROPERTIES PREFIX "" OUTPUT_NAME "papa_schlumpf")

# On mingw link all compiler libraries static.
IF((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))
	set_property(TARGET TARGET_papa_schlumpf PROPERTY LINK_FLAGS "-static-libgcc -static-libstdc++")
ENDIF((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))


# Install the lua module.
INSTALL(TARGETS TARGET_papa_schlumpf
        DESTINATION ${INSTALL_DIR_LUA_MODULES})


# Add tests for this module.
IF((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))
	# Here are the MinGW specific tests.
	ADD_TEST(NAME bit_MinGW_DLL_dependencies
	         COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_HOME_DIRECTORY}/cmake/tests/mingw_dll_dependencies.py -u lua5.1 $<TARGET_FILE:TARGET_papa_schlumpf>)
ENDIF((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))
