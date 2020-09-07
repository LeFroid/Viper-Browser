if (KF5Wallet_FOUND)
  message("-- Building Plugin: KWallet Integration")
  add_subdirectory(credential_store_kwallet)
endif()

