file(COPY "${CMAKE_CURRENT_LIST_DIR}/../../../../LICENSE_en.txt" DESTINATION ${CMAKE_BINARY_DIR}/bin)
file(RENAME "${CMAKE_BINARY_DIR}/bin/LICENSE_en.txt" "${CMAKE_BINARY_DIR}/bin/LICENSE.txt")

AddTarget(${PROJECT_NAME}	app
	PROJECT_GROUP App
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}"
	LINK_LIBRARIES
		Boost::headers
		Qt${QT_MAJOR_VERSION}::Widgets
	LINK_TARGETS
		flidjvu
		flint
		gui
		gutil
		logging
		logic
		platform
		rest
		util
		ver
	DEPENDENCIES
		locales
)

if(APPLE)
	set(_app_bundle "${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}.app")
	set(_app_frameworks "${_app_bundle}/Contents/Frameworks")
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "${_app_frameworks}"
		COMMAND ${CMAKE_COMMAND} -Dsrc="${CMAKE_BINARY_DIR}/lib" -Ddst="${_app_frameworks}" -P "${CMAKE_SOURCE_DIR}/cmake/copy_bundle_libs.cmake"
	)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		BUILD_RPATH "@executable_path/../Frameworks"
		INSTALL_RPATH "@executable_path/../Frameworks"
	)
endif()
