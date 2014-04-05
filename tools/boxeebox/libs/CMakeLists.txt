cmake_minimum_required(VERSION 2.8)

set(TARGET "i686-pc-linux-gnu")
set(TARGET_DIR ${SYSROOT}/usr/local)
set(BASIC_CONF <SOURCE_DIR>/configure --prefix=${TARGET_DIR} --host=${TARGET} --enable-shared --disable-static --disable-nls)
set(BASIC_CMAKE CFLAGS=-I${TARGET_DIR}/include CC=${TARGET}-gcc CXX=${TARGET}-g++ RANLIB=${TARGET}-ranlib cmake -DPKG_CONFIG_EXECUTABLE=${TARGET}-pkg-config -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_INSTALL_PREFIX=${TARGET_DIR} -DZLIB_ROOT=${SYSROOT}/usr -DZLIB_LIBRARIES=${SYSROOT}/usr/lib/libz.so -DZLIB_INCLUDE_DIRS=${SYSROOT}/usr/include <SOURCE_DIR>)

set(PYTHON_VERSION 2.7.2)

find_package(PythonInterp)

string(REGEX MATCH ${SYSROOT}/usr/bin SRPATH $ENV{PATH})

if(NOT SRPATH)
	message(FATAL_ERROR "${SYSROOT}/usr/bin must be in your path")
endif()

execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine OUTPUT_VARIABLE HOSTMACHINE)
string(REPLACE "\n" "" HOSTMACHINE ${HOSTMACHINE})

include(ExternalProject)

ExternalProject_Add(
    yasm
    URL                   http://www.tortall.net/projects/yasm/releases/yasm-1.2.0.tar.gz
    URL_MD5               4cfc0686cf5350dd1305c4d905eb55a6
    CONFIGURE_COMMAND     ${BASIC_CONF} --disable-warnerror --disable-profiling --disable-gcov --disable-python --disable-python-bindings --enable-nls --disable-rpath --without-dmalloc --with-gnu-ld --without-libiconv-prefix --without-libintl-prefix --disable-debug
    BUILD_COMMAND         make
    INSTALL_COMMAND       make install
)

ExternalProject_Add(
    giflib
    URL                   http://sourceforge.net/projects/giflib/files/giflib-5.x/giflib-5.0.5.tar.gz/download
    URL_MD5               7c1c9434ce892d57a78c63cf15bbfbcc
    CONFIGURE_COMMAND     ${BASIC_CONF}
    BUILD_COMMAND         make
    INSTALL_COMMAND       make install
)

ExternalProject_Add(
    libxml2
    URL                   http://xmlsoft.org/sources/libxml2-2.9.1.tar.gz
    URL_MD5               9c0cfef285d5c4a5c80d00904ddab380
    CONFIGURE_COMMAND     ${BASIC_CONF} --without-python --without-iconv
    BUILD_COMMAND         make
    INSTALL_COMMAND       make install
)

ExternalProject_Add(
    libplist
    URL                   http://www.libimobiledevice.org/downloads/libplist-1.8.tar.bz2
    URL_MD5               2a9e0258847d50f9760dc3ece25f4dc6
    PATCH_COMMAND         sed -ie "s/TARGET_LINK_LIBRARIES( plist /TARGET_LINK_LIBRARIES( plist m z  /" src/CMakeLists.txt
    COMMAND               patch -p1 < ${CMAKE_SOURCE_DIR}/libplist-patches/0001-dontbuildswig.patch
    BUILD_COMMAND         ${BASIC_CMAKE}          
    COMMAND               make CFLAGS=-I${TARGET_DIR}/include LDFLAGS=-L${TARGET_DIR}/lib 
    INSTALL_COMMAND       make install
    DEPENDS               libxml2
)

