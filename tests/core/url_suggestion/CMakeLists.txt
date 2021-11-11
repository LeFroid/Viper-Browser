include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(HistorySuggestorTest HistorySuggestorTest.cpp)
target_link_libraries(HistorySuggestorTest viper-core viper-ui Qt6::Test Threads::Threads)

add_test(NAME HistorySuggestor-Test COMMAND HistorySuggestorTest)
