project (logger C)

# option(CONFIG_MODULE_LOGGER_WITH_FILE   "Support file appender" OFF)
# option(CONFIG_MODULE_LOGGER_WITH_SYSLOG "Support syslog appender" OFF)

file(GLOB source *)

# if (CONFIG_MODULE_LOGGER_WITH_FILE)
#     add_definitions(-DCONFIG_MODULE_LOGGER_WITH_FILE=1)
# endif ()

# if (CONFIG_MODULE_LOGGER_WITH_SYSLOG)
#     add_definitions(-DCONFIG_MODULE_LOGGER_WITH_SYSLOG=1)
# endif ()

# add_library(${PROJECT_NAME} STATIC ${source})

# target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# set_target_properties(${name} PROPERTIES PREFIX "lib")
