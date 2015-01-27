cmake_minimum_required(VERSION 2.8)

set(TARGET "i686-pc-linux-gnu")
set(TARGET_DIR ${SYSROOT}/usr/local)
set(BASIC_CONF <SOURCE_DIR>/configure --prefix=${TARGET_DIR} --host=${TARGET} --enable-shared --disable-static --disable-nls)
set(BASIC_CMAKE CFLAGS=-I${TARGET_DIR}/include CC=${TARGET}-gcc CXX=${TARGET}-g++ RANLIB=${TARGET}-ranlib cmake -DPKG_CONFIG_EXECUTABLE=${TARGET}-pkg-config -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_INSTALL_PREFIX=${TARGET_DIR} -DZLIB_ROOT=${SYSROOT}/usr -DZLIB_LIBRARIES=${SYSROOT}/usr/lib/libz.so -DZLIB_INCLUDE_DIRS=${SYSROOT}/usr/include <SOURCE_DIR>)
set(ENABLE_MYSQL ON CACHE BOOL "Whether to build with mysql support or not")
set(ENABLE_ICONV OFF CACHE BOOL "Whether to build with iconv support or not")
set(ENABLE_GETTEXT OFF CACHE BOOL "Whether to build with gettext support or not")
set(ENABLE_GLIB_LINUX ON CACHE BOOL "Whether to build with glib_linux support or not")
set(ENABLE_GLIB_OSX OFF CACHE BOOL "Whether to build with glib_osx support or not")

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
	libgpg-error
	URL 			ftp://ftp.gnupg.org/gcrypt/libgpg-error/libgpg-error-1.12.tar.bz2
	URL_MD5 		8f0eb41a344d19ac2aa9bd101dfb9ce6
	CONFIGURE_COMMAND 	${BASIC_CONF}
)

ExternalProject_Add(
	libgcrypt
	URL 			ftp://ftp.gnupg.org/gcrypt/libgcrypt/libgcrypt-1.6.2.tar.gz
	URL_MD5 		d19adc062edff0ebc7e887212733ef1f
	CONFIGURE_COMMAND 	GPG_ERROR_CONFIG=${TARGET_DIR}/bin/gpg-error-config ${BASIC_CONF} --disable-asm
	DEPENDS libgpg-error
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
    ncurses
    URL                   http://ftp.gnu.org/pub/gnu/ncurses/ncurses-5.9.tar.gz
    URL_MD5               8cb9c412e5f2d96bc6f459aa8c6282a1
    CONFIGURE_COMMAND     ${BASIC_CONF}
    BUILD_COMMAND         make
    INSTALL_COMMAND       make install
)

ExternalProject_Add(
    readline
    URL                   ftp://ftp.cwru.edu/pub/bash/readline-6.2.tar.gz
    URL_MD5               67948acb2ca081f23359d0256e9a271c
    CONFIGURE_COMMAND     ${BASIC_CONF} CFLAGS=-I${TARGET_DIR}/include LDFLAGS=-L${TARGET_DIR}/lib --with-curses --without-purify
    BUILD_COMMAND         make
    INSTALL_COMMAND       make install
	DEPENDS               ncurses
)

ExternalProject_Add(
    afpfs-ng
    URL                   http://mirrors.xbmc.org/build-deps/darwin-libs/afpfs-ng-0.8.1.tar.bz2
    URL_MD5               1bdd9f8a06e6085ea4cc38ce010ef60b
    PATCH_COMMAND
        COMMAND           patch -p1 < ${CMAKE_SOURCE_DIR}/afpfs-ng-patches/fix_afpfs-ng_includes.patch
        COMMAND           patch -p1 < ${CMAKE_SOURCE_DIR}/afpfs-ng-patches/01-gcrypt.patch
        COMMAND           patch -p1 < ${CMAKE_SOURCE_DIR}/afpfs-ng-patches/02-pointer.patch
        COMMAND           patch -p1 < ${CMAKE_SOURCE_DIR}/afpfs-ng-patches/04-boxee1.patch
        COMMAND           patch -p1 < ${CMAKE_SOURCE_DIR}/afpfs-ng-patches/05-boxee2.patch
        COMMAND           patch -p1 < ${CMAKE_SOURCE_DIR}/afpfs-ng-patches/06-boxee3.patch
        COMMAND           patch -p1 < ${CMAKE_SOURCE_DIR}/afpfs-ng-patches/07-boxee4.patch
        COMMAND           patch -p1 < ${CMAKE_SOURCE_DIR}/afpfs-ng-patches/08-boxee5.patch
        COMMAND           patch -p1 < ${CMAKE_SOURCE_DIR}/afpfs-ng-patches/10-fix-errno.patch
        COMMAND           patch -p1 < ${CMAKE_SOURCE_DIR}/afpfs-ng-patches/11-fix-stat.patch
        COMMAND           patch -p0 < ${CMAKE_SOURCE_DIR}/afpfs-ng-patches/android.patch
        COMMAND           autoreconf -vif
    CONFIGURE_COMMAND     ${BASIC_CONF} CFLAGS=-I${TARGET_DIR}/include LDFLAGS=-L${TARGET_DIR}/lib ac_cv_func_malloc_0_nonnull=yes --disable-fuse --enable-gcrypt
    BUILD_COMMAND         make
    INSTALL_COMMAND       make install
    BUILD_IN_SOURCE       1
    DEPENDS               readline ncurses
)

