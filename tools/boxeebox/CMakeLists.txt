cmake_minimum_required(VERSION 2.8)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
	message(FATAL_ERROR "You shouldn't run this from within the source dir. Instead\n\tmkdir build && cd build && cmake ..")
endif()

get_filename_component(SR "${CMAKE_SOURCE_DIR}/sysroot" REALPATH)

set(SYSROOT ${SR} CACHE PATH "The sysroot directory where everything will be installed")
set(PARALLEL "-j8" CACHE STRING "Arguments for make for parallel builds")
set(TARGET i686-pc-linux-gnu)
set(TARGET_DIR ${CMAKE_SOURCE_DIR}/dist CACHE PATH "Directory to install everything to")
set(BOXEEBOX_IP "boxeebox" CACHE STRING "ipaddress or hostname of your boxeebox (only used for remote debugging)")
message("Using Sysroot: ${SYSROOT}")


find_program(HAVE_PV NAMES pv)

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


set(local ${SYSROOT}/usr/local)

file(WRITE ${CMAKE_BINARY_DIR}/xbmc_configure_helper.sh
"
#!/bin/sh
PF=`${SYSROOT}/usr/bin/${TARGET}-pkg-config --libs python`
export LDFLAGS=\"-L${local}/lib -lgcc -lm -ltinyxml -lgdl -lismd_core -lismd_viddec -lismd_vidpproc -lismd_audio -lismd_vidrend -lismd_demux -lismd_bufmon -losal -lplatform_config -Wl,-rpath=${SYSROOT}/usr/local/lib -Wl,-rpath=${SYSROOT}/lib -Wl,-rpath=${SYSROOT}/usr/lib\"
export CFLAGS=\"-DTARGET_BOXEE=1 -DTIXML_USE_STL -mmmx -msse -msse2 -I${local}/include -DHAS_INTEL_SMD -march=i686\"
export CXXFLAGS=\"$CFLAGS\"
export FREETYPE2_CFLAGS=\"-I${local}/include/freetype2\"
export PYTHON_VERSION=\"2.7.2\"
export PYTHON_NOVERSIONCHECK=\"y\"
export PYTHON=\"${local}/bin/python\"
export PYTHON_CPPFLAGS=\"-I${local}/include/python2.7\"
export PYTHON_LDFLAGS=\"\$PF -ldl -lpthread -lutil\"
${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR}/../../ ./configure --prefix=${local} --host=${TARGET} --with-ffmpeg --disable-gl --disable-pulse --disable-sdl --enable-mysql --disable-x11 --with-arch=i686 --with-platform=linux --enable-gles --enable-external-libraries --disable-joystick --enable-debug --enable-texturepacker --enable-nfs --enable-rtmp --enable-libcec=yes --enable-libusb --with-lirc-device=/var/run/lirc/lircd --enable-airplay
"
)
execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/xbmc_configure_helper.sh)

add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/../../Makefile
	COMMAND PATH=$ENV{PATH}:${SYSROOT}/usr/bin ${CMAKE_BINARY_DIR}/xbmc_configure_helper.sh
	DEPENDS libs ${CMAKE_SOURCE_DIR}/../../configure
)

add_custom_target(
	xbmc ALL
	COMMAND PATH=$ENV{PATH}:${SYSROOT}/usr/bin make ${PARALLEL}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/../../
	DEPENDS ${CMAKE_SOURCE_DIR}/../../Makefile
)

add_custom_target(
	xbmc-install
	COMMAND PATH=$ENV{PATH}:${SYSROOT}/usr/bin make install
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/../../
	DEPENDS ${CMAKE_SOURCE_DIR}/../../xbmc.bin
)

set(libs
	libjasper.so.1	
	libpng15.so.15	
	libyajl.so.2
	libssh.so.4
	liblzo2.so.2
	libtiff.so.5
	libpython2.7.so.1.0
	libpcre.so.1
	libcdio.so.14
	libtag.so.1
	libsmbclient.so.0
	libnfs.so.1
	librtmp.so.1
	libdbus-1.so
	libexpat.so
	libtdb.so.1
	libtalloc.so.2
	libtevent.so.0
	libmpeg2.so.0	
	libFLAC.so
	libmodplug.so
	libogg.so
	libvorbisfile.so
	libvorbis.so
	libvorbisenc.so
	libenca.so.0
	libass.so.5
	libao.so
	libcec.so.2	
	libfribidi.so.0
	libfreetype.so.6
	libgcrypt.so.20
	libcurl.so.4
	libfontconfig.so.1
	libxml2.so
	libxslt.so
	libshairplay.so.0
	airport.key
	libplist.so.1
	libcddb.so.2
	libbluray.so.1
	libgif.so.6
	libssl.so.1.0.0
	libcrypto.so.1.0.0
	libreadline.so.6
	libafpclient.so.0
)

