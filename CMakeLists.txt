#
# Copyright (c) 2025-present Henri Michelon
#
# This software is released under the MIT License.
# https://opensource.org/licenses/MIT
#
cmake_minimum_required(VERSION 3.29)

#######################################################
project(lysa_ecs)
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.env.cmake")
    message(FATAL_ERROR "Please create a .env.cmake file with the LYSA_PROJECT_DIR variable")
endif ()
include(.env.cmake)
if (NOT DEFINED LYSA_PROJECT_DIR)
    message(FATAL_ERROR "Please set LYSA_PROJECT_DIR in the .env.cmake file")
endif()
set(LUA_BINDING ON)

#######################################################
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF)
endif()
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if(WIN32)
    add_compile_definitions(UNICODE _UNICODE)
endif ()

#######################################################
set(LYSA_TARGET "lysa")
add_subdirectory(${LYSA_PROJECT_DIR} external_lib_build/${LYSA_TARGET})
set(LYSA_SRC_DIR ${LYSA_PROJECT_DIR}/src)

#######################################################
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

#######################################################
function(build_target TARGET_NAME SRCS MODULES)
    message("Building target ${TARGET_NAME}")
    add_library(${TARGET_NAME} ${SRCS})
    target_sources(${TARGET_NAME}
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
            ${COMMON_MODULES}
            ${MODULES}
    )
    lysa_compile_options(${TARGET_NAME})
    if (MSVC)
        target_precompile_headers(${TARGET_NAME} PRIVATE
                "<intrin.h>"
        )
    endif()
    target_include_directories(${TARGET_NAME} PUBLIC ${SRC_DIR}/depends/flecs)
    target_link_libraries(${TARGET_NAME} ${LYSA_TARGET} std-cxx-modules)
    if(WIN32)
        target_compile_definitions(${TARGET_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
        if(MINGW)
            target_link_options(${TARGET_NAME} PRIVATE "-mwindows")
        endif()
    endif()
endfunction()

#######################################################
if(LUA_BINDING)
    message("Building Lua bindings for Lysa ECS")
    add_compile_definitions(LUA_BINDING)
    set(LUA_BINDINGS_SRC
            ${SRC_DIR}/bindings/lua/LuaBindings.cpp
    )
    set(LUA_BINDINGS_MODULES
            ${SRC_DIR}/bindings/lua/LuaBindings.ixx
    )
endif ()

#######################################################
set(LYSA_ECS_SRC
        ${SRC_DIR}/ecs/components/Components.cpp
        ${SRC_DIR}/ecs/components/Transform.cpp
        ${SRC_DIR}/ecs/systems/Systems.cpp
        ${SRC_DIR}/ecs/systems/Transform.cpp
        ${SRC_DIR}/depends/flecs/src/flecs.c
        ${LUA_BINDINGS_SRC}
       )
set(LYSA_ECS_MODULES
        ${SRC_DIR}/ecs/ECS.ixx
        ${SRC_DIR}/ecs/Flecs.ixx
        ${SRC_DIR}/ecs/components/Components.ixx
        ${SRC_DIR}/ecs/components/Transform.ixx
        ${SRC_DIR}/ecs/systems/Systems.ixx
        ${LUA_BINDINGS_MODULES}
       )
build_target(${PROJECT_NAME} "${LYSA_ECS_SRC}" "${LYSA_ECS_MODULES}")
