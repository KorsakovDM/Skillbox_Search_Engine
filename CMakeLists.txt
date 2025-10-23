cmake_minimum_required(VERSION 3.21)

# ---- Project ----
project(search_engine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Warnings
if(MSVC)
  add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Paths
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(INC_DIR "${CMAKE_SOURCE_DIR}/include")
set(RES_DIR "${CMAKE_SOURCE_DIR}/resources")

# ---- Dependencies ----
include(FetchContent)

# nlohmann/json
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# GoogleTest (use modern release to avoid old cmake issues)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(CTest)
enable_testing()

# ---- Library ----
add_library(search_engine
  ${SRC_DIR}/ConverterJSON.cpp
  ${SRC_DIR}/InvertedIndex.cpp
  ${SRC_DIR}/SearchServer.cpp
)
target_include_directories(search_engine PUBLIC ${INC_DIR})
target_link_libraries(search_engine PUBLIC nlohmann_json::nlohmann_json)

# ---- App ----
add_executable(app_search_engine
  ${SRC_DIR}/main.cpp
)
target_link_libraries(app_search_engine PRIVATE search_engine)

# Common output dirs
function(set_outputs tgt)
  get_target_property(_type ${tgt} TYPE)
  if(_type STREQUAL "EXECUTABLE")
    set_target_properties(${tgt} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/bin/Debug"
      RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
    )
  elseif(_type STREQUAL "STATIC_LIBRARY" OR _type STREQUAL "SHARED_LIBRARY")
    set_target_properties(${tgt} PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/bin/Debug"
      ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
      LIBRARY_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/bin/Debug"
      LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
    )
  endif()
endfunction()

set_outputs(search_engine)
set_outputs(app_search_engine)

# ---- Tests ----
add_executable(search_engine_tests
  ${CMAKE_SOURCE_DIR}/tests/sample_test.cpp
  ${CMAKE_SOURCE_DIR}/tests/InvertedIndex_test.cpp
  ${CMAKE_SOURCE_DIR}/tests/SearchServer_test.cpp
)
target_link_libraries(search_engine_tests PRIVATE search_engine gtest_main)
set_outputs(search_engine_tests)

include(GoogleTest)
gtest_discover_tests(search_engine_tests)
