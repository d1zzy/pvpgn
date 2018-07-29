set(D2DBS_SOURCES
	charlock.cpp charlock.h cmdline.cpp cmdline.h d2ladder.cpp d2ladder.h 
	dbsdupecheck.cpp dbsdupecheck.h dbserver.cpp dbserver.h dbspacket.cpp 
	dbspacket.h handle_signal.cpp handle_signal.h main.cpp prefs.cpp 
	prefs.h setup.h version.h ../win32/d2dbs_winmain.cpp ../win32/d2dbs_resource.h 
	../win32/d2dbs_resource.rc)

if(WITH_WIN32_GUI)
  set(D2DBS_BIN_NAME "D2DBS")
  add_executable(${D2DBS_BIN_NAME} WIN32 ${D2DBS_SOURCES})
else(WITH_WIN32_GUI)
  set(D2DBS_BIN_NAME "D2DBSConsole")
  add_executable(${D2DBS_BIN_NAME} ${D2DBS_SOURCES})
endif(WITH_WIN32_GUI)

target_link_libraries(${D2DBS_BIN_NAME} PRIVATE common compat fmt win32 ${NETWORK_LIBRARIES})
install(TARGETS ${D2DBS_BIN_NAME} DESTINATION ${SBINDIR})
