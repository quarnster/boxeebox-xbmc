cmake_minimum_required(VERSION 2.8)

set(TARGET "i686-pc-linux-gnu")
get_filename_component(SYSROOT "${CMAKE_SOURCE_DIR}/../toolchain/build/sysroot" REALPATH)
set(TARGET_DIR ${SYSROOT}/usr/local)
set(BASIC_CONF <SOURCE_DIR>/configure --prefix=${TARGET_DIR} --host=${TARGET} --enable-shared --disable-static --disable-nls)
set(BASIC_CMAKE CFLAGS=-I${TARGET_DIR}/include CC=${TARGET}-gcc CXX=${TARGET}-g++ RANLIB=${TARGET}-ranlib cmake -DPKG_CONFIG_EXECUTABLE=${TARGET}-pkg-config -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_INSTALL_PREFIX=${TARGET_DIR} -DZLIB_ROOT=${SYSROOT}/usr -DZLIB_LIBRARIES=${SYSROOT}/usr/lib/libz.so -DZLIB_INCLUDE_DIRS=${SYSROOT}/usr/include <SOURCE_DIR>)
set(PARALLEL -j8)
string(REGEX MATCH ${SYSROOT}/usr/bin SRPATH $ENV{PATH})
if(NOT SRPATH)
	message(FATAL_ERROR "${SYSROOT}/usr/bin must be in your path")
endif()

set(PARALLEL -j8)

include(ExternalProject)

ExternalProject_Add(
	bzip2
	URL					http://www.bzip.org/1.0.6/bzip2-1.0.6.tar.gz
	URL_MD5				00b516f4704d4a7cb50a1d97e6e8e15b
	PATCH_COMMAND		cat Makefile | sed s,CC=gcc,CC=${TARGET}-gcc, | sed s,RANLIB=ranlib,RANLIB=${TARGET}-ranlib, | sed s,PREFIX=/usr/local,PREFIX=${TARGET_DIR},g > tmp
		COMMAND 		${CMAKE_COMMAND} -E copy tmp Makefile
	CONFIGURE_COMMAND	""
	BUILD_COMMAND		""
	INSTALL_COMMAND		make install
	BUILD_IN_SOURCE 	1
)

ExternalProject_Add(
	lzo
	URL					http://www.oberhumer.com/opensource/lzo/download/lzo-2.06.tar.gz
	URL_MD5				95380bd4081f85ef08c5209f4107e9f8
	CONFIGURE_COMMAND	${BASIC_CONF}
)

ExternalProject_Add(
	zlib
	URL 				http://zlib.net/zlib-1.2.8.tar.gz
	URL_MD5 			44d667c142d7cda120332623eab69f40
	CONFIGURE_COMMAND	CROSS_PREFIX=${TARGET} AR=${TARGET}-ar CC=${TARGET}-gcc RANLIB=${TARGET}-ranlib <SOURCE_DIR>/configure --uname=Linux --prefix=${SYSROOT}/usr
	BUILD_COMMAND 		""
	INSTALL_COMMAND 	make ${PARALLEL} install
	BUILD_IN_SOURCE 	1
)

ExternalProject_Add(
	openssl
	URL 				http://www.openssl.org/source/openssl-1.0.1e.tar.gz
	URL_MD5 			66bf6f10f060d561929de96f9dfe5b8c
	CONFIGURE_COMMAND 	CC=${TARGET}-gcc RANLIB=${TARGET}-ranlib AR=${TARGET}-ar INSTALL_PREFIX=${TARGET_DIR} <SOURCE_DIR>/Configure linux-elf --openssldir=/ 
	DEPENDS zlib
	BUILD_IN_SOURCE 	1
)

ExternalProject_Add(
	libtiff
	URL 				http://download.osgeo.org/libtiff/tiff-4.0.3.tar.gz
	URL_MD5 			051c1068e6a0627f461948c365290410
	CONFIGURE_COMMAND	${BASIC_CONF}
)	

# ExternalProject_Add(
# 	python
# 	URL 				http://www.python.org/ftp/python/$PYTHON_VERSION/Python-$PYTHON_VERSION.tar.bz2
# 	URL_MD5 			0 
# 	CONFIGURE_COMMAND 	$BOXEE_WORKING_DIR/Python-$PYTHON_VERSION/configure
# 		COMMAND make Parser/pgen python.exe
# 		COMMAND mv python.exe python2.7
# 		COMMAND mv Parser/pgen Parser/hostpgen
# 		COMMAND make distclean
# 		COMMAND ac_cv_file__dev_ptmx=no ac_cv_file__dev_ptc=no ${BASIC_CONF}
# 		COMMAND rm -f Include/graminit.h
# 		COMMAND rm -f Python/graminit.c
# 		COMMAND Parser/hostpgen $BOXEE_WORKING_DIR/Python-$PYTHON_VERSION/Grammar/Grammar Include/graminit.h Python/graminit.c
# )


