find_package(SDL2 REQUIRED)

add_library(imgui
	include/imconfig.h
	include/imgui_impl_opengl3.h
	include/imgui_impl_sdl.h
	include/imgui.h
	src/imgui_demo.cpp
	src/imgui_draw.cpp
	src/imgui_impl_opengl3.cpp
	src/imgui_impl_sdl.cpp
	src/imgui_internal.h
	src/imgui_widgets.cpp
	src/imgui.cpp
	src/imstb_rectpack.h
	src/imstb_textedit.h
	src/imstb_truetype.h)
target_include_directories(imgui PUBLIC include PRIVATE src)
target_link_libraries(imgui PUBLIC SDL2::Core glad)