ExternalProject_Add(
    libshairplay
    GIT_REPOSITORY        https://github.com/juhovh/shairplay.git
    PATCH_COMMAND         ./autogen.sh
	COMMAND            patch -p1 < ${CMAKE_SOURCE_DIR}/libshairplay-patches/xcode-llmvfix.patch
	COMMAND            patch -p0 < ${CMAKE_SOURCE_DIR}/libshairplay-patches/01-add_fairplay_handshake.patch
    CONFIGURE_COMMAND     ${BASIC_CONF} --sysconfdir=/
      BUILD_COMMAND       make
	COMMAND            make install
    INSTALL_COMMAND       cp -P ${CMAKE_BINARY_DIR}/libshairplay-prefix/src/libshairplay/airport.key ${SYSROOT}/usr/local/lib
    DEPENDS               libxml2 libplist
)

ExternalProject_Add(
	mysql
	URL 				http://ftp.gwdg.de/pub/misc/mysql/Downloads/MySQL-5.1/mysql-5.1.72.tar.gz
	URL_MD5 			ed79cd48e3e7402143548917813cdb80
	PATCH_COMMAND
	    COMMAND patch -Np1  < ${CMAKE_SOURCE_DIR}/01-mysqlclient-cross-compile.patch
	CONFIGURE_COMMAND <SOURCE_DIR>/configure CFLAGS=-I${TARGET_DIR}/include LDFLAGS=-L${TARGET_DIR}/lib --prefix=${TARGET_DIR} --host=${TARGET} ac_cv_c_stack_direction=-1 ac_cv_sys_restartable_syscalls=yes --localstatedir=/storage/.mysql --with-unix-socket-path=/var/tmp/mysql.socket --with-tcp-port=3306 --enable-static --disable-shared --with-low-memory --enable-largefile --with-big-tables --with-mysqld-user=mysqld --with-extra-charsets=all --with-extra-charsets=complex --with-named-thread-libs=-lc --with-named-curses-libs=-lncurses --enable-thread-safe-client --enable-assembler --enable-local-infile --without-debug --without-docs --without-man --with-readline --without-libwrap --without-server --without-embedded-server --without-libedit --with-query-cache --with-pthread --with-named-thread-libs=-lpthread
        BUILD_COMMAND make -C include my_config.h
	    COMMAND   make -C mysys libmysys.a
	    COMMAND   make -C strings libmystrings.a
	    COMMAND   make -C vio libvio.a
	    COMMAND   make -C dbug libdbug.a
	    COMMAND   make -C regex libregex.a
	    COMMAND   make -C sql gen_lex_hash
	    COMMAND   make -C mysys libmysys.a
	    COMMAND   make -C scripts comp_sql
	    COMMAND   make -C extra comp_err
	INSTALL_COMMAND make -C libmysql
	    COMMAND   make -C libmysql/ install
	    COMMAND   make -C scripts/ install-binSCRIPTS
	    COMMAND   make -C include/ install
	    COMMAND   sudo ln -sf ${SYSROOT}/usr/local/bin/mysql_config /usr/bin/mysql_config
)

ExternalProject_Add(
    libxslt
    URL               ftp://xmlsoft.org/libxslt/libxslt-1.1.28.tar.gz
    URL_MD5           9667bf6f9310b957254fdcf6596600b7
    PATCH_COMMAND     sed -ie "s|runtest$$(EXEEXT)||" "libxslt/Makefile.in"
	COMMAND        sed -ie "s|testrecurse$$(EXEEXT)||" "libxslt/Makefile.in"
	COMMAND        sed -ie "s|xsltproc||" "libxslt/Makefile.in"
    CONFIGURE_COMMAND ${BASIC_CONF} --without-python --with-debug --with-debugger --with-mem-debug --with-libxml-prefix=${TARGET_DIR} ac_cv_path_LIBGCRYPT_CONFIG=/usr/local/bin/
    BUILD_COMMAND     make
    INSTALL_COMMAND   make install
    DEPENDS libxml2
)

ExternalProject_Add(
	libenca
	URL		    https://github.com/nijel/enca/archive/1.14.tar.gz
	URL_MD5		    17e94759a1cb3e12cb8fe9e9439a1c18
	CONFIGURE_COMMAND   ac_cv_file__dev_random=yes ac_cv_file__dev_urandom=no ac_cv_file__dev_srandom=no ac_cv_file__dev_arandom=no ${BASIC_CONF} --disable-external --without-librecode --disable-rpath --with-gnu-ld
	BUILD_COMMAND       make
	BUILD_IN_SOURCE     1
)

