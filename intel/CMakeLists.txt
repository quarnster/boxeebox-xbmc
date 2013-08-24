cmake_minimum_required(VERSION 2.8)

find_package(PythonInterp)

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


add_custom_command(OUTPUT ${SYSROOT}/usr/local/lib/libgdl.so
	COMMAND cat ${CMAKE_SOURCE_DIR}/headers/gdl.h | ${PYTHON_EXECUTABLE} -c \"import sys, re\; print '\#include <gdl.h>\\n%s' % '\\n'.join(filter(lambda a: not '\\ngdl_init' in a, ['%s {}' %x for x in re.compile(r'^\(GDL_API\\s[^\;]+\)\;', re.M).findall(sys.stdin.read())]))\" > ${CMAKE_BINARY_DIR}/libgdl.c
	COMMAND i686-pc-linux-gnu-gcc -I${CMAKE_SOURCE_DIR}/headers -shared -o ${SYSROOT}/usr/local/lib/libgdl.so ${CMAKE_BINARY_DIR}/libgdl.c
)

add_custom_command(OUTPUT ${SYSROOT}/usr/local/lib/libismd_core.so
	COMMAND cat "${CMAKE_SOURCE_DIR}/headers/ismd*.h" | ${PYTHON_EXECUTABLE} -c \"import sys, re\; print '\#include <ismd_core.h>\\n\#include <ismd_audio.h>\\n\#include <ismd_viddec.h>\\n\#include <ismd_vidpproc.h>\\n\#include <ismd_demux.h>\\n\#include <ismd_vidrend.h>\\n\#include <ismd_tsout.h>\\n%s' % '\\n'.join(filter(lambda a: not '  result  ' in a, ['%s {}' %x for x in re.compile(r'^\(\\s*ismd_result_t\\s[^\;]+\)\;', re.M).findall(sys.stdin.read())]))\" > ${CMAKE_BINARY_DIR}/libismd_core.c
	COMMAND i686-pc-linux-gnu-gcc -I${CMAKE_SOURCE_DIR}/headers -shared -o ${SYSROOT}/usr/local/lib/libismd_core.so ${CMAKE_BINARY_DIR}/libismd_core.c
)


add_custom_command(OUTPUT OUTPUT ${SYSROOT}/usr/local/lib/libosal.so
	COMMAND i686-pc-linux-gnu-gcc -I${CMAKE_SOURCE_DIR}/headers -shared -o ${SYSROOT}/usr/local/lib/libosal.so ${CMAKE_SOURCE_DIR}/libosal.c
)

add_custom_command(OUTPUT OUTPUT ${SYSROOT}/usr/local/lib/libplatform_config.so
	COMMAND i686-pc-linux-gnu-gcc -I${CMAKE_SOURCE_DIR}/headers -shared -o ${SYSROOT}/usr/local/lib/libplatform_config.so ${CMAKE_SOURCE_DIR}/libplatform_config.c
)

add_custom_target(install ALL
	DEPENDS ${headers}
			${SYSROOT}/usr/local/lib/libgdl.so
			${SYSROOT}/usr/local/lib/libismd_core.so
			${SYSROOT}/usr/local/lib/libosal.so
			${SYSROOT}/usr/local/lib/libplatform_config.so
)
