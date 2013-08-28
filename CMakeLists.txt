cmake_minimum_required(VERSION 2.8)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
	message(FATAL_ERROR "You shouldn't run this from within the source dir. Instead\n\tmkdir build && cd build && cmake ..")
endif()

get_filename_component(SR "${CMAKE_SOURCE_DIR}/sysroot" REALPATH)

set(SYSROOT ${SR} CACHE PATH "The sysroot directory where everything will be installed")
set(PARALLEL "-j8" CACHE STRING "Arguments for make for parallel builds")
set(TARGET_DIR ${CMAKE_SOURCE_DIR}/dist CACHE PATH "Directory to install everything to")
set(BOXEEBOX_IP "boxeebox" CACHE STRING "ipaddress or hostname of your boxeebox (only used for remote debugging)")
message("Using Sysroot: ${SYSROOT}")


include(ExternalProject)

file(WRITE case_test.txt)
if(EXISTS Case_Test.txt)
	message(FATAL_ERROR "The filesystem needs to be case sensitive. You can always create and mount a case sensitive file system image file.")
endif()

add_custom_target(xbmc-refresh
	COMMAND 			${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/xbmc ${CMAKE_BINARY_DIR}/xbmc-prefix/src/xbmc
	COMMAND 			touch ${CMAKE_BINARY_DIR}/xbmc-prefix/src/xbmc-stamp/xbmc-download
)

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
	INSTALL_COMMAND		PATH=$ENV{PATH}:${SYSROOT}/usr/bin make install
	DEPENDS				libs intel
)
add_dependencies(xbmc xbmc-refresh)

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
	COMMAND 			${CMAKE_COMMAND} -E copy_directory ${SYSROOT}/usr/local/lib/xbmc/addons ${TARGET_DIR}/addons
	COMMAND 			${CMAKE_COMMAND} -E copy_directory ${SYSROOT}/usr/local/lib/xbmc/system ${TARGET_DIR}/system
	DEPENDS 			${shares}
)

add_custom_target(
	quickdist
	COMMAND 			PATH=$ENV{PATH}:${SYSROOT}/usr/bin ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/xbmc-prefix/src/xbmc-build/ make copydeps
	COMMAND 			PATH=$ENV{PATH}:${SYSROOT}/usr/bin ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/xbmc-prefix/src/xbmc-build/xbmc-prefix/src/xbmc make -j8
	COMMAND 			PATH=$ENV{PATH}:${SYSROOT}/usr/bin ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/xbmc-prefix/src/xbmc-build/xbmc-prefix/src/xbmc install-binaries
	COMMAND 			${CMAKE_COMMAND} -E copy ${SYSROOT}/usr/local/lib/xbmc/xbmc.bin ${CMAKE_BINARY_DIR}/xbmc.bin
	COMMAND 			${SYSROOT}/usr/bin/i686-pc-linux-gnu-strip ${CMAKE_BINARY_DIR}/xbmc.bin
	COMMAND 			${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/xbmc.bin ${TARGET_DIR}/xbmc.bin
	DEPENDS 			xbmc-refresh
)
add_custom_target(
	dist
	COMMAND 			${CMAKE_COMMAND} -E copy ${SYSROOT}/usr/local/lib/xbmc/xbmc.bin ${CMAKE_BINARY_DIR}/xbmc.bin
	COMMAND 			${SYSROOT}/usr/bin/i686-pc-linux-gnu-strip ${CMAKE_BINARY_DIR}/xbmc.bin
	COMMAND 			${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/xbmc.bin ${TARGET_DIR}/xbmc.bin
	DEPENDS 			xbmc ${libs} libstdc++ libz
)

add_custom_command(
	OUTPUT boxeelibs
	COMMAND 			${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/boxeelibs
	COMMAND 			${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/boxeelibs wget ftp://${BOXEEBOX_IP}/lib/*
)

file(WRITE ${CMAKE_BINARY_DIR}/gdb.txt
"
set solib-search-path ${CMAKE_BINARY_DIR}/boxeelibs
file ${SYSROOT}/usr/local/lib/xbmc/xbmc.bin
target remote ${BOXEEBOX_IP}:12345
"
)

add_custom_target(
	debug
	COMMAND 			${SYSROOT}/usr/bin/i686-pc-linux-gnu-gdb -x ${CMAKE_BINARY_DIR}/gdb.txt
	DEPENDS 			xbmc boxeelibs
)

add_custom_target(refresh
	COMMAND 			cp -r ${CMAKE_SOURCE_DIR}/toolchain/* ${CMAKE_BINARY_DIR}/toolchain-prefix/src/toolchain/
	COMMAND 			cp -r ${CMAKE_SOURCE_DIR}/libs/* ${CMAKE_BINARY_DIR}/libs-prefix/src/libs/
	COMMAND 			cp -r ${CMAKE_SOURCE_DIR}/intel/* ${CMAKE_BINARY_DIR}/intel-prefix/src/intel/
	COMMAND 			touch ${CMAKE_BINARY_DIR}/toolchain-prefix/src/toolchain-stamp/toolchain-download
	COMMAND 			touch ${CMAKE_BINARY_DIR}/libs-prefix/src/libs-stamp/libs-download
	COMMAND 			touch ${CMAKE_BINARY_DIR}/intel-prefix/src/intel-stamp/intel-download
	DEPENDS 			xbmc-refresh
)
