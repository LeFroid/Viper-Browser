include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(AdBlockFilterTest_src
    AdBlockFilterTest.cpp
    AdBlockManager.cpp
)

add_executable(AdBlockFilterTest ${AdBlockFilterTest_src})

target_link_libraries(AdBlockFilterTest viper-core Qt6::Test Qt6::WebEngineCore)

add_test(NAME AdBlockFilter-Test COMMAND AdBlockFilterTest)
