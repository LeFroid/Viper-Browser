#----------------------------------------------------------------
# Generated CMake target import file for configuration "None".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "KF5::Wallet" for configuration "None"
set_property(TARGET KF5::Wallet APPEND PROPERTY IMPORTED_CONFIGURATIONS NONE)
set_target_properties(KF5::Wallet PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_NONE "Qt5::DBus;Qt5::Widgets;KF5::WindowSystem;KF5::ConfigCore"
  IMPORTED_LOCATION_NONE "${_IMPORT_PREFIX}/lib64/libKF5Wallet.so.5.45.0"
  IMPORTED_SONAME_NONE "libKF5Wallet.so.5"
  )

list(APPEND _IMPORT_CHECK_TARGETS KF5::Wallet )
list(APPEND _IMPORT_CHECK_FILES_FOR_KF5::Wallet "${_IMPORT_PREFIX}/lib64/libKF5Wallet.so.5.45.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
