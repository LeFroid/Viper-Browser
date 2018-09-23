include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(RegExpTest_src
    tst_RegExpFilter.cpp
)

add_executable(RegExpTest ${RegExpTest_src})

target_link_libraries(RegExpTest Qt5::Test)

add_test(NAME RegExp-Test COMMAND RegExpTest)
