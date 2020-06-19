cmake_minimum_required(VERSION 3.0)

project(logger-lib)

option(CONFIG_MODULE_LOGGER_BUILD_EXAMPLE "Build logger example" OFF)

add_subdirectory(lib)

if (CONFIG_MODULE_LOGGER_BUILD_EXAMPLE)
  add_subdirectory(example)
endif ()
