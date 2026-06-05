include_guard(GLOBAL)

include(GenerateExportHeader)
include(${CMAKE_CURRENT_LIST_DIR}/utils.cmake)
set(SCRIPT_HELPERS_DIR ${CMAKE_CURRENT_LIST_DIR}/../helpers)

function(AddTarget name type)
    # name                      # Имя цели, обязательно
    # type                      # Тип цели shared_lib|static_lib|app|app_bundle|app_console|header_only
    set(__options
    		SKIP_INSTALL         # не включать в установку
            #QT                  # используется Qt
            #NEED_PROTECTION     # таргет необходимо защищать (только для Win)
            #EXPORT_INCLUDE      # выставлять текущую директорию цели как публичный include при её компоновке.
            #STATIC_RUNTIME      # использовать static runtime (/MT) стандартной библиотеки (только MSVC)
            #NO_DEBUG_SYMBOLS    # отключить создание PDB для цели (даже если они включены глобально)
    )
    set(__one_val_required

    )
    set(__one_val_optional
            SOURCE_DIRECTORY     # Путь к папке с исходниками
            PROJECT_GROUP       # Группа проекта, может быть составной "Proc/Codec"
            #OUTPUT_NAME         # Задаёт выходное имя таргета
            #OUTPUT_SUBDIRECTORY # Путь к папке относительно BIN_DIR
            #BUNDLE_INFO_PLIST   # Путь к Info.plist для bundle
            #BUNDLE_ICONS        # Путь к icns для bundle
            #ENTITLEMENTS        # Entitlement с которыми будет подписан таргет для AppStore, либо для активации hardened runtime
            #WIN_APP_ICON        # Путь до иконки приложения (win)
            #WIN_RESOURCE_PATH   # Кастомный путь до win_resources.rc.in
            #AMALGAMATION_MODE   # тип объединенной сборки. all|moc|none , all = все исходники объединяются, moc - только moc-файлы. По умолчанию - moc.
            CXX_STANDARD        # версия С++. 17|20, по умолчанию 20.
    )
    set(__multi_val
            COMPILER_OPTIONS             # дополнительные опции для компилятора
            COMPILE_DEFINITIONS          # дополнительные дефайны препроцессора (без -D)
            EXCLUDE_SOURCES              # регулярное выражение для исключения исходников. E.g. "blabla\\.(cpp|h)"
            INCLUDE_DIRECTORIES          # Дополнительные include
            LIBRARIES_DIRECTORIES        # Дополнительные пути для библиотек
            #RESOURCE_MODULES             # Зависимые модули ресурсов
            #RESOURCE_PACKAGES            # Пакеты дополнительных ресурсов, объявленные при помощи RegisterResource
            #REMOTE_RESOURCE_PACKAGES     # Пакеты дополнительных удалённых ресурсов, объявленные при помощи RegisterRemoteResource

            #QT_USE                       # используемые модули Qt.
            #MACOS_FRAMEWORKS             # Фреймворки MacOS. E.g. IOKit

            #MODULES                      # имена подключаемых 3rdparty-модулей. E.g. boost, OpenGLSupport, boost::filesystem, WinLicense, opencv::imgcodecs. Модули могут иметь зависимости - например, boost::filesystem еще и подключает хедеры.
            LINK_TARGETS                 # зависимые цели для компоновки. E.g. CoreInt.
            LINK_LIBRARIES               # дополнительные библиотеки для компоновк. E.g. [ WIN32 d3d.lib ]

            #PLUGINS                      # Зависимости для сборки, которые будут использованы при фиксапе
            DEPENDENCIES                 # Указываются другие цели, сборка которых должна происходить раньше этой
            #QT_QML_MODULES               # QML плагины
            #QRC                          # Дополнительные *.qrc файлы с ресурсами, будут подключены и влинкованы в этот модуль
            #FORMS                        # Дополнительные *.ui файлы
            SOURCES                      # Дополнительные файлы с исходниками.
            #LINK_FLAGS                   # Флаги компоновки
            #MAC_XIB                      # Маковские  ui-файлы
            #CONFIGS                      # Здесь указываются файлы конфигурирующие данное приложение (пресеты, ...)
            WIN_RC                       # Дополнительные *.rc файлы с виндовыми ресурсами, будут подключены и влинкованы в этот модуль
            #REPO_DEPENDENCIES            # Зависимые репозитории (их NAME), которые будут добавлены в качестве INCLUDE_DIRS.
            #UIC_POSTPROCESS_SCRIPTS      # Список файлов cmake, используемых как команды поспроцессинга UIC (вызываются после него)
            PRECOMPILED_HEADERS          # Список прекомпилируемых хедеров
    )
    message(STATUS "configuring ${name}")
    ParseArgumentsWithConditions(ARG "${__options}" "${__one_val_required}" "${__one_val_optional}" "${__multi_val}" ${ARGN})

    __AddTarget_CreateTarget(${name} ${type} ${ARG_SKIP_INSTALL})
    __AddTarget_AddSources(${name} "${ARG_PROJECT_GROUP}" "${ARG_SOURCE_DIRECTORY}" "${ARG_EXCLUDE_SOURCES}" ${ARG_SOURCES})
    __AddTarget_AddCompilerOptions(${name} ${ARG_COMPILER_OPTIONS})

    target_compile_definitions(${name} PRIVATE ${ARG_COMPILE_DEFINITIONS})

    target_include_directories(${name} PRIVATE ${ARG_INCLUDE_DIRECTORIES})
    if (NOT ${type} STREQUAL header_only AND ARG_SOURCE_DIRECTORY)
        target_include_directories(${name} PRIVATE ${ARG_SOURCE_DIRECTORY})
    endif ()

    target_precompile_headers(${name} PRIVATE ${ARG_PRECOMPILED_HEADERS})

    target_link_directories(${name} PRIVATE ${ARG_LIBRARIES_DIRECTORIES})
    target_link_libraries(${name} LINK_PRIVATE ${ARG_LINK_LIBRARIES})
    target_link_libraries(${name} LINK_PRIVATE ${ARG_LINK_TARGETS})
    if (ARG_DEPENDENCIES)
	    add_dependencies(${name} ${ARG_DEPENDENCIES})
    endif ()
    if (ARG_WIN_RC)
	    target_sources(${name} PRIVATE ${ARG_WIN_RC})
	    if (ARG_PROJECT_GROUP)
	        source_group(Resources FILES ${ARG_WIN_RC})
	    endif ()
    endif ()
