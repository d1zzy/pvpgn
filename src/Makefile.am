if COND_BNETD
MAYBE_BNETD = bnetd
else
MAYBE_BNETD =
endif

if COND_D2CS
MAYBE_D2CS = d2cs
else
MAYBE_D2CS =
endif

if COND_D2DBS
MAYBE_D2DBS = d2dbs
else
MAYBE_D2DBS =
endif

if COND_BNPCAP
MAYBE_BNPCAP = bnpcap
else
MAYBE_BNPCAP =
endif

SUBDIRS = compat common tinycdb win32 $(MAYBE_BNETD) $(MAYBE_D2CS) $(MAYBE_D2DBS) \
	bntrackd client bnpass bniutils $(MAYBE_BNPCAP) autoconf