if(ENABLE_MYSQL)
ExternalProject_Add(
	mysql
	URL 		  http://mysql.mirrors.pair.com/Downloads/MySQL-5.1/mysql-5.1.72.tar.gz
	URL_MD5 	  ed79cd48e3e7402143548917813cdb80
	PATCH_COMMAND
	    COMMAND       patch -Np1  < ${CMAKE_SOURCE_DIR}/01-mysqlclient-cross-compile.patch
	CONFIGURE_COMMAND ac_cv_c_bigendian=no <SOURCE_DIR>/configure CFLAGS=-I${TARGET_DIR}/include LDFLAGS=-L${TARGET_DIR}/lib --prefix=${TARGET_DIR} --host=${TARGET} ac_cv_c_stack_direction=-1 ac_cv_sys_restartable_syscalls=yes --localstatedir=/storage/.mysql --with-unix-socket-path=/var/tmp/mysql.socket --with-tcp-port=3306 --enable-static --disable-shared --with-low-memory --enable-largefile --with-big-tables --with-mysqld-user=mysqld --with-extra-charsets=all --with-extra-charsets=complex --with-named-thread-libs=-lc --with-named-curses-libs=-lncurses --enable-thread-safe-client --enable-assembler --enable-local-infile --without-debug --without-docs --without-man --with-readline --without-libwrap --without-server --without-embedded-server --without-libedit --with-query-cache --with-pthread --with-named-thread-libs=-lpthread
	BUILD_COMMAND     make -C include my_config.h
	    COMMAND       make -C mysys libmysys.a
	    COMMAND       make -C strings libmystrings.a
	    COMMAND       make -C vio libvio.a
	    COMMAND       make -C dbug libdbug.a
	    COMMAND       make -C regex libregex.a
	    COMMAND       make -C sql gen_lex_hash
	    COMMAND       make -C mysys libmysys.a
	    COMMAND       make -C scripts comp_sql
	    COMMAND       make -C extra comp_err
	INSTALL_COMMAND   make -C libmysql
	    COMMAND       make -C libmysql/ install
	    COMMAND       make -C scripts/ install-binSCRIPTS
	    COMMAND       make -C include/ install
	    COMMAND       sudo ln -sf ${SYSROOT}/usr/local/bin/mysql_config /usr/bin/mysql_config
	    DEPENDS       readline ncurses
)
endif()

ExternalProject_Add(
    libxslt
    URL               ftp://xmlsoft.org/libxslt/libxslt-1.1.28.tar.gz
    URL_MD5           9667bf6f9310b957254fdcf6596600b7
    PATCH_COMMAND     sed -ie "s|runtest$$(EXEEXT)||" "libxslt/Makefile.in"
	COMMAND        sed -ie "s|testrecurse$$(EXEEXT)||" "libxslt/Makefile.in"
	COMMAND        sed -ie "s|xsltproc||" "libxslt/Makefile.in"
    CONFIGURE_COMMAND ${BASIC_CONF} --without-python --with-debug --with-debugger --with-mem-debug --without-crypto --with-libxml-prefix=${TARGET_DIR}
    BUILD_COMMAND     make
    INSTALL_COMMAND   make install
    DEPENDS libxml2
)

if(ENABLE_ICONV)
ExternalProject_Add(
	iconv
	URL                  http://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.14.tar.gz
	URL_MD5              e34509b1623cec449dfeb73d7ce9c6c6
	CONFIGURE_COMMAND    ${BASIC_CONF}
	BUILD_COMMAND        make
)
endif()

ExternalProject_Add(
	libusb
	URL                 http://mirrors.xbmc.org/build-deps/darwin-libs/libusb-0.1.12.tar.gz
	URL_MD5             caf182cbc7565dac0fd72155919672e6
	CONFIGURE_COMMAND   <SOURCE_DIR>/configure --prefix=${TARGET_DIR} --host=${TARGET}  --disable-shared --disable-build-docs
	BUILD_COMMAND       make
    INSTALL_COMMAND         make install
    BUILD_IN_SOURCE         1
)

ExternalProject_Add(
	bzip2
	URL                 http://www.bzip.org/1.0.6/bzip2-1.0.6.tar.gz
	URL_MD5             00b516f4704d4a7cb50a1d97e6e8e15b
	PATCH_COMMAND       sed -e "s,CC=gcc,CC=${TARGET}-gcc," -e "s,RANLIB=ranlib,RANLIB=${TARGET}-ranlib," -e "s,PREFIX=/usr/local,PREFIX=${TARGET_DIR},g" -i Makefile
	CONFIGURE_COMMAND   ""
	BUILD_COMMAND       ""
	INSTALL_COMMAND     make install
	BUILD_IN_SOURCE     1
)

ExternalProject_Add(
	lzo
	URL                 http://www.oberhumer.com/opensource/lzo/download/lzo-2.06.tar.gz
	URL_MD5             95380bd4081f85ef08c5209f4107e9f8
	CONFIGURE_COMMAND   ${BASIC_CONF}
)

ExternalProject_Add(
	zlib
	URL                     http://zlib.net/zlib-1.2.8.tar.gz
	URL_MD5                 44d667c142d7cda120332623eab69f40
	CONFIGURE_COMMAND	CROSS_PREFIX=${TARGET} AR=${TARGET}-ar CC=${TARGET}-gcc RANLIB=${TARGET}-ranlib <SOURCE_DIR>/configure --uname=Linux --prefix=${SYSROOT}/usr
	BUILD_COMMAND 		""
	INSTALL_COMMAND 	make ${PARALLEL} install
	BUILD_IN_SOURCE 	1
)

ExternalProject_Add(
	openssl
	URL                     https://www.openssl.org/source/openssl-1.0.2.tar.gz
	URL_MD5                 38373013fc85c790aabf8837969c5eba
	CONFIGURE_COMMAND 	CC=${TARGET}-gcc RANLIB=${TARGET}-ranlib AR=${TARGET}-ar <SOURCE_DIR>/Configure --prefix=${SYSROOT}/usr/local shared --openssldir=/ --libdir=lib linux-elf threads enable-camellia enable-seed enable-tlsext enable-rfc3779 enable-cms enable-md2 no-krb5 no-mdc2 no-rc5 no-ec no-ec2mno-ecdh no-ecdsa no-srp
	BUILD_COMMAND           make
	INSTALL_COMMAND 	make install
	BUILD_IN_SOURCE 	1
	DEPENDS                 zlib
)

ExternalProject_Add(
	libtiff
	URL			http://download.osgeo.org/libtiff/tiff-4.0.3.tar.gz
	URL_MD5			051c1068e6a0627f461948c365290410
	CONFIGURE_COMMAND	${BASIC_CONF}
)

ExternalProject_Add(
	flex
	URL			http://sourceforge.net/projects/flex/files/flex-2.5.39.tar.bz2/download
	URL_MD5			77d44c6bb8c0705e0017ab9a84a1502b
	PATCH_COMMAND 		sed -e "s/AR\ =\ ar/AR\ =\ ${TARGET}-ar/g" -i Makefile.in
	CONFIGURE_COMMAND	ac_cv_func_realloc_0_nonnull=yes ac_cv_func_malloc_0_nonnull=yes ${BASIC_CONF}
	BUILD_COMMAND 		make ${PARALLEL}
)

