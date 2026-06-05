include_directories(${CMAKE_CURRENT_LIST_DIR})

AddTarget(zip	shared_lib
	PROJECT_GROUP Foundation
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}"
    INCLUDE_DIRECTORIES
        "${EXT_ROOT}/bit7z/include"
	LINK_LIBRARIES
		Qt${QT_MAJOR_VERSION}::Gui
        bit7z
	LINK_TARGETS
		fnd
		logging
		platform
)

set(7zip_BIN_FILENAME)
set(7zip_INSTALL_PATH)
set(7zip_BIN_PATH)
if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL Windows)
	set(7zip_BIN_FILENAME 7z.dll)
	set(7zip_INSTALL_PATH .)
	set(7zip_BIN_PATH ${CMAKE_BINARY_DIR}/bin)
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL Linux OR ${CMAKE_HOST_SYSTEM_NAME} STREQUAL Darwin)
	set(7zip_BIN_FILENAME 7z.so)
	if(APPLE)
		set(7zip_INSTALL_PATH ../Frameworks)
	else()
		set(7zip_INSTALL_PATH lib)
	endif()
	set(7zip_BIN_PATH ${CMAKE_BINARY_DIR}/lib)
else()
	message(FATAL_ERROR "Unsupported host system: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${7zip_BIN_PATH}")
execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${7zip_BIN_DIR}/${7zip_BIN_FILENAME}" "${7zip_BIN_PATH}/${7zip_BIN_FILENAME}")
install(FILES "${7zip_BIN_DIR}/${7zip_BIN_FILENAME}" DESTINATION ${7zip_INSTALL_PATH})

