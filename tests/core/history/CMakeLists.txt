include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(HistoryManagerTest_src
    HistoryManagerTest.cpp
)
set(HistoryStoreTest_src
    HistoryStoreTest.cpp
)

add_executable(HistoryManagerTest ${HistoryManagerTest_src})
add_executable(HistoryStoreTest ${HistoryStoreTest_src})

target_link_libraries(HistoryManagerTest viper-core viper-ui Qt6::Test Threads::Threads)
target_link_libraries(HistoryStoreTest viper-core viper-ui Qt6::Test Threads::Threads)

add_test(NAME HistoryManager-Test COMMAND HistoryManagerTest)
add_test(NAME HistoryStore-Test COMMAND HistoryStoreTest)