ExternalProject_Add(
	expat
	URL			http://sourceforge.net/projects/expat/files/expat/2.1.0/expat-2.1.0.tar.gz/download
	URL_MD5			dd7dab7a5fea97d2a6a43f511449b7cd
	CONFIGURE_COMMAND	${BASIC_CONF}
)

ExternalProject_Add(
	yajl
	GIT_REPOSITORY  	https://github.com/lloyd/yajl.git
	GIT_TAG			2.1.0
	UPDATE_COMMAND           ""
	CONFIGURE_COMMAND 	${BASIC_CMAKE}
)

ExternalProject_Add(
	tinyxml
	URL			http://sourceforge.net/projects/tinyxml/files/tinyxml/2.6.2/tinyxml_2_6_2.tar.gz/download
	URL_MD5			c1b864c96804a10526540c664ade67f0
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
	URL 		    http://www.sqlite.org/2014/sqlite-autoconf-3080704.tar.gz
	URL_MD5 	    33bb8db0038317ce1b0480ca1185c7ba
	CONFIGURE_COMMAND   <SOURCE_DIR>/configure --prefix=${TARGET_DIR} --host=${TARGET}  --disable-shared --enable-static --enable-threadsafe --disable-readline --enable-dynamic-extensions
	BUILD_COMMAND       make CXXFLAGS+=-DSQLITE_ENABLE_COLUMN_METADATA=1 CFLAGS+=-DSQLITE_TEMP_STORE=3  CFLAGS+=-DSQLITE_ENABLE_STAT3 CFLAGS+=-DSQLITE_DEFAULT_MMAP_SIZE=268435456
)

ExternalProject_Add(
	libcdio
	URL			http://ftp.gnu.org/gnu/libcdio/libcdio-0.90.tar.gz
	URL_MD5			1b245b023fb03a58d030fd2800db3247
	CONFIGURE_COMMAND 	${BASIC_CONF} --disable-example-progs --without-cpp-progs --without-cd-drive --without-cd-info --without-cdda-player  --without-cd-read --without-iso-info --without-iso-read
)

ExternalProject_Add(
        libcddb
        URL                   http://prdownloads.sourceforge.net/libcddb/libcddb-1.3.2.tar.bz2
	URL_MD5               8bb4a6f542197e8e9648ae597cd6bc8a
	CONFIGURE_COMMAND     ac_cv_func_realloc_0_nonnull=yes ac_cv_func_malloc_0_nonnull=yes ${BASIC_CONF}
	DEPENDS               libcdio
)

ExternalProject_Add(
	taglib
	URL 			http://taglib.github.io/releases/taglib-1.8.tar.gz
	URL_MD5 		dcb8bd1b756f2843e18b1fdf3aaeee15
	PATCH_COMMAND 		sed -e "54,61d" -i taglib/toolkit/taglib.h
	CONFIGURE_COMMAND	${BASIC_CMAKE}
)

ExternalProject_Add(
	libpng
	URL 			http://sourceforge.net/projects/libpng/files/libpng15/older-releases/1.5.13/libpng-1.5.13.tar.gz/download
	URL_MD5 		9c5a584d4eb5fe40d0f1bc2090112c65
	CONFIGURE_COMMAND 	CFLAGS=-I${TARGET_DIR}/include LDFLAGS=-L${TARGET_DIR}/lib ${BASIC_CONF}
	BUILD_COMMAND		CFLAGS=-I${TARGET_DIR}/include make ${PARALLEL}
	DEPENDS			zlib
)

ExternalProject_Add(
	pcre
	URL 			http://sourceforge.net/projects/pcre/files/pcre/8.36/pcre-8.36.tar.bz2/download
	URL_MD5 		b767bc9af0c20bc9c1fe403b0d41ad97
	CONFIGURE_COMMAND 	${BASIC_CONF} --enable-utf8 --enable-unicode-properties
)

ExternalProject_Add(
	libmicrohttpd
	URL 			ftp://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.34.tar.gz
	URL_MD5 		2947eee13c2c8affb95023a0cb6fda0c
	PATCH_COMMAND 		sed -e "s,#ifdef LINUX,#ifdef LINUX\\n#define SOCK_NONBLOCK	O_NONBLOCK\\n#define EPOLL_CLOEXEC O_CLOEXEC," -e "s,epoll_create1,epoll_create,g" -i src/microhttpd/daemon.c
	CONFIGURE_COMMAND 	CFLAGS=-I{TARGET_DIR}/include LDFLAGS=-lc ac_cv_path_LIBGCRYPT_CONFIG=${SYSROOT}/usr/local/bin/ <SOURCE_DIR>/configure --prefix=${TARGET_DIR} --host=${TARGET} --disable-shared --enable-static --disable-curl --disable-https
	DEPENDS 		libgcrypt
)

ExternalProject_Add(
	freetype2
	URL 			http://sourceforge.net/projects/freetype/files/freetype2/2.5.4/freetype-2.5.4.tar.gz/download
	URL_MD5 		99c5ab378218218f25afcd5171898faf
	CONFIGURE_COMMAND 	LIBPNG_CFLAGS=-I${SYSROOT}/usr/local/include LIBPNG_LDFLAGS=-L${SYSROOT}/usr/local/lib ${BASIC_CONF}
	BUILD_COMMAND		make ${PARALLEL}
	DEPENDS                 libpng
)

ExternalProject_Add(
	gdbm
	URL 			ftp://ftp.gnu.org/gnu/gdbm/gdbm-1.11.tar.gz
	URL_MD5 		72c832680cf0999caedbe5b265c8c1bd
	PATCH_COMMAND		sed -e "s,finish_cmds=.*,,g" -i configure
	   COMMAND 	        sed -e "s,-o .(BINOWN) -g .(BINGRP),,g" -i Makefile.in
	CONFIGURE_COMMAND 	${BASIC_CONF}
)

