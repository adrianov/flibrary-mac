include( cmake/CPM.cmake )

if( BIT7Z_CUSTOM_7ZIP_PATH STREQUAL "" )
    # 7-zip source code
    CPMAddPackage( NAME 7-zip
                   GITHUB_REPOSITORY "rikyoz/7-Zip"
                   VERSION ${BIT7Z_7ZIP_VERSION}
                   DOWNLOAD_ONLY YES )
    if( 7-zip_ADDED )
        message( STATUS "7-zip source code available at ${7-zip_SOURCE_DIR}" )
        add_library( 7-zip INTERFACE IMPORTED )
        target_include_directories( 7-zip INTERFACE "${7-zip_SOURCE_DIR}/CPP/" )
    endif()
endif()

# ghc::filesystem library
if( NOT USE_STANDARD_FILESYSTEM OR NOT STANDARD_FILESYSTEM_COMPILES OR BIT7Z_BUILD_TESTS )
    CPMAddPackage( NAME ghc_filesystem
                   GITHUB_REPOSITORY rikyoz/filesystem
                   GIT_TAG b99c2aebd5ddd6fb2f190731ba80b949fc3842b5
                   DOWNLOAD_ONLY YES )
    if( ghc_filesystem_ADDED )
        message( STATUS "ghc::filesystem source code available at ${ghc_filesystem_SOURCE_DIR}" )
        add_library( ghc_filesystem INTERFACE IMPORTED )
        target_include_directories( ghc_filesystem SYSTEM INTERFACE ${ghc_filesystem_SOURCE_DIR}/include )
    endif()
endif()
