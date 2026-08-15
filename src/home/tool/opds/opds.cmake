if (${QT_MAJOR_VERSION} STREQUAL "5")
	return()
endif()

if(APPLE)
	set(_opds_type app_console)
else()
	set(_opds_type app)
endif()

AddTarget(opds	${_opds_type}
	PROJECT_GROUP    Tool
	SOURCE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
	LINK_LIBRARIES
		Boost::headers
		Qt${QT_MAJOR_VERSION}::Concurrent
		Qt${QT_MAJOR_VERSION}::Core
		Qt${QT_MAJOR_VERSION}::Gui
		Qt${QT_MAJOR_VERSION}::HttpServer
		Qt${QT_MAJOR_VERSION}::Network
	LINK_TARGETS
		flint
		fnd
		logging
		logic
		platform
		util
		ver
		zip
	[ APPLE SKIP_INSTALL ]
)

if(APPLE)
	set_target_properties(opds PROPERTIES
		BUILD_RPATH "@executable_path/../Frameworks"
		INSTALL_RPATH "@executable_path/../Frameworks"
	)
	add_dependencies(${PROJECT_NAME} opds)
endif()
