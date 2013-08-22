cmake_minimum_required(VERSION 2.8)

set(TARGET "i686-pc-linux-gnu")
set(TARGET_DIR "${CMAKE_BINARY_DIR}/target/usr")

set(BINUTILS_VERSION "2.17" CACHE "Version of binutils to build" STRING)
set(GCC_VERSION "4.2.1" CACHE "Version of GCC to build" STRING)

set(PARALLEL -j8)

include(ExternalProject)


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
	URL_MD5 			1d81edd0569de4d84091526fd578dd7b
	CONFIGURE_COMMAND	<SOURCE_DIR>/configure --prefix=${TARGET_DIR} --target ${TARGET} --disable-nls --disable-werror
	BUILD_COMMAND		make ${PARALLEL}
	DEPENDS 			kernel
)

ExternalProject_Add(
	boot-gcc
	PREFIX 				${CMAKE_BINARY_DIR}/gcc
	URL 				ftp://ftp.mirrorservice.org/sites/sourceware.org/pub/gcc/releases/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.bz2
	URL_MD5 			cba410e6ff70f7d7f4be7a0267707fd0
	CONFIGURE_COMMAND	<SOURCE_DIR>/configure --target=${TARGET} --prefix=${TARGET_DIR} --disable-nls --without-headers --with-newlib --enable-languages=c --disable-decimal-float --disable-shared --disable-threads --disable-libmudflap --disable-libssp --disable-libgomp  #--disable-libquadmath  --disable-libatomic
	BUILD_COMMAND		make ${PARALLEL}
	INSTALL_COMMAND make install
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
	CONFIGURE_COMMAND	CC=${TARGET_DIR}/bin/${TARGET}-gcc <SOURCE_DIR>/configure --target=${TARGET} --host=${TARGET} --disable-nls --enable-kernel=2.6.28 --prefix=/usr --with-headers=${TARGET_DIR}/include libc_cv_forced_unwind=yes libc_cv_c_cleanup=yes libc_cv_ctors_header=yes CFLAGS=-O2\ -march=i686
	INSTALL_COMMAND		make install_root=${TARGET_DIR} prefix="" install
	DEPENDS boot-gcc
)

ExternalProject_Add(
	binutils2
	PREFIX 				${CMAKE_BINARY_DIR}/binutils
	URL 				http://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.bz2
	URL_MD5 			1d81edd0569de4d84091526fd578dd7b
	CONFIGURE_COMMAND	<SOURCE_DIR>/configure --prefix=${TARGET_DIR} --target ${TARGET} --disable-nls --disable-werror
	BUILD_COMMAND		make ${PARALLEL}
	DEPENDS 			glibc
)


ExternalProject_Add(
	gcc2
	PREFIX 				${CMAKE_BINARY_DIR}/gcc
	URL 				ftp://ftp.mirrorservice.org/sites/sourceware.org/pub/gcc/releases/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.bz2
	URL_MD5 			cba410e6ff70f7d7f4be7a0267707fd0
	CONFIGURE_COMMAND	<SOURCE_DIR>/configure --target=${TARGET} --prefix=${TARGET_DIR} --enable-shared --disable-threads --enable-languages=c,c++ --disable-nls --disable-multilib --disable-libgomp --disable-bootstrap --with-headers=${TARGET_DIR}/include
	BUILD_COMMAND 		make ${PARALLEL}
	DEPENDS 			binutils2
)

# function(dl URL)
# 	string(REGEX REPLACE ".*/([^/]+)" "sources/\\1" SRC ${URL})
# 	string(REGEX REPLACE ".*/([^/]+)(\\.tar\\.gz|\\.tar\\.bz2)" "sources/\\1" EXTRACTED ${URL})
# 	if(ARGV1)
# 		set(TARGET ${ARGV1})
# 	else()
# 		set(TARGET ${EXTRACTED})
# 	endif()
# 	if(NOT EXISTS ${CMAKE_BINARY_DIR}/${SRC})
# 		message("Fetching ${URL} -> ${SRC}")
# 		file(DOWNLOAD "${URL}" ${CMAKE_BINARY_DIR}/${SRC} SHOW_PROGRESS)
# 	endif()
# 	if(NOT EXISTS ${CMAKE_BINARY_DIR}/${TARGET})
# 		message("Extracting ${SRC}")
# 		execute_process(
# 			COMMAND ${CMAKE_COMMAND} -E tar xfz ${CMAKE_BINARY_DIR}/${SRC}
# 			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/sources
# 		)
# 		if(ARGV1)
# 			execute_process(
# 				COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_BINARY_DIR}/${EXTRACTED} ${CMAKE_BINARY_DIR}/${TARGET}
# 			)
# 		endif()
# 	endif()
# endfunction()

