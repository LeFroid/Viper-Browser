include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(DatabaseWorkerTest_src
    FakeDatabaseWorker.cpp
    DatabaseWorkerTest.cpp
)

add_executable(DatabaseWorkerTest ${DatabaseWorkerTest_src})

target_link_libraries(DatabaseWorkerTest viper-core sqlite-wrapper-cpp Qt6::Test Qt6::WebEngineCore)

add_test(NAME DatabaseWorker-Test COMMAND DatabaseWorkerTest)