ExternalProject_Add(
	libusb
	URL					http://mirrors.xbmc.org/build-deps/darwin-libs/libusb-0.1.12.tar.gz
	URL_MD5				caf182cbc7565dac0fd72155919672e6
	CONFIGURE_COMMAND   <SOURCE_DIR>/configure --prefix=${TARGET_DIR} --host=${TARGET}  --disable-shared --disable-build-docs
	BUILD_COMMAND       make
    INSTALL_COMMAND		make install
    BUILD_IN_SOURCE     1
)

ExternalProject_Add(
	libcec
	URL 				https://github.com/Pulse-Eight/libcec/archive/libcec-2.1.3.tar.gz
	URL_MD5				ee2f2c71516cea30ac992ddbe8f7ed1c
	PATCH_COMMAND       ./bootstrap
	CONFIGURE_COMMAND 	${BASIC_CONF} --sysconfdir=/etc
	BUILD_COMMAND       make
    INSTALL_COMMAND     make install
)

ExternalProject_Add(
	bzip2
	URL					http://www.bzip.org/1.0.6/bzip2-1.0.6.tar.gz
	URL_MD5				00b516f4704d4a7cb50a1d97e6e8e15b
	PATCH_COMMAND		sed -e "s,CC=gcc,CC=${TARGET}-gcc," -e "s,RANLIB=ranlib,RANLIB=${TARGET}-ranlib," -e "s,PREFIX=/usr/local,PREFIX=${TARGET_DIR},g" -i Makefile
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
	URL 				ftp://ftp.mirrorservice.org/sites/ftp.openssl.org/source/openssl-1.0.1e.tar.gz
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

ExternalProject_Add(
	flex
	URL 				http://sourceforge.net/projects/flex/files/flex-2.5.37.tar.bz2/download
	URL_MD5 			c75940e1fc25108f2a7b3ef42abdae06
	PATCH_COMMAND 		sed -e "s/AR\ =\ ar/AR\ =\ ${TARGET}-ar/g" -i Makefile.in
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
	CONFIGURE_COMMAND 	${BASIC_CMAKE}
)

ExternalProject_Add(
	tinyxml
	URL 				http://sourceforge.net/projects/tinyxml/files/tinyxml/2.6.2/tinyxml_2_6_2.tar.gz/download
	URL_MD5 			c1b864c96804a10526540c664ade67f0
	CONFIGURE_COMMAND 	""
	BUILD_COMMAND 		make CXX=${TARGET}-g++ LD=${TARGET}-g++ CXXFLAGS=-DTIXML_USE_STL
		COMMAND ${TARGET}-ar r libtinyxml.a tinyxml.o tinystr.o tinyxmlerror.o tinyxmlparser.o
	INSTALL_COMMAND ${CMAKE_COMMAND} -E copy libtinyxml.a ${TARGET_DIR}/lib/
		COMMAND ${CMAKE_COMMAND} -E copy tinyxml.h ${TARGET_DIR}/include
		COMMAND ${CMAKE_COMMAND} -E copy tinystr.h ${TARGET_DIR}/include
	BUILD_IN_SOURCE 	1
)

ExternalProject_Add(
	sqlite
	URL 		    https://sqlite.org/2014/sqlite-autoconf-3080403.tar.gz
	URL_MD5 	    0f5459cde43cb269e5120ecd2c671ced
	CONFIGURE_COMMAND   <SOURCE_DIR>/configure --prefix=${TARGET_DIR} --host=${TARGET}  --disable-shared --enable-static --enable-threadsafe --disable-readline --enable-dynamic-extensions
	BUILD_COMMAND       make CXXFLAGS+=-DSQLITE_ENABLE_COLUMN_METADATA=1 CFLAGS+=-DSQLITE_TEMP_STORE=3  CFLAGS+=-DSQLITE_ENABLE_STAT3 CFLAGS+=-DSQLITE_DEFAULT_MMAP_SIZE=268435456
)

ExternalProject_Add(
	libcdio
	URL 				http://ftp.gnu.org/gnu/libcdio/libcdio-0.90.tar.gz
	URL_MD5				1b245b023fb03a58d030fd2800db3247
	CONFIGURE_COMMAND 	${BASIC_CONF}
)

ExternalProject_Add(
    libcddb
    URL                   http://prdownloads.sourceforge.net/libcddb/libcddb-1.3.2.tar.bz2
	URL_MD5               8bb4a6f542197e8e9648ae597cd6bc8a
    CONFIGURE_COMMAND     ${BASIC_CONF}
	DEPENDS               libcdio
)

ExternalProject_Add(
	taglib
	URL 				http://taglib.github.io/releases/taglib-1.8.tar.gz
	URL_MD5 			dcb8bd1b756f2843e18b1fdf3aaeee15
	PATCH_COMMAND 		sed -e "54,61d" -i taglib/toolkit/taglib.h
	CONFIGURE_COMMAND	${BASIC_CMAKE}
)

ExternalProject_Add(
	libpng
	URL 				http://sourceforge.net/projects/libpng/files/libpng15/older-releases/1.5.13/libpng-1.5.13.tar.gz/download
	URL_MD5 			9c5a584d4eb5fe40d0f1bc2090112c65
	CONFIGURE_COMMAND 	CFLAGS=-I${TARGET_DIR}/include LDFLAGS=-L${TARGET_DIR}/lib ${BASIC_CONF}
	BUILD_COMMAND		CFLAGS=-I${TARGET_DIR}/include make ${PARALLEL}
	DEPENDS				zlib
)

ExternalProject_Add(
	pcre
	URL 				http://sourceforge.net/projects/pcre/files/pcre/8.33/pcre-8.33.tar.bz2/download
	URL_MD5 			21514018448ac10062ece7a668fca949
	CONFIGURE_COMMAND 	${BASIC_CONF} --disable-stack-for-recursion --enable-pcre8 --disable-pcre16 --disable-pcre32 --enable-jit --enable-utf --enable-unicode-properties --enable-newline-is-anycrlf
)

ExternalProject_Add(
	jasper
	URL 				http://www.ece.uvic.ca/~frodo/jasper/software/jasper-1.900.1.zip
	URL_MD5 			a342b2b4495b3e1394e161eb5d85d754
	CONFIGURE_COMMAND 	${BASIC_CONF}
	BUILD_COMMAND 		make ${PARALLEL}
)

ExternalProject_Add(
	libmicrohttpd
	URL 				ftp://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.28.tar.gz
	URL_MD5 			16c3cb66c839c6f929677471401a5b53
	PATCH_COMMAND 		sed -e "s,#ifdef LINUX,#ifdef LINUX\\n#define SOCK_NONBLOCK	O_NONBLOCK\\n#define EPOLL_CLOEXEC O_CLOEXEC," -e "s,epoll_create1,epoll_create,g" -i src/microhttpd/daemon.c
	CONFIGURE_COMMAND 	CFLAGS=-I{TARGET_DIR}/include LDFLAGS=-lc ${BASIC_CONF}
	DEPENDS 			openssl
)

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
	PATCH_COMMAND		sed -e "s,finish_cmds=.*,,g" -i configure
		COMMAND 		sed -e "s,-o .(BINOWN) -g .(BINGRP),,g" -i Makefile.in
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
	PATCH_COMMAND 		sed -e "s/-fforce-mem//g" -i configure
	    COMMAND patch -p1 < ${CMAKE_SOURCE_DIR}/libmad-patches/01-libmad-pkgconfig.patch
	    COMMAND patch -p1 < ${CMAKE_SOURCE_DIR}/libmad-patches/02-libmad-thumb-fix.patch
	CONFIGURE_COMMAND 	${BASIC_CONF}
)