# dl(http://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.bz2)
# dl(ftp://ftp.mirrorservice.org/sites/sourceware.org/pub/gcc/releases/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.bz2)
# dl(http://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.14.tar.gz sources/gcc-${GCC_VERSION}/libiconv)
# dl(http://multiprecision.org/mpc/download/mpc-1.0.1.tar.gz sources/gcc-${GCC_VERSION}/mpc)
# dl(http://www.mpfr.org/mpfr-current/mpfr-3.1.2.tar.bz2 sources/gcc-${GCC_VERSION}/mpfr)
# dl(ftp://ftp.gmplib.org/pub/gmp-5.1.2/gmp-5.1.2.tar.bz2 sources/gcc-${GCC_VERSION}/gmp)
# dl(ftp://ftp.kernel.org/pub/linux/kernel/v2.6/linux-2.6.28.8.tar.bz2)
# dl(http://ftp.gnu.org/gnu/glibc/glibc-2.7.tar.bz2)

# file(MAKE_DIRECTORY ${BUILD}/binutils)
# file(MAKE_DIRECTORY ${BUILD}/binutils2)
# file(MAKE_DIRECTORY ${BUILD}/glibc)
# file(MAKE_DIRECTORY ${BUILD}/full-gcc)
# file(MAKE_DIRECTORY ${BUILD}/boot-gcc)

# add_custom_command(OUTPUT ${KERNEL}/.config
# 	COMMAND make ARCH=i386 CROSS_COMPILE=i686-linux-elf- menuconfig
# 	WORKING_DIRECTORY ${KERNEL}
# 	COMMENT "Just exit the config, the defaults are fine"
# )

# add_custom_command(OUTPUT ${KERNEL}/include/linux/version.h
# 	COMMAND make ARCH=i386 CROSS_COMPILE=i686-linux-elf- include/linux/version.h
# 	WORKING_DIRECTORY ${KERNEL}
# 	DEPENDS ${KERNEL}/.config
# )

# add_custom_command(OUTPUT ${PREFIX}/include/linux
# 	COMMAND ${CMAKE_COMMAND} -E copy_directory ${KERNEL}/include/linux ${PREFIX}/include/linux
# 	COMMAND ${CMAKE_COMMAND} -E copy_directory ${KERNEL}/arch/x86/include/asm ${PREFIX}/include/asm
# 	COMMAND ${CMAKE_COMMAND} -E copy_directory ${KERNEL}/include/asm-generic ${PREFIX}/include/asm-generic
# 	DEPENDS ${KERNEL}/include/linux/version.h
# )

# add_custom_command(OUTPUT ${BUILD}/binutils/Makefile
# 	COMMAND ${SOURCE}/binutils-${BINUTILS_VERSION}/configure --target=${TARGET} --prefix="${PREFIX}" --disable-nls --disable-werror
# 	WORKING_DIRECTORY ${BUILD}/binutils
# 	DEPENDS ${PREFIX}/include/linux
# )

# add_custom_command(OUTPUT ${PREFIX}/bin/${TARGET}-ld
# 	COMMAND make -j8
# 	COMMAND make install
# 	WORKING_DIRECTORY ${BUILD}/binutils
# 	DEPENDS ${BUILD}/binutils/Makefile
# )

# add_custom_command(OUTPUT ${BUILD}/binutils2/Makefile
# 	COMMAND ${SOURCE}/binutils-${BINUTILS_VERSION}/configure --target=${TARGET} --prefix="${PREFIX}" --disable-nls --disable-werror --with-lib-path=${PREFIX}/usr/lib
# 	WORKING_DIRECTORY ${BUILD}/binutils2
# 	DEPENDS ${PREFIX}/usr/lib/libc.so
# )

# add_custom_command(OUTPUT ${BUILD}/binutils2/ld/ld-new
# 	COMMAND make -j8
# 	COMMAND make install
# 	WORKING_DIRECTORY ${BUILD}/binutils2
# 	DEPENDS ${BUILD}/binutils2/Makefile
# )


# add_custom_command(OUTPUT ${BUILD}/boot-gcc/Makefile
# 	COMMAND ${SOURCE}/gcc-${GCC_VERSION}/configure --target=${TARGET} --prefix="${PREFIX}" --disable-nls --without-headers --with-newlib --enable-languages=c --disable-decimal-float --disable-shared --disable-threads --disable-libmudflap --disable-libssp --disable-libgomp  #--disable-libquadmath  --disable-libatomic
# 	WORKING_DIRECTORY ${BUILD}/boot-gcc
# 	DEPENDS ${PREFIX}/bin/${TARGET}-ld
# )

# add_custom_command(OUTPUT ${PREFIX}/bin/${TARGET}-gcc
# 	COMMAND make -j8 
# 	COMMAND make install
# 	WORKING_DIRECTORY ${BUILD}/boot-gcc
# 	DEPENDS ${BUILD}/boot-gcc/Makefile
# )

