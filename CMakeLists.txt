cmake_minimum_required(VERSION 3.1.0)

project(Viper-Browser VERSION 1.0.0)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2 -msse3 -msse4")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(UNIX)
  include(GNUInstallDirs)
  set(VIPER_PLUGIN_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/viper-browser/plugins")
else()
  set(VIPER_PLUGIN_DIR "${CMAKE_INSTALL_PREFIX}/plugins")
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)

# Get Qt
set(QT_VERSION_MIN "6.0.0")
find_package(Qt6 ${QT_VERSION_MIN} REQUIRED COMPONENTS Core Concurrent Gui Network PrintSupport Svg QuickWidgets Widgets WebChannel WebEngineWidgets)

if (ENABLE_TESTING)
    find_package(Qt6 ${QT_VERSION_MIN} REQUIRED COMPONENTS Test WebEngineCore)
endif()

# TODO: KF5Wallet incompatible with qt6. Need some version that is compatible with qt 6
option(DISABLE_KWALLET "Disable KWallet plugin" ON)
option(DISABLE_LIBSECRET "Force lib secret plugin not to be compiled." ON)

# Check for credential storage backends
if(UNIX AND NOT APPLE)
    find_package(KF5Wallet)
    if (KF5Wallet_FOUND)
        message("-- Found KWallet")
    endif()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Secret libsecret-1)
    if (Secret_FOUND)
        message("-- Found Secret")
    endif()
endif()

find_package (SQLite3)
include_directories(${SQLite3_INCLUDE_DIR})

configure_file(${CMAKE_SOURCE_DIR}/src/config.h.in ${CMAKE_SOURCE_DIR}/src/config.h)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/core/adblock
    ${CMAKE_SOURCE_DIR}/src/core/app
    ${CMAKE_SOURCE_DIR}/src/core/autofill
    ${CMAKE_SOURCE_DIR}/src/core/bookmarks
    ${CMAKE_SOURCE_DIR}/src/core/cache
    ${CMAKE_SOURCE_DIR}/src/core/cookies
    ${CMAKE_SOURCE_DIR}/src/core/credentials
    ${CMAKE_SOURCE_DIR}/src/core/database
    ${CMAKE_SOURCE_DIR}/src/core/database/sqlite
    ${CMAKE_SOURCE_DIR}/src/core/downloads
    ${CMAKE_SOURCE_DIR}/src/core/extensions
    ${CMAKE_SOURCE_DIR}/src/core/highlighters
    ${CMAKE_SOURCE_DIR}/src/core/history
    ${CMAKE_SOURCE_DIR}/src/core/icons
    ${CMAKE_SOURCE_DIR}/src/core/ipc
    ${CMAKE_SOURCE_DIR}/src/core/network
    ${CMAKE_SOURCE_DIR}/src/core/search
    ${CMAKE_SOURCE_DIR}/src/core/session
    ${CMAKE_SOURCE_DIR}/src/core/settings
    ${CMAKE_SOURCE_DIR}/src/core/text_finder
    ${CMAKE_SOURCE_DIR}/src/core/threading
    ${CMAKE_SOURCE_DIR}/src/core/url_suggestion
    ${CMAKE_SOURCE_DIR}/src/core/user_agents
    ${CMAKE_SOURCE_DIR}/src/core/user_scripts
    ${CMAKE_SOURCE_DIR}/src/core/utility
    ${CMAKE_SOURCE_DIR}/src/core/web
    ${CMAKE_SOURCE_DIR}/src/ui/adblock
    ${CMAKE_SOURCE_DIR}/src/ui/autofill
    ${CMAKE_SOURCE_DIR}/src/ui/bookmarks
    ${CMAKE_SOURCE_DIR}/src/ui/cookies
    ${CMAKE_SOURCE_DIR}/src/ui/downloads
    ${CMAKE_SOURCE_DIR}/src/ui/history
    ${CMAKE_SOURCE_DIR}/src/ui/network
    ${CMAKE_SOURCE_DIR}/src/ui/preferences
    ${CMAKE_SOURCE_DIR}/src/ui/url_suggestion
    ${CMAKE_SOURCE_DIR}/src/ui/user_agents
    ${CMAKE_SOURCE_DIR}/src/ui/user_scripts
    ${CMAKE_SOURCE_DIR}/src/ui/web
    ${CMAKE_SOURCE_DIR}/src/ui/widgets
    ${CMAKE_SOURCE_DIR}/src/ui/window
)

add_subdirectory(src)

if (ENABLE_TESTING)
    enable_testing(true)
    add_subdirectory(tests)
endif()

if(UNIX AND NOT APPLE)
    install(FILES data/viper-browser.png DESTINATION share/icons/hicolor/64x64/apps)
    install(FILES data/viper-browser.desktop DESTINATION share/applications)
endif()
