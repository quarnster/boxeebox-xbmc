cmake_minimum_required(VERSION 2.8)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
	message(FATAL_ERROR "You shouldn't run this from within the source dir. Instead\n\tmkdir build && cd build && cmake ..")
endif()

get_filename_component(SR "${CMAKE_SOURCE_DIR}/sysroot" REALPATH)

set(SYSROOT ${SR} CACHE PATH "The sysroot directory where everything will be installed")
set(PARALLEL "-j8" CACHE STRING "Arguments for make for parallel builds")
message("Using Sysroot: ${SYSROOT}")


include(ExternalProject)

file(WRITE case_test.txt)
if(EXISTS Case_Test.txt)
	message(FATAL_ERROR "The filesystem needs to be case sensitive. You can always create and mount a case sensitive file system image file.")
endif()

ExternalProject_Add(
	toolchain
	URL 				${CMAKE_SOURCE_DIR}/toolchain
	CMAKE_COMMAND 		PATH=$ENV{PATH}:${SYSROOT}/usr/bin cmake
	CMAKE_ARGS			-DSYSROOT=${SYSROOT} -DPARALLEL=${PARALLEL}
	BUILD_COMMAND 		PATH=$ENV{PATH}:${SYSROOT}/usr/bin make
	INSTALL_COMMAND		""
)

ExternalProject_Add(
	libs
	URL 				${CMAKE_SOURCE_DIR}/libs
	CMAKE_COMMAND 		PATH=$ENV{PATH}:${SYSROOT}/usr/bin cmake
	CMAKE_ARGS			-DSYSROOT=${SYSROOT} -DPARALLEL=${PARALLEL}
	BUILD_COMMAND 		PATH=$ENV{PATH}:${SYSROOT}/usr/bin make
	INSTALL_COMMAND		""
	DEPENDS				toolchain
)

ExternalProject_Add(
	xbmc
	URL 				${CMAKE_SOURCE_DIR}/xbmc
	CMAKE_COMMAND 		PATH=$ENV{PATH}:${SYSROOT}/usr/bin cmake
	CMAKE_ARGS			-DSYSROOT=${SYSROOT} -DPARALLEL=${PARALLEL}
	BUILD_COMMAND 		PATH=$ENV{PATH}:${SYSROOT}/usr/bin make
	INSTALL_COMMAND		""
	DEPENDS				libs
)
