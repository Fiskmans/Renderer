cmake_minimum_required(VERSION 3.8.0)

project(Renderer)

set(CMAKE_CXX_STANDARD 20)
set(BUILD_SHARED_LIBS OFF)

Include(FetchContent)

FetchContent_Declare(
  fisk_tools
  GIT_REPOSITORY https://github.com/Fiskmans/fisk_tools.git
  GIT_TAG        04ab82e465cbd075143f544d26061e7f7e1d979c
)

FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG        35e4f1bf6492e11485336c1518485413c599e00e
)

FetchContent_MakeAvailable(fisk_tools)
FetchContent_MakeAvailable(assimp)

add_subdirectory(imgui)
add_subdirectory(render_lib)
add_subdirectory(render_node)

if(WIN32)
	add_subdirectory(controller)
endif()
