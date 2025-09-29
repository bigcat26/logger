project(logger C)

option(CONFIG_LOGGER_OSAL_LINUX      "Linux OSAL" ON)
option(CONFIG_LOGGER_OSAL_WIN32      "Win32 OSAL" OFF)
option(CONFIG_LOGGER_APPENDER_FILE   "Build with file appender" OFF)
option(CONFIG_LOGGER_APPENDER_SYSLOG "Build with syslog appender" OFF)

file (GLOB src_files src/* include/logger/*)

if(CONFIG_LOGGER_OSAL_LINUX)
    list(APPEND src_files src/port/osal_linux.c)
elseif(CONFIG_LOGGER_OSAL_WIN32)
    list(APPEND src_files src/port/osal_win32.c)
else()
    list(APPEND src_files src/port/osal_none.c)
endif()

if(CONFIG_LOGGER_APPENDER_FILE)
    add_definitions(-DCONFIG_LOGGER_WITH_FILE=1)
endif()

if(CONFIG_LOGGER_APPENDER_SYSLOG)
    add_definitions(-DCONFIG_LOGGER_WITH_SYSLOG=1)
endif()

add_library(${PROJECT_NAME} STATIC ${src_files})
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(${PROJECT_NAME} PUBLIC include)

# if(WIN32)
#   set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "lib")
# endif()
