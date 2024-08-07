list(APPEND files "imconfig.h")
list(APPEND files "imgui.h" "imgui.cpp")
list(APPEND files "imgui_demo.cpp")
list(APPEND files "imgui_draw.cpp")
list(APPEND files "imgui_internal.h")
list(APPEND files "imgui_tables.cpp")
list(APPEND files "imgui_widgets.cpp")
list(APPEND files "imstb_rectpack.h")
list(APPEND files "imstb_textedit.h")
list(APPEND files "imstb_truetype.h")

if (WIN32)
list(APPEND files "imgui_impl_dx11.cpp" "imgui_impl_dx11.h")
list(APPEND files "imgui_impl_win32.cpp" "imgui_impl_win32.h")
endif()

add_library(fisk_imgui ${files})

target_include_directories(fisk_imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