ExternalProject_Add(
	libjpeg-turbo
	URL 				http://sourceforge.net/projects/libjpeg-turbo/files/1.3.0/libjpeg-turbo-1.3.0.tar.gz/download
	URL_MD5 			e1e65cc711a1ade1322c06ad4a647741
    CONFIGURE_COMMAND   <SOURCE_DIR>/configure --prefix=${TARGET_DIR} --host=${TARGET} --disable-shared --enable-static --disable-nls --with-jpeg8 --without-simd
    BUILD_COMMAND       make
    INSTALL_COMMAND     make install
    BUILD_IN_SOURCE     1
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
	URL 				http://curl.haxx.se/download/curl-7.35.0.tar.gz
	URL_MD5 			f5ae45ed6e86debb721b68392b5ce13c
	CONFIGURE_COMMAND 	${BASIC_CONF}
	BUILD_COMMAND 		make ${PARALLEL}
	DEPENDS openssl zlib bzip2
)

ExternalProject_Add(
	libao
	URL 				http://downloads.xiph.org/releases/ao/libao-1.1.0.tar.gz
	URL_MD5 			2b2508c29bc97e4dc218fa162cf883c8
	CONFIGURE_COMMAND 	${BASIC_CONF} --disable-pulse
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
	URL 			http://libass.googlecode.com/files/libass-0.10.2.tar.gz
	URL_MD5 		167b784dacded3a65224925c610084cc
	CONFIGURE_COMMAND 	${BASIC_CONF} --with-gnu-ld --enable-enca --enable-fontconfig --disable-harfbuzz --disable-silent-rules
	DEPENDS libpng libenca fribidi freetype2 fontconfig yasm
)
# TODO:Fails due to libSDL
ExternalProject_Add(
	libmpeg2
	URL 				http://libmpeg2.sourceforge.net/files/libmpeg2-0.5.1.tar.gz
	URL_MD5 			0f92c7454e58379b4a5a378485bbd8ef
    PATCH_COMMAND
	    COMMAND patch -p1 < ${CMAKE_SOURCE_DIR}/libmpeg2-patches/01-libmpeg2-add-asm-leading-underscores.patch
		COMMAND patch -p0 < ${CMAKE_SOURCE_DIR}/libmpeg2-patches/02-neon.patch
		COMMAND patch -p1 < ${CMAKE_SOURCE_DIR}/libmpeg2-patches/03-config-fix.patch
		COMMAND patch -p1 < ${CMAKE_SOURCE_DIR}/libmpeg2-patches/04-clang-fix.patch
	CONFIGURE_COMMAND 	${BASIC_CONF} --disable-sdl
)

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
	URL 				ftp://ftp.gnupg.org/gcrypt/libgcrypt/libgcrypt-1.6.0.tar.gz
	URL_MD5 			d698fe863cb7b351316f840fee972512
	CONFIGURE_COMMAND 	GPG_ERROR_CONFIG=${TARGET_DIR}/bin/gpg-error-config ${BASIC_CONF} --disable-asm
	DEPENDS libgpg-error
)

