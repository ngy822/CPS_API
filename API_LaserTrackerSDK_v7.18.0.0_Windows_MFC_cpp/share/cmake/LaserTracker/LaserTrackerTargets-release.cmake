#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "api::ltbackend" for configuration "Release"
set_property(TARGET api::ltbackend APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(api::ltbackend PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libltbackend.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/libltbackend.dll"
  )

list(APPEND _cmake_import_check_targets api::ltbackend )
list(APPEND _cmake_import_check_files_for_api::ltbackend "${_IMPORT_PREFIX}/lib/libltbackend.lib" "${_IMPORT_PREFIX}/bin/libltbackend.dll" )

# Import target "api::ltinterface" for configuration "Release"
set_property(TARGET api::ltinterface APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(api::ltinterface PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libltinterface.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "api::ltgui;api::ltbackend"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/libltinterface.dll"
  )

list(APPEND _cmake_import_check_targets api::ltinterface )
list(APPEND _cmake_import_check_files_for_api::ltinterface "${_IMPORT_PREFIX}/lib/libltinterface.lib" "${_IMPORT_PREFIX}/bin/libltinterface.dll" )

# Import target "api::ltgui" for configuration "Release"
set_property(TARGET api::ltgui APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(api::ltgui PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libltgui.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "Qt6::Widgets;Qt6::Core;api::ltbackend"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/libltgui.dll"
  )

list(APPEND _cmake_import_check_targets api::ltgui )
list(APPEND _cmake_import_check_files_for_api::ltgui "${_IMPORT_PREFIX}/lib/libltgui.lib" "${_IMPORT_PREFIX}/bin/libltgui.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
