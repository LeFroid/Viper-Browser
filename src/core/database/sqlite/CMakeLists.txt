set(sqlite-wrapper_src
    internal/implementation.cpp
    Database.cpp
    PreparedStatement.cpp
)
add_library(sqlite-wrapper-cpp STATIC ${sqlite-wrapper_src})
target_link_libraries(sqlite-wrapper-cpp ${SQLite3_LIBRARY})

