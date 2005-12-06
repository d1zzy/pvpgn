AM_CPPFLAGS=-I$(top_srcdir)/src -DBNETD_DEFAULT_CONF_FILE=\"$(sysconfdir)/bnetd.conf\"

sbin_PROGRAMS = bnetd

bnetd_SOURCES = account.c account_wrap.c adbanner.c alias_command.c anongame.c \
	anongame_gameresult.c anongame_infos.c anongame_maplists.c attrgroup.c \
	attrlayer.c autoupdate.c channel.c channel_conv.c character.c clan.c \
	cmdline.c command.c command_groups.c connection.c file.c file_cdb.c \
	file_plain.c friends.c game.c game_conv.c handle_anongame.c \
	handle_bnet.c handle_bot.c handle_d2cs.c handle_file.c handle_init.c \
	handle_irc.c handle_telnet.c handle_udp.c helpfile.c ipban.c irc.c \
	ladder_binary.c ladder.c ladder_calc.c mail.c main.c message.c news.c \
	output.c prefs.c realm.c runprog.c server.c sql_dbcreator.c \
	sql_mysql.c sql_odbc.c sql_pgsql.c sql_sqlite3.c storage.c \
	storage_file.c storage_sql.c support.c team.c tick.c timer.c topic.c \
	tournament.c tracker.c udptest_send.c versioncheck.c watch.c

bnetd_LDADD = $(top_builddir)/src/common/libcommon.a \
	$(top_builddir)/src/compat/libcompat.a \
	$(top_builddir)/src/tinycdb/libtinycdb.a \
	$(top_builddir)/src/zlib/libzlib.a

noinst_HEADERS = account.h account_wrap.h adbanner.h alias_command.h \
	anongame_gameresult.h anongame.h anongame_infos.h anongame_maplists.h \
	attrgroup.h attr.h attrlayer.h autoupdate.h channel_conv.h channel.h \
	character.h clan.h cmdline.h command_groups.h command.h connection.h \
	file_cdb.h file.h file_plain.h friends.h game_conv.h game.h ipban.h \
	handle_anongame.h handle_bnet.h handle_bot.h handle_d2cs.h helpfile.h \
	handle_file.h handle_init.h handle_irc.h handlers.h handle_telnet.h \
	handle_udp.h irc.h ladder_binary.h ladder_calc.h ladder.h mail.h \
	message.h news.h output.h prefs.h quota.h realm.h runprog.h server.h \
	sql_dbcreator.h sql_mysql.h sql_odbc.h sql_pgsql.h sql_sqlite3.h \
	storage_file.h storage.h storage_sql.h support.h team.h tick.h \
	timer.h topic.h tournament.h udptest_send.h versioncheck.h watch.h
