project(logger C)

option(CONFIG_LOGGER_OSAL_POSIX      "POSIX OSAL (Linux/macOS)" ON)
option(CONFIG_LOGGER_OSAL_WIN32      "Win32 OSAL" OFF)
option(CONFIG_LOGGER_APPENDER_FILE   "Build with file appender" OFF)
option(CONFIG_LOGGER_APPENDER_SYSLOG "Build with syslog appender" OFF)

file (GLOB src_files src/*.c include/logger/*)

if(CONFIG_LOGGER_OSAL_POSIX)
    list(APPEND src_files src/port/osal_posix.c)
    # 检查必要的头文件
    include(CheckIncludeFile)
    check_include_file("pthread.h" HAVE_PTHREAD_H)
    check_include_file("unistd.h" HAVE_UNISTD_H)
    
    if(APPLE)
        check_include_file("malloc/malloc.h" HAVE_MALLOC_MALLOC_H)
        if(HAVE_MALLOC_MALLOC_H)
            add_definitions(-DHAVE_MALLOC_MALLOC_H=1)
        endif()
    elseif(UNIX)
        check_include_file("malloc.h" HAVE_MALLOC_H)
        if(HAVE_MALLOC_H)
            add_definitions(-DHAVE_MALLOC_H=1)
        endif()
    endif()
    
    # 检查 pthread 库
    find_package(Threads REQUIRED)
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
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

# 链接必要的库
if(CONFIG_LOGGER_OSAL_POSIX)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif()

# if(WIN32)
#   set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "lib")
# endif()
