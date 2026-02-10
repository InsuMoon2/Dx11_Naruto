#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Microsoft::Effects11" for configuration "Debug"
set_property(TARGET Microsoft::Effects11 APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(Microsoft::Effects11 PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/Effects11.lib"
  )

list(APPEND _cmake_import_check_targets Microsoft::Effects11 )
list(APPEND _cmake_import_check_files_for_Microsoft::Effects11 "${_IMPORT_PREFIX}/debug/lib/Effects11.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
