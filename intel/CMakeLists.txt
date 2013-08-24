cmake_minimum_required(VERSION 2.8)

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


add_custom_target(install ALL
	DEPENDS ${headers}
)
