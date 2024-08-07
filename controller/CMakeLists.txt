
list(APPEND FILES main.cpp)

list(APPEND FILES COMObject.h)
list(APPEND FILES RaytracerOutputViewer.h RaytracerOutputViewer.cpp)
list(APPEND FILES GraphicsFramework.h GraphicsFramework.cpp)
list(APPEND FILES ImguiHelper.h ImguiHelper.cpp)
list(APPEND FILES Window.h Window.cpp)
list(APPEND FILES RenderClient.h RenderClient.cpp)


add_executable(Controller ${FILES})

target_link_libraries(Controller PUBLIC fisk_tools)
target_link_libraries(Controller PUBLIC fisk_imgui)
target_link_libraries(Controller PUBLIC render_lib)
target_link_libraries(Controller PUBLIC assimp)

target_link_libraries(Controller PUBLIC d3d11.lib) 
target_link_libraries(Controller PUBLIC d3dcompiler.lib)

target_include_directories(Controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})