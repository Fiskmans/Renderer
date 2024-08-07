cmake_minimum_required(VERSION 3.8.0)

project(Renderer)

set(CMAKE_CXX_STANDARD 20)
if (WIN32)
	set(BUILD_SHARED_LIBS OFF)
endif()

Include(FetchContent)

FetchContent_Declare(
  fisk_tools
  GIT_REPOSITORY https://github.com/Fiskmans/fisk_tools.git
  GIT_TAG        26845d9086685a5cde445c8ec0e8fce5a86a317a
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