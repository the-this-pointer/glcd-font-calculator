cmake_minimum_required(VERSION 3.23)
project(GlcdFontCalculator)

set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/inc ${GLEW_INCLUDE_DIRS} ${GLFW3_INCLUDE_DIRS})

add_executable(GlcdFontCalculator
        src/main.c
        src/edit_utils.c)

target_link_libraries(GlcdFontCalculator
        glfw3 opengl32 m GLU32 ${GLEW_LIBRARIES})
