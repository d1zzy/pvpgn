add_executable(d2dbs charlock.cpp d2ladder.cpp dbserver.cpp dbspacket.cpp 
cmdline.cpp prefs.cpp handle_signal.cpp dbsdupecheck.cpp main.cpp)
target_link_libraries(d2dbs common compat)

IF(WIN32)
target_link_libraries(d2dbs win32 ws2_32)
ENDIF(WIN32)
