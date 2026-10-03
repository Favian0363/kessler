cmake_minimum_required(VERSION 3.20)
project(kessler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(KESSLER_BUILD_TESTS "Build unit tests" ON)
option(KESSLER_SANITIZE    "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)

add_library(kessler_warnings INTERFACE)
if(MSVC)
  target_compile_options(kessler_warnings INTERFACE /W4 /permissive-)
else()
  target_compile_options(kessler_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
endif()

if(KESSLER_SANITIZE AND NOT MSVC)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/vallado/SGP4.cpp)
  add_library(vallado_sgp4 third_party/vallado/SGP4.cpp)
  target_include_directories(vallado_sgp4 SYSTEM PUBLIC third_party/vallado)
  set(KESSLER_HAVE_VALLADO ON)
endif()

# ---- Core library ----
add_library(kessler_core
  src/tle.cpp
)
target_include_directories(kessler_core PUBLIC include)
target_link_libraries(kessler_core PRIVATE kessler_warnings)

# ---- Command-line app ----
add_executable(kessler apps/kessler_cli.cpp)
target_link_libraries(kessler PRIVATE kessler_core kessler_warnings)

# ---- Tests ----
if(KESSLER_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1)
  FetchContent_MakeAvailable(Catch2)

  enable_testing()
  add_executable(kessler_tests
    tests/test_tle.cpp
  )
  target_link_libraries(kessler_tests PRIVATE kessler_core kessler_warnings Catch2::Catch2WithMain)

  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(Catch)
  catch_discover_tests(kessler_tests)
endif()