endfunction()

function(__AddTarget_CreateTarget target type skip_install)
    set(TargetType)
    set(CreateTarget library)

    if (${type} STREQUAL static_lib)
    	set(TargetType STATIC)
    elseif (${type} STREQUAL header_only)
        set(TargetType INTERFACE)
    elseif (${type} STREQUAL shared_lib)
        set(TargetType SHARED)
    elseif (${type} STREQUAL app)
    	if (WIN32)
	        set(TargetType WIN32)
	    elseif (APPLE)
	        set(TargetType MACOSX_BUNDLE)
	    endif()
        set(CreateTarget executable)
    elseif (${type} STREQUAL app_console)
        set(CreateTarget executable)
    else ()
        message(FATAL_ERROR "Unknown type TYPE: ${type}")
    endif ()

    set(ext)
    if (CreateTarget STREQUAL library)
        add_library(${target} ${TargetType})
	    set(ext dll)
    elseif (CreateTarget STREQUAL executable)
        add_executable(${target} ${TargetType})
	    set(ext exe)
    endif ()

    if (WIN32)
    	CreateWinRC(${target} ${ext})
    endif()

    # Для shared_lib создаём файл экспорта
    if (${type} STREQUAL shared_lib)
        generate_export_header(${target} EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/export/${target}.h)
        target_include_directories(${target} PUBLIC ${CMAKE_BINARY_DIR})
    endif ()
    
	set_target_properties(${target}
	    PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
			RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
			RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
	)

	if(APPLE AND ${type} STREQUAL app)
		set_target_properties(${target} PROPERTIES
			MACOSX_BUNDLE_GUI_IDENTIFIER "com.homecompa.${target}"
			MACOSX_BUNDLE_BUNDLE_NAME "${target}"
			MACOSX_BUNDLE_SHORT_VERSION_STRING "${PRODUCT_VERSION}"
			MACOSX_BUNDLE_BUNDLE_VERSION "${PRODUCT_VERSION_FULL}"
		)
	endif()

	if(NOT WIN32 AND ${type} STREQUAL shared_lib)
		set_target_properties(${target} PROPERTIES VERSION ${PRODUCT_VERSION} SOVERSION ${MAJOR_PRODUCT_VERSION}.${MINOR_PRODUCT_VERSION})
	endif()
	
    if (NOT skip_install AND ((${type} STREQUAL shared_lib) OR (${CreateTarget} STREQUAL "executable")))
		if(APPLE AND ${CreateTarget} STREQUAL executable)
			install(TARGETS ${target} BUNDLE DESTINATION .)
		else()
			install(TARGETS ${target} RUNTIME DESTINATION .)
		endif()
    endif ()

