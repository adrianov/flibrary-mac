AddTarget(flipdf		shared_lib
	PROJECT_GROUP		Foundation
	SOURCE_DIRECTORY	"${CMAKE_CURRENT_LIST_DIR}"
	LINK_LIBRARIES
		poppler::poppler
		Qt${QT_MAJOR_VERSION}::Core
		Qt${QT_MAJOR_VERSION}::Gui
	LINK_TARGETS
		logging
)
