#set(CMAKE_AUTORCC ON)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/assets)
include_directories(${CMAKE_SOURCE_DIR}/src/app/assets)

qt6_add_resources(viper_qrc assets/application.qrc)

set(viper_browser_src
    main.cpp
)

add_executable(viper-browser ${viper_qrc} ${viper_browser_src})

target_link_libraries(viper-browser
    sqlite-wrapper-cpp
    viper-core
    viper-ui
    Threads::Threads
)


install(TARGETS viper-browser DESTINATION bin)
