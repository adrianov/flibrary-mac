include_guard(GLOBAL)

set(CONAN_PROFILE)
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	set(CONAN_PROFILE msvc2022_x86_64)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
	set(CONAN_PROFILE gcc_x86_64)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL AppleClang)
	set(CONAN_PROFILE apple_clang_arm64)
else()
	message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

function(conan_install path profile)
	set(CONAN_ADDITIONAL_PARAMETERS)
	if(${CMAKE_GENERATOR} STREQUAL "Ninja")
		set(CONAN_ADDITIONAL_PARAMETERS "-c tools.cmake.cmaketoolchain:generator=Ninja")
	endif()

	execute_process(
		COMMAND conan install ${path}
			--output-folder ${CMAKE_BINARY_DIR}
			-pr:b ${profile}
			-pr:h ${profile}
			--build=missing
			${CONAN_ADDITIONAL_PARAMETERS}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	)

	set (CMAKE_POLICY_DEFAULT_CMP0091 NEW)
	cmake_policy(SET CMP0091 NEW)
endfunction()
