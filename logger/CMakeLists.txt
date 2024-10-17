project(logger C)

option(CONFIG_LOGGER_WITH_FILE   "Support file appender" OFF)
option(CONFIG_LOGGER_WITH_SYSLOG "Support syslog appender" OFF)

file (GLOB src src/* include/logger/*)

if (CONFIG_LOGGER_WITH_FILE)
    add_definitions(-DCONFIG_LOGGER_WITH_FILE=1)
endif ()

if (CONFIG_LOGGER_WITH_SYSLOG)
    add_definitions(-DCONFIG_LOGGER_WITH_SYSLOG=1)
endif ()

add_library(${PROJECT_NAME} STATIC ${src})
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(${PROJECT_NAME} PUBLIC include)

if(WIN32)
  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "lib")
endif()
