cmake_minimum_required(VERSION 3.28)
project(MLReviewBackend VERSION 0.1.0 LANGUAGES CXX)
enable_testing()

option(ENABLE_SSL "Enable SSL connections" OFF)
option(WITH_CORS "Compile with CORS *" OFF)
option(UUSS_SRC "Enable UUSS-specific source" ON)
option(BUILD_TESTS "Compile unit tests" ON)
option(COMPILE_DATABASE_UTILITIES "Utilites for interacting with the datbase" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

include(CheckCXXCompilerFlag)
set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
if (NOT ${BUILD_SHARED_LIBS})
   set(Boost_USE_STATIC_LIBS ON) 
   set(OPENSSL_USE_STATIC_LIBS TRUE)
   set(GeographicLib_USE_STATIC_LIBS ON)
   set(LibXml2_USE_STATIC_LIBS TRUE)
   set(CURL_USE_STATIC_LIBS ON)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON) 
find_package(SOCI COMPONENTS Core PostgreSQL REQUIRED)
find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(GeographicLib REQUIRED)
find_package(Boost COMPONENTS program_options date_time iostreams REQUIRED)
#find_package(jwt-cpp 0.7.0...2.1.0 REQUIRED)
find_package(PostgreSQL REQUIRED)
#find_package(OpenLDAP REQUIRED)
find_package(CURL REQUIRED)
#find_package(MiniSEED REQUIRED)
find_package(libmseed REQUIRED)
find_package(uAuthenticator REQUIRED)
find_package(DataLinkClient)
if (${BUILD_TESTS})
   find_package(Catch2 3 REQUIRED)
endif()
find_package(SFF)
find_package(ZLIB)
if (${ENABLE_SSL})
   find_package(OpenSSL COMPONENTS SSL Crypto REQUIRED)
else()
   find_package(OpenSSL COMPONENTS SSL Crypto)
endif()
if (${OPENSSL_FOUND})
   message("Found SSL version ${OPENSSL_VERSION}")
endif()

find_package(Git)
if (Git_FOUND)
   execute_process(
     COMMAND ${GIT_EXECUTABLE} --no-pager log -n 1 HEAD "--pretty=format:%h"
     OUTPUT_VARIABLE mlReviewBackend_GITTAG
     RESULT_VARIABLE gitresult
     ERROR_QUIET
     OUTPUT_STRIP_TRAILING_WHITESPACE
   )   
   if (gitresult EQUAL 0)
      message(STATUS "Last Git commit: ${mlReviewBackend_GITTAG}")
   endif()
else()
   set(mlReviewBackend_GITTAG "") 
endif()
configure_file(${CMAKE_SOURCE_DIR}/src/version.hpp.in
               ${CMAKE_SOURCE_DIR}/include/mlReview/version.hpp)

set(HEADER_FILES
    include/mlReview/database/aqms/arrival.hpp
    include/mlReview/database/aqms/assocaro.hpp
    include/mlReview/database/aqms/credit.hpp
    include/mlReview/database/aqms/event.hpp
    include/mlReview/database/aqms/origin.hpp
    include/mlReview/database/connection/mongodb.hpp
    include/mlReview/database/connection/postgresql.hpp
    include/mlReview/database/machineLearning/arrival.hpp
    include/mlReview/database/machineLearning/event.hpp
    include/mlReview/database/machineLearning/origin.hpp
    include/mlReview/messages/error.hpp
    include/mlReview/messages/message.hpp
    include/mlReview/service/actions/acceptEventToAWS.hpp
    include/mlReview/service/actions/deleteEventFromAWS.hpp
    include/mlReview/service/catalog/arrival.hpp
    include/mlReview/service/catalog/event.hpp
    include/mlReview/service/catalog/magnitude.hpp
    include/mlReview/service/catalog/origin.hpp
    include/mlReview/service/catalog/resource.hpp
    include/mlReview/service/catalog/response.hpp
    include/mlReview/service/exceptions.hpp
    include/mlReview/service/handler.hpp
    include/mlReview/service/resource.hpp
    include/mlReview/service/stations/resource.hpp
    include/mlReview/service/stations/response.hpp
    include/mlReview/service/stations/station.hpp
    include/mlReview/service/waveforms/resource.hpp
    include/mlReview/service/waveforms/response.hpp
    include/mlReview/version.hpp
    include/mlReview/waveServer/client.hpp
    include/mlReview/waveServer/dataLink.hpp
    include/mlReview/waveServer/fdsn.hpp
    include/mlReview/waveServer/multiClient.hpp
    include/mlReview/waveServer/request.hpp
    include/mlReview/waveServer/segment.hpp
    include/mlReview/waveServer/waveform.hpp
    include/mlReview/webServer/listener.hpp)

