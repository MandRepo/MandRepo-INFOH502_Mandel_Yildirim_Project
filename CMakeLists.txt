#Template for INFOH502
#Heavily inspired from https://github.com/Polytonic/Glitter/blob/master/CMakeLists.txt

#project setup
project(infoh502_Project)

#set the minimal version of cmake and the minimal version of the c++ language
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 14)     
set(CMAKE_VERBOSE_MAKEFILE ON)

#define some variable
set(COMPILE_PROJECT ON CACHE BOOL "Compile the project for INFO-502 assignement")


find_package(OpenGL REQUIRED)

#for glad library
add_library( glad STATIC libraries/glad/src/glad.c)
set(GLAD_INCLUDE "libraries/glad/include")
target_include_directories(glad PUBLIC ${GLAD_INCLUDE})

#for GLFW
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)

#Search for glfw in case it is installed
find_package(glfw3 QUIET)
if (NOT glfw3_FOUND)
    #if glfw is not found, we will use the sources from the submodules
    add_subdirectory(libraries/glfw)
    include_directories(libraries/glfw/include/)
endif()



include_directories(libraries/glad/include/
                    libraries/glm/
                    libraries/stb/)

                 
if(COMPILE_PROJECT)
    add_subdirectory(project)
endif()