foreach(lib ${libs})
	if(HAVE_PV)
		add_custom_target(
			${lib}
			COMMAND 		${HAVE_PV} ${SYSROOT}/usr/local/lib/${lib} > ${TARGET_DIR}/${lib}
		)
	else()
		add_custom_target(
			${lib}
			COMMAND 		${CMAKE_COMMAND} -E copy_if_different ${SYSROOT}/usr/local/lib/${lib} ${TARGET_DIR}/${lib}
		)
	endif()
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

add_custom_target(
	pythonlibs
	COMMAND 			${CMAKE_COMMAND} -E copy_directory ${SYSROOT}/usr/local/lib/python2.7 ${TARGET_DIR}/python2.7
	# If we don't null out the site.py it'll complain about a broken installation.
	# Unclear if this is much better, but atleast some of python seems to run fine.
	COMMAND 			echo "" > ${TARGET_DIR}/python2.7/site.py
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
		DEPENDS			xbmc-install
	)
endforeach()

add_custom_target(
	shares
	COMMAND 			${CMAKE_COMMAND} -E copy_directory ${SYSROOT}/usr/local/lib/xbmc/addons ${TARGET_DIR}/addons
	COMMAND 			${CMAKE_COMMAND} -E copy_directory ${SYSROOT}/usr/local/lib/xbmc/system ${TARGET_DIR}/system
	DEPENDS 			${shares}
)

if(HAVE_PV)
	set(COPY ${HAVE_PV} ${CMAKE_BINARY_DIR}/xbmc.bin > ${TARGET_DIR}/xbmc.bin)
else()
	set(COPY ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/xbmc.bin ${TARGET_DIR}/xbmc.bin)
endif()

add_custom_target(
	quickdist
	COMMAND 			${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../../xbmc.bin ${CMAKE_BINARY_DIR}/xbmc.bin
	COMMAND 			${SYSROOT}/usr/bin/i686-pc-linux-gnu-strip ${CMAKE_BINARY_DIR}/xbmc.bin
	COMMAND 			${COPY}
	DEPENDS 			xbmc ${CMAKE_SOURCE_DIR}/../../xbmc.bin
)

add_custom_target(
	libdist
	DEPENDS ${libs} libstdc++ libz
)

add_custom_target(
	gconv
	COMMAND 			${CMAKE_COMMAND} -E copy_directory ${SYSROOT}/usr/lib/gconv ${TARGET_DIR}/gconv
)

add_custom_target(
	dist
	DEPENDS shares libdist pythonlibs quickdist gconv
)

add_custom_target(
	libs-refresh
	COMMAND       cp -r ${CMAKE_SOURCE_DIR}/libs/* ${CMAKE_BINARY_DIR}/libs-prefix/src/libs/
	COMMAND       touch ${CMAKE_BINARY_DIR}/libs-prefix/src/libs-stamp/libs-download
)

add_custom_target(refresh
	COMMAND       cp -r ${CMAKE_SOURCE_DIR}/toolchain/* ${CMAKE_BINARY_DIR}/toolchain-prefix/src/toolchain/
	COMMAND       cp -r ${CMAKE_SOURCE_DIR}/intel/* ${CMAKE_BINARY_DIR}/intel-prefix/src/intel/
	COMMAND       touch ${CMAKE_BINARY_DIR}/toolchain-prefix/src/toolchain-stamp/toolchain-download
	COMMAND       touch ${CMAKE_BINARY_DIR}/intel-prefix/src/intel-stamp/intel-download
	DEPENDS libs-refresh
)

add_custom_target(
egl
COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/libs/egl/EGL ${SYSROOT}/usr/local/include/EGL
COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/libs/egl/GLES2 ${SYSROOT}/usr/local/include/GLES2
COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/libs/egl/KHR ${SYSROOT}/usr/local/include/KHR
)

add_custom_target(
pvr
cd ${CMAKE_SOURCE_DIR}
COMMAND rm -rf ${CMAKE_SOURCE_DIR}/../../pvr-addons
COMMAND git clone https://github.com/opdenkamp/xbmc-pvr-addons.git
COMMAND  ${CMAKE_COMMAND} -E copy  ${CMAKE_SOURCE_DIR}/pvr-addons.patch ${CMAKE_SOURCE_DIR}/build/xbmc-pvr-addons
COMMAND patch -p1 < ${CMAKE_SOURCE_DIR}/build/xbmc-pvr-addons/pvr-addons.patch
COMMAND mv  ${CMAKE_SOURCE_DIR}/build/xbmc-pvr-addons ${CMAKE_SOURCE_DIR}/../../pvr-addons
COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR}/../../ ./bootstrap
)
