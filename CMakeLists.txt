cmake_minimum_required(VERSION 2.8)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
	message(FATAL_ERROR "You shouldn't run this from within the source dir. Instead\n\tmkdir build && cd build && cmake ..")
endif()

get_filename_component(SR "${CMAKE_SOURCE_DIR}/sysroot" REALPATH)

set(SYSROOT ${SR} CACHE PATH "The sysroot directory where everything will be installed")
set(PARALLEL "-j8" CACHE STRING "Arguments for make for parallel builds")
set(TARGET_DIR ${CMAKE_SOURCE_DIR}/dist CACHE PATH "Directory to install everything to")
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
	intel
	URL					${CMAKE_SOURCE_DIR}/intel
	CMAKE_COMMAND 		PATH=$ENV{PATH}:${SYSROOT}/usr/bin cmake
	CMAKE_ARGS			-DSYSROOT=${SYSROOT} -DPARALLEL=${PARALLEL}
	BUILD_COMMAND 		PATH=$ENV{PATH}:${SYSROOT}/usr/bin make
	INSTALL_COMMAND 	""
	DEPENDS 			toolchain
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
	DEPENDS				libs intel
)

set(libs
	libjasper.so.1
	libpostproc.so.52
	libpng16.so.16
	libswresample.so.0
	libyajl.so.2
	libssh.so.4
	libmicrohttpd.so.10
	liblzo2.so.2
	libtiff.so.5
	libpython2.7.so.1.0
	libpcre.so.1
	libcdio.so.14
	libtag.so.1
	libavcodec.so.54
	libavfilter.so.3
	libavformat.so.54
	libavutil.so.52
	libswscale.so.2
	libjpeg.so.62
	libasound.so.2
)
foreach(lib ${libs})
	add_custom_target(
		${lib}
		COMMAND 		${CMAKE_COMMAND} -E copy_if_different ${SYSROOT}/usr/local/lib/${lib} ${TARGET_DIR}/${lib}
		DEPENDS			xbmc
	)
endforeach()

if(EXISTS ${SYSROOT}/usr/i686-pc-linux-gnu/lib/libstdc++.so.6)
	set(stdsrc ${SYSROOT}/usr/i686-pc-linux-gnu/lib/libstdc++.so.6)
else()
	set(stdsrc ${SYSROOT}/usr/lib/libstdc++.so.6)
endif()

add_custom_target(
	libstdc++
	COMMAND 			${CMAKE_COMMAND} -E copy_if_different ${stdsrc} ${TARGET_DIR}/libstdc++.so.6
)

add_custom_target(
	libz
	COMMAND 			${CMAKE_COMMAND} -E copy_if_different ${SYSROOT}/usr/lib/libz.so.1 ${TARGET_DIR}/libz.so.1
)

set(shares
	addons
	language
	media
	portable_data
	sounds
	system
	userdata
)

foreach(share ${shares})
	add_custom_target(
		${share}
		COMMAND 		${CMAKE_COMMAND} -E copy_directory ${SYSROOT}/usr/local/share/xbmc/${share} ${TARGET_DIR}/${share}
		DEPENDS			xbmc
	)
endforeach()

add_custom_target(
	shares
	COMMAND 			${CMAKE_COMMAND} -E copy_directory ${SYSROOT}/usr/local/share/xbmc/addons ${TARGET_DIR}/addons
	DEPENDS 			${shares}
)

add_custom_target(
	dist
	COMMAND 			${CMAKE_COMMAND} -E copy_if_different ${SYSROOT}/usr/local/lib/xbmc/xbmc.bin ${TARGET_DIR}/xbmc.bin
	DEPENDS 			xbmc ${libs} libstdc++ libz
)
