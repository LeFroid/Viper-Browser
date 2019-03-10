include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(BrowserDependencies_src
    ${CMAKE_SOURCE_DIR}/src/app/BrowserApplication.cpp
    ${CMAKE_SOURCE_DIR}/src/app/BrowserScripts.cpp
)

set(FaviconManagerTest_src
    FaviconManagerTest.cpp
)

add_executable(FaviconManagerTest ${FaviconManagerTest_src} ${BrowserDependencies_src})

target_link_libraries(FaviconManagerTest viper-core viper-ui Qt5::Test Threads::Threads)

#add_test(NAME FaviconManager-Test COMMAND FaviconManagerTest)
