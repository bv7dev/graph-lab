cmake_minimum_required(VERSION 3.20)

project(
    graph-lab
    VERSION 0.1.0
    DESCRIPTION "Visualizing graphs with OpenGL"
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


# ----- Fetch External dependencies ------------------------------------------------

# Fetch dependencies using FetchContent
include(FetchContent)

# Speed up FetchContent by not updating on every configure
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Disable updating on every configure")

# Cache FetchContent downloads to avoid re-downloading
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps" CACHE PATH "Base directory for FetchContent downloads")

# Enable parallel downloads
set(FETCHCONTENT_PARALLEL ON CACHE BOOL "Enable parallel downloads for FetchContent")

# Disable unnecessary GLFW features to speed up configuration and build
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

# Fetch GLAD generator
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
)

# Fetch doctest
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
)

# Disable unnecessary GLM features to speed up configuration
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

# Fetch GLM (header-only, no build required)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

# Fetch ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.4
)

# Fetch tinygltf (header-only glTF loader)
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v2.9.3
)

# Make dependencies available
FetchContent_MakeAvailable(glfw glad doctest glm imgui tinygltf)

# ----- Setup external libraries ---------------------------------------------

# Disable strict warnings for GLFW (it has many unused parameters)
target_compile_options(glfw PRIVATE 
    -Wno-unused-parameter 
    -Wno-sign-compare 
    -Wno-missing-field-initializers 
    -Wno-pedantic
    -Wno-error
)

# Generate GLAD files for OpenGL 3.3 Core
set(GLAD_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/glad")
file(MAKE_DIRECTORY ${GLAD_OUTPUT_DIR})

# Generate GLAD files using the glad generator (only if not already generated)
if(NOT EXISTS "${GLAD_OUTPUT_DIR}/src/glad.c" OR NOT EXISTS "${GLAD_OUTPUT_DIR}/include/glad/glad.h")
    # Only find Python when we need to generate GLAD files
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    
    message(STATUS "Generating GLAD files...")
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -m glad --generator=c --spec=gl --api=gl=3.3 --profile=core --out-path=${GLAD_OUTPUT_DIR}
        WORKING_DIRECTORY ${glad_SOURCE_DIR}
        RESULT_VARIABLE GLAD_GENERATION_RESULT
    )

    if(NOT GLAD_GENERATION_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to generate GLAD files")
    endif()
    message(STATUS "GLAD files generated successfully")
else()
    message(STATUS "GLAD files already exist, skipping generation")
endif()

# Create GLAD library (C library)
add_library(glad_generated
    ${GLAD_OUTPUT_DIR}/src/glad.c
)

target_include_directories(glad_generated PUBLIC ${GLAD_OUTPUT_DIR}/include)

# Set C standard for GLAD
set_target_properties(glad_generated PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

# Disable strict warnings for GLAD (it has many function pointer conversions)
set_target_properties(glad_generated PROPERTIES
    COMPILE_FLAGS "-Wno-pedantic -Wno-error -Wno-unused-parameter -Wno-sign-compare -Wno-missing-field-initializers"
)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Create ImGui library
add_library(imgui_lib
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui_lib PUBLIC 
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui_lib PUBLIC glfw glad_generated)

# Disable strict warnings for ImGui
target_compile_options(imgui_lib PRIVATE 
    -Wno-unused-parameter 
    -Wno-sign-compare 
    -Wno-missing-field-initializers
    -Wno-pedantic
    -Wno-error
)

# Create tinygltf wrapper library (header-only but with implementation in one file)
# This allows us to disable warnings for the tinygltf implementation
add_library(tinygltf_impl STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/util/gltf_loader.cpp)

target_include_directories(tinygltf_impl PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${tinygltf_SOURCE_DIR}
)

target_link_libraries(tinygltf_impl PUBLIC glm::glm)

# Disable strict warnings for tinygltf and its embedded STB libraries
target_compile_options(tinygltf_impl PRIVATE 
    -Wno-unused-parameter 
    -Wno-sign-compare 
    -Wno-missing-field-initializers
    -Wno-pedantic
    -Wno-error
    -Wno-stringop-overflow
)

target_compile_features(tinygltf_impl PRIVATE cxx_std_23)

# ----- Project source files -------------------------------------------------

# Automatically collect all source files from subdirectories (excluding apps)
file(GLOB_RECURSE LIBRARY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Remove app files and gltf_loader.cpp from library sources
file(GLOB APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/apps/*.cpp")
list(REMOVE_ITEM LIBRARY_SOURCES ${APP_SOURCES})
list(REMOVE_ITEM LIBRARY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/util/gltf_loader.cpp")

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
        ${glfw_SOURCE_DIR}/include
        ${GLAD_OUTPUT_DIR}/include
        ${tinygltf_SOURCE_DIR}
    )
    
    # Apply strict compiler flags
    target_compile_options(${APP_NAME} PRIVATE 
        -Wall -Wextra -Wpedantic -Werror
    )
    
    # Link libraries
    target_link_libraries(${APP_NAME} PRIVATE
        OpenGL::GL
        glm::glm
        glad_generated
        tinygltf_impl
        imgui_lib
        glfw
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
            ${doctest_SOURCE_DIR}/doctest
            ${glfw_SOURCE_DIR}/include
            ${GLAD_OUTPUT_DIR}/include
            ${tinygltf_SOURCE_DIR}
        )
        
        # Link libraries
        target_link_libraries(${TEST_NAME} PRIVATE
            doctest::doctest
            OpenGL::GL
            glm::glm
            glad_generated
            tinygltf_impl
            imgui_lib
            glfw
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