ExternalProject_Add(
	flex
	URL 				http://sourceforge.net/projects/flex/files/flex-2.5.37.tar.bz2/download
	URL_MD5 			c75940e1fc25108f2a7b3ef42abdae06
	PATCH_COMMAND 		cat Makefile.in | sed s/AR\ =\ ar/AR\ =\ ${TARGET}-ar/g > Makefile.new
		COMMAND ${CMAKE_COMMAND} -E copy Makefile.new Makefile.in 
	CONFIGURE_COMMAND	${BASIC_CONF}
	BUILD_COMMAND 		make ${PARALLEL}
)

ExternalProject_Add(
	expat
	URL 				http://sourceforge.net/projects/expat/files/expat/2.1.0/expat-2.1.0.tar.gz/download
	URL_MD5 			dd7dab7a5fea97d2a6a43f511449b7cd
	CONFIGURE_COMMAND	${BASIC_CONF}
)

ExternalProject_Add(
	yayl
	GIT_REPOSITORY  	https://github.com/lloyd/yajl.git
	GIT_TAG				2.0.1
	URL_MD5 			99
	CONFIGURE_COMMAND 	${BASIC_CMAKE} 
)

ExternalProject_Add(
	tinyxml
	URL 				http://sourceforge.net/projects/tinyxml/files/tinyxml/2.6.2/tinyxml_2_6_2.tar.gz/download
	URL_MD5 			c1b864c96804a10526540c664ade67f0
	CONFIGURE_COMMAND 	""
	BUILD_COMMAND 		make CXX=${TARGET}-g++ LD=${TARGET}-g++ CXXFLAGS=-DTIXML_USE_STL
		COMMAND ${TARGET}-ar r libtinyxml.a tinyxml.o tinystr.o tinyxmlerror.o
	INSTALL_COMMAND ${CMAKE_COMMAND} -E copy libtinyxml.a ${TARGET_DIR}/lib/
		COMMAND ${CMAKE_COMMAND} -E copy tinyxml.h ${TARGET_DIR}/include
		COMMAND ${CMAKE_COMMAND} -E copy tinystr.h ${TARGET_DIR}/include
	BUILD_IN_SOURCE 	1
)

ExternalProject_Add(
	sqlite
	URL 				http://www.sqlite.org/2013/sqlite-autoconf-3071700.tar.gz
	URL_MD5 			18c285053e9562b848209cb0ee16d4ab
	CONFIGURE_COMMAND	${BASIC_CONF}
)

ExternalProject_Add(
	libcdio
	URL 				http://ftp.gnu.org/gnu/libcdio/libcdio-0.90.tar.gz
	URL_MD5				0
	CONFIGURE_COMMAND 	${BASIC_CONF}
)

ExternalProject_Add(
	taglib
	URL 				http://taglib.github.io/releases/taglib-1.8.tar.gz
	URL_MD5 			dcb8bd1b756f2843e18b1fdf3aaeee15
	CONFIGURE_COMMAND	${BASIC_CMAKE}
)
ExternalProject_Add(
	libpng
	URL 				http://sourceforge.net/projects/libpng/files/libpng16/1.6.3/libpng-1.6.3.tar.gz/download
	URL_MD5 			98b652b6c174c35148f1f5f513fe490d
	CONFIGURE_COMMAND 	CFLAGS=-I${TARGET_DIR}/include LDFLAGS=-L${TARGET_DIR}/lib ${BASIC_CONF}
	BUILD_COMMAND		CFLAGS=-I${TARGET_DIR}/include make ${PARALLEL}
	DEPENDS				zlib
)

ExternalProject_Add(
	pcre
	URL 				http://sourceforge.net/projects/pcre/files/pcre/8.33/pcre-8.33.tar.bz2/download
	URL_MD5 			21514018448ac10062ece7a668fca949
	CONFIGURE_COMMAND 	${BASIC_CONF}
)

ExternalProject_Add(
	jasper
	URL 				http://www.ece.uvic.ca/~frodo/jasper/software/jasper-1.900.1.zip
	URL_MD5 			a342b2b4495b3e1394e161eb5d85d754
	CONFIGURE_COMMAND 	${BASIC_CONF}
	BUILD_COMMAND 		make ${PARALLEL}
)

# ExternalProject_Add(
# 	libmicrohttpd
# 	URL 				ftp://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.28.tar.gz
# 	URL_MD5 			16c3cb66c839c6f929677471401a5b53
# 	CONFIGURE_COMMAND 	${BASIC_CONF}
#	DEPENDS 			openssl
# )

