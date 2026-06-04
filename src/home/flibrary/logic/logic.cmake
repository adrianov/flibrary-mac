AddTarget(logic	shared_lib
	PROJECT_GROUP App
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}"
	SOURCES
		"${CMAKE_CURRENT_LIST_DIR}/Annotation/AnnotationBookCache.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/Annotation/AnnotationCacheCodec.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/Annotation/AnnotationCacheCodecDb.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/Annotation/AnnotationCacheCodecItem.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/Annotation/AnnotationControllerFormat.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/Annotation/AnnotationDbLoader.cpp"
	LINK_LIBRARIES
		Boost::headers
		plog::plog
		Qt${QT_MAJOR_VERSION}::Gui
		Qt${QT_MAJOR_VERSION}::Network
		[ QT5 Qt${QT_MAJOR_VERSION}::Widgets ]
	LINK_TARGETS
		dbfactory
		flidjvu
		flint
		flipdf
		inpx
		joke
		logging
		network
		platform
		rest
		settings
		util
		zip
)