set(LIBRARY_SRC
    src/version.cpp
    src/service/catalog/arrival.cpp
    src/service/catalog/event.cpp
    src/service/catalog/origin.cpp
    src/service/stations/station.cpp
    src/database/aqms/arrival.cpp
    src/database/aqms/assocaro.cpp
    src/database/aqms/credit.cpp
    src/database/aqms/event.cpp
    src/database/aqms/origin.cpp
    src/database/connection/mongodb.cpp
    src/database/connection/postgresql.cpp
    src/database/machineLearning/arrival.cpp
    src/database/machineLearning/event.cpp
    src/database/machineLearning/origin.cpp
    src/waveServer/client.cpp
    src/waveServer/fdsn.cpp
    src/waveServer/multiClient.cpp
    src/waveServer/request.cpp
    src/waveServer/segment.cpp
    src/waveServer/waveform.cpp)
if (${DataLinkClient_FOUND})
   message("Found DataLinkClient library")
   set(HEADER_FILES ${HEADER_FILES} include/mlReview/waveServer/dataLink.hpp)
   set(LIBRARY_SRC  ${LIBRARY_SRC}  src/waveServer/dataLink.cpp)
endif()
if (${BUILD_SHARED_LIBS})
   add_library(mlReview SHARED ${LIBRARY_SRC})
else()
   add_library(mlReview STATIC ${LIBRARY_SRC})
endif()
target_link_libraries(mlReview
                      PRIVATE SOCI::Core SOCI::PostgreSQL
                              GeographicLib::GeographicLib
                              nlohmann_json::nlohmann_json
                              LibXml2::LibXml2
                              CURL::libcurl
                              Boost::headers)
#                              ${LDAP_LIBRARY} ${LBER_LIBRARY})
if (${DataLinkClient_FOUND})
   target_link_libraries(mlReview PRIVATE DataLinkClient::DataLinkClient)
endif()
if (${BUILD_SHARED_LIBS})
   target_link_libraries(mlReview PRIVATE mseed::mseed)
   target_link_libraries(mlReview PRIVATE mongo::mongocxx_shared mongo::bsoncxx_shared)
else()
   target_link_libraries(mlReview PRIVATE mseed::mseed_static)
   target_link_libraries(mlReview PRIVATE mongo::mongocxx_static mongo::bsoncxx_static)
endif()
target_include_directories(mlReview
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
                                   ${CMAKE_CURRENT_SOURCE_DIR}/include
                                   ${PostgreSQL_INCLUDE_DIRS})
set_target_properties(mlReview PROPERTIES
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES
                      CXX_EXTENSIONS NO)
if (${SFF_FOUND})
   message("Found SFF")
   target_compile_definitions(mlReview PRIVATE WITH_SFF)
   target_include_directories(mlReview PRIVATE ${SFF_INCLUDE_DIRECTORY})
   target_link_libraries(mlReview PRIVATE ${SFF_LIBRARY})
endif()
#string(COMPARE GREATER_EQUAL ${MINISEED_VERSION} "3.1.5" USE_MS_VERSION_315)
if (USE_MS_VERSION_315)
   message("Using ms_sid2nslc_n")
   target_compile_definitions(mlReview PRIVATE USE_MS_VERSION_315)
endif()
target_sources(mlReview
               PUBLIC FILE_SET
                      HEADERS
                      BASE_DIRS
                         include
                      FILES
                         ${HEADER_FILES}
               )   



add_executable(mlReviewBackend
               src/main.cpp
#               src/database/postgresql.cpp
               src/service/handler.cpp
               src/service/resource.cpp
               src/service/actions/acceptEventToAWS.cpp
               src/service/actions/deleteEventFromAWS.cpp
               src/service/catalog/resource.cpp
               src/service/catalog/response.cpp
               src/service/stations/resource.cpp
               src/service/stations/response.cpp
               src/service/waveforms/resource.cpp
               src/service/waveforms/response.cpp
               src/messages/message.cpp
               src/messages/error.cpp
               #src/waveServer/client.cpp
               #src/waveServer/fdsn.cpp
               #src/waveServer/request.cpp
               #src/waveServer/segment.cpp
               #src/waveServer/waveform.cpp
               src/webServer/listener.cpp)