ExternalProject_Add(
	fribidi
	URL 			http://fribidi.org/download/fribidi-0.19.6.tar.bz2
	URL_MD5 		ce93d862344991173dabb609bf93ca1d
	CONFIGURE_COMMAND 	${BASIC_CONF} --without-glib --disable-debug --disable-deprecated --disable-silent-rules --enable-malloc --enable-charsets --with-gnu-ld
	DEPENDS			freetype2
)

ExternalProject_Add(
	fontconfig
	URL 			http://www.freedesktop.org/software/fontconfig/release/fontconfig-2.11.1.tar.gz
	URL_MD5 		e75e303b4f7756c2b16203a57ac87eba
	CONFIGURE_COMMAND 	${BASIC_CONF} --with-default-fonts=/usr/share/fonts/liberation --disable-docs --without-add-fonts --disable-dependency-tracking ac_cv_path_FREETYPE_CONFIG=${SYSROOT}/usr/local/bin/freetype-config
	DEPENDS fribidi freetype2 zlib expat libxml2
)

ExternalProject_Add(
	libjpeg-turbo
	URL 			http://sourceforge.net/projects/libjpeg-turbo/files/1.3.1/libjpeg-turbo-1.3.1.tar.gz/download
	URL_MD5 		2c3a68129dac443a72815ff5bb374b05
	CONFIGURE_COMMAND       <SOURCE_DIR>/configure --prefix=${TARGET_DIR} --host=${TARGET} --disable-shared --enable-static --disable-nls --with-jpeg8 --without-simd
	BUILD_COMMAND           make
	INSTALL_COMMAND         make install
	BUILD_IN_SOURCE         1
	DEPENDS                 yasm
)

ExternalProject_Add(
	jasper
	URL 			http://www.ece.uvic.ca/~frodo/jasper/software/jasper-1.900.1.zip
	URL_MD5 		a342b2b4495b3e1394e161eb5d85d754
	CONFIGURE_COMMAND 	${BASIC_CONF}
	BUILD_COMMAND 		make ${PARALLEL}
	DEPENDS                 libjpeg-turbo
)

ExternalProject_Add(
	fftw3
	URL 			http://www.fftw.org/fftw-3.3.4.tar.gz
	URL_MD5 		2edab8c06b24feeb3b82bbb3ebf3e7b3
	CONFIGURE_COMMAND 	${BASIC_CONF}
	BUILD_COMMAND 		make ${PARALLEL}
)

ExternalProject_Add(
	libogg
	URL 			http://downloads.xiph.org/releases/ogg/libogg-1.3.1.tar.gz
	URL_MD5 		ba526cd8f4403a5d351a9efaa8608fbc
	CONFIGURE_COMMAND 	${BASIC_CONF}
	BUILD_COMMAND 		make ${PARALLEL}
)

ExternalProject_Add(
	libvorbis
	URL 			http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.3.tar.gz
	URL_MD5 		6b1a36f0d72332fae5130688e65efe1f
	CONFIGURE_COMMAND 	${BASIC_CONF}
	BUILD_COMMAND 		make ${PARALLEL}
	DEPENDS libogg
)

ExternalProject_Add(
	flac
	URL 			http://downloads.xiph.org/releases/flac/flac-1.2.1.tar.gz
	URL_MD5 		153c8b15a54da428d1f0fadc756c22c7
	CONFIGURE_COMMAND 	${BASIC_CONF} --disable-oggtest --disable-xmms-plugin
	BUILD_COMMAND 		""
	INSTALL_COMMAND		make -C src ${PARALLEL} install
		COMMAND 	make -C include ${PARALLEL} install
	DEPENDS libvorbis
)

ExternalProject_Add(
	libao
	URL 			http://downloads.xiph.org/releases/ao/libao-1.2.0.tar.gz
	URL_MD5 		9f5dd20d7e95fd0dd72df5353829f097
	CONFIGURE_COMMAND 	${BASIC_CONF} --disable-pulse
	BUILD_COMMAND 		make ${PARALLEL}
)

# TODO:Fails due to libSDL
ExternalProject_Add(
	libmpeg2
	URL 			http://libmpeg2.sourceforge.net/files/libmpeg2-0.5.1.tar.gz
	URL_MD5 		0f92c7454e58379b4a5a378485bbd8ef
    PATCH_COMMAND
	COMMAND patch -p1 < ${CMAKE_SOURCE_DIR}/libmpeg2-patches/01-libmpeg2-add-asm-leading-underscores.patch
	COMMAND patch -p0 < ${CMAKE_SOURCE_DIR}/libmpeg2-patches/02-neon.patch
	COMMAND patch -p1 < ${CMAKE_SOURCE_DIR}/libmpeg2-patches/03-config-fix.patch
	COMMAND patch -p1 < ${CMAKE_SOURCE_DIR}/libmpeg2-patches/04-clang-fix.patch
	CONFIGURE_COMMAND 	${BASIC_CONF} --disable-sdl
)

ExternalProject_Add(
	libmodplug
	URL 			http://sourceforge.net/projects/modplug-xmms/files/libmodplug/0.8.8.5/libmodplug-0.8.8.5.tar.gz/download
	URL_MD5 		5f30241db109d647781b784e62ddfaa1
	CONFIGURE_COMMAND 	${BASIC_CONF}
)

ExternalProject_Add(
    libbluray
    URL                         ftp://ftp.videolan.org/pub/videolan/libbluray/0.6.2/libbluray-0.6.2.tar.bz2
    URL_MD5                     f4d2f2cab53f976cbb22cbae069057bd
    CONFIGURE_COMMAND           ${BASIC_CONF} --disable-examples --disable-doxygen-doc
    BUILD_COMMAND               make
    INSTALL_COMMAND             make install
    DEPENDS                     libxml2  freetype2
)