ExternalProject_Add(
	freetype2
	URL 				http://sourceforge.net/projects/freetype/files/freetype2/2.5.0/freetype-2.5.0.1.tar.bz2/download
	URL_MD5 			c72e9010b1d986d556fc0b2b5fcbf31a
	CONFIGURE_COMMAND 	${BASIC_CONF} --without-png
	BUILD_COMMAND		make ${PARALLEL}
)

ExternalProject_Add(
	gdbm
	URL 				ftp://ftp.gnu.org/gnu/gdbm/gdbm-1.8.3.tar.gz
	URL_MD5 			1d1b1d5c0245b1c00aff92da751e9aa1
	PATCH_COMMAND		cat configure | sed s,finish_cmds=.*,,g > configure.new
		COMMAND 		${CMAKE_COMMAND} -E copy configure.new configure
		COMMAND 		chmod +x configure
		COMMAND 		cat Makefile.in | sed "s,-o .(BINOWN) -g .(BINGRP),,g" > Makefile.in.new
		COMMAND 		${CMAKE_COMMAND} -E copy Makefile.in.new Makefile.in
	CONFIGURE_COMMAND 	${BASIC_CONF}
)

ExternalProject_Add(
	fribidi
	URL 				http://fribidi.org/download/fribidi-0.19.5.tar.bz2
	URL_MD5 			925bafb97afee8a2fc2d0470c072a155
	CONFIGURE_COMMAND 	${BASIC_CONF}
	DEPENDS				freetype2
)

ExternalProject_Add(
	fontconfig
	URL 				http://www.freedesktop.org/software/fontconfig/release/fontconfig-2.10.93.tar.bz2
	URL_MD5 			0f8e5c63c2d6931626185c02802375a3
	CONFIGURE_COMMAND 	${BASIC_CONF}
	DEPENDS fribidi
)

ExternalProject_Add(
	libmad
	URL 				http://sourceforge.net/projects/mad/files/libmad/0.15.1b/libmad-0.15.1b.tar.gz/download
	URL_MD5 			1be543bc30c56fb6bea1d7bf6a64e66c
	PATCH_COMMAND 		cat configure | sed s/-fforce-mem//g > tmp
		COMMAND 		chmod +x tmp
		COMMAND 		${CMAKE_COMMAND} -E rename tmp configure
	CONFIGURE_COMMAND 	${BASIC_CONF}
)

ExternalProject_Add(
	libjpeg
	URL 				http://sourceforge.net/projects/libjpeg/files/libjpeg/6b/jpegsrc.v6b.tar.gz/download
	URL_MD5 			dbd5f3b47ed13132f04c685d608a7547
	PATCH_COMMAND 		cat configure | sed s/-fforce-mem//g > tmp
		COMMAND 		chmod +x tmp
		COMMAND 		${CMAKE_COMMAND} -E rename tmp configure
	CONFIGURE_COMMAND 	ac_cv_c_const=yes RANLIB=${TARGET}-ranlib CC=${TARGET}-g++ <SOURCE_DIR>/configure --host=${TARGET} --prefix=${TARGET_DIR}
	BUILD_COMMAND 		""
	INSTALL_COMMAND		mkdir -p ${TARGET_DIR}/man/man1/
		COMMAND 		make ${PARALLEL} install-lib
	BUILD_IN_SOURCE 	1
)

ExternalProject_Add(
	fftw3
	URL 				http://www.fftw.org/fftw-3.3.3.tar.gz 
	URL_MD5 			0a05ca9c7b3bfddc8278e7c40791a1c2
	CONFIGURE_COMMAND 	${BASIC_CONF}
	BUILD_COMMAND 		make ${PARALLEL}
)

ExternalProject_Add(
	libsamplerate
	URL 				http://www.mega-nerd.com/SRC/libsamplerate-0.1.8.tar.gz
	URL_MD5 			1c7fb25191b4e6e3628d198a66a84f47
	CONFIGURE_COMMAND 	${BASIC_CONF}
)

ExternalProject_Add(
	libogg
	URL 				http://downloads.xiph.org/releases/ogg/libogg-1.3.1.tar.gz
	URL_MD5 			ba526cd8f4403a5d351a9efaa8608fbc
	CONFIGURE_COMMAND 	${BASIC_CONF}
	BUILD_COMMAND 		make ${PARALLEL}
)


ExternalProject_Add(
	libvorbis
	URL 				http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.3.tar.gz
	URL_MD5 			6b1a36f0d72332fae5130688e65efe1f
	CONFIGURE_COMMAND 	${BASIC_CONF}
	BUILD_COMMAND 		make ${PARALLEL}
	DEPENDS libogg
)

