cmake_minimum_required(VERSION 3.20)
project(kessler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # lets clangd / your editor understand the project

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(KESSLER_BUILD_TESTS "Build unit tests" ON)
option(KESSLER_SANITIZE    "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)

# ---- Strict warnings, applied ONLY to our own code (not third-party code) ----
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


# ---- Core library ----
add_library(kessler_core
  src/tle.cpp
  src/time.cpp
  src/sgp4_elements.cpp
)
target_include_directories(kessler_core PUBLIC include)
target_link_libraries(kessler_core PRIVATE kessler_warnings)

# ---- Vallado reference SGP4 + our propagator (only once SGP4.cpp is added) ----
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/vallado/SGP4.cpp)
  add_library(vallado_sgp4 third_party/vallado/SGP4.cpp)
  target_include_directories(vallado_sgp4 SYSTEM PUBLIC third_party/vallado)
  target_compile_options(vallado_sgp4 PRIVATE -w)  # don't show warnings from third-party code

  add_library(kessler_propagation src/propagator.cpp)
  target_link_libraries(kessler_propagation PUBLIC kessler_core vallado_sgp4 PRIVATE kessler_warnings)
  set(KESSLER_HAVE_VALLADO ON)
else()
  message(STATUS "third_party/vallado/SGP4.cpp not found: propagation disabled")
endif()

# ---- Command-line app ----
add_executable(kessler apps/kessler_cli.cpp)
target_link_libraries(kessler PRIVATE kessler_core kessler_warnings)

# ---- Tests ----
if(KESSLER_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(Catch2)

  enable_testing()
  add_executable(kessler_tests
    tests/test_tle.cpp
    tests/test_time.cpp
    tests/test_sgp4_elements.cpp
  )
  target_link_libraries(kessler_tests PRIVATE kessler_core kessler_warnings Catch2::Catch2WithMain)

  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(Catch)
  catch_discover_tests(kessler_tests)
endif()

# ---- Differential verification vs. Vallado's own pipeline ----
if(KESSLER_BUILD_TESTS AND KESSLER_HAVE_VALLADO)
  add_executable(verify_vallado tests/verify_vallado.cpp)
  target_link_libraries(verify_vallado PRIVATE kessler_propagation kessler_warnings)
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/SGP4-VER.TLE)
    add_test(NAME vallado_differential_improved
             COMMAND verify_vallado ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/SGP4-VER.TLE i)
    add_test(NAME vallado_differential_afspc
             COMMAND verify_vallado ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/SGP4-VER.TLE a)
    set_tests_properties(vallado_differential_improved vallado_differential_afspc
      PROPERTIES ENVIRONMENT "ASAN_OPTIONS=replace_str=0")
  endif()
endif()
