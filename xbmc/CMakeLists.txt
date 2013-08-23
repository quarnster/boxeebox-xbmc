cmake_minimum_required(VERSION 2.8)

set(TARGET "i686-pc-linux-gnu")
set(TARGET_DIR "${SYSROOT}/usr/local")

include(ExternalProject)



file(WRITE ${CMAKE_BINARY_DIR}/xbmc_configure_helper.sh
"
#!/bin/sh
PF=`${SYSROOT}/usr/bin/${TARGET}-pkg-config --libs python`
export LDFLAGS=\"-L${TARGET_DIR}/lib -lgcc -lm -ltinyxml\"
export CFLAGS=\"-DTARGET_BOXEE=1 -DTIXML_USE_STL -I${TARGET_DIR}/include\"
export CXXFLAGS=\"$CFLAGS\"
export FREETYPE2_CFLAGS=\"-I${TARGET_DIR}/include/freetype2\"
export PYTHON_VERSION=\"2.7.2\"
export PYTHON_NOVERSIONCHECK=\"y\"
export PYTHON=\"${TARGET_DIR}/bin/python\"
export PYTHON_CPPFLAGS=\"-I${TARGET_DIR}/include/python2.7\"
export PYTHON_LDFLAGS=\"\$PF -ldl -lpthread -lutil\"
../xbmc/configure --prefix=${TARGET_DIR} --host=${TARGET} --disable-gl --disable-sdl --disable-mysql --disable-x11 --disable-samba  --with-arch=i686 --with-platform=linux --enable-gles --enable-external-libraries --disable-joystick --disable-debug --disable-texturepacker
"
)
execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/xbmc_configure_helper.sh)

ExternalProject_Add(
	xbmc
	URL 				http://mirrors.xbmc.org/releases/source/xbmc-12.2.tar.gz
	URL_MD5 			489f3877decae4e265ece54f9eaef0ba
	PATCH_COMMAND 		sed -e "s,have_builtin_sync_add_and_fetch=yes,have_builtin_sync_add_and_fetch=no,g" -e "s,have_builtin_sync_sub_and_fetch=yes,have_builtin_sync_sub_and_fetch=no,g" -e "s,have_builtin_sync_val_compare_and_swap=yes,have_builtin_sync_val_compare_and_swap=no,g" -i configure
	CONFIGURE_COMMAND	${CMAKE_BINARY_DIR}/xbmc_configure_helper.sh 
	BUILD_COMMAND 		make ${PARALLEL}
	BUILD_IN_SOURCE 	1
)