ExternalProject_Add(
    libbluray
    URL                   ftp://ftp.videolan.org/pub/videolan/libbluray/0.5.0/libbluray-0.5.0.tar.bz2
    URL_MD5               04cf15d0f3581a955d3a2ccb9dc06e9e
    CONFIGURE_COMMAND     ${BASIC_CONF} --disable-examples --disable-doxygen-doc
    BUILD_COMMAND         make
    INSTALL_COMMAND       make install
    DEPENDS               libxml2  freetype2
)

ExternalProject_Add(
	libssh
	URL 				http://www.libssh.org/files/0.5/libssh-0.5.3.tar.gz
	URL_MD5 			9ad01838d3b89d98e900e0f6260a88cc
	PATCH_COMMAND 		sed -e "s,add_subdirectory\(examples\),,g" -i CMakeLists.txt
	CONFIGURE_COMMAND 	${BASIC_CMAKE} -DWITH_STATIC_LIB=ON -DCMAKE_SHARED_LINKER_FLAGS=-ldl -DCMAKE_EXE_LINKER_FLAGS=-ldl
	BUILD_COMMAND 		""
	INSTALL_COMMAND 	make install
	DEPENDS libgcrypt openssl
)

ExternalProject_Add(
	util-linux
	URL					https://www.kernel.org/pub/linux/utils/util-linux/v2.23/util-linux-2.23.2.tar.bz2
	URL_MD5				39a02ad0b3b37824c394f40930b7aa38
	CONFIGURE_COMMAND	${BASIC_CONF} --without-ncurses
	BUILD_COMMAND		""
	INSTALL_COMMAND		make install-uuidincHEADERS
)

