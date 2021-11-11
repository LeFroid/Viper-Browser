include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(BookmarkManagerTest_src
    BookmarkManagerTest.cpp
)

set(BookmarkIntegrationTest_src
    BookmarkIntegrationTest.cpp
)

add_executable(BookmarkManagerTest ${BookmarkManagerTest_src})
add_executable(BookmarkIntegrationTest ${BookmarkIntegrationTest_src})

target_link_libraries(BookmarkManagerTest viper-core viper-ui Qt6::Test Threads::Threads)
target_link_libraries(BookmarkIntegrationTest viper-core viper-ui Qt6::Test Threads::Threads)

add_test(NAME BookmarkManager-Test COMMAND BookmarkManagerTest)
add_test(NAME BookmarkIntegration-Test COMMAND BookmarkIntegrationTest)
