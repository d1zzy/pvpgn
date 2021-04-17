set(WIN32_SOURCES
  service.cpp service.h console_output.h console_output.cpp dirent.h windump.cpp windump.h
)

add_library(win32 STATIC ${WIN32_SOURCES})
