if ((NOT DISABLE_KWALLET) AND KF5Wallet_FOUND)
  message("-- Building Plugin: KWallet Integration")
  add_subdirectory(credential_store_kwallet)
endif()

if ((NOT DISABLE_LIBSECRET) AND Secret_FOUND)
    message("-- Building Plugin: Secret Integration")
    add_subdirectory(credential_store_secret)
endif()
