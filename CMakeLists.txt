cmake_minimum_required(VERSION 3.10)

project(sear_counters VERSION 2.0.0 LANGUAGES C)

option(SEA_COUNTERS_INSTALL "Enable install" ON)

# Default to Release for single-configuration generators when no build type
# was provided by the user.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()
option(SEA_SEA_COUNTERS_BUILD_TESTS "Build tests" OFF)

if(SEA_COUNTERS_INSTALL)
  include(GNUInstallDirs)
endif()

if(SEA_COUNTERS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

add_library(sea_counters INTERFACE)



add_library(sea_counters::sea_counters ALIAS sea_counters)
target_include_directories(
  sea_counters
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)


if(SEA_COUNTERS_INSTALL)
  include(CMakePackageConfigHelpers)

  set(SEA_COUNTERS_VERSION_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/module/CountersConfigVersion.cmake")
  set(SEA_COUNTERS_PROJECT_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/module/CountersConfig.cmake")
  set(SEA_COUNTERS_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/cmake/Counters")

  if(${CMAKE_VERSION} VERSION_LESS "3.14")
    write_basic_package_version_file("${SEA_COUNTERS_VERSION_CONFIG}" VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
  else()
    write_basic_package_version_file("${SEA_COUNTERS_VERSION_CONFIG}" VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion ARCH_INDEPENDENT)
  endif()
  configure_package_config_file("cmake/config.cmake.in"
                                "${SEA_COUNTERS_PROJECT_CONFIG}"
                                INSTALL_DESTINATION "${SEA_COUNTERS_CONFIG_INSTALL_DIR}")

  install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/counters" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  install(FILES "${SEA_COUNTERS_PROJECT_CONFIG}" "${SEA_COUNTERS_VERSION_CONFIG}" DESTINATION "${SEA_COUNTERS_CONFIG_INSTALL_DIR}")
  install(EXPORT ${PROJECT_NAME}-targets NAMESPACE Counters:: DESTINATION "${SEA_COUNTERS_CONFIG_INSTALL_DIR}")

  install(TARGETS sea_counters
          EXPORT ${PROJECT_NAME}-targets
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endif()
