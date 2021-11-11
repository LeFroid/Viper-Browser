include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

set(FastHashTest_src
    FastHashTest.cpp
)

set(CommonUtil_RegExpTest_src
    CommonUtil_RegExpTest.cpp
)

add_executable(FastHashTest ${FastHashTest_src})
add_executable(CommonUtil-RegExpTest ${CommonUtil_RegExpTest_src})

target_link_libraries(FastHashTest viper-core Qt6::Test)
target_link_libraries(CommonUtil-RegExpTest viper-core Qt6::Test)

add_test(NAME FastHash-Test COMMAND FastHashTest)
add_test(NAME CommonUtil-RegExp-Test COMMAND CommonUtil-RegExpTest)
