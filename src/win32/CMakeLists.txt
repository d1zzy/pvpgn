set(WIN32_SOURCES
  service.cpp service.h
)

set(WIN32_GUI_SOURCES
	gui_printf.h gui_printf.cpp console_output.h console_output.cpp
)

add_library(win32 ${WIN32_SOURCES})

# build the GUI version too
# (this will practically recompile every file again)
if(WITH_WIN32_GUI)
add_library(win32_gui
  ${WIN32_SOURCES} ${WIN32_GUI_SOURCES}
)
set_target_properties(win32_gui PROPERTIES COMPILE_FLAGS "-DWIN32_GUI")
endif(WITH_WIN32_GUI)
