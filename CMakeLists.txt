cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif()

project(cnpy LANGUAGES CXX)

find_package(ZLIB REQUIRED)

add_library(cnpy STATIC cnpy.cpp)
add_library(cnpy::cnpy ALIAS cnpy)
target_link_libraries(cnpy PUBLIC ZLIB::ZLIB)
target_compile_features(cnpy PUBLIC cxx_std_11)
target_include_directories(
    cnpy
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

if(NOT DEFINED CNPY_INSTALL)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(CNPY_INSTALL TRUE)
    else()
        set(CNPY_INSTALL FALSE)
    endif()
endif()
if(CNPY_INSTALL)
    install(
        TARGETS "cnpy"
        ARCHIVE DESTINATION lib
        PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    install(FILES cnpy.h DESTINATION include)
    install(
        EXPORT cnpy-targets
        FILE cnpy-targets.cmake
        DESTINATION lib/cmake/cnpy
    )
    install(TARGETS cnpy EXPORT cnpy-targets DESTINATION lib)
endif()