endfunction()

function(__AddTarget_AddSources target project_group source_directory exclude_sources) # ARGN - list extended sources
    if (NOT source_directory)
        target_sources(${target} PRIVATE ${ARGN})
        return()
    endif ()

    # Находим все файлы в дириктории с исходникамит
    file(GLOB_RECURSE allFiles "${source_directory}/[^.]*") # Пропуск скрытых файлов

    # Исключаем ненужные
    if (exclude_sources)
        set(regexList "${exclude_sources}")
        string(REPLACE ";" "|" regexList "(${regexList})") # склеиваем все регулярки в одну по "или".
        list(FILTER allFiles EXCLUDE REGEX "${regexList}")
    endif ()
    
    # исключаем платформозависимые от чужой платформы
    set(ALL_SYSTEMS Windows Linux Darwin)
    list(REMOVE_ITEM ALL_SYSTEMS ${CMAKE_HOST_SYSTEM_NAME})
    foreach(system ${ALL_SYSTEMS})
        list(FILTER allFiles EXCLUDE REGEX ".*/${system}/.*")    	
    endforeach()
    
    # Раскидываем по группам, тем самым исключая ненужные типы файлов
    set(headers ${allFiles})
    list(FILTER headers INCLUDE REGEX "\\.(h|hpp)$")
    set(sources ${allFiles})
    list(FILTER sources INCLUDE REGEX "\\.(c|cc|cpp|mm|m)$")
    set(qt_forms ${allFiles})
    list(FILTER qt_forms INCLUDE REGEX "\\.ui$")
    set(qt_resources ${allFiles})
    list(FILTER qt_resources INCLUDE REGEX "\\.qrc$")
    set(qt_ts ${allFiles})
    list(FILTER qt_ts INCLUDE REGEX "\\.ts$")
    set(cmake_scripts ${allFiles})
    list(FILTER cmake_scripts INCLUDE REGEX "(\\.cmake|CMakeLists.txt)$")

    target_sources(${target} PRIVATE ${ARGN} ${headers} ${sources} ${qt_forms} ${qt_resources} ${qt_ts} ${cmake_scripts})

    # Организуем структуру для MSVS
    if(project_group)
        set_property(TARGET ${name} PROPERTY FOLDER ${project_group})
        source_group(TREE ${source_directory} PREFIX Sources FILES ${sources})
        source_group(TREE ${source_directory} PREFIX Headers FILES ${headers})
        source_group(TREE ${source_directory} PREFIX Forms FILES ${qt_forms})
        source_group(TREE ${source_directory} PREFIX Resources FILES ${qt_resources} ${qt_ts})
        source_group(TREE ${source_directory} PREFIX "" FILES ${cmake_scripts})
        set(generated_regexp
                "(\\\\|/)ui_.+\\.h$"
                "cmake_pch.hxx"
                "autouic_.+.stamp"
                "mocs_compilation_.+.cpp"
        )
        string(REPLACE ";" "|" generated_regexp "(${generated_regexp})")
        source_group(Generated REGULAR_EXPRESSION "${generated_regexp}")
    endif()