ExternalProject_Add(
	libssh
	URL                     http://git.libssh.org/projects/libssh.git/snapshot/libssh-a48711ae7ef890c94e2a824afb899df385c406ee.tar.gz
	URL_MD5                 a31106e0060c6dce355b29bd5388b436
	PATCH_COMMAND
	COMMAND                 patch -p0 < ${CMAKE_SOURCE_DIR}/md5.patch
	COMMAND                 sed -e "s,add_subdirectory\(examples\),,g" -i CMakeLists.txt
	COMMAND                 sed -ie "s|-fstack-protector|-fnostack-protector|" "cmake/Modules/DefineCompilerFlags.cmake"
	CONFIGURE_COMMAND 	${BASIC_CMAKE} -DCMAKE_INSTALL_PREFIX=${TARGET_DIR} -DWITH_STATIC_LIB=1 -DWITH_SERVER=OFF  -DWITH_GCRYPT=OFF -DWITH_EXAMPLES=0 -DTHREADS_PTHREAD_ARG=0 -DCMAKE_SHARED_LINKER_FLAGS=-ldl -DCMAKE_EXE_LINKER_FLAGS=-ldl
	BUILD_COMMAND 		""
	INSTALL_COMMAND 	make install
	DEPENDS                 openssl
)

ExternalProject_Add(
	util-linux
	URL                     https://www.kernel.org/pub/linux/utils/util-linux/v2.25/util-linux-2.25.2.tar.gz
	URL_MD5                 f4b25b1c89e9021cc19911e8a2d2c2e5
	CONFIGURE_COMMAND	${BASIC_CONF} --without-ncurses --without-python
	BUILD_COMMAND		""
	INSTALL_COMMAND		make install-uuidincHEADERS
)

ExternalProject_Add(
	samba
	URL                     http://ftp.samba.org/pub/samba/stable/samba-3.6.24.tar.gz
	URL_MD5                 d98425c0c2b73e08f048d31ffc727fb0
	CONFIGURE_COMMAND	CC=${TARGET}-gcc CFLAGS=-I${SYSROOT}/usr/local/include LDFLAGS=-L${SYSROOT}/usr/local/lib ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>/source3 ./configure --prefix=${TARGET_DIR} --host=${TARGET} samba_cv_CC_NEGATIVE_ENUM_VALUES=yes libreplace_cv_HAVE_GETADDRINFO=no ac_cv_file__proc_sys_kernel_core_pattern=yes  samba_cv_HAVE_STAT_ST_FLAGS=no samba_cv_DARWIN_INITGROUPS=no --without-ldap --without-krb5 --without-ads --disable-cups --enable-swat=no --with-winbind=no --without-dnsupdate --without-libsmbsharemodes --without-pam --without-pam_smbpass --without-libaddns
 	BUILD_COMMAND 		${CMAKE_COMMAND} -E chdir <SOURCE_DIR>/source3 make ${PARALLEL} libsmbclient libtalloc libtevent libtdb
	INSTALL_COMMAND 	${CMAKE_COMMAND} -E chdir <SOURCE_DIR>/source3 make ${PARALLEL} installlibsmbclient installlibtalloc installlibtevent installlibtdb
	BUILD_IN_SOURCE 	1
	DEPENDS 		util-linux
)

ExternalProject_Add(
        nfs
        GIT_REPOSITORY          https://github.com/sahlberg/libnfs.git
        GIT_TAG                 9c1d38488e3bf454ac13c526b5f3c9e1301ef7a7
        UPDATE_COMMAND          ""
        PATCH_COMMAND           ./bootstrap
        CONFIGURE_COMMAND       ${BASIC_CONF} --disable-examples --disable-tirpc
        BUILD_COMMAND           make
        INSTALL_COMMAND         make install
        BUILD_IN_SOURCE         1
)

ExternalProject_Add(
        rtmpdump
        GIT_REPOSITORY          https://github.com/BurntSushi/rtmpdump-ksv.git
        CONFIGURE_COMMAND       ""
        UPDATE_COMMAND          ""
        BUILD_COMMAND           make CRYPTO=OPENSSL DESTDIR=${SYSROOT} LIBS_posix=-L${TARGET_DIR}/lib XLIBS=-ldl INC=-I${TARGET_DIR}/include CROSS_COMPILE=${TARGET}-
        INSTALL_COMMAND         make install DESTDIR=${SYSROOT}
        BUILD_IN_SOURCE         1
        DEPENDS                 zlib openssl
)

ExternalProject_Add(
	curl
	GIT_REPOSITORY          https://github.com/bagder/curl.git
	GIT_TAG                 curl-7_39_0
	PATCH_COMMAND           ./buildconf
	UPDATE_COMMAND          ""
	CONFIGURE_COMMAND 	LDFLAGS=-ldl ${BASIC_CONF} --prefix=${SYSROOT}/usr/local --with-ssl --with-librtmp --disable-debug --enable-optimize --enable-warnings --disable-manual --disable-curldebug --disable-ares --enable-largefile --enable-http --enable-file--disable-ldap --disable-ldaps --disable-rtsp --enable-proxy --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smtp --disable-gophper --enable-ipv6 --enable-versioned-symbols --enable-nonblocking --enable-verbose --disable-sspi --enable-crypto-auth --enable-cookies --enable-hidden-symbols --disable-soname-bump --with-gnu-ld --without-krb4 --without-spnego --without-gssapi --with-zlib --without-egd-socket --without-polarssl --without-nss --without-ca-path --without-libssh2 --without-libidn --enable-threaded-resolver --without-gnutls
	BUILD_COMMAND 		make
	DEPENDS                 openssl zlib bzip2 rtmpdump
)

file(WRITE ${CMAKE_BINARY_DIR}/dbus_configure_helper.sh
"
export CC=${TARGET}-gcc
export CXX=${TARGET}-g++
export CFLAGS=-I${TARGET_DIR}/include
export LDFLAGS=-L${TARGET_DIR}/lib
export LIBS=\"-lpthread -lrt\"
${CMAKE_BINARY_DIR}/dbus-prefix/src/dbus/configure --prefix=${TARGET_DIR} --host=${TARGET} --without-x --disable-xml-docs --disable-doxygen-docs --without-dbus-glib --disable-tests ac_cv_have_abstract_sockets=yes
")
execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/dbus_configure_helper.sh)

ExternalProject_Add(
        dbus
        URL               http://dbus.freedesktop.org/releases/dbus/dbus-1.9.2.tar.gz
        URL_MD5           a568c8853eeced5a4346fa2dd8e9eda6
        CONFIGURE_COMMAND ${CMAKE_BINARY_DIR}/dbus_configure_helper.sh
        BUILD_COMMAND     make
        INSTALL_COMMAND   make install
        BUILD_IN_SOURCE   1
        DEPENDS           expat
)

