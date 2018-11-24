include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

set(RegExpTest_src
    tst_RegExpFilter.cpp
)

add_executable(RegExpTest ${RegExpTest_src})

target_link_libraries(RegExpTest viper-core Qt5::Test)

add_test(NAME RegExp-Test COMMAND RegExpTest)
