include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(FaviconManagerTest_src
    FaviconManagerTest.cpp
)

add_executable(FaviconManagerTest ${FaviconManagerTest_src})

target_link_libraries(FaviconManagerTest viper-core viper-ui Qt6::Test Threads::Threads)

#add_test(NAME FaviconManager-Test COMMAND FaviconManagerTest)
