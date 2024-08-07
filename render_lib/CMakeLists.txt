
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Version.h "extern const char* GlobalGitVersion;")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Version.cpp "const char* GlobalGitVersion = \"")

execute_process(
			COMMAND git rev-parse HEAD 
			OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/_Version.cpp
			OUTPUT_STRIP_TRAILING_WHITESPACE)

file(READ ${CMAKE_CURRENT_BINARY_DIR}/_Version.cpp VERSION_HASH)
string(STRIP "${VERSION_HASH}" VERSION_HASH)
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/_Version.cpp)
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Version.cpp "${VERSION_HASH}\";")

list(APPEND FILES ConvertVector.h)
list(APPEND FILES Hit.h)
list(APPEND FILES MultichannelTexture.h)
list(APPEND FILES PartitionBy.h)
list(APPEND FILES RaytracerConcept.h)
list(APPEND FILES Orchestrator.h)
list(APPEND FILES ThreadedRenderer.h)
list(APPEND FILES RendererTypes.h)
list(APPEND FILES CheckeredRenderer.h)
list(APPEND FILES NetworkedRenderer.h)
list(APPEND FILES NodeLimits.h)
list(APPEND FILES RenderCollection.h)

list(APPEND FILES Camera.h Camera.cpp)
list(APPEND FILES Material.h Material.cpp)
list(APPEND FILES RayRenderer.h RayRenderer.cpp)
list(APPEND FILES RegionGenerator.h RegionGenerator.cpp)
list(APPEND FILES PolyObject.h PolyObject.cpp)
list(APPEND FILES Scene.h Scene.cpp)
list(APPEND FILES Sky.h Sky.cpp)
list(APPEND FILES RenderConfig.h RenderConfig.cpp)

list(APPEND FILES Protocol.h Protocol.cpp)


list(APPEND FILES IIntersector.h)
list(APPEND FILES intersectors/DumbIntersector.h intersectors/DumbIntersector.cpp)
list(APPEND FILES intersectors/ClusteredIntersector.h intersectors/ClusteredIntersector.cpp)
list(APPEND FILES ${CMAKE_CURRENT_BINARY_DIR}/Version.h ${CMAKE_CURRENT_BINARY_DIR}/Version.cpp)


add_library(render_lib ${FILES})

target_link_libraries(render_lib PUBLIC fisk_tools)
target_link_libraries(render_lib PUBLIC fisk_imgui)
target_link_libraries(render_lib PUBLIC assimp)

target_include_directories(render_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(render_lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})