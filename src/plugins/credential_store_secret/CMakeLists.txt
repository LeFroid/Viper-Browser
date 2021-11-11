set(credential_store_secret_src
    CredentialStoreSecret.cpp
)

add_library(CredentialStoreSecret SHARED ${credential_store_secret_src})

target_include_directories(CredentialStoreSecret PUBLIC ${Secret_INCLUDE_DIRS})
target_link_libraries(CredentialStoreSecret
  viper-core
  Qt6::Core
  ${Secret_LIBRARIES}
)
install(TARGETS CredentialStoreSecret DESTINATION "${VIPER_PLUGIN_DIR}")