ExternalProject_Add(
    libffi
    URL                   ftp://sourceware.org/pub/libffi/libffi-3.1.tar.gz
    URL_MD5               f5898b29bbfd70502831a212d9249d10
    CONFIGURE_COMMAND     ${BASIC_CONF} --disable-debug --enable-structs --enable-raw-api --disable-purify-safety --with-gnu-ld
    BUILD_COMMAND         make
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
./configure --prefix=${TARGET_DIR} --host=${TARGET} --enable-shared --disable-ipv6  --build=${HOSTMACHINE} --target=${TARGET} --with-system-expat --with-system-ffi
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
	DEPENDS zlib expat bzip2 gdbm openssl sqlite libffi libxml2
)

ExternalProject_Add(
    libdaemon
    URL                   http://pkgs.fedoraproject.org/repo/pkgs/libdaemon/libdaemon-0.14.tar.gz/509dc27107c21bcd9fbf2f95f5669563/libdaemon-0.14.tar.gz
    URL_MD5               509dc27107c21bcd9fbf2f95f5669563
    CONFIGURE_COMMAND     ac_cv_func_setpgrp_void=no <SOURCE_DIR>/configure --prefix=${TARGET_DIR} --host=${TARGET} --enable-static --disable-shared --disable-lynx
    BUILD_COMMAND         make
)

if(ENABLE_GETTEXT)
ExternalProject_Add(
	gettext
	URL                  http://ftp.gnu.org/pub/gnu/gettext/gettext-0.19.3.tar.gz
	URL_MD5              c365029ffc866fc4e485d9e5ca60b260
	PATCH_COMMAND        patch -p1 -N < ${CMAKE_SOURCE_DIR}/gettext-dont_build_docs_and_examples.patch
	CONFIGURE_COMMAND    ${BASIC_CONF}
	BUILD_COMMAND        make
	DEPENDS iconv
)
endif()

if(ENABLE_GLIB_LINUX)

file(WRITE ${CMAKE_BINARY_DIR}/pre_glib_configure_helper.sh
"
export CFLAGS=-march=i686
export LIBS=\"-lpthread -lz -lffi\"
${CMAKE_BINARY_DIR}/pre_glib-prefix/src/pre_glib/configure --prefix=${TARGET_DIR} --build=${TARGET} CFLAGS=-march=i686 CFLAGS=-I${TARGET_DIR}/include LDFLAGS=-L${TARGET_DIR}/lib LIBFFI_CFLAGS=-I${TARGET_DIR}/lib/libffi-3.1 LIBFFI_LIBS=-L${TARGET_DIR}/lib --enable-shared --with-libiconv=no --enable-debug=no --disable-man --disable-dtrace --disable-systemtap --disable-rebuilds --enable-gtk-doc-html=no --disable-selinux --disable-fam --enable-xattr --enable-Bsymbolic --with-threads=posix --enable-gtk-doc=no 
")
execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/pre_glib_configure_helper.sh)

ExternalProject_Add(
    pre_glib
    URL                   https://github.com/GNOME/glib/archive/2.40.0.tar.gz
    URL_MD5               4e453a219faf86e51f47191a1f8cb910
    PATCH_COMMAND         NOCONFIGURE=yes ./autogen.sh
    CONFIGURE_COMMAND     ${CMAKE_BINARY_DIR}/pre_glib_configure_helper.sh
    BUILD_COMMAND         make
    BUILD_IN_SOURCE       1
    DEPENDS               libffi python pcre zlib libxml2 dbus libdaemon
)

file(WRITE ${CMAKE_BINARY_DIR}/glib_configure_helper.sh
"
export CC=${TARGET}-gcc
export CXX=${TARGET}-g++
export CFLAGS=\"-march=i686 -I${TARGET_DIR}/include -I${SYSROOT}/usr/include\"
export LIBS=\"-lpthread -lrt -ldl -lresolv -lz -lffi\"
export CPPFLAGS=\"-I${TARGET_DIR}/include -I${SYSROOT}/usr/include\"
export LIBFFI_CFLAGS="-I${TARGET_DIR}/lib/libffi-3.1/include\"
export LIBFFI_LIBS="-I${TARGET_DIR}/lib -lffi\"
export ZLIB_CFLAGS="-I${SYSROOT}/usr/include\"
export ZLIB_LIBS="-I${SYSROOT}/usr/lib -lz\"
export PCRE_CFLAGS="-I${TARGET_DIR}/include\"
export PCRE_LIBS="-I${TARGET_DIR}/lib -lpcre\"
${CMAKE_BINARY_DIR}/glib-prefix/src/glib/configure --prefix=${TARGET_DIR} --host=${TARGET} --build=${HOSTMACHINE} --target=${TARGET} CROSS_COMPILE=${TARGET}- LDFLAGS=-L${TARGET_DIR}/lib LIBFFI_CFLAGS=-I${TARGET_DIR}/lib/libffi-3.1/include LIBFFI_LIBS=-L${TARGET_DIR}/lib --enable-shared glib_cv_uscore=yes ac_cv_func_posix_getpwuid_r=yes ac_cv_func_posix_getgrgid_r=yes glib_cv_stack_grows=no glib_cv_rtldglobal_broken=no -with-libiconv=no --enable-debug=no --disable-man --disable-dtrace --disable-systemtap --disable-rebuilds --enable-gtk-doc-html=no --disable-selinux --disable-fam --enable-xattr --enable-Bsymbolic --with-threads=posix --enable-gtk-doc=no
")
execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/glib_configure_helper.sh)

ExternalProject_Add(
    glib
    URL                   https://github.com/GNOME/glib/archive/2.40.0.tar.gz
    URL_MD5               4e453a219faf86e51f47191a1f8cb910
	PATCH_COMMAND         patch -p1 -N < ${CMAKE_SOURCE_DIR}/glib-01-dont-build-tests.patch
          COMMAND         NOCONFIGURE=yes ./autogen.sh
    CONFIGURE_COMMAND     ${CMAKE_BINARY_DIR}/glib_configure_helper.sh
    BUILD_COMMAND         make
    BUILD_IN_SOURCE       1
    DEPENDS               libffi python pcre zlib libxml2 dbus libdaemon pre_glib
)
endif()

