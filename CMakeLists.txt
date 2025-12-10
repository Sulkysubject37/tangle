cmake_minimum_required(VERSION 3.15)
project(tangle VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

# FTXUI dependency: allow using system-installed package (e.g. Homebrew)
# or falling back to FetchContent when building standalone.
option(TANGLE_USE_SYSTEM_FTXUI "Use system-installed FTXUI instead of FetchContent" OFF)

if(TANGLE_USE_SYSTEM_FTXUI)
  find_package(ftxui CONFIG REQUIRED COMPONENTS component)
else()
  include(FetchContent)
  FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG v6.1.9
  )
  FetchContent_MakeAvailable(ftxui)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(TANGLE_WITH_JSON "Enable JSON support" OFF)
option(BUILD_TANGLE_CLI "Build tangle CLI" ON)
option(BUILD_TANGLE_TUI "Build tangle TUI" ON)
# TANGLE_USE_SYSTEM_FTXUI is defined above near the FTXUI dependency setup.

# ----------------------------------------------------------------------------
# Library: tangle
# ----------------------------------------------------------------------------

# Create the library target
add_library(tangle_lib)

# Public headers (for consumers of the library)
target_include_directories(tangle_lib
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Add all source files from the src directory
target_sources(tangle_lib
  PRIVATE
    src/graph.cpp
    src/io/edgelist_io.cpp
    src/algo/centrality.cpp
    src/algo/community.cpp
    src/annotate/annotation_db.cpp
    src/annotate/go_enrichment.cpp
    src/io/string_importer.cpp
    src/export/sbml_exporter.cpp
    src/io/biogrid_importer.cpp
)

# ----------------------------------------------------------------------------
# Executable: tangle (CLI)
# ----------------------------------------------------------------------------

if(BUILD_TANGLE_CLI)
  add_executable(tangle src/main.cpp)
  target_link_libraries(tangle PRIVATE tangle_lib)
endif()

# ----------------------------------------------------------------------------
# JSON support
# ----------------------------------------------------------------------------
if(TANGLE_WITH_JSON)
  message(STATUS "JSON support enabled.")
  target_compile_definitions(tangle_lib PUBLIC TANGLE_WITH_JSON=1)
  target_compile_definitions(tangle PRIVATE TANGLE_WITH_JSON=1)
endif()

# ----------------------------------------------------------------------------
# Executable: tangle-tui
# ----------------------------------------------------------------------------
if(BUILD_TANGLE_TUI)
  add_executable(tangle-tui src/tui.cpp)
  target_link_libraries(tangle-tui PRIVATE tangle_lib ftxui::component)
endif()

# ----------------------------------------------------------------------------
# Installation
# ----------------------------------------------------------------------------
install(TARGETS tangle_lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(BUILD_TANGLE_CLI)
  install(TARGETS tangle
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(BUILD_TANGLE_TUI)
  install(TARGETS tangle-tui
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# ----------------------------------------------------------------------------
# Tests
# ----------------------------------------------------------------------------

# Enable testing
enable_testing()

# Test executable for Catch2
add_executable(catch_tests tests/catch_main.cpp)
target_link_libraries(catch_tests PRIVATE tangle_lib)

# Define CATCH_CONFIG_MAIN to provide main() for Catch2
target_compile_definitions(catch_tests PRIVATE CATCH_CONFIG_MAIN)

# Add the Catch2 test executable to CTest
add_test(NAME CatchTests COMMAND catch_tests)

# TUI Workflow Test
add_executable(tui_workflow_test tests/tui_workflow_test.cpp)
target_link_libraries(tui_workflow_test PRIVATE tangle_lib)
add_test(NAME TUI_Workflow_Test COMMAND tui_workflow_test)

# ----------------------------------------------------------------------------
# CPack: generic binary/source tarballs
# ----------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")

include(CPack)
