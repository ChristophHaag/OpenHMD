project (openglexample C)
include_directories(${CMAKE_BINARY_DIR}/include ${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
link_directories(${CMAKE_BINARY_DIR})

link_directories(${CMAKE_BINARY_DIR}/examples/opengl3)
message ("link ${CMAKE_BINARY_DIR}/examples/opengl3")

add_library(openhmd-simplerenderer simplerenderer.c)

add_executable(opengl3example gl.c main.c)

target_link_libraries(opengl3example PRIVATE openhmd openhmd-simplerenderer m ${SDL2_LIBRARY} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})
