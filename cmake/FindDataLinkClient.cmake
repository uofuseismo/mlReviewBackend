# Already in cache, be silent
if (DataLinkClient_INCLUDE_DIR AND DataLinkClient_LIBRARY)
    set(DataLinkClient_FIND_QUIETLY TRUE)
endif()

# Find the include directory
find_path(DataLinkClient_INCLUDE_DIR
          NAMES libdali.h
          HINTS $ENV{DataLinkClient_ROOT}/include
                $ENV{DataLinkClient_ROOT}/
                /usr/local/include)
# Find the library components
if (${BUILD_SHARED_LIBS})
   find_library(DataLinkClient_LIBRARY
                NAMES libdali.so
                PATHS $ENV{DataLink}/lib/
                      $ENV{DataLink}/
                      /usr/local/lib64
                      /usr/local/lib
               )
else()
   find_library(DataLinkClient_LIBRARY
                NAMES libdali.a
                PATHS $ENV{DataLink}/lib/
                      $ENV{DataLink}/
                      /usr/local/lib64
                      /usr/local/lib
               )   
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DataLinkClient
                                  FOUND_VAR DataLinkClient_FOUND
                                  REQUIRED_VARS DataLinkClient_INCLUDE_DIR DataLinkClient_LIBRARY)
if (DataLinkClient_FOUND AND NOT TARGET DataLinkClient::DataLinkClient)
   add_library(DataLinkClient::DataLinkClient UNKNOWN IMPORTED)
   set_target_properties(DataLinkClient::DataLinkClient PROPERTIES
                         IMPORTED_LOCATION "${DataLinkClient_LIBRARY}"
                         INTERFACE_INCLUDE_DIRECTORIES "${DataLinkClient_INCLUDE_DIR}")
endif()
mark_as_advanced(DataLinkClient_INCLUDE_DIR DataLinkClient_LIBRARY)