if(ENABLE_GLIB_OSX)
file(WRITE ${CMAKE_BINARY_DIR}/glib_configure_helper.sh
"
export CFLAGS=\"-march=i686 -I${TARGET_DIR}/include\"
export LIBS=\"-lpthread -lz -lffi\"
${CMAKE_BINARY_DIR}/glib-prefix/src/glib/configure --prefix=${TARGET_DIR} --host=${TARGET} --build=${HOSTMACHINE} --target=${TARGET} CROSS_COMPILE=${TARGET}- LDFLAGS=-L${TARGET_DIR}/lib LIBFFI_CFLAGS=-I${TARGET_DIR}/lib/libffi-3.1/include LIBFFI_LIBS=-L${TARGET_DIR}/lib --enable-shared glib_cv_uscore=yes ac_cv_func_posix_getpwuid_r=yes ac_cv_func_posix_getgrgid_r=yes glib_cv_stack_grows=no glib_cv_rtldglobal_broken=no -with-libiconv=yes --enable-debug=no --disable-man --disable-dtrace --disable-systemtap --disable-rebuilds --enable-gtk-doc-html=no --disable-selinux --disable-fam --enable-xattr --enable-Bsymbolic --with-threads=posix --enable-gtk-doc=no
")
execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/glib_configure_helper.sh)

ExternalProject_Add(
    glib
    URL                   https://github.com/GNOME/glib/archive/2.40.0.tar.gz
    URL_MD5               4e453a219faf86e51f47191a1f8cb910
    PATCH_COMMAND         NOCONFIGURE=yes ./autogen.sh
    CONFIGURE_COMMAND     ${CMAKE_BINARY_DIR}/glib_configure_helper.sh
    BUILD_COMMAND         make
    BUILD_IN_SOURCE       1
    DEPENDS               libffi python pcre zlib libxml2 iconv gettext
)
endif()

ExternalProject_Add(
    libplist
    URL                   http://www.libimobiledevice.org/downloads/libplist-1.12.tar.bz2
    URL_MD5               8b04b0f09f2398022dcd4fba75012997
    CONFIGURE_COMMAND     ${BASIC_CONF} --without-cython
    BUILD_COMMAND         make CFLAGS=-I${TARGET_DIR}/include LDFLAGS=-L${TARGET_DIR}/lib
    INSTALL_COMMAND       make install
    DEPENDS               libxml2 glib
)

ExternalProject_Add(
    libshairplay
    GIT_REPOSITORY        https://github.com/juhovh/shairplay.git
    PATCH_COMMAND         ./autogen.sh
        COMMAND           patch -p1 < ${CMAKE_SOURCE_DIR}/libshairplay-patches/xcode-llmvfix.patch
        COMMAND           patch -p0 < ${CMAKE_SOURCE_DIR}/libshairplay-patches/01-add_fairplay_handshake.patch
    UPDATE_COMMAND          ""
    CONFIGURE_COMMAND     ${BASIC_CONF} --sysconfdir=/
      BUILD_COMMAND       make
        COMMAND           make install
    INSTALL_COMMAND       cp -P ${CMAKE_BINARY_DIR}/libshairplay-prefix/src/libshairplay/airport.key ${SYSROOT}/usr/local/lib
        DEPENDS           libxml2 libplist
)

file(WRITE ${CMAKE_BINARY_DIR}/avahi_configure_helper.sh
"
export CC=${TARGET}-gcc
export CXX=${TARGET}-g++
export CFLAGS=-I${TARGET_DIR}/include
export LDFLAGS=-L${TARGET_DIR}/lib
export LIBS=\"-lpthread -lrt -lgdbm\"
${CMAKE_BINARY_DIR}/avahi-prefix/src/avahi/configure --prefix=${TARGET_DIR} --host=${TARGET} py_cv_mod_gtk_=yes py_cv_mod_dbus_=yes ac_cv_func_chroot=no --with-distro=none --disable-glib --disable-qt3 --disable-qt4 --disable-gtk --disable-gtk3 --enable-dbus --disable-dbm --disable-gdbm --enable-libdaemon --disable-python --disable-pygtk --disable-python-dbus --disable-mono --disable-monodoc --disable-autoipd --disable-doxygen-doc --disable-doxygen-dot --disable-doxygen-man --disable-doxygen-rtf --disable-doxygen-xml --disable-doxygen-chm --disable-doxygen-chi --disable-doxygen-html --disable-doxygen-ps --disable-doxygen-pdf --disable-core-docs --disable-manpages --disable-xmltoman --disable-tests --disable-compat-libdns_sd --disable-compat-howl --with-xml=expat --with-avahi-user=avahi --with-avahi-group=avahi --disable-gobject --disable-nls
")
execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/avahi_configure_helper.sh)

ExternalProject_Add(
    avahi
    URL                   http://avahi.org/download/avahi-0.6.31.tar.gz
    URL_MD5               2f22745b8f7368ad5a0a3fddac343f2d
    CONFIGURE_COMMAND     ${CMAKE_BINARY_DIR}/avahi_configure_helper.sh
    BUILD_COMMAND         make
    BUILD_IN_SOURCE       1
    DEPENDS               libdaemon libffi glib dbus expat
)

ExternalProject_Add(
	libenca
	URL                 https://github.com/nijel/enca/archive/1.16.tar.gz
	URL_MD5             4f39869a757e96deb372884e6f474a27
	PATCH_COMMAND       sed -i -e "s,.(AM_V_CC).(COMPILE),cc," "tools/Makefile.in"
	COMMAND             sed -i -e "s,.(AM_V_CCLD).(LINK),cc -o \$@," "tools/Makefile.in" VERBATIM
	CONFIGURE_COMMAND   ac_cv_file__dev_random=yes ac_cv_file__dev_urandom=no ac_cv_file__dev_srandom=no ac_cv_file__dev_arandom=no ${BASIC_CONF} --disable-external --without-librecode --disable-rpath --with-gnu-ld
	BUILD_COMMAND       make
	BUILD_IN_SOURCE     1
	DEPENDS             avahi
)