endfunction()

function(__AddTarget_AddCompilerOptions target) # ARGN - list options
    foreach (option ${ARGN})
        set(supported true)
        if (NOT MSVC) # TODO check variable MSVC
 #           CheckCXXCompilerFlagCached(${option} supported)
        endif ()
        if (${supported})
            target_compile_options(${target} PRIVATE ${option})
        endif ()
    endforeach ()
endfunction()

# Создание unit-теста с использованием Google Testing Framework
# Для создания цели используется AddTarget, в котором:
#   - type = app_console
#   - к LINK_TARGETS добавляется GTest::gtest_main
#   - опция MOCK к LINK_TARGETS добавляет GTest::gmock
# Аргументы аналогичны аргументам AddTarget
include(GoogleTest)
function(AddGTest name)
    set(__options MOCK)
    set(__one_val)
    set(__multi_val LINK_LIBRARIES)
    cmake_parse_arguments(ARG "${__options}" "${__one_val}" "${__multi_val}" ${ARGN})

    AddTarget(ut_${name} app_console
            LINK_LIBRARIES GTest::gtest_main [ ARG_MOCK GTest::gmock ] ${ARG_LINK_LIBRARIES}
            ${ARG_UNPARSED_ARGUMENTS}
    )
    gtest_add_tests(TARGET ut_${name} SKIP_DEPENDENCY)
    __AddTarget__AddBuildTests()
    add_dependencies(BUILD_TESTS ut_${name})
endfunction()

# Создание unit-теста с использованием Boost Test Library: The Unit Test Framework
# Для создания цели используется AddTarget, в котором:
#   - type = app_console
#   - PROJECT_GROUP = UT
#   - к COMPILE_DEFINITIONS добавляется BOOST_TEST_DYN_LINK
#   - к LINK_LIBRARIES добавляется Boost::unit_test_framework
# Аргументы аналогичны аргументам AddTarget
function(AddBoostTest name)
    set(__options)
    set(__one_val)
    set(__multi_val
            LINK_LIBRARIES
            COMPILE_DEFINITIONS
    )
    cmake_parse_arguments(ARGS "${__options}" "${__one_val}" "${__multi_val}" ${ARGN})

    AddTarget(ut_${name}            app_console
            PROJECT_GROUP           UT
            COMPILE_DEFINITIONS     BOOST_TEST_DYN_LINK ${ARGS_COMPILE_DEFINITIONS}
            LINK_LIBRARIES          Boost::unit_test_framework ${ARGS_LINK_LIBRARIES}
            ${ARGS_UNPARSED_ARGUMENTS}
    )
    add_test(NAME ut_${name} COMMAND ut_${name})

    __AddTarget__AddBuildTests()
    add_dependencies(BUILD_TESTS ut_${name})
endfunction()

function(CreateWinRC name ext)
    set(FILE_NAME ${name})
    set(FILE_EXT ${ext})
    set(PRODUCT_VERSION_COMMA ${MAJOR_PRODUCT_VERSION},${MINOR_PRODUCT_VERSION},${PATCH_PRODUCT_VERSION})
    set(FILE_VERSION_COMMA ${PRODUCT_VERSION_COMMA})
    if (BUILD_PRODUCT_VERSION)
	    set(FILE_VERSION_COMMA ${PRODUCT_VERSION_COMMA},${BUILD_PRODUCT_VERSION})
	endif()

    set(RC_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/resources/${name}.rc")
    configure_file(${SCRIPT_HELPERS_DIR}/win_resources.rc.in ${RC_FILE_NAME} @ONLY)
    target_sources(${name} PRIVATE ${RC_FILE_NAME})
	source_group(Resources FILES ${RC_FILE_NAME})
endfunction()

function(__AddTarget__AddBuildTests)
	if(NOT TARGET BUILD_TESTS)
		add_custom_target(BUILD_TESTS)
		if(WIN32)
            set_property(TARGET BUILD_TESTS PROPERTY FOLDER "CMakePredefinedTargets")
		endif()
	endif()
endfunction()
