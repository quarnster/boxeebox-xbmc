cmake_minimum_required(VERSION 2.8)

find_package(PythonInterp)

set(ENABLE_TESTS OFF CACHE BOOL "Whether to build the tests or not")


execute_process(COMMAND ls -1 ${CMAKE_SOURCE_DIR}/headers OUTPUT_VARIABLE headers)
string(REGEX REPLACE "\n" ";" headers ${headers})

foreach(header ${headers})
	if(${header} STREQUAL "os")
		add_custom_target(
			${header}
			COMMAND 		${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/headers/${header} ${SYSROOT}/usr/local/include/${header} 
		)
	else()
		add_custom_target(
			${header}
			COMMAND 		${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/headers/${header} ${SYSROOT}/usr/local/include/${header} 
		)
	endif()
endforeach()


set(ismds
	ismd_core
	ismd_viddec
	ismd_vidpproc
	ismd_vidrend
	ismd_audio
	ismd_demux
)
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/libgdl.so
	COMMAND cat ${CMAKE_SOURCE_DIR}/headers/gdl.h | ${PYTHON_EXECUTABLE} -c \"import sys, re\; print '\#include <gdl.h>\\n%s' % '\\n'.join(filter(lambda a: not '\\ngdl_init' in a, ['%s {}' %x for x in re.compile(r'^\(GDL_API\\s[^\;]+\)\;', re.M).findall(sys.stdin.read())]))\" > ${CMAKE_BINARY_DIR}/libgdl.c
	COMMAND i686-pc-linux-gnu-gcc -I${CMAKE_SOURCE_DIR}/headers -shared -o ${CMAKE_BINARY_DIR}/libgdl.so ${CMAKE_BINARY_DIR}/libgdl.c
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/libgdl.so ${SYSROOT}/usr/local/lib/libgdl.so
)

foreach(ismd ${ismds})
	add_custom_command(
		OUTPUT ${ismd}.so
		COMMAND cat "${CMAKE_SOURCE_DIR}/headers/${ismd}.h" | ${PYTHON_EXECUTABLE} -c \"import sys, re\; print '\#include <ismd_core.h>\\n\#include <ismd_audio.h>\\n\#include <ismd_viddec.h>\\n\#include <ismd_vidpproc.h>\\n\#include <ismd_demux.h>\\n\#include <ismd_vidrend.h>\\n\#include <ismd_tsout.h>\\n%s' % '\\n'.join(filter(lambda a: not '  result  ' in a, ['%s {}' %x for x in re.compile(r'^\(\\s*ismd_result_t\\s[^\;]+\)\;', re.M).findall(sys.stdin.read())]))\" > ${CMAKE_BINARY_DIR}/lib${ismd}.c
		COMMAND i686-pc-linux-gnu-gcc -I${CMAKE_SOURCE_DIR}/headers -shared -o ${CMAKE_BINARY_DIR}/lib${ismd}.so ${CMAKE_BINARY_DIR}/lib${ismd}.c
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/lib${ismd}.so ${SYSROOT}/usr/local/lib/lib${ismd}.so
	)
endforeach()

add_custom_command(OUTPUT OUTPUT ${CMAKE_BINARY_DIR}/libosal.so
	COMMAND i686-pc-linux-gnu-gcc -I${CMAKE_SOURCE_DIR}/headers -shared -o ${CMAKE_BINARY_DIR}/libosal.so ${CMAKE_SOURCE_DIR}/libosal.c
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/libosal.so ${SYSROOT}/usr/local/lib/libosal.so
)

add_custom_command(OUTPUT OUTPUT ${CMAKE_BINARY_DIR}/libplatform_config.so
	COMMAND i686-pc-linux-gnu-gcc -I${CMAKE_SOURCE_DIR}/headers -shared -o ${CMAKE_BINARY_DIR}/libplatform_config.so ${CMAKE_SOURCE_DIR}/libplatform_config.c
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/libplatform_config.so ${SYSROOT}/usr/local/lib/libplatform_config.so
)

string(REGEX REPLACE "(.)(;|$)" "\\1.so\\2" ismds_so "${ismds}")
add_custom_target(install ALL
	DEPENDS ${headers}
			libgdl.so
			${ismds_so}
			libosal.so
			libplatform_config.so
)


if(ENABLE_TESTS) 
	set(CGO_CFLAGS "-I/Volumes/Skivavbild/boxee-xbmc/sysroot/usr/local/include/")
	set(CGO_LDFLAGS "-L/Volumes/Skivavbild/boxee-xbmc/sysroot/usr/local/lib -lismd_audio -lismd_core -losal -lm -lgdl")


	macro(GoTest name)
		set(TESTS ${TESTS} ${name})
		add_custom_target(
			${name}
			COMMAND CC=${SYSROOT}/usr/bin/i686-pc-linux-gnu-gcc CGO_CFLAGS=${CGO_CFLAGS} CGO_LDFLAGS=${CGO_LDFLAGS} CGO_ENABLED=1 GOOS=linux GOARCH=386 go build -ldflags="-extld=${SYSROOT}/usr/bin/i686-pc-linux-gnu-gcc" ${CMAKE_SOURCE_DIR}/tests/${name}.go
			COMMAND ftp -u boxeebox:/tmp/${name} ${name}
			DEPENDS ${CMAKE_SOURCE_DIR}/tests/${name}.go
		)
	endmacro()

	GoTest(audio)

endif()