ExternalProject_Add(
	samba
	URL                     http://ftp.samba.org/pub/samba/stable/samba-3.6.22.tar.gz
	URL_MD5                 59add4bb178ebc188d857bc13a508c0b
	CONFIGURE_COMMAND	CC=${TARGET}-gcc CFLAGS=-I${SYSROOT}/usr/local/include LDFLAGS=-L${SYSROOT}/usr/local/lib ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>/source3 ./configure --prefix=${TARGET_DIR} --host=${TARGET} samba_cv_CC_NEGATIVE_ENUM_VALUES=yes libreplace_cv_HAVE_GETADDRINFO=no ac_cv_file__proc_sys_kernel_core_pattern=yes  samba_cv_HAVE_STAT_ST_FLAGS=no samba_cv_DARWIN_INITGROUPS=no --without-ldap --without-krb5 --without-ads --disable-cups --enable-swat=no --with-winbind=no --without-dnsupdate --without-libsmbsharemodes --without-pam --without-pam_smbpass --without-libaddns
 	BUILD_COMMAND 		${CMAKE_COMMAND} -E chdir <SOURCE_DIR>/source3 make ${PARALLEL} libsmbclient libtalloc libtevent libtdb
	INSTALL_COMMAND 	${CMAKE_COMMAND} -E chdir <SOURCE_DIR>/source3 make ${PARALLEL} installlibsmbclient installlibtalloc installlibtevent installlibtdb
	BUILD_IN_SOURCE 	1
	DEPENDS 		util-linux
)

ExternalProject_Add(
        nfs
        URL               https://sites.google.com/site/libnfstarballs/li/libnfs-1.9.2.tar.gz
        URL_MD5           c2697061fbccf2de610fd3b03f2d1a34
        PATCH_COMMAND     patch -p0 < ${CMAKE_SOURCE_DIR}/timeval.patch
        CONFIGURE_COMMAND ${BASIC_CONF} --disable-examples --disable-tirpc
        BUILD_COMMAND     make
        INSTALL_COMMAND   make install
        BUILD_IN_SOURCE   1
)

ExternalProject_Add(
        rtmpdump
        GIT_REPOSITORY https://github.com/BurntSushi/rtmpdump-ksv.git
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make DESTDIR=${SYSROOT} LIBS_posix=-L${TARGET_DIR}/lib XLIBS=-ldl INC=-I${TARGET_DIR}/include CROSS_COMPILE=${TARGET}-
        INSTALL_COMMAND make install DESTDIR=${SYSROOT}
        BUILD_IN_SOURCE 1
        DEPENDS zlib openssl
)

ExternalProject_Add(
        dbus_expat
        URL http://downloads.sourceforge.net/expat/expat-2.1.0.tar.gz
        URL_MD5 dd7dab7a5fea97d2a6a43f511449b7cd
        CONFIGURE_COMMAND ${BASIC_CONF} CC=${TARGET}-gcc
        BUILD_COMMAND make
        INSTALL_COMMAND make install
)

file(WRITE ${CMAKE_BINARY_DIR}/dbus_configure_helper.sh
"
export CC=${TARGET}-gcc
export CXX=${TARGET}-g++
export CFLAGS=-I${TARGET_DIR}/include
export LDFLAGS=-L${TARGET_DIR}/lib
export LIBS=\"-lpthread -lrt\"
${CMAKE_BINARY_DIR}/dbus-prefix/src/dbus/configure --prefix=${TARGET_DIR} --host=${TARGET} --without-x --disable-xml-docs --disable-doxygen-docs ac_cv_have_abstract_sockets=yes
")
execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/dbus_configure_helper.sh)

