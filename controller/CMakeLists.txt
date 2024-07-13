
list(APPEND FILES main.cpp)

list(APPEND FILES COMObject.h)
list(APPEND FILES ConvertVector.h)
list(APPEND FILES Hit.h)
list(APPEND FILES MultichannelTexture.h)
list(APPEND FILES PartitionBy.h)
list(APPEND FILES RaytracerConcept.h)
list(APPEND FILES Orchestrator.h)
list(APPEND FILES ThreadedRenderer.h)
list(APPEND FILES RendererTypes.h)

list(APPEND FILES Camera.h Camera.cpp)
list(APPEND FILES Material.h Material.cpp)
list(APPEND FILES RayRenderer.h RayRenderer.cpp)
list(APPEND FILES RaytracerOutputViewer.h RaytracerOutputViewer.cpp)
list(APPEND FILES RegionGenerator.h RegionGenerator.cpp)
list(APPEND FILES Scene.h Scene.cpp)
list(APPEND FILES Sky.h Sky.cpp)

list(APPEND FILES GraphicsFramework.h GraphicsFramework.cpp)
list(APPEND FILES ImguiHelper.h ImguiHelper.cpp)
list(APPEND FILES Window.h Window.cpp)

list(APPEND FILES IIntersector.h)
list(APPEND FILES intersectors/DumbIntersector.h intersectors/DumbIntersector.cpp)


add_executable(Controller ${FILES})

target_link_libraries(Controller PUBLIC fisk_tools)
target_link_libraries(Controller PUBLIC fisk_imgui)

target_link_libraries(Controller PUBLIC d3d11.lib) 
target_link_libraries(Controller PUBLIC d3dcompiler.lib)

target_include_directories(Controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})