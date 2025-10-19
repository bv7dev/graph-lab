cmake_minimum_required(VERSION 3.20)

project(
    cppseed
    VERSION 0.1.0
    DESCRIPTION "A C++23 project template"
    LANGUAGES CXX C
)

# Export compile commands for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set preferred compiler
find_program(GCC_14_COMPILER g++-14)
if(GCC_14_COMPILER)
    set(CMAKE_CXX_COMPILER ${GCC_14_COMPILER} CACHE STRING "C++ compiler" FORCE)
    message(STATUS "Using g++-14: ${CMAKE_CXX_COMPILER}")
else()
    message(STATUS "g++-14 not found, using default compiler")
endif()

# Detect compiler features
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++23" COMPILER_SUPPORTS_CXX23)
if(COMPILER_SUPPORTS_CXX23)
    message(STATUS "Compiler supports C++23")
else()
    message(WARNING "Compiler may not fully support C++23")
endif()

# Debug-specific options
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
    add_compile_options(-O0 -g)
    # Enable additional debug information
    add_compile_options(-fno-omit-frame-pointer)
else()
    # Release-specific options
    add_compile_options(-O3 -DNDEBUG)
endif()


# ----- External dependencies ------------------------------------------------

# Fetch dependencies using FetchContent
include(FetchContent)

# Speed up FetchContent by not updating on every configure
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Disable updating on every configure")

# Cache FetchContent downloads to avoid re-downloading
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps" CACHE PATH "Base directory for FetchContent downloads")

# Enable parallel downloads
set(FETCHCONTENT_PARALLEL ON CACHE BOOL "Enable parallel downloads for FetchContent")

# Fetch doctest
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
)

# Make dependencies available
FetchContent_MakeAvailable(doctest)


# ----- Project source files -------------------------------------------------

# Automatically collect all source files from subdirectories (excluding apps)
file(GLOB_RECURSE LIBRARY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Remove app files from library sources
file(GLOB APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/apps/*.cpp")
list(REMOVE_ITEM LIBRARY_SOURCES ${APP_SOURCES})

message(STATUS "Library sources found: ${LIBRARY_SOURCES}")

# Automatically discover and build all applications in src/apps/
file(GLOB APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/apps/*.cpp")

if(NOT APP_SOURCES)
    message(WARNING "No application source files found in src/apps/")
endif()

# List to track all created executables for installation
set(APP_EXECUTABLES)


# ----- Application targets ----------------------------------------------------

foreach(APP_SOURCE ${APP_SOURCES})
    # Get the filename without extension (e.g., demo3d.cpp -> demo3d)
    get_filename_component(APP_NAME ${APP_SOURCE} NAME_WE)
    
    message(STATUS "Configuring application: ${APP_NAME}")
    
    # Create executable for this app
    add_executable(${APP_NAME}
        ${APP_SOURCE}
        ${LIBRARY_SOURCES}
    )
    
    # Include directories
    target_include_directories(${APP_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    # Apply strict compiler flags
    target_compile_options(${APP_NAME} PRIVATE 
        -Wall -Wextra -Wpedantic -Werror
    )
    
    # Link libraries
    target_link_libraries(${APP_NAME} PRIVATE
        doctest::doctest
    )
    
    # Set C++ standard
    target_compile_features(${APP_NAME} PRIVATE 
        cxx_std_23
    )
    
    # Add to list of executables for installation
    list(APPEND APP_EXECUTABLES ${APP_NAME})
endforeach()


# ----- Testing ---------------------------------------------------------------

# Testing - Auto-discover test files (like apps)
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()

    # Automatically discover and build all test executables in tests/
    file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp")
    
    if(NOT TEST_SOURCES)
        message(WARNING "No test source files found in tests/")
    endif()

    # List to track all created test executables
    set(TEST_EXECUTABLES)

    foreach(TEST_SOURCE ${TEST_SOURCES})
        # Get the filename without extension (e.g., test_math.cpp -> test_math)
        get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
        
        message(STATUS "Configuring test: ${TEST_NAME}")
        
        # Create executable for this test
        add_executable(${TEST_NAME}
            ${TEST_SOURCE}
            ${LIBRARY_SOURCES}
        )
        
        # Include directories
        target_include_directories(${TEST_NAME} PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        )
        
        # Link libraries (tests need all dependencies)
        target_link_libraries(${TEST_NAME} PRIVATE
            doctest::doctest
        )
        
        # Set C++ standard
        target_compile_features(${TEST_NAME} PRIVATE 
            cxx_std_23
        )
        
        # Register test with CTest
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
        
        # Add to list of test executables
        list(APPEND TEST_EXECUTABLES ${TEST_NAME})
    endforeach()
    
    message(STATUS "Test executables: ${TEST_EXECUTABLES}")
endif()
