/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 2005 Dizzy
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include "common/setup_before.h"
#include "cmdline.h"

#include <cstdio>

#include "common/xalloc.h"
#ifdef WIN32
# include "win32/service.h"
#endif
#ifdef WIN32_GUI
# include "win32/gui_printf.h"
# define printf gui_printf
#endif
#include "compat/strcasecmp.h"
#include "common/eventlog.h"
#include "common/conf.h"
#include "common/setup_after.h"

namespace pvpgn
{

namespace bnetd
{

static struct {
#ifdef DO_DAEMONIZE
    unsigned foreground;
#endif
    const char *preffile;
    const char *hexfile;
    unsigned debug;
} cmdline_config;

static unsigned exitflag;
static const char *progname;

#ifdef DO_DAEMONIZE
static int conf_set_foreground(const char *valstr);
static int conf_setdef_foreground(void);
#endif

static int conf_set_preffile(const char *valstr);
static int conf_setdef_preffile(void);

static int conf_set_hexfile(const char *valstr);
static int conf_setdef_hexfile(void);

static int conf_set_debug(const char *valstr);
static int conf_setdef_debug(void);

static int conf_set_help(const char *valstr);
static int conf_setdef_help(void);

static int conf_set_version(const char *valstr);
static int conf_setdef_version(void);

#ifdef WIN32
static int conf_set_service(const char *valstr);
static int conf_setdef_service(void);

static int conf_set_servaction(const char *valstr);
static int conf_setdef_servaction(void);
#endif

static t_conf_entry conftab[] = {
    { "c",		conf_set_preffile,	NULL,	conf_setdef_preffile },
    { "config",		conf_set_preffile,	NULL,	conf_setdef_preffile },
    { "d",		conf_set_hexfile,	NULL,	conf_setdef_hexfile },
    { "hexdump",	conf_set_hexfile,	NULL,	conf_setdef_hexfile },
#ifdef DO_DAEMONIZE
    { "f",		conf_set_foreground,	NULL,	conf_setdef_foreground },
    { "foreground",	conf_set_foreground,	NULL,	conf_setdef_foreground },
#endif
    { "D",		conf_set_debug,		NULL,	conf_setdef_debug },
    { "debug",		conf_set_debug,		NULL,	conf_setdef_debug },
    { "h",		conf_set_help,		NULL,	conf_setdef_help },
    { "help",		conf_set_help,		NULL,	conf_setdef_help },
    { "usage",		conf_set_help,		NULL,	conf_setdef_help },
    { "v",		conf_set_version,	NULL,	conf_setdef_version },
    { "version",	conf_set_version,	NULL,	conf_setdef_version },
#ifdef WIN32
    { "service",	conf_set_service,	NULL,	conf_setdef_service },
    { "s",		conf_set_servaction,	NULL,	conf_setdef_servaction },
#endif
    { NULL,		NULL,			NULL,	NULL }
};

static void usage(void)
{
    std::fprintf(stderr,
            "usage: %s [<options>]\n"
            "    -c FILE, --config=FILE   use FILE as configuration file (default is " BNETD_DEFAULT_CONF_FILE ")\n"
            "    -d FILE, --hexdump=FILE  do hex dump of packets into FILE\n"
#ifdef DO_DAEMONIZE
            "    -f, --foreground         don't daemonize\n"
#endif
            "    -D, --debug              run in debug mode (run in foreground and log to stdout)\n"
            "    -h, --help, --usage      show this information and exit\n"
            "    -v, --version            print version number and exit\n"
#ifdef WIN32
            "    Running as service functions:\n"
            "    --service                run as service\n"
            "    -s install               install service\n"
            "    -s uninstall             uninstall service\n"
#endif
            ,progname);
}

extern int cmdline_load(int argc, char** argv)
{
    int res;

    if (argc<1 || !argv || !argv[0]) {
	std::fprintf(stderr,"bad arguments\n");
        return -1;
    }

    exitflag = 0;
    progname = argv[0];

    res = conf_load_cmdline(argc, argv, conftab);
    if (res < 0) return -1;
    return exitflag ? 0 : 1;
}

extern void cmdline_unload(void)
{
    conf_unload(conftab);
}

#ifdef DO_DAEMONIZE
extern int cmdline_get_foreground(void)
{
    return cmdline_config.foreground;
}

static int conf_set_foreground(const char *valstr)
{
    return conf_set_bool(&cmdline_config.foreground, valstr, 0);
}

static int conf_setdef_foreground(void)
{
    return conf_set_bool(&cmdline_config.foreground, NULL, 0);
}
#endif

extern const char* cmdline_get_preffile(void)
{
    return cmdline_config.preffile;
}

static int conf_set_preffile(const char *valstr)
{
    return conf_set_str(&cmdline_config.preffile, valstr, NULL);
}

static int conf_setdef_preffile(void)
{
    return conf_set_str(&cmdline_config.preffile, NULL, BNETD_DEFAULT_CONF_FILE);
}


extern const char* cmdline_get_hexfile(void)
{
    return cmdline_config.hexfile;
}

static int conf_set_hexfile(const char *valstr)
{
    return conf_set_str(&cmdline_config.hexfile, valstr, NULL);
}

static int conf_setdef_hexfile(void)
{
    return conf_set_str(&cmdline_config.hexfile, NULL, NULL);
}


static int conf_set_debug(const char *valstr)
{
    conf_set_bool(&cmdline_config.debug, valstr, 0);
    if (cmdline_config.debug) eventlog_set_debugmode(1);
#ifdef DO_DAEMONIZE
    cmdline_config.foreground = 1;
#endif
    return 0;
}

static int conf_setdef_debug(void)
{
    return conf_set_bool(&cmdline_config.debug, NULL, 0);
}

static int conf_set_help(const char *valstr)
{
    unsigned tmp = 0;

    conf_set_bool(&tmp, valstr, 0);
    if (tmp) {
	usage();
	exitflag = 1;
    }

    return 0;
}

static int conf_setdef_help(void)
{
    return 0;
}


static int conf_set_version(const char *valstr)
{
    unsigned tmp = 0;

    conf_set_bool(&tmp, valstr, 0);
    if (tmp) {
        printf(PVPGN_SOFTWARE" version "PVPGN_VERSION"\n");
	exitflag = 1;
    }

    return 0;
}

static int conf_setdef_version(void)
{
    return 0;
}

#ifdef WIN32
static int conf_set_service(const char *valstr)
{
    unsigned tmp = 0;

    conf_set_bool(&tmp, valstr, 0);
    if (tmp) {
        Win32_ServiceRun();
	exitflag = 1;
    }

    return 0;
}

static int conf_setdef_service(void)
{
    return 0;
}


static int conf_set_servaction(const char *valstr)
{
    const char* tmp = NULL;

    conf_set_str(&tmp, valstr, NULL);

    if (tmp) {
	if (!strcasecmp(tmp, "install")) {
	    std::fprintf(stderr, "Installing service");
	    Win32_ServiceInstall();
	} else if (!strcasecmp(tmp, "uninstall")) {
	    std::fprintf(stderr, "Uninstalling service");
	    Win32_ServiceUninstall();
	} else {
	    std::fprintf(stderr, "Unknown service action '%s'\n", tmp);
	}

	exitflag = 1;
	xfree((void *)tmp);
    }

    return 0;
}

static int conf_setdef_servaction(void)
{
    return 0;
}
#endif

}

}
