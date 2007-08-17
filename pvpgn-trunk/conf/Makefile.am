if COND_BNETD
bnetd_confs = bnetd.conf ad.conf anongame_infos.conf address_translation.conf \
	autoupdate.conf bnalias.conf bnban.conf bnetd_default_user.cdb \
	bnetd_default_user.plain bnhelp.conf bnissue.txt bnmaps.conf \
	bnmotd.txt bnxpcalc.conf bnxplevel.conf channel.conf \
	command_groups.conf news.txt realm.conf sql_DB_layout.conf \
	supportfile.conf topics.conf tournament.conf versioncheck.conf \
	sql_DB_layout2.conf
bnetd_extras = bnetd.conf.in bnetd.conf.win32
else
bnetd_confs =
bnetd_extras =
endif

if COND_D2CS
d2cs_confs = d2cs.conf address_translation.conf d2server.ini
d2cs_extras = d2cs.conf.in d2cs.conf.win32
else
d2cs_confs =
d2cs_extras =
endif

if COND_D2DBS
d2dbs_confs = d2dbs.conf
d2dbs_extras = d2dbs.conf.in d2dbs.conf.win32
else
d2dbs_confs =
d2dbs_extras =
endif

dist_sysconf_DATA = $(bnetd_confs) $(d2cs_confs) $(d2dbs_confs)

EXTRA_DIST = $(bnetd_extras) $(d2cs_extras) $(d2dbs_extras)

bnetd.conf:bnetd.conf.in $(top_builddir)/config.status
	$(SED) -e "s@/usr/local/var/@${localstatedir}/@g" -e "s@/usr/local/etc/@${sysconfdir}/@g" $(top_srcdir)/conf/bnetd.conf.in > $@

d2cs.conf:d2cs.conf.in $(top_builddir)/config.status
	$(SED) -e "s@/usr/local/var/@${localstatedir}/@g" -e "s@/usr/local/etc/@${sysconfdir}/@g" $(top_srcdir)/conf/d2cs.conf.in > $@

d2dbs.conf:d2dbs.conf.in $(top_builddir)/config.status
	$(SED) -e "s@/usr/local/var/@${localstatedir}/@g" -e "s@/usr/local/etc/@${sysconfdir}/@g" $(top_srcdir)/conf/d2dbs.conf.in > $@

CONFIG_CLEAN_FILES = bnetd.conf d2cs.conf d2dbs.conf
