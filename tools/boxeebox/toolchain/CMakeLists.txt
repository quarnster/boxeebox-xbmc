cmake_minimum_required(VERSION 2.8)

set(TARGET "i686-pc-linux-gnu")
set(TARGET_DIR "${SYSROOT}/usr")

set(BINUTILS_VERSION "2.18" CACHE STRING "Version of binutils to build")
set(GCC_VERSION "4.2.1" CACHE STRING "Version of GCC to build")
set(ENABLE_GDB OFF CACHE BOOL "Wether to build gdb or not")

include(ExternalProject)

execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine OUTPUT_VARIABLE HOSTMACHINE)
string(REPLACE "\n" "" HOSTMACHINE ${HOSTMACHINE})

ExternalProject_Add(
	kernel
	URL					ftp://ftp.kernel.org/pub/linux/kernel/v2.6/linux-2.6.28.8.tar.bz2
	URL_MD5				8440a90637cff154195895743ef498a1
	PATCH_COMMAND		cp ${CMAKE_SOURCE_DIR}/kernel.config .config
	CONFIGURE_COMMAND	""
	BUILD_COMMAND		make ARCH=i386 CROSS_COMPILE=i686-linux-elf- include/linux/version.h
	INSTALL_COMMAND		${CMAKE_COMMAND} -E copy_directory include/linux ${TARGET_DIR}/include/linux
		COMMAND			${CMAKE_COMMAND} -E copy_directory arch/x86/include/asm ${TARGET_DIR}/include/asm
		COMMAND			${CMAKE_COMMAND} -E copy_directory include/asm-generic ${TARGET_DIR}/include/asm-generic
	BUILD_IN_SOURCE		1
)

ExternalProject_Add(
	boot-binutils
	PREFIX 				${CMAKE_BINARY_DIR}/binutils
	URL 				http://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.bz2
	URL_MD5 			ccd264a5fa9ed992a21427c69cba91d3
	PATCH_COMMAND 		sed -e "6128,6135d" -i.bak configure # prevents binutls from failing due to newer makeinfo
	CONFIGURE_COMMAND	 <SOURCE_DIR>/configure --with-sysroot=${SYSROOT} --with-lib-path=${TARGET_DIR}/lib --prefix=${TARGET_DIR} --target ${TARGET} --disable-nls --disable-werror --program-prefix=${TARGET}-
	BUILD_COMMAND		make ${PARALLEL}
	DEPENDS 			kernel
)

ExternalProject_Add(
	boot-gcc
	PREFIX 				${CMAKE_BINARY_DIR}/gcc
	URL 				ftp://ftp.mirrorservice.org/sites/sourceware.org/pub/gcc/releases/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.bz2
	URL_MD5 			cba410e6ff70f7d7f4be7a0267707fd0
	CONFIGURE_COMMAND	<SOURCE_DIR>/configure --with-sysroot=${SYSROOT} --target=${TARGET} --prefix=${TARGET_DIR} --disable-nls --without-headers --with-newlib --enable-languages=c --disable-decimal-float --disable-shared --disable-threads --disable-libmudflap --disable-libssp --disable-libgomp --with-local-prefix=${TARGET_DIR} --with-native-system-header-dir=${TARGET_DIR}/include --enable-bootstrap  #--disable-libquadmath  --disable-libatomic
	BUILD_COMMAND		make all-gcc ${PARALLEL}
	INSTALL_COMMAND make install-gcc
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${TARGET_DIR}/lib/gcc/${TARGET}/${GCC_VERSION}/libgcc.a  ${TARGET_DIR}/lib/gcc/${TARGET}/${GCC_VERSION}/libgcc_eh.a
	DEPENDS 			boot-binutils
)

ExternalProject_Add(
	glibc
	URL 				http://ftp.gnu.org/gnu/glibc/glibc-2.7.tar.bz2
	URL_MD5 			065c5952b439deba40083ccd67bcc8f7
	PATCH_COMMAND		cat elf/Makefile | sed s,\ readelf\ ,\ ${PREFIX}/bin/${TARGET}-readelf\ ,g > elf/Makefile.new
		COMMAND 		${CMAKE_COMMAND} -E copy elf/Makefile.new elf/Makefile
		COMMAND 		cat configure | sed s,\ readelf\ ,\ ${TARGET_DIR}/bin/${TARGET}-readelf\ ,g > configure.new
		COMMAND 		${CMAKE_COMMAND} -E copy configure.new configure
		COMMAND 		chmod +x configure
	BUILD_COMMAND		make ${PARALLEL}
	CONFIGURE_COMMAND	CC=${TARGET}-gcc <SOURCE_DIR>/configure --build=${HOSTMACHINE} --target=${TARGET} --host=${TARGET} --disable-nls --enable-kernel=2.6.28 --prefix=/usr --with-headers=${TARGET_DIR}/include libc_cv_forced_unwind=yes libc_cv_c_cleanup=yes libc_cv_ctors_header=yes CFLAGS=-O2\ -march=i686 --program-prefix=${TARGET}-
	INSTALL_COMMAND		make ${PARALLEL} install_root=${SYSROOT} install
	DEPENDS boot-gcc
)

