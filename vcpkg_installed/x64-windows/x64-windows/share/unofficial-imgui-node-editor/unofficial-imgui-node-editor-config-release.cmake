#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "unofficial::imgui-node-editor::imgui-node-editor" for configuration "Release"
set_property(TARGET unofficial::imgui-node-editor::imgui-node-editor APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(unofficial::imgui-node-editor::imgui-node-editor PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/imgui-node-editor.lib"
  )

list(APPEND _cmake_import_check_targets unofficial::imgui-node-editor::imgui-node-editor )
list(APPEND _cmake_import_check_files_for_unofficial::imgui-node-editor::imgui-node-editor "${_IMPORT_PREFIX}/lib/imgui-node-editor.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
