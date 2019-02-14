include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(BookmarkManagerTest_src
    ${CMAKE_SOURCE_DIR}/src/app/BrowserApplication.cpp
    ${CMAKE_SOURCE_DIR}/src/app/BrowserScripts.cpp
    BookmarkManagerTest.cpp
)

add_executable(BookmarkManagerTest ${BookmarkManagerTest_src})

target_link_libraries(BookmarkManagerTest viper-core viper-ui Qt5::Test)

add_test(NAME BookmarkManager-Test COMMAND BookmarkManagerTest)
