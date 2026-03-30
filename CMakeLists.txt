cmake_minimum_required(VERSION 3.20)

message(STATUS "Running under CMake v" ${CMAKE_VERSION})


function(np_prepare_target target)
    if(MSVC)
        target_compile_definitions(${target} PUBLIC
            UNICODE
            _UNICODE
        )
        target_compile_options(${target} PUBLIC
            /DWIN32
            /EHsc
            /nologo
            /GR
            /W4
            /MP
            /permissive-
            /Zc:__cplusplus
            /wd4127
            /wd4267
            /wd4244
            /wd4455
            /wd4996
        )
    else()

    endif()

endfunction()

#####################################################################

function(np_add_compiler_warnings target)

    if(NOT MSVC)
        target_compile_options(${target} PRIVATE
            -ftabstop=2
            -pedantic
            -Wformat=2
            -W
            -Wall
            -Werror
            -Wextra
            -Wconversion

            $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_EQUAL:$<CXX_COMPILER_VERSION>,13.1.0>>:-Wno-dangling-reference>
            $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_EQUAL:$<CXX_COMPILER_VERSION>,13.1.0>>:-Wno-maybe-uninitialized> # This is temporarily masking a warning that hasn't been investigated but should be
            $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_EQUAL:$<CXX_COMPILER_VERSION>,13.1.0>>:-Wno-null-dereference> # This is temporarily masking a warning that hasn't been investigated but should be
            $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_GREATER:$<CXX_COMPILER_VERSION>,7.0.0>>:-Wduplicated-branches>
            $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_GREATER:$<CXX_COMPILER_VERSION>,6.0.0>>:-Wduplicated-cond>
            $<$<AND:$<CXX_COMPILER_ID:Clang>,$<VERSION_GREATER:$<CXX_COMPILER_VERSION>,3.7.0>>:-Wmove>
            -Wimplicit-fallthrough
            -Wno-deprecated-declarations
            -Wnon-virtual-dtor
            $<$<AND:$<CXX_COMPILER_ID:GNU>,$<AND:$<VERSION_GREATER:$<CXX_COMPILER_VERSION>,6.0.0>,$<NOT:$<VERSION_EQUAL:$<CXX_COMPILER_VERSION>,13.1.0>>>>:-Wnull-dereference> # This is temporarily masking a warning that hasn't been investigated but should be
            $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_GREATER:$<CXX_COMPILER_VERSION>,7.0.0>>:-Wrestrict>
            -Wshadow
            -Wsign-compare
            # -Wcast-qual
            # -Wold-style-cast
            # -Wsign-conversion
            -Wno-cast-qual
            -Wno-old-style-cast
            -Wno-sign-conversion
            -Wno-shorten-64-to-32
        )

    endif()

endfunction()

#####################################################################

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12" CACHE STRING "Minimum OS X deployment version")
endif()

#####################################################################

project(PeriodicLoops VERSION 0.0.1 LANGUAGES C CXX)

# Add the local modules directory to pick up local Find-module files
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

find_package(JUCE CONFIG REQUIRED)

# Disallow cycles
set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_NO_CYCLES ON)

# Specify using standard C++20 throughout
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS "OFF")

enable_testing()

add_subdirectory(src)

