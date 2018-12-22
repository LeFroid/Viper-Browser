include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

set(FastHashTest_src
    tst_FastHash.cpp
)

add_executable(FastHashTest ${FastHashTest_src})

target_link_libraries(FastHashTest viper-core Qt5::Test)

add_test(NAME FastHash-Test COMMAND FastHashTest)
