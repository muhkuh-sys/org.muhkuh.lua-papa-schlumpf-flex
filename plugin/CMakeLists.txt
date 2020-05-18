cmake_minimum_required(VERSION 3.5)

PROJECT("papa_schlumpf_flex_plugin"
        VERSION ${MUHKUH_VERSION_ALL})

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

#----------------------------------------------------------------------------
#
# Build a package for the selected platform.
#

SET(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_BINARY_DIR}/../../repository/org/muhkuh/lua/papa-schlumpf-flex/${PROJECT_VERSION})
SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
SET(CPACK_PACKAGE_FILE_NAME "lua${BUILDCFG_LUA_VERSION}-papa-schlumpf-flex-${PROJECT_VERSION}-${JONCHKI_PLATFORM}")

	SET(CPACK_GENERATOR "TXZ")
	INCLUDE(CPack)

	# This is the folder where the external project TARGET_bitop installed the files.
	SET(SUB_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}")

	IF(${JONCHKI_PLATFORM_DIST_ID} STREQUAL "windows")
		# Package a windows build.

		# Install the DLL without execute permissions.
		INSTALL(FILES ${SUB_INSTALL_DIR}/papa_schlumpf.dll
		        DESTINATION lua_plugins/)

		CONFIGURE_FILE(installer/lua${BUILDCFG_LUA_VERSION}/install_windows.lua
		               ${CMAKE_CURRENT_BINARY_DIR}/package/install.lua
		               @ONLY)
	ELSE(${JONCHKI_PLATFORM_DIST_ID} STREQUAL "windows")
		# Package a linux build.

		INSTALL(FILES ${SUB_INSTALL_DIR}/papa_schlumpf.so
		        DESTINATION lua_plugins/)

		CONFIGURE_FILE(installer/lua${BUILDCFG_LUA_VERSION}/install_linux.lua
		               ${CMAKE_CURRENT_BINARY_DIR}/package/install.lua
		               @ONLY)
	ENDIF(${JONCHKI_PLATFORM_DIST_ID} STREQUAL "windows")

	INSTALL(FILES ${CMAKE_HOME_DIRECTORY}/lua/papa_schlumpf_flex.lua
	        DESTINATION lua/)

	INSTALL(FILES ${CMAKE_HOME_DIRECTORY}/targets/papa_schlumpf_stumpf_cifx4000.img
	        DESTINATION firmware/)

	INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/package/install.lua
	        DESTINATION .)

	CONFIGURE_FILE(installer/lua${BUILDCFG_LUA_VERSION}/papa-schlumpf-flex.xml
	               ${CPACK_OUTPUT_FILE_PREFIX}/lua${BUILDCFG_LUA_VERSION}-papa-schlumpf-flex-${PROJECT_VERSION}-${JONCHKI_PLATFORM}.xml
	               @ONLY)


	#-----------------------------------------------------------------------------
	#
	# Create a custom target named "pack".
	# It builds the package and creates the hashes.
	#
	ADD_CUSTOM_TARGET(pack
	                  COMMAND ${CMAKE_MAKE_PROGRAM}
	                  COMMAND ${CMAKE_MAKE_PROGRAM} package
	                  COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_HOME_DIRECTORY}/cmake/tools/generate_hash.py ${CPACK_OUTPUT_FILE_PREFIX}/lua${BUILDCFG_LUA_VERSION}-papa-schlumpf-flex-${PROJECT_VERSION}-${JONCHKI_PLATFORM}.xml
	                  COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_HOME_DIRECTORY}/cmake/tools/generate_hash.py ${CPACK_OUTPUT_FILE_PREFIX}/lua${BUILDCFG_LUA_VERSION}-papa-schlumpf-flex-${PROJECT_VERSION}-${JONCHKI_PLATFORM}.tar.xz
	                  COMMENT "Pack the artifact and generate the hashes...")


	#-----------------------------------------------------------------------------
	#
	# Filter the pom.xml file.
	#
	SET(POM_ARTIFACT_ID  "lua${BUILDCFG_LUA_VERSION}-papa-schlumpf-flex")
	SET(POM_GROUP_ID     "org.muhkuh.lua")
	SET(POM_VERSION      "${PROJECT_VERSION}")
	SET(POM_PACKAGING    "tar.xz")
	CONFIGURE_FILE(installer/lua${BUILDCFG_LUA_VERSION}/pom.xml ${CPACK_OUTPUT_FILE_PREFIX}/lua${BUILDCFG_LUA_VERSION}-papa-schlumpf-flex-${PROJECT_VERSION}.pom)
