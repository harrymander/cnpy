cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif()

project(cnpy LANGUAGES CXX)

find_package(ZLIB REQUIRED)

add_library(cnpy STATIC cnpy.cpp)
add_library(cnpy::cnpy ALIAS cnpy)
target_link_libraries(cnpy PUBLIC ZLIB::ZLIB)
target_compile_features(cnpy PRIVATE cxx_std_11)
target_include_directories(
    cnpy
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)
if(NOT MSVC)
    target_compile_options(cnpy PRIVATE -Wall -Wextra)
endif()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CNPY_INSTALL_DEFAULT TRUE)
else()
    set(CNPY_INSTALL_DEFAULT FALSE)
endif()
option(CNPY_INSTALL "Add install targets" ${CNPY_INSTALL_DEFAULT})

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
        NAMESPACE cnpy::
        DESTINATION lib/cmake/cnpy
    )
    install(TARGETS cnpy EXPORT cnpy-targets DESTINATION lib)

    include(CMakePackageConfigHelpers)
    set(config_install_dest share/cmake/cnpy)
    set(config_file ${CMAKE_CURRENT_BINARY_DIR}/cnpyConfig.cmake)
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cnpyConfig.cmake.in
        ${config_file}
        INSTALL_DESTINATION ${config_install_dest}
    )
    install(FILES ${config_file} DESTINATION ${config_install_dest})
endif()