ExternalProject_Add(
	libass
	GIT_REPOSITORY   	https://github.com/libass/libass.git
	GIT_TAG                 0.12.0
	UPDATE_COMMAND           ""
	PATCH_COMMAND           ./autogen.sh
	CONFIGURE_COMMAND 	${BASIC_CONF} --with-gnu-ld --enable-enca --enable-fontconfig --disable-harfbuzz --disable-silent-rules --enable-asm --disable-test
	DEPENDS libpng libenca fribidi freetype2 fontconfig yasm
)

ExternalProject_Add(
	boost
	URL			http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz/download
	URL_MD5			93780777cfbf999a600f62883bd54b17
	PATCH_COMMAND 		${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/boost.user-config.jam ./tools/build/v2/user-config.jam
	CONFIGURE_COMMAND 	./bootstrap.sh --prefix=${TARGET_DIR}
	BUILD_COMMAND 		""
	INSTALL_COMMAND 	./b2 include=${TARGET_DIR}/include linkflags="-L${TARGET_DIR}/lib" toolset=gcc-i686 link=static target-os=linux --without-mpi --without-context --without-log --without-python --without-coroutine ${PARALLEL} install
	BUILD_IN_SOURCE 	1
	DEPENDS			zlib bzip2
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

ExternalProject_Add(
	gmp
	URL 				https://gmplib.org/download/gmp/gmp-6.0.0a.tar.bz2
	URL_MD5				b7ff2d88cae7f8085bd5006096eed470
	CONFIGURE_COMMAND 	<SOURCE_DIR>/configure --prefix=${TARGET_DIR} --host=${TARGET} --build=${HOSTMACHINE} --target=${TARGET} CROSS_COMPILE=${TARGET}- LDFLAGS=-L${TARGET_DIR}/lib --enable-shared --enable-cxx --enable-cxx ABI=32
	BUILD_COMMAND 		make ${PARALLEL}
	INSTALL_COMMAND     make install
)

file(WRITE ${CMAKE_BINARY_DIR}/nettle_configure_helper.sh
"
export CFLAGS=\"-march=i686 -I${TARGET_DIR}/include\"
export CPPFLAGS=-I${TARGET_DIR}/include
export LDFLAGS=-L${TARGET_DIR}/lib
${CMAKE_BINARY_DIR}/nettle-prefix/src/nettle/configure --prefix=${TARGET_DIR} --host=${TARGET} --build=${HOSTMACHINE} --target=${TARGET} CROSS_COMPILE=${TARGET}- LDFLAGS=-L${TARGET_DIR}/lib --enable-shared --disable-openssl
")
execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/nettle_configure_helper.sh)

ExternalProject_Add(
    nettle
    URL                   http://ftp.gnu.org/gnu/nettle/nettle-2.7.1.tar.gz
    URL_MD5               003d5147911317931dd453520eb234a5
    CONFIGURE_COMMAND     ${CMAKE_BINARY_DIR}/nettle_configure_helper.sh
    BUILD_COMMAND         make
    BUILD_IN_SOURCE       1
    DEPENDS               openssl gmp
)

ExternalProject_Add(
    gnutls
    URL                   ftp://ftp.gnutls.org/gcrypt/gnutls/v3.3/gnutls-3.3.10.tar.xz
    URL_MD5               c0a72b2c0553fe1c4992e30835808012
	PATCH_COMMAND         patch -p0 < ${CMAKE_SOURCE_DIR}/size-max.patch   
    CONFIGURE_COMMAND     <SOURCE_DIR>/configure --prefix=${TARGET_DIR} --host=${TARGET} --build=${HOSTMACHINE} --target=${TARGET} CROSS_COMPILE=${TARGET}- LDFLAGS=-L${TARGET_DIR}/lib --enable-shared --without-p11-kit --disable-nls --enable-local-libopts GMP_CFLAGS=${TARGET_DIR}/include
    BUILD_COMMAND         make
    BUILD_IN_SOURCE       1
    DEPENDS               nettle
)

ExternalProject_Add(
	ffmpeg
	GIT_REPOSITORY           https://github.com/xbmc/FFmpeg.git
	GIT_TAG                  2.4.6-Helix 
	PATCH_COMMAND            sed -i".bak" -e "s%pkg_config_default=pkg-config%export PKG_CONFIG_LIBDIR=${TARGET_DIR}/lib/pkgconfig:${SYSROOT}/usr/lib/pkgconfig%" configure
	CONFIGURE_COMMAND        <SOURCE_DIR>/configure --prefix=${TARGET_DIR} --cross-prefix=${TARGET}- --enable-cross-compile --target-os=linux --sysroot=${SYSROOT} --extra-ldflags=-L${TARGET_DIR}/lib --extra-cflags=-L${TARGET_DIR}/include --arch=i686 --cpu=i686 --enable-shared --disable-static --sysinclude={TARGET_DIR}/include --disable-programs --enable-gpl --disable-devices --disable-doc --disable-ffplay --disable-ffmpeg --disable-ffprobe --disable-ffserver --enable-runtime-cpudetect --enable-postproc --enable-pthreads --enable-muxer=spdif --enable-muxer=adts --enable-muxer=asf --enable-muxer=mpegts --enable-muxer=ipod --enable-encoder=ac3 --enable-encoder=aac --enable-encoder=wmav2 --enable-protocol=http --disable-vaapi --disable-vdpau --enable-nonfree --enable-asm --enable-libvorbis --enable-muxer=ogg --enable-encoder=libvorbis --enable-bzlib --enable-optimizations --enable-yasm --enable-gnutls --enable-zlib --extra-libs=-lrt --extra-libs=-lpthread --extra-libs=-lgnutls --extra-libs=-lnettle --extra-libs=-lhogweed --extra-libs=-lgmp --extra-libs=-lz --incdir=${TARGET}/include --libdir=${TARGET}/lib --pkg-config=pkg-config
	UPDATE_COMMAND           ""
	BUILD_COMMAND            make ${PARALLEL}
	INSTALL_COMMAND          make install
	COMMAND                  ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_BINARY_DIR}/ffmpeg-prefix/src/ffmpeg-build/i686-pc-linux-gnu/include ${TARGET_DIR}/include
	COMMAND                  ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_BINARY_DIR}/ffmpeg-prefix/src/ffmpeg-build/i686-pc-linux-gnu/lib/pkgconfig ${TARGET_DIR}/lib/pkgconfig
	DEPENDS                  yasm gnutls zlib bzip2 libvorbis nettle
)
