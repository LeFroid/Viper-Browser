#ckwg +4
# Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
# KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
# Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.

# Locate the system installed SQLite3
# The following variables will be set:
#
# SQLite3_FOUND       - Set to true if SQLite3 can be found
# SQLite3_INCLUDE_DIR - The path to the SQLite3 header files
# SQLite3_LIBRARY     - The full path to the SQLite3 library

if( SQLite3_DIR )
  if( SQLite3_FIND_VERSION )
    find_package( SQLite3 ${SQLite3_FIND_VERSION} NO_MODULE)
  else()
    find_package( SQLite3 NO_MODULE)
  endif()
elseif( NOT SQLite3_FOUND )
  message(STATUS "Searching for sqlite3.h")
  find_path( SQLite3_INCLUDE_DIR sqlite3.h )
  message(STATUS "Searching for sqlite3.h - ${SQLite3_INCLUDE_DIR}")
                    
  message(STATUS "Searching for libsqlite3")
  find_library( SQLite3_LIBRARY sqlite3 )
  message(STATUS "Searching for libsqlite3 - ${SQLite3_LIBRARY}")
  
  include( FindPackageHandleStandardArgs )
  FIND_PACKAGE_HANDLE_STANDARD_ARGS( SQLite3 SQLite3_INCLUDE_DIR SQLite3_LIBRARY )
  if( SQLITE3_FOUND )
    # Determine the SQLite version found
    file( READ ${SQLite3_INCLUDE_DIR}/sqlite3.h SQLite3_INCLUDE_FILE )
    string( REGEX REPLACE
      ".*# *define *SQLITE_VERSION *\\\"([0-9\\.]+)\\\".*" "\\1"
      SQLite3_VERSION "${SQLite3_INCLUDE_FILE}" )
    string( REGEX REPLACE 
      "([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" 
      SQLite3_VERSION_MAJOR "${SQLite3_VERSION}" )
    string( REGEX REPLACE 
      "[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" 
      SQLite3_VERSION_MINOR "${SQLite3_VERSION}" )
    string( REGEX REPLACE 
      "[0-9]+\\.[0-9]+\\.([0-9])+" "\\1" 
      SQLite3_VERSION_PATCH "${SQLite3_VERSION}" )

    # Determine version compatibility
    if( SQLite3_FIND_VERSION )
      if( SQLite3_FIND_VERSION_EXACT )
        if( SQLite3_FIND_VERSION VERSION_EQUAL SQLite3_VERSION )
          message( STATUS "SQLite3 version: ${SQLite3_VERSION}" )
          set( SQLite3_FOUND TRUE )
        endif()
      else()
        if( (SQLite3_FIND_VERSION VERSION_LESS  SQLite3_VERSION) OR
            (SQLite3_FIND_VERSION VERSION_EQUAL SQLite3_VERSION) )
            message( STATUS "SQLite3 version: ${SQLite3_VERSION}" )
          set( SQLite3_FOUND TRUE )
        endif()
      endif()
    else()
      message( STATUS "SQLite3 version: ${SQLite3_VERSION}" )
      set( SQLite3_FOUND TRUE )
    endif()
    unset( SQLITE3_FOUND )
  endif()
endif()
