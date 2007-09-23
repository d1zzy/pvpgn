set(WIN32_SOURCES
  service.cpp service.h console_output.h console_output.cpp
)

add_library(win32 ${WIN32_SOURCES})

if(WITH_WIN32_GUI)

set_target_properties(win32 PROPERTIES COMPILE_FLAGS "-DWIN32_GUI")
endif(WITH_WIN32_GUI)
