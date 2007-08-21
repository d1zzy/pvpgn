add_executable(d2dbs 
	charlock.cpp charlock.h cmdline.cpp cmdline.h d2ladder.cpp d2ladder.h 
	dbsdupecheck.cpp dbsdupecheck.h dbserver.cpp dbserver.h dbspacket.cpp 
	dbspacket.h handle_signal.cpp handle_signal.h main.cpp prefs.cpp 
	prefs.h setup.h version.h)
target_link_libraries(d2dbs common compat win32 ${NETWORK_LIBRARIES})
