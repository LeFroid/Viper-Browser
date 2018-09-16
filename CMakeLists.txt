cmake_minimum_required(VERSION 3.1.0)

project(Viper-Browser VERSION 0.8)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 14)

# Get Qt
set(QT_VERSION_MIN "5.9.0")
find_package(Qt5 ${QT_VERSION_MIN} REQUIRED COMPONENTS Core Concurrent Gui Network PrintSupport Sql Svg QuickWidgets Widgets WebChannel WebEngineWidgets)

# Check for credential storage backends
find_package(KF5Wallet)
if (KF5Wallet_FOUND)
    add_definitions(-DUSE_KWALLET)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/AdBlock
    ${CMAKE_SOURCE_DIR}/src/AutoFill
    ${CMAKE_SOURCE_DIR}/src/Bookmarks
    ${CMAKE_SOURCE_DIR}/src/Cache
    ${CMAKE_SOURCE_DIR}/src/Cookies
    ${CMAKE_SOURCE_DIR}/src/Credentials
    ${CMAKE_SOURCE_DIR}/src/Downloads
    ${CMAKE_SOURCE_DIR}/src/Extensions
    ${CMAKE_SOURCE_DIR}/src/Highlighters
    ${CMAKE_SOURCE_DIR}/src/History
    ${CMAKE_SOURCE_DIR}/src/Network
    ${CMAKE_SOURCE_DIR}/src/Preferences
    ${CMAKE_SOURCE_DIR}/src/URLSuggestion
    ${CMAKE_SOURCE_DIR}/src/UserAgents
    ${CMAKE_SOURCE_DIR}/src/UserScripts
    ${CMAKE_SOURCE_DIR}/src/Web
    ${CMAKE_SOURCE_DIR}/src/Widgets
    ${CMAKE_SOURCE_DIR}/src/Window
)

add_subdirectory(src)