# add_custom_command(OUTPUT ${PREFIX}/lib/gcc/${TARGET}/${GCC_VERSION}/libgcc_eh.a
# 	COMMAND ${CMAKE_COMMAND} -E create_symlink ${PREFIX}/lib/gcc/${TARGET}/${GCC_VERSION}/libgcc.a `${PREFIX}/bin/${TARGET}-gcc -print-libgcc-file-name | sed 's/libgcc/&_eh/'`
# 	DEPENDS ${PREFIX}/bin/${TARGET}-gcc
# )

# add_custom_command(OUTPUT ${SOURCE}/glibc-2.7/configure.bak
# 	COMMAND ${CMAKE_COMMAND} -E copy ${SOURCE}/glibc-2.7/configure ${SOURCE}/glibc-2.7/configure.bak
# 	DEPENDS ${PREFIX}/lib/gcc/${TARGET}/${GCC_VERSION}/libgcc_eh.a
# )

# add_custom_command(OUTPUT ${SOURCE}/glibc-2.7/configure
# 	COMMAND cat ${SOURCE}/glibc-2.7/configure.bak | sed s,\ readelf\ ,\ ${PREFIX}/bin/${TARGET}-readelf\ ,g > ${SOURCE}/glibc-2.7/configure
# 	DEPENDS ${SOURCE}/glibc-2.7/configure.bak
# )

# add_custom_command(OUTPUT ${SOURCE}/glibc-2.7/sysdeps/i386/configure.bak
# 	COMMAND ${CMAKE_COMMAND} -E copy ${SOURCE}/glibc-2.7/sysdeps/i386/configure ${SOURCE}/glibc-2.7/sysdeps/i386/configure.bak
# )

# add_custom_command(OUTPUT ${SOURCE}/glibc-2.7/sysdeps/i386/configure
# 	COMMAND cat ${SOURCE}/glibc-2.7/sysdeps/i386/configure.bak | sed s,\ readelf\ ,\ ${PREFIX}/bin/${TARGET}-readelf\ ,g > ${SOURCE}/glibc-2.7/sysdeps/i386/configure
# 	DEPENDS ${SOURCE}/glibc-2.7/sysdeps/i386/configure.bak
# )

# add_custom_command(OUTPUT ${SOURCE}/glibc-2.7/elf/Makefile.bak
# 	COMMAND ${CMAKE_COMMAND} -E copy ${SOURCE}/glibc-2.7/elf/Makefile ${SOURCE}/glibc-2.7/elf/Makefile.bak
# )

# add_custom_command(OUTPUT ${SOURCE}/glibc-2.7/elf/Makefile
# 	COMMAND cat ${SOURCE}/glibc-2.7/elf/Makefile.bak | sed s,\ readelf\ ,\ ${PREFIX}/bin/${TARGET}-readelf\ ,g > ${SOURCE}/glibc-2.7/elf/Makefile
# 	DEPENDS ${SOURCE}/glibc-2.7/elf/Makefile.bak
# )


# add_custom_command(OUTPUT ${BUILD}/glibc/Makefile
# 	COMMAND CC=${PREFIX}/bin/${TARGET}-gcc ${SOURCE}/glibc-2.7/configure --target=${TARGET} --host=${TARGET} --build=`${SOURCE}/glibc-2.7/scripts/config.guess`  --disable-nls --enable-kernel=2.6.28 --prefix=/usr --with-headers=${PREFIX}/include libc_cv_forced_unwind=yes libc_cv_c_cleanup=yes libc_cv_ctors_header=yes CFLAGS=-O2\ -march=i686
# 	WORKING_DIRECTORY ${BUILD}/glibc
# 	DEPENDS ${SOURCE}/glibc-2.7/configure
# 	DEPENDS ${SOURCE}/glibc-2.7/sysdeps/i386/configure
# 	DEPENDS ${SOURCE}/glibc-2.7/elf/Makefile
# )

# add_custom_command(OUTPUT ${PREFIX}/usr/lib/libc.so
# 	COMMAND make -j8
# 	COMMAND make install_root=${PREFIX}/usr prefix="" install
# 	WORKING_DIRECTORY ${BUILD}/glibc
# 	DEPENDS ${BUILD}/glibc/Makefile
# )

# add_custom_command(OUTPUT ${BUILD}/full-gcc/Makefile
# 	COMMAND ${SOURCE}/gcc-${GCC_VERSION}/configure --target=${TARGET} --prefix="${PREFIX}" --enable-shared --disable-threads --enable-languages=c --disable-nls --disable-multilib --disable-libgomp --disable-bootstrap 
# 	WORKING_DIRECTORY ${BUILD}/full-gcc
# 	DEPENDS ${PREFIX}/usr/lib/libc.so
# 	DEPENDS ${BUILD}/binutils2/ld/ld-new
# )

# add_custom_command(OUTPUT ${PREFIX}/bin/${TARGET}-g++
# 	COMMAND make -j8 
# 	COMMAND make install
# 	WORKING_DIRECTORY ${BUILD}/full-gcc
# 	DEPENDS ${BUILD}/full-gcc/Makefile
# )

# add_custom_target(build ALL DEPENDS ${PREFIX}/bin/${TARGET}-g++)
