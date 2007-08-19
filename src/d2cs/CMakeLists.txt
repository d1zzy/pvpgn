add_executable(d2cs bnetd.cpp cmdline.cpp connection.cpp d2charfile.cpp 
d2charlist.cpp d2gs.cpp d2ladder.cpp game.cpp gamequeue.cpp handle_bnetd.cpp 
handle_d2cs.cpp handle_d2gs.cpp handle_init.cpp handle_signal.cpp main.cpp 
net.cpp prefs.cpp s2s.cpp server.cpp serverqueue.cpp)
target_link_libraries(d2cs common compat win32)
