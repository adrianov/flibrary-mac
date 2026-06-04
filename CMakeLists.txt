cmake_minimum_required(VERSION 3.12)

if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS version" FORCE)
endif()

execute_process(COMMAND git log -1 --format=%h WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "Git commit hash: ${GIT_HASH}")

set(CMAKE_GENERATOR_PLATFORM)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")

set(COMPANY_NAME HomeCompa)
set(COPYRIGHT "© 2026 heimdallr")
set(PRODUCT_DESCRIPTION "another e-book cataloger")
set(PRODUCT_ICON "${CMAKE_CURRENT_LIST_DIR}/src/home/resources/icons/main.ico")

if ("${QT_MAJOR_VERSION}" STREQUAL "")
	set(QT_MAJOR_VERSION 6)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(QT${QT_MAJOR_VERSION} "QT${QT_MAJOR_VERSION}")

set(EXT_ROOT ${CMAKE_SOURCE_DIR}/src/ext CACHE STRING "EXT_ROOT")
set(BUILDSCRIPTS_ROOT ${EXT_ROOT}/scripts CACHE STRING "BUILDSCRIPTS_ROOT")
set(FOUNDATION_ROOT ${EXT_ROOT}/foundation CACHE STRING "FOUNDATION_ROOT")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(MAJOR_PRODUCT_VERSION 2)
set(MINOR_PRODUCT_VERSION 6)
set(PATCH_PRODUCT_VERSION 4)
file(READ ${CMAKE_CURRENT_LIST_DIR}/src/home/flibrary/version/build.h BUILD_PRODUCT_VERSION)
string(REGEX MATCH "[0-9]+" BUILD_PRODUCT_VERSION ${BUILD_PRODUCT_VERSION})

set(PRODUCT_VERSION ${MAJOR_PRODUCT_VERSION}.${MINOR_PRODUCT_VERSION}.${PATCH_PRODUCT_VERSION})
set(PRODUCT_VERSION_FULL ${PRODUCT_VERSION}.${BUILD_PRODUCT_VERSION})
set(PRODUCT_UID 80B196FB-9529-459D-960E-0E0F00CE0981)

project(FLibrary VERSION ${PRODUCT_VERSION})

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE Debug)
endif ()
set(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE})
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if (NOT DEFINED 7zip_BIN_DIR)
	if(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
		set(7zip_BIN_DIR "/usr/lib/7zip" CACHE STRING "Path to 7zip binaries (required)")
	elseif(APPLE)
		set(7zip_BIN_DIR "/opt/homebrew/lib" CACHE STRING "Path to 7zip binaries (required)")
	endif()
endif()
if ("${7zip_BIN_DIR}" STREQUAL "")
	message(FATAL_ERROR "7zip_BIN_DIR is not set. Please provide a value using -D7zip_BIN_DIR='Path/to/7zip/binaries'")
endif ()

include(${BUILDSCRIPTS_ROOT}/cmake/conan.cmake)
conan_install(
	${CMAKE_SOURCE_DIR}/src/home/script/conan
	${BUILDSCRIPTS_ROOT}/conan/profiles/${CONAN_PROFILE}_${CMAKE_BUILD_TYPE}
	)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")

if(APPLE AND CMAKE_OSX_SYSROOT STREQUAL "macosx")
	execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE _OSX_SDK OUTPUT_STRIP_TRAILING_WHITESPACE)
	set(CMAKE_OSX_SYSROOT "${_OSX_SDK}" CACHE STRING "" FORCE)
endif()

