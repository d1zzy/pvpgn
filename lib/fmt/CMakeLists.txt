set(FMT_SOURCES ${FMT_SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/core.h
	${CMAKE_CURRENT_SOURCE_DIR}/format.cc
	${CMAKE_CURRENT_SOURCE_DIR}/format.h
	${CMAKE_CURRENT_SOURCE_DIR}/format-inl.h
	${CMAKE_CURRENT_SOURCE_DIR}/ostream.h
	${CMAKE_CURRENT_SOURCE_DIR}/posix.cc
	${CMAKE_CURRENT_SOURCE_DIR}/posix.h
	${CMAKE_CURRENT_SOURCE_DIR}/printf.h
	${CMAKE_CURRENT_SOURCE_DIR}/ranges.h
	${CMAKE_CURRENT_SOURCE_DIR}/time.h
)

add_library(fmt ${FMT_SOURCES})

target_include_directories(fmt
    PUBLIC 
        $<INSTALL_INTERFACE:lib>    
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib>
)