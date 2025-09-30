project(logger-example)

add_executable(${PROJECT_NAME} main.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} logger)