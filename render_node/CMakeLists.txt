

list(APPEND FILES main.cpp)
list(APPEND FILES RenderServer.h RenderServer.cpp)

add_executable(render_node ${FILES})

target_link_libraries(render_node PUBLIC render_lib)

target_include_directories(render_node PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})