if(MSVC)
	if (NOT ${CMAKE_CXX_FLAGS} STREQUAL "")
		string(REGEX REPLACE "/W3" " " CMAKE_C_FLAGS ${CMAKE_CXX_FLAGS})
		string(REGEX REPLACE "/W3" " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
	endif()

	add_compile_options(
		/MP     # multi-processor compilation
		/W4		# warnings level 4
		/WX		# warning is error
		/we5038 # reorder
		/wd4250 # inherits via dominance
		/wd4251 # member (ala std::unique_ptr<some>) needs to have dll-interface to be used by clients of class 'some'
		/wd4275 # non dll-interface class 'ISome' used as base for dll-interface class 'Some'
		/wd4503	# decorated name length exceeded, name was truncated
		/bigobj # number of sections exceeded object file format limit
	)
else()
	SET(CMAKE_SKIP_BUILD_RPATH FALSE)
	SET(CMAKE_INSTALL_RPATH "@executable_path/../lib")
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

	add_compile_options(
		-Wall
		-Wextra
		-Werror
		-pedantic
		-Wno-deprecated-declarations
		-Wno-missing-field-initializers
		-Wno-array-bounds
		-fPIC		
	)
	if(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
		add_compile_options(-Wno-virtual-move-assign)
	endif()
	if(APPLE)
		add_compile_options(
			-Wno-gnu-anonymous-struct
			-Wno-nested-anon-types
			-Wno-defaulted-function-deleted
			-Wno-unused-lambda-capture
			-Wno-overloaded-virtual
			-Wno-inconsistent-missing-override
			-Wno-missing-template-arg-list-after-template-kw
			-Wno-call-to-pure-virtual-from-ctor-dtor
			-Wno-unused-function
		)
	endif()
endif()

set(QtModules Core Gui Widgets Xml Network Svg Concurrent)
if(QT6)
	list(APPEND QtModules HttpServer WebSockets)
endif()
if(UNIX AND NOT APPLE)
	list(APPEND QtModules DBus)
endif()
if(UNIX AND NOT WIN32 AND NOT APPLE)
	list(APPEND QtModules WaylandClient)
endif()

set(QtPlugins
	QSvgIcon
	QGif QICNS QICO QJpeg QSvg QTga QTiff QWbmp QWebp
)
if(QT5)
	list(APPEND QtPlugins
		QWindowsIntegration
		QWindowsVistaStyle
	)
elseif(QT6)
	list(APPEND QtPlugins
		QTlsBackendCertOnly
	)
	if(WIN32)
		list(APPEND QtPlugins
			QWindowsIntegration
			QModernWindowsStyle
			QSchannelBackend
		)
	elseif(APPLE)
		list(APPEND QtPlugins
			QCocoaIntegration
			QMacStyle
			QSecureTransportBackend
		)
	else()
		list(APPEND QtPlugins
			QWaylandIntegration QXcbIntegration
			QTlsBackendOpenSSL
			QNetworkManagerNetworkInformation QGlibNetworkInformation QConnManNetworkInformation
			QGtk3Theme QXdgDesktopPortalTheme
			QWaylandAdwaitaDecoration QWaylandBradientDecoration
			DrmEglServerBuffer QWaylandEglClientBuffer ShmServerBuffer
			QWaylandFullScreenShellV1Integration QWaylandIviShellIntegration QWaylandQtShellIntegration QWaylandWlShellIntegration QWaylandXdgShellIntegration
		)
	endif()
endif()

find_package(Qt${QT_MAJOR_VERSION} REQUIRED COMPONENTS ${QtModules})

set(BIT7Z_AUTO_FORMAT ON CACHE BOOL "enable auto format support" FORCE)
set(BIT7Z_USE_NATIVE_STRING ON CACHE BOOL "enable native string" FORCE)
set(BIT7Z_USE_STD_BYTE ON CACHE BOOL "enable std::byte" FORCE)
if(APPLE)
	# Homebrew sevenzip (26.x) uses modern 7-Zip IUnknown; legacy mode crashes at runtime.
	set(BIT7Z_7ZIP_VERSION "23.01" CACHE STRING "7-zip SDK version" FORCE)
	set(BIT7Z_USE_LEGACY_IUNKNOWN OFF CACHE BOOL "legacy IUnknown for old p7zip" FORCE)
endif()
add_subdirectory(${CMAKE_SOURCE_DIR}/src/ext/bit7z)
if(MSVC)
	set_property(TARGET bit7z PROPERTY FOLDER ThirdParty)
endif()

find_package(plog REQUIRED)
find_package(Boost REQUIRED)
find_package(XercesC REQUIRED)
find_package(ICU REQUIRED COMPONENTS tu io uc i18n data)
find_package(libjxl REQUIRED)
find_package(cimg REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(djvulibre REQUIRED)
find_package(poppler REQUIRED)
find_package(poppler-data REQUIRED)

include(${BUILDSCRIPTS_ROOT}/cmake/target.cmake)
include(${BUILDSCRIPTS_ROOT}/cmake/sdk.cmake)
include(${BUILDSCRIPTS_ROOT}/cmake/utils.cmake)

CopyAndInstallICU()
CopyAndInstallQtModules(${QtModules})
CopyAndInstallQtPlugins(${QtPlugins})

include_directories(${FOUNDATION_ROOT})
include(${FOUNDATION_ROOT}/fnd/fnd.cmake)
include(${FOUNDATION_ROOT}/logging/logging.cmake)
include(${FOUNDATION_ROOT}/network/network.cmake)
include(${FOUNDATION_ROOT}/jxl/jxl.cmake)
include(${FOUNDATION_ROOT}/zip/zip.cmake)
include(${FOUNDATION_ROOT}/database/database.cmake)
include(${FOUNDATION_ROOT}/util/util.cmake)
include(${FOUNDATION_ROOT}/utilgui/utilgui.cmake)
include(${FOUNDATION_ROOT}/icu/icu.cmake)
include(${FOUNDATION_ROOT}/djvu/djvu.cmake)
include(${FOUNDATION_ROOT}/pdf/pdf.cmake)
include(${FOUNDATION_ROOT}/platform/platform.cmake)
include(${FOUNDATION_ROOT}/platformgui/platformgui.cmake)
include(${FOUNDATION_ROOT}/settings/settings.cmake)

file(WRITE "${CMAKE_BINARY_DIR}/var/7z" "${7zip_BIN_DIR}/")
file(WRITE "${CMAKE_BINARY_DIR}/var/version" ${PRODUCT_VERSION})
file(WRITE "${CMAKE_BINARY_DIR}/var/uid" ${PRODUCT_UID})
set(OS_NAME ${CMAKE_HOST_SYSTEM_NAME})
if (WIN32 AND QT5)
	set(OS_NAME Win7)
endif()
file(WRITE "${CMAKE_BINARY_DIR}/var/os" ${OS_NAME})

set(PLOG_SHARED_IMPORT PLOG_GLOBAL)
if(WIN32)
	set(PLOG_SHARED_IMPORT PLOG_IMPORT)
endif()
add_compile_definitions(${PLOG_SHARED_IMPORT})
add_compile_definitions(UNICODE _UNICODE NOMINMAX _USE_MATH_DEFINES)
add_compile_definitions(HYPODERMIC_OVERRIDE_CONSTRUCTOR_ARGUMENT_COUNT=32)
add_compile_definitions(LOCALIZED_APPLICATION)

include_directories("src/ext/Hypodermic")

include("src/ext/sqlite.cmake")
include("src/home/home.cmake")

include("src/home/script/cmake/pack.cmake")
install(FILES "${CMAKE_SOURCE_DIR}/LICENSE_en.txt" DESTINATION . RENAME LICENSE.txt)

if(APPLE)
	AddTarget(fb2_epub_test app_console
		PROJECT_GROUP Tests
		SOURCES
			"${CMAKE_SOURCE_DIR}/tests/fb2_epub_test.cpp"
			"${CMAKE_SOURCE_DIR}/tests/fb2_epub_test_checks.cpp"
		LINK_TARGETS util zip settings platform
		LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core
	)
endif()

if (PERSONAL_BUILD_NAME)
	message(WARNING "Personal ${PERSONAL_BUILD_NAME} build")
	execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 5)
endif()