ExternalProject_Add(
        dbus
        URL               http://dbus.freedesktop.org/releases/dbus/dbus-1.6.14.tar.gz
        URL_MD5           1c22aebdf952e92908800f1ec2aeb0c4
        CONFIGURE_COMMAND ${CMAKE_BINARY_DIR}/dbus_configure_helper.sh
        BUILD_COMMAND     make
        INSTALL_COMMAND   make install
        BUILD_IN_SOURCE   1
        DEPENDS           dbus_expat
)

file(WRITE ${CMAKE_BINARY_DIR}/python_configure_helper.sh
"
export CC=${TARGET}-gcc
export CXX=${TARGET}-g++
export CPPFLAGS=\"-I${TARGET_DIR}/include -I${SYSROOT}/usr/include\"
export CFLAGS=-I${TARGET_DIR}/include
export LDFLAGS=\"-L${TARGET_DIR}/lib -L${SYSROOT}/usr/lib\"
export MACHDEP=Linux
export ac_sys_system=Linux
export ac_cv_prog_AR=${TARGET}-ar
export ac_cv_file__dev_ptmx=no
export ac_cv_file__dev_ptc=no
export enable_toolbox_glue=no
./configure --prefix=${TARGET_DIR} --host=${TARGET} --enable-shared --disable-ipv6  --build=${HOSTMACHINE} --target=${TARGET} --with-system-expat
")

execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/python_configure_helper.sh)

# Note to self: diff -x generated -r -u Python-2.7.2 python | grep -v '^Only in' > /Volumes/Skivavbild/boxee-xbmc/tools/boxeebox/libs/Python-2.7.2-xcompile.patch
ExternalProject_Add(
	python
	URL 				http://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tar.bz2
	URL_MD5 			ba7b2f11ffdbf195ee0d111b9455a5bd
	PATCH_COMMAND 	<SOURCE_DIR>/configure
		COMMAND make -j8 Parser/pgen python
		COMMAND mv python hostpython
		COMMAND mv Parser/pgen Parser/hostpgen
		COMMAND make distclean
		COMMAND patch -p1 -N < ${CMAKE_SOURCE_DIR}/Python-2.7.2-xcompile.patch
		COMMAND patch -p1 -N < ${CMAKE_SOURCE_DIR}/Python-2.7.2-default-is-optimized.patch
		COMMAND sed -e "13735,13738d" -i configure
		COMMAND sed -e "185d" -i Makefile.pre.in
		COMMAND sed -e "954,959d" -i Makefile.pre.in
	CONFIGURE_COMMAND   ${CMAKE_BINARY_DIR}/python_configure_helper.sh
		COMMAND rm -f Include/graminit.h
		COMMAND rm -f Python/graminit.c
		COMMAND Parser/hostpgen <SOURCE_DIR>/Grammar/Grammar Include/graminit.h Python/graminit.c
	BUILD_COMMAND 		HOSTPYTHON=./hostpython HOSTPGEN=./Parser/hostpgen CROSS_COMPILE_TARGET=yes HOSTARCH=${TARGET} BUILDARCH=${HOSTMACHINE} make ${PARALLEL}
	INSTALL_COMMAND 	HOSTPYTHON=./hostpython HOSTPGEN=./Parser/hostpgen CROSS_COMPILE_TARGET=yes HOSTARCH=${TARGET} BUILDARCH=${HOSTMACHINE} make install
	BUILD_IN_SOURCE 	1
	DEPENDS zlib expat bzip2 gdbm openssl sqlite
)

