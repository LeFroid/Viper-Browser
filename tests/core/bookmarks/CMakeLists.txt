include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(BookmarkTest_src
    ${CMAKE_SOURCE_DIR}/src/app/BrowserApplication.cpp
    ${CMAKE_SOURCE_DIR}/src/app/BrowserScripts.cpp
    BookmarkManagerTest.cpp
)

add_executable(BookmarkTest ${BookmarkTest_src})

target_link_libraries(BookmarkTest viper-core viper-ui Qt5::Test)

add_test(NAME Bookmark-Test COMMAND BookmarkTest)