ExternalProject_Add(
	flac
	URL 				http://downloads.xiph.org/releases/flac/flac-1.2.1.tar.gz
	URL_MD5 			153c8b15a54da428d1f0fadc756c22c7
	CONFIGURE_COMMAND 	${BASIC_CONF} --disable-oggtest --disable-xmms-plugin
	BUILD_COMMAND 		""
	INSTALL_COMMAND		make -C src ${PARALLEL} install
		COMMAND 		make -C include ${PARALLEL} install
	DEPENDS libvorbis
)

ExternalProject_Add(
	curl
	URL 				http://curl.haxx.se/download/curl-7.31.0.tar.bz2
	URL_MD5 			b7bea20579ac2f696338ae03f2c19ba5
	CONFIGURE_COMMAND 	${BASIC_CONF}
	BUILD_COMMAND 		make ${PARALLEL}
)

ExternalProject_Add(
	libao
	URL 				http://downloads.xiph.org/releases/ao/libao-1.1.0.tar.gz
	URL_MD5 			2b2508c29bc97e4dc218fa162cf883c8
	CONFIGURE_COMMAND 	${BASIC_CONF}
	BUILD_COMMAND 		make ${PARALLEL}
)

ExternalProject_Add(
	vorbis-tools
	URL 				http://downloads.xiph.org/releases/vorbis/vorbis-tools-1.4.0.tar.gz
	URL_MD5 			567e0fb8d321b2cd7124f8208b8b90e6
	CONFIGURE_COMMAND 	LDFLAGS=-ldl ${BASIC_CONF} 
	DEPENDS flac
)
ExternalProject_Add(
	libass
	URL 				https://libass.googlecode.com/files/libass-0.10.1.tar.gz
	URL_MD5 			6cace482a013a3c4bf3b31a68ac66026
	CONFIGURE_COMMAND 	${BASIC_CONF}
	DEPENDS libpng
)
# TODO:Fails due to libSDL
# ExternalProject_Add(
# 	libmpeg2
# 	URL 				http://libmpeg2.sourceforge.net/files/libmpeg2-0.5.1.tar.gz
# 	URL_MD5 			0f92c7454e58379b4a5a378485bbd8ef
# 	CONFIGURE_COMMAND 	${BASIC_CONF}
# )
ExternalProject_Add(
	libmodplug
	URL 				http://sourceforge.net/projects/modplug-xmms/files/libmodplug/0.8.8.4/libmodplug-0.8.8.4.tar.gz/download
	URL_MD5 			fddc3c704c5489de2a3cf0fedfec59db
	CONFIGURE_COMMAND 	${BASIC_CONF}
)

ExternalProject_Add(
	libgpg-error
	URL 				ftp://ftp.gnupg.org/gcrypt/libgpg-error/libgpg-error-1.12.tar.bz2
	URL_MD5 			8f0eb41a344d19ac2aa9bd101dfb9ce6
	CONFIGURE_COMMAND 	${BASIC_CONF}
)
ExternalProject_Add(
	libgcrypt
	URL 				ftp://ftp.gnupg.org/gcrypt/libgcrypt/libgcrypt-1.5.3.tar.bz2
	URL_MD5 			993159b2924ae7b0e4eaff0743c2db35
	CONFIGURE_COMMAND 	GPG_ERROR_CONFIG=${TARGET_DIR}/bin/gpg-error-config ${BASIC_CONF} --disable-asm
	DEPENDS libgpg-error
)

ExternalProject_Add(
	libssh
	URL 				http://www.libssh.org/files/0.5/libssh-0.5.3.tar.gz
	URL_MD5 			9ad01838d3b89d98e900e0f6260a88cc
	PATCH_COMMAND 		cat CMakeLists.txt | sed s,add_subdirectory\(examples\),,g > CMakeLists.new
		COMMAND 		${CMAKE_COMMAND} -E copy CMakeLists.new CMakeLists.txt
	CONFIGURE_COMMAND 	${BASIC_CMAKE} -DWITH_STATIC_LIB=ON -DCMAKE_SHARED_LINKER_FLAGS=-ldl -DCMAKE_EXE_LINKER_FLAGS=-ldl
	BUILD_COMMAND 		""
	INSTALL_COMMAND 	make install
	DEPENDS libgcrypt openssl
)

# ExternalProject_Add(
# 	ffmpeg
# 	URL 				http://ffmpeg.org/releases/ffmpeg-1.2.2.tar.bz2
# 	URL_MD5 			99
# 	CONFIGURE_COMMAND 	<SOURCE_DIR>/configure --cross-prefix=${TARGET}- --enable-cross-compile --sysroot=${SYSROOT} --target-os=linux --arch=i686 --prefix=${TARGET_DIR} --enable-shared --disable-static --disable-programs --enable-gpl
# )

