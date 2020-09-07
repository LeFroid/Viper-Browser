set(credential_store_kwallet_src
    CredentialStoreKWallet.cpp
)

add_library(CredentialStoreKwallet SHARED ${credential_store_kwallet_src})

target_link_libraries(CredentialStoreKwallet
  viper-core
  Qt5::Core
  KF5::Wallet
)
install(TARGETS CredentialStoreKwallet DESTINATION "${VIPER_PLUGIN_DIR}")
