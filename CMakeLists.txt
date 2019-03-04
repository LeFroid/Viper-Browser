cmake_minimum_required(VERSION 3.1.0)

project(Viper-Browser VERSION 0.8)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 14)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)

# Get Qt
set(QT_VERSION_MIN "5.9.0")
find_package(Qt5 ${QT_VERSION_MIN} REQUIRED COMPONENTS Core Concurrent Gui Network PrintSupport Sql Svg QuickWidgets Widgets WebChannel WebEngineWidgets)

if (ENABLE_TESTING)
    find_package(Qt5 ${QT_VERSION_MIN} REQUIRED COMPONENTS Test WebEngine)
endif()

# Check for credential storage backends
if(UNIX AND NOT APPLE)
    find_package(KF5Wallet)
    if (KF5Wallet_FOUND)
        message("Using KWallet for Autofill backend")
        add_definitions(-DUSE_KWALLET)
    endif()
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/core/adblock
    ${CMAKE_SOURCE_DIR}/src/core/autofill
    ${CMAKE_SOURCE_DIR}/src/core/bookmarks
    ${CMAKE_SOURCE_DIR}/src/core/cache
    ${CMAKE_SOURCE_DIR}/src/core/cookies
    ${CMAKE_SOURCE_DIR}/src/core/credentials
    ${CMAKE_SOURCE_DIR}/src/core/database
    ${CMAKE_SOURCE_DIR}/src/core/downloads
    ${CMAKE_SOURCE_DIR}/src/core/extensions
    ${CMAKE_SOURCE_DIR}/src/core/highlighters
    ${CMAKE_SOURCE_DIR}/src/core/history
    ${CMAKE_SOURCE_DIR}/src/core/icons
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
    install(FILES data/viper-browser.png DESTINATION share/icons/hicolor/64x64/viper-browser.png)
    install(FILES data/viper-browser.desktop DESTINATION share/applications)
endif()