ExternalProject_Add(
	gcc2
	PREFIX 				${CMAKE_BINARY_DIR}/gcc
	URL 				ftp://ftp.mirrorservice.org/sites/sourceware.org/pub/gcc/releases/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.bz2
	URL_MD5 			cba410e6ff70f7d7f4be7a0267707fd0
	CONFIGURE_COMMAND	ac_cv_header_sys_filio_h=no <SOURCE_DIR>/configure --with-sysroot=${SYSROOT} --target=${TARGET} --prefix=${TARGET_DIR} --disable-libstdcxx-pch --enable-clocale=gnu --enable-shared --enable-threads --enable-languages=c,c++ --disable-nls --disable-multilib --disable-libgomp --disable-libmudflap --disable-libssp --disable-bootstrap --with-local-prefix=${TARGET_DIR} --with-native-system-header-dir=${TARGET_DIR}/include --program-prefix=${TARGET}-
	BUILD_COMMAND 		ac_cv_header_sys_filio_h=no make ${PARALLEL}
	DEPENDS 			glibc
)

if(ENABLE_GDB)
ExternalProject_Add(
	gdb
	PREFIX 				${CMAKE_BINARY_DIR}/gdb
	URL 				http://ftp.gnu.org/gnu/gdb/gdb-6.8a.tar.bz2
	URL_MD5				da20d043e6729f74b909bd507bcae5c9
	CONFIGURE_COMMAND 	<SOURCE_DIR>/configure --with-sysroot=${SYSROOT} --target=${TARGET} --prefix=${TARGET_DIR} --program-prefix=${TARGET}- --disable-werror
	BUILD_COMMAND 		make ${PARALLEL}
	DEPENDS 			gcc2
)

ExternalProject_Add(
	termcap
	URL 				ftp://ftp.gnu.org/gnu/termcap/termcap-1.3.1.tar.gz
	URL_MD5 			ffe6f86e63a3a29fa53ac645faaabdfa
	CONFIGURE_COMMAND 	CC=${TARGET}-gcc RANLIB=${TARGET}-ranlib <SOURCE_DIR>/configure --target=${TARGET} --prefix=${SYSROOT}/usr/local/
	BUILD_COMMAND 		make ${PARALLEL}
	DEPENDS 			gcc2
)

ExternalProject_Add(
	targetgdb
	PREFIX 				${CMAKE_BINARY_DIR}/gdb
	URL 				http://ftp.gnu.org/gnu/gdb/gdb-6.8a.tar.bz2
	URL_MD5				da20d043e6729f74b909bd507bcae5c9
	CONFIGURE_COMMAND 	LDFLAGS=-L${SYSROOT}/usr/local/lib <SOURCE_DIR>/configure --host=${TARGET} --prefix=${SYSROOT}/usr/local/ --disable-werror
	BUILD_COMMAND 		make ${PARALLEL}
	DEPENDS 			gcc2 termcap
)
endif()

file(WRITE "${SYSROOT}/usr/bin/${TARGET}-pkg-config" "
#!/bin/bash

set -e
export PKG_CONFIG_DIR=
export PKG_CONFIG_LIBDIR=${SYSROOT}/usr/lib/pkgconfig:${SYSROOT}/usr/share/pkgconfig:${SYSROOT}/usr/local/lib/pkgconfig:${SYSROOT}/usr/local/share/pkgconfig
export PKG_CONFIG_SYSROOT_DIR=${SYSROOT}

exec pkg-config \"\$@\" | python -c \"import sys; print sys.stdin.read().replace('${SYSROOT}${SYSROOT}', '${SYSROOT}'),\" && test \${PIPESTATUS[0]} -eq 0
")
execute_process(COMMAND chmod +x ${SYSROOT}/usr/bin/${TARGET}-pkg-config)