ExternalProject_Add(
	boost
	URL 				http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz/download
	URL_MD5 			93780777cfbf999a600f62883bd54b17
	PATCH_COMMAND 		${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/boost.user-config.jam ./tools/build/v2/user-config.jam
	CONFIGURE_COMMAND 	./bootstrap.sh --prefix=${TARGET_DIR}
	BUILD_COMMAND 		""
	INSTALL_COMMAND 	./b2 include=${TARGET_DIR}/include linkflags="-L${TARGET_DIR}/lib" toolset=gcc-i686 link=static target-os=linux --without-mpi --without-context --without-log --without-python --without-coroutine ${PARALLEL} install
	BUILD_IN_SOURCE 	1
	DEPENDS 			zlib bzip2
)

if(NOT EXISTS "${TARGET_DIR}/include/EGL/egl.h")
	message("Downloading egl headers")
	file(DOWNLOAD http://www.khronos.org/registry/egl/api/EGL/egl.h ${TARGET_DIR}/include/EGL/egl.h)
	file(DOWNLOAD http://www.khronos.org/registry/egl/api/EGL/eglext.h ${TARGET_DIR}/include/EGL/eglext.h)
	file(WRITE ${TARGET_DIR}/include/EGL/eglplatform.h "
#pragma once
#define EGLAPI
#define EGLAPIENTRY
typedef int NativeDisplayType;
typedef void *NativeWindowType;
typedef void *NativePixmapType;
typedef NativeDisplayType EGLNativeDisplayType;
typedef NativePixmapType  EGLNativePixmapType;
typedef NativeWindowType  EGLNativeWindowType;
typedef int EGLint;
")
	file(DOWNLOAD http://www.khronos.org/registry/egl/api/KHR/khrplatform.h ${TARGET_DIR}/include/KHR/khrplatform.h)
endif()

if(NOT EXISTS "${TARGET_DIR}/include/GLES2/gl2.h")
	message("Downloading gles2 headers")
	file(DOWNLOAD "http://www.khronos.org/registry/gles/api/GLES2/gl2.h" ${TARGET_DIR}/include/GLES2/gl2.h)
	file(DOWNLOAD "http://www.khronos.org/registry/gles/api/GLES2/gl2ext.h" ${TARGET_DIR}/include/GLES2/gl2ext.h)
	file(DOWNLOAD "http://www.khronos.org/registry/gles/api/GLES2/gl2platform.h" ${TARGET_DIR}/include/GLES2/gl2platform.h)
endif()

add_custom_command(OUTPUT ${TARGET_DIR}/lib/libEGL.so
	COMMAND cat ${TARGET_DIR}/include/EGL/egl.h | ${PYTHON_EXECUTABLE} -c \"import sys, re\; print '\#include <EGL/egl.h>\\n%s' % '\\n'.join(['%s {}' %x for x in re.compile(r'^\(EGLAPI\\s[^\;]+\)\;', re.M).findall(sys.stdin.read())])\" > ${CMAKE_BINARY_DIR}/libEGL.c
	COMMAND "${SYSROOT}/usr/bin/${TARGET}-gcc" ${CMAKE_C_FLAGS} -I${TARGET_DIR}/include -shared -o ${TARGET_DIR}/lib/libEGL.so ${CMAKE_BINARY_DIR}/libEGL.c
	DEPENDS "${TARGET_DIR}/include/EGL/egl.h"
)

add_custom_command(OUTPUT ${TARGET_DIR}/lib/libGLESv2.so
	COMMAND cat ${TARGET_DIR}/include/GLES2/gl2.h | ${PYTHON_EXECUTABLE} -c \"import sys, re\; print '\#include <GLES2/gl2.h>\\n%s' % '\\n'.join(['%s {}' %x for x in re.compile(r'^\(GL_APICALL\\s[^\;]+\)\;', re.M).findall(sys.stdin.read())])\" > ${CMAKE_BINARY_DIR}/libGLESv2.c
	COMMAND "${SYSROOT}/usr/bin/${TARGET}-gcc" ${CMAKE_C_FLAGS} -I${TARGET_DIR}/include -shared -o ${TARGET_DIR}/lib/libGLESv2.so ${CMAKE_BINARY_DIR}/libGLESv2.c
	DEPENDS "${TARGET_DIR}/include/GLES2/gl2.h"
)

add_custom_target(dummylibs ALL DEPENDS DEPENDS ${TARGET_DIR}/lib/libEGL.so ${TARGET_DIR}/lib/libGLESv2.so)
