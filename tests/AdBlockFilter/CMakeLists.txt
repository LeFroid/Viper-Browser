include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(AdBlockFilterTest_src
    tst_AdBlockFilterTest.cpp
    AdBlockManager.cpp
    ${CMAKE_SOURCE_DIR}/src/AdBlock/AdBlockFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/AdBlock/AdBlockFilterParser.cpp
    ${CMAKE_SOURCE_DIR}/src/AdBlock/AdBlockSubscription.cpp
    ${CMAKE_SOURCE_DIR}/src/Web/URL.cpp
)

add_executable(AdBlockFilterTest ${AdBlockFilterTest_src})

target_link_libraries(AdBlockFilterTest Qt5::Core Qt5::Test Qt5::WebEngine)

add_test(NAME AdBlockFilter-Test COMMAND AdBlockFilterTest)
