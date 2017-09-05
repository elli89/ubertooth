#
# This file is part of Ubertooth.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#
# Based slightly upon the hackrf cmake setup.

include(${PROJECT_SOURCE_DIR}/../cmake/set_release.cmake)
add_definitions( -DRELEASE="${RELEASE_STRING}" )
message(STATUS "Setting version string ${RELEASE_STRING}")

# Targets
set(cpp_sources ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/dfu_ubertooth.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/basic_ubertooth.cpp
                # ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_callback.c
                ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_control.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/packetsource_ubertooth.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/sighandler.cpp
                CACHE INTERNAL "List of CPP sources")
set(cpp_headers ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth.h
                ${CMAKE_CURRENT_SOURCE_DIR}/dfu_ubertooth.h
                ${CMAKE_CURRENT_SOURCE_DIR}/basic_ubertooth.h
                # ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_callback.h
                ${CMAKE_CURRENT_SOURCE_DIR}/packetsource.h
                ${CMAKE_CURRENT_SOURCE_DIR}/packetsource_ubertooth.h
                ${CMAKE_CURRENT_SOURCE_DIR}/sighandler.h
                ${CMAKE_CURRENT_SOURCE_DIR}/dfu_interface.h
                ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_interface.h
                CACHE INTERNAL "List of CPP headers")

# Include and link to libbtbb and libusb-1.0
find_package(BTBB REQUIRED)
find_package(USB1 REQUIRED)

include_directories(${LIBUSB_INCLUDE_DIR} ${LIBBTBB_INCLUDE_DIR})
LIST(APPEND LIBUBERTOOTH_LIBS ${LIBUSB_LIBRARIES} ${LIBBTBB_LIBRARIES})

if( ${BUILD_SHARED_LIB} )
	# Shared library
	message(STATUS "Building shared library")
	add_library(ubertooth SHARED ${cpp_sources})
	target_link_libraries(ubertooth ${LIBUBERTOOTH_LIBS})
	set_target_properties(ubertooth PROPERTIES VERSION ${MAJOR_VERSION}.${MINOR_VERSION} SOVERSION 1)
	set_target_properties(ubertooth PROPERTIES CLEAN_DIRECT_OUTPUT 1)
	install(TARGETS ubertooth
			LIBRARY DESTINATION lib${LIB_SUFFIX}
			COMPONENT sharedlibs)
endif( ${BUILD_SHARED_LIB} )

if( ${BUILD_STATIC_LIB} )
	# Static library
	message(STATUS "Building static library")
	add_library(ubertooth-static STATIC ${cpp_sources})
	target_link_libraries(ubertooth-static ${LIBUBERTOOTH_LIBS})
	set_target_properties(ubertooth-static PROPERTIES OUTPUT_NAME "ubertooth")
	set_target_properties(ubertooth-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
	install(TARGETS ubertooth-static
			DESTINATION lib${LIB_SUFFIX}
			COMPONENT staticlibs)
endif( ${BUILD_STATIC_LIB} )

install(FILES ${cpp_headers}
		DESTINATION include
		COMPONENT headers)
