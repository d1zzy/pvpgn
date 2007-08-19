add_executable(bnetd account.cpp account_wrap.cpp adbanner.cpp alias_command.cpp anongame.cpp 
anongame_gameresult.cpp anongame_infos.cpp anongame_maplists.cpp attrgroup.cpp 
attrlayer.cpp autoupdate.cpp channel.cpp channel_conv.cpp character.cpp clan.cpp 
cmdline.cpp command.cpp command_groups.cpp connection.cpp file.cpp file_cdb.cpp 
file_plain.cpp friends.cpp game.cpp game_conv.cpp handle_anongame.cpp 
handle_bnet.cpp handle_bot.cpp handle_d2cs.cpp handle_file.cpp handle_init.cpp 
handle_irc.cpp handle_telnet.cpp handle_udp.cpp helpfile.cpp ipban.cpp irc.cpp 
ladder.cpp ladder_calc.cpp mail.cpp main.cpp message.cpp news.cpp 
output.cpp prefs.cpp realm.cpp runprog.cpp server.cpp sql_dbcreator.cpp 
sql_mysql.cpp sql_odbc.cpp sql_pgsql.cpp sql_sqlite3.cpp storage.cpp 
storage_file.cpp storage_sql.cpp support.cpp team.cpp tick.cpp timer.cpp topic.cpp 
tournament.cpp tracker.cpp udptest_send.cpp versioncheck.cpp watch.cpp 
storage_sql2.cpp sql_common.cpp handle_wol.cpp handle_irc_common.cpp 
handle_apireg.cpp)
target_link_libraries(bnetd compat common tinycdb 
${ZLIB_LIBRARIES} ${MYSQL_LIBRARIES} ${SQLITE3_LIBRARIES})
