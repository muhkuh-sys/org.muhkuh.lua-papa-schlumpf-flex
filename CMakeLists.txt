cmake_minimum_required(VERSION 3.5)

#-----------------------------------------------------------------------------
#
# The project options.
#
OPTION(BUILDCFG_ONLY_JONCHKI_CFG "Only build the jonchki configuration files. The default is OFF."
       "OFF")

MESSAGE("BUILDCFG_ONLY_JONCHKI_CFG: ${BUILDCFG_ONLY_JONCHKI_CFG}")


# Generate a version number from a file or VCS.
INCLUDE(cmake/muhkuh_version.cmake)


PROJECT("papa_schlumpf"
        VERSION ${MUHKUH_VERSION_ALL})

# Get the VCS version for the jonchki configuration.
INCLUDE(cmake/version.cmake)

# Detect the platform.
INCLUDE(cmake/platform.cmake)

# Filter the jonchki configurations.
CONFIGURE_FILE(plugin/installer/lua${BUILDCFG_LUA_VERSION}/papa-schlumpf-flex.xml
               ${CMAKE_CURRENT_BINARY_DIR}/lua${BUILDCFG_LUA_VERSION}-papa-schlumpf-flex-${PROJECT_VERSION}.xml
               @ONLY)


IF( NOT BUILDCFG_ONLY_JONCHKI_CFG STREQUAL "ON" )
	SET(libusb_DIR "${CMAKE_CURRENT_BINARY_DIR}/../external/libusb/install/cmake")

	ADD_SUBDIRECTORY(plugin)
ENDIF( NOT BUILDCFG_ONLY_JONCHKI_CFG STREQUAL "ON" )
