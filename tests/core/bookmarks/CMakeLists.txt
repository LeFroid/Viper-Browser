include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(BrowserDependencies_src
    ${CMAKE_SOURCE_DIR}/src/app/BrowserApplication.cpp
    ${CMAKE_SOURCE_DIR}/src/app/BrowserScripts.cpp
)

set(BookmarkManagerTest_src
    BookmarkManagerTest.cpp
)

set(BookmarkIntegrationTest_src
    BookmarkIntegrationTest.cpp
)

add_executable(BookmarkManagerTest ${BrowserDependencies_src} ${BookmarkManagerTest_src})
add_executable(BookmarkIntegrationTest ${BrowserDependencies_src} ${BookmarkIntegrationTest_src})

target_link_libraries(BookmarkManagerTest viper-core viper-ui Qt5::Test Threads::Threads)
target_link_libraries(BookmarkIntegrationTest viper-core viper-ui Qt5::Test Threads::Threads)

add_test(NAME BookmarkManager-Test COMMAND BookmarkManagerTest)
add_test(NAME BookmarkIntegration-Test COMMAND BookmarkIntegrationTest)
