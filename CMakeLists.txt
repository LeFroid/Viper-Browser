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
    ${CMAKE_SOURCE_DIR}/Browser
    ${CMAKE_SOURCE_DIR}/Browser/AdBlock
    ${CMAKE_SOURCE_DIR}/Browser/AutoFill
    ${CMAKE_SOURCE_DIR}/Browser/Bookmarks
    ${CMAKE_SOURCE_DIR}/Browser/Cache
    ${CMAKE_SOURCE_DIR}/Browser/Cookies
    ${CMAKE_SOURCE_DIR}/Browser/Credentials
    ${CMAKE_SOURCE_DIR}/Browser/Downloads
    ${CMAKE_SOURCE_DIR}/Browser/Extensions
    ${CMAKE_SOURCE_DIR}/Browser/Highlighters
    ${CMAKE_SOURCE_DIR}/Browser/History
    ${CMAKE_SOURCE_DIR}/Browser/Network
    ${CMAKE_SOURCE_DIR}/Browser/Preferences
    ${CMAKE_SOURCE_DIR}/Browser/URLSuggestion
    ${CMAKE_SOURCE_DIR}/Browser/UserAgents
    ${CMAKE_SOURCE_DIR}/Browser/UserScripts
    ${CMAKE_SOURCE_DIR}/Browser/Web
    ${CMAKE_SOURCE_DIR}/Browser/Widgets
    ${CMAKE_SOURCE_DIR}/Browser/Window
)

add_subdirectory(Browser)
