
project("project")

#Put the sources into a variable
set(SOURCE_MAIN_PROJECT "main.cpp" "camera.h" "shader.h" "object.h" "shadow.h")

#These commands are there to specify the path to the folder containing the object and textures files as macro
#With these you can just use PATH_TO_OBJECTS and PATH_TO_TEXTURE in your c++ code and the compiler will replace it by the correct expression
add_compile_definitions(PATH_TO_OBJECTS="${CMAKE_CURRENT_SOURCE_DIR}/objects")
add_compile_definitions(PATH_TO_TEXTURE="${CMAKE_CURRENT_SOURCE_DIR}/textures")
add_compile_definitions(PATH_TO_FRAG="${CMAKE_CURRENT_SOURCE_DIR}/frag")
add_compile_definitions(PATH_TO_VERT="${CMAKE_CURRENT_SOURCE_DIR}/vert")

add_compile_definitions(PATH_TO_MAIN_SHADERS="${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(main ${SOURCE_MAIN_PROJECT})
#Specify which libraries you want to use with your executable
target_link_libraries(main PUBLIC OpenGL::GL glfw glad)