#               src/ldap.cpp
#               src/listener.cpp
#               src/callback.cpp
#               src/authenticator.cpp
#               src/permissions.cpp)

target_link_libraries(mlReviewBackend
                      PUBLIC nlohmann_json::nlohmann_json
                      PRIVATE mlReview
                              SOCI::PostgreSQL SOCI::Core
                              Boost::date_time
                              Boost::program_options
                              Boost::iostreams
                              spdlog::spdlog_header_only
                              GeographicLib::GeographicLib
                              uAuthenticator::libuAuthenticator
                              #${uAuthenticator_LIBRARY}
                              CURL::libcurl
                              #${LDAP_LIBRARY} ${LBER_LIBRARY}
                              )
if (${BUILD_SHARED_LIBS})
   target_link_libraries(mlReviewBackend PRIVATE mongo::mongocxx_shared mongo::bsoncxx_shared)
   target_link_libraries(mlReviewBackend PRIVATE mseed::mseed)
else()
   target_link_libraries(mlReviewBackend PRIVATE mongo::mongocxx_static mongo::bsoncxx_static)
   target_link_libraries(mlReviewBackend PRIVATE mseed::mseed_static)
endif()
target_include_directories(mlReviewBackend
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
                                   ${CMAKE_CURRENT_SOURCE_DIR}/include
                                   ${PostgreSQL_INCLUDE_DIRS}
                                   ${OPENLDAP_INCLUDE_DIR}
                                   Boost::headers)
if (${SFF_FOUND})
   target_link_libraries(mlReviewBackend PRIVATE ${SFF_LIBRARY})
   target_link_directories(mlReviewbackend PRIVATE ${SFF_INCLUDE_DIR})
endif()
set_target_properties(mlReviewBackend PROPERTIES
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES 
                      CXX_EXTENSIONS NO) 
