add_executable(bnilist bnilist.cpp fileio.cpp fileio.h tga.cpp tga.h)
target_link_libraries(bnilist PRIVATE common compat)

add_executable(bni2tga bni2tga.cpp fileio.cpp fileio.h)
target_link_libraries(bni2tga PRIVATE common)

add_executable(bniextract bniextract.cpp fileio.cpp fileio.h tga.cpp tga.h bni.cpp bni.h)
target_link_libraries(bniextract PRIVATE common compat)

add_executable(bnibuild bnibuild.cpp fileio.cpp fileio.h bni.cpp bni.h tga.cpp tga.h)
target_link_libraries(bnibuild PRIVATE common compat)

add_executable(tgainfo tgainfo.cpp fileio.cpp fileio.h tga.cpp tga.h)
target_link_libraries(tgainfo PRIVATE common compat)

install(TARGETS bnilist bni2tga bniextract bnibuild tgainfo DESTINATION ${BINDIR})
