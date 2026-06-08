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
		platformgui
		rest
		util
		ver
	DEPENDENCIES
		locales
)

if(APPLE)
	set(_info_plist "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_Info.plist")
	set(MACOSX_BUNDLE_EXECUTABLE_NAME "${PROJECT_NAME}")
	set(MACOSX_BUNDLE_ICON_FILE "${PROJECT_NAME}")
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.homecompa.${PROJECT_NAME}")
	set(MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}")
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PRODUCT_VERSION}")
	set(MACOSX_BUNDLE_BUNDLE_VERSION "${PRODUCT_VERSION_FULL}")
	configure_file("${CMAKE_CURRENT_LIST_DIR}/Info.plist.in" "${_info_plist}" @ONLY)
	set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${_info_plist}")

	set(_app_bundle "${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}.app")
	set(_app_frameworks "${_app_bundle}/Contents/Frameworks")
	set(_app_icon_ico "${CMAKE_SOURCE_DIR}/src/home/resources/icons/main.ico")
	set(_app_icon_icns "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.icns")

	add_custom_command(
		OUTPUT "${_app_icon_icns}"
		COMMAND "${CMAKE_SOURCE_DIR}/cmake/make_icns.sh" "${_app_icon_ico}" "${_app_icon_icns}"
		DEPENDS "${_app_icon_ico}" "${CMAKE_SOURCE_DIR}/cmake/make_icns.sh"
		COMMENT "Generating ${PROJECT_NAME}.icns"
	)

	target_sources(${PROJECT_NAME} PRIVATE "${_app_icon_icns}")
	set_source_files_properties("${_app_icon_icns}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources GENERATED TRUE)
	set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE "${PROJECT_NAME}")

	add_custom_target(${PROJECT_NAME}_icon DEPENDS "${_app_icon_icns}")
	add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_icon)

	find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${QT_BIN_DIR}")
	if(NOT MACDEPLOYQT_EXECUTABLE)
		message(FATAL_ERROR "macdeployqt not found")
	endif()

		set(_qt_lib_dir "")
		execute_process(COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_LIBS OUTPUT_VARIABLE _qt_lib_dir OUTPUT_STRIP_TRAILING_WHITESPACE)

		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "${_app_frameworks}"
		COMMAND ${CMAKE_COMMAND} -Dsrc="${CMAKE_BINARY_DIR}/lib" -Ddst="${_app_frameworks}" -P "${CMAKE_SOURCE_DIR}/cmake/copy_bundle_libs.cmake"
		COMMAND ${CMAKE_COMMAND} -Dsrc="${CMAKE_BINARY_DIR}/lib" -Ddst="${_app_bundle}/Contents/Resources" -P "${CMAKE_SOURCE_DIR}/cmake/copy_data_files.cmake"
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${CMAKE_BINARY_DIR}/bin/locales"
			"${_app_bundle}/Contents/Resources/locales"
		COMMAND ${CMAKE_COMMAND} -E rm -rf "${_app_bundle}/Contents/PlugIns"
		COMMAND ${CMAKE_COMMAND} -E rm -f "${_app_bundle}/Contents/Resources/qt.conf"
		COMMAND "${CMAKE_SOURCE_DIR}/cmake/run_macdeployqt.sh" "${MACDEPLOYQT_EXECUTABLE}" "${_app_bundle}" -libpath="${_qt_lib_dir}"
		COMMAND ${CMAKE_COMMAND}
			-DBUNDLE="${_app_bundle}"
			-DEXECUTABLE_NAME="${PROJECT_NAME}"
			-P "${CMAKE_SOURCE_DIR}/cmake/fixup_bundle.cmake"
		COMMAND /System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister -f -R "${_app_bundle}"
	)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		BUILD_RPATH "@executable_path/../Frameworks"
		INSTALL_RPATH "@executable_path/../Frameworks"
	)
endif()