target_compile_definitions(mlReviewBackend PUBLIC ENABLE_CORS)
if (${OPENSSL_FOUND})
   message("Linking to SSL")
   target_compile_definitions(mlReviewBackend PUBLIC WITH_OPENSSL)
   target_link_libraries(mlReviewBackend PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()
if (${ZLIB_FOUND})
   message("Linking to libz")
   target_compile_definitions(mlReviewBackend PUBLIC WITH_ZLIB)
   target_link_libraries(mlReviewBackend PRIVATE ZLIB::ZLIB)
   #target_include_directories(mlReviewBackend PRIVATE ${ZLIB_INCLUDE_DIRS})
endif()

#if (${COMPILE_DATABASE_UTILITIES})
#  add_executable(eventBuilder
#                 src/eventBuilder.cpp)
#  target_link_libraries(eventBuilder
#                        PRIVATE mlReview 
#                                SOCI::PostgreSQL SOCI::Core
#                                Boost::date_time
#                                Boost::program_options
#                                spdlog::spdlog
#                                nlohmann_json::nlohmann_json)
#  if (${BUILD_SHARED_LIBS})
#     target_link_libraries(eventBuilder PRIVATE mongo::mongocxx_shared mongo::bsoncxx_shared)
#  else()
#     target_link_libraries(eventBuilder PRIVATE mongo::mongocxx_static mongo::bsoncxx_static)
#  endif()
#  target_include_directories(eventBuilder
#                             PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
#                                     ${CMAKE_CURRENT_SOURCE_DIR}/include
#                                     ${PostgreSQL_INCLUDE_DIRS}
#                                     mseed::mseed
#                                     Boost::headers)
#  set_target_properties(eventBuilder PROPERTIES
#                        CXX_STANDARD 20
#                        CXX_STANDARD_REQUIRED YES 
#                        CXX_EXTENSIONS NO) 

#  add_executable(createEvent
#                 src/createEvent.cpp)
#  target_link_libraries(createEvent
#                        PRIVATE mlReview
#                                SOCI::PostgreSQL SOCI::Core
#                                Boost::date_time
#                                Boost::program_options
#                                spdlog::spdlog
#                                nlohmann_json::nlohmann_json
#                                mongo::mongocxx_shared mongo::bsoncxx_shared)
#  target_include_directories(createEvent
#                             PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
#                                     ${CMAKE_CURRENT_SOURCE_DIR}/include
#                                     ${PostgreSQL_INCLUDE_DIRS}
#                                     Boost::headers)
#  set_target_properties(createEvent PROPERTIES
#                        CXX_STANDARD 20
#                        CXX_STANDARD_REQUIRED YES 
#                        CXX_EXTENSIONS NO) 
#endif()

##########################################################################################
#                                        Testing                                         #
##########################################################################################

if (PROJECT_IS_TOP_LEVEL AND ${BUILD_TESTS})
   message("Will build unit tests")

   set(TEST_SRC
       testing/database/aqms/arrival.cpp
       testing/database/aqms/assocaro.cpp
       testing/database/aqms/credit.cpp
       testing/database/aqms/event.cpp
       testing/database/aqms/origin.cpp
       testing/database/machineLearning/arrival.cpp
       testing/database/machineLearning/event.cpp
       testing/database/machineLearning/origin.cpp
       testing/service/catalog/arrival.cpp
       testing/service/catalog/event.cpp
       testing/service/catalog/origin.cpp
       testing/service/stations/station.cpp
       testing/waveServer/request.cpp
       testing/waveServer/segment.cpp
       testing/waveServer/waveform.cpp)
   add_executable(unitTests ${TEST_SRC})
   target_compile_definitions(unitTests
                              PRIVATE TESTING_DATA_DIR="${CMAKE_SOURCE_DIR}/testing/data")
   set_target_properties(unitTests PROPERTIES
                         CXX_STANDARD 23
                         CXX_STANDARD_REQUIRED YES
                         CXX_EXTENSIONS NO)
   target_link_libraries(unitTests
                         PRIVATE
                            mlReview
                            nlohmann_json::nlohmann_json
                            spdlog::spdlog_header_only
                            Threads::Threads
                            Catch2::Catch2 Catch2::Catch2WithMain)
   target_include_directories(unitTests
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
                              )
   add_test(NAME unitTests
            COMMAND $<TARGET_FILE:unitTests>)
endif()

##########################################################################################
#                                        Containers                                      #
##########################################################################################
include(CPackComponent)
if (${BUILD_CONTAINER})
   include(CPackComponent)
   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/Dockerfile.u24.in
                  ${CMAKE_CURRENT_BINARY_DIR}/Dockerfile.u24 @ONLY)
   add_custom_target(${PROJECT_NAME}-u24
                     COMMENT "Creating container file for ${PROJECT_NAME}"
                     COMMAND ${CMAKE_CPACK_COMMAND} -G TGZ 
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} VERBATIM)
   add_dependencies(${PROJECT_NAME}-u24 ${binaries} mlReview)
   find_program(buildah_EXECUTABLE buildah)
   if (buildah_EXECUTABLE)
      add_custom_target(buildah
                        COMMENT "Preparing the container with buildah"
                        COMMAND ${buildah_EXECUTABLE} bud -t mlreviewbackend:${PROJECT_VERSION} -t mlreviewbackend:latest -f ./Dockerfile.u24
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} VERBATIM)
      add_dependencies(buildah ${PROJECT_NAME}-u24)
   endif()
endif()



##########################################################################################
#                                      Installation                                      #
##########################################################################################
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
configure_file(${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
               ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake @ONLY)
#configure_package_config_file(
#    cmake/${PROJECT_NAME}.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
#    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
#write_basic_package_version_file(
#    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
#    VERSION "${version}"
#    COMPATIBILITY AnyNewerVersion
#)
install(TARGETS mlReview
        EXPORT ${PROJECT_NAME}Targets
        FILE_SET HEADERS
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        COMPONENT libraries)
install(TARGETS mlReviewBackend
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT applications)
export(EXPORT ${PROJECT_NAME}Targets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")
install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
#install(FILES
#        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
#        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
#        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})



##########################################################################################
#                                     CPACK Packaging                                    #
##########################################################################################
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "UUSS")
set(CPACK_PACKAGE_CONTACT "ben.baker@utah.edu")
set(CPACK_PACKAGE_LICENSE "MIT")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The backend API for the duty review page.")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_COMPONENTS_ALL executables)
if (WIN32)
    set(CPACK_GENERATOR ZIP WIX)
elseif (APPLE)
    set(CPACK_GENERATOR TGZ productbuild)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CPACK_GENERATOR TGZ RPM)
else()
    set(CPACK_GENERATOR TGZ)
endif()
set(CPACK_SOURCE_IGNORE_FILES
  /\\.git/
  \\.swp
  \\.orig
  /CMakeLists\\.txt\\.user
  /private/
)
include(CPack) # Put this last!

