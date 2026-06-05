AddTarget(platformgui	shared_lib
	PROJECT_GROUP		Foundation
	SOURCE_DIRECTORY	"${CMAKE_CURRENT_LIST_DIR}"
	LINK_LIBRARIES
		Qt${QT_MAJOR_VERSION}::Core
		Qt${QT_MAJOR_VERSION}::Widgets
	LINK_TARGETS
	    logging
)

if(APPLE)
	target_link_libraries(platformgui PRIVATE "-framework AppKit")
endif()
