/*
 * Copyright (C) 1999  Rob Crittenden (rcrit@greyoak.com)
 * Copyright (C) 1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifndef INCLUDED_LADDER_TYPES
#define INCLUDED_LADDER_TYPES

#ifdef LADDER_INTERNAL_ACCESS

#ifdef JUST_NEED_TYPES
# include "account.h"
# include "ladder_binary.h"
# include "common/tag.h"
#else
# define JUST_NEED_TYPES
# include "account.h"
# include "ladder_binary.h"
# include "common/tag.h"
# undef JUST_NEED_TYPES
#endif

#endif

#define W3_XPCALC_MAXLEVEL	50


typedef enum
{
    ladder_sort_highestrated,
    ladder_sort_mostwins,
    ladder_sort_mostgames
} t_ladder_sort;

typedef enum
{
    ladder_time_active,
    ladder_time_current
} t_ladder_time;

typedef enum
{
    ladder_id_none=0,
    ladder_id_normal=1,
    ladder_id_ironman=3,
    ladder_id_solo=5,
    ladder_id_team=6,
    ladder_id_ffa=7
} t_ladder_id;

extern char * ladder_id_str[];

typedef enum
{
    ladder_option_none=0,
    ladder_option_disconnectisloss=1
} t_ladder_option;

typedef struct ladder_internal
#ifdef LADDER_INTERNAL_ACCESS
 {
   int uid;
   int xp;
   int level;
   unsigned int teamcount;            /* needed for AT ladder */
   t_account *account;
   struct ladder_internal *prev; /* user with less XP */
   struct ladder_internal *next; /* user with more XP */
 }
#endif
 t_ladder_internal;

 typedef struct ladder
#ifdef LADDER_INTERNAL_ACCESS
 {
    t_ladder_internal *first;
    t_ladder_internal *last;
    int dirty;                        /* 0==no changes, 1==something changed */
    t_binary_ladder_types type;
    t_clienttag clienttag;
    t_ladder_id  ladder_id;
 }
#endif
 t_ladder;

#ifdef LADDER_INTERNAL_ACCESS

typedef struct
{
    int startxp, neededxp, lossfactor, mingames;
} t_xplevel_entry;

typedef struct
{
    int higher_winxp, higher_lossxp, lower_winxp, lower_lossxp;
} t_xpcalc_entry;
#endif

#endif

/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_LADDER_PROTOS
#define INCLUDED_LADDER_PROTOS

#define JUST_NEED_TYPES
#include "account.h"
#include "game.h"
#include "ladder_calc.h"
#include "ladder_binary.h"
#include "common/tag.h"
#undef JUST_NEED_TYPES

extern int ladder_init_account(t_account * account, t_clienttag clienttag, t_ladder_id id);
extern int ladder_check_map(char const * mapname, t_game_maptype maptype, t_clienttag clienttag);

extern t_account * ladder_get_account_by_rank(unsigned int rank, t_ladder_sort lsort, t_ladder_time ltime, t_clienttag clienttag, t_ladder_id id);
extern unsigned int ladder_get_rank_by_account(t_account * account, t_ladder_sort lsort, t_ladder_time ltime, t_clienttag clienttag, t_ladder_id id);

extern int ladder_update(t_clienttag clienttag, t_ladder_id id, unsigned int count, t_account * * players, t_game_result * results, t_ladder_info * info, t_ladder_option opns);

extern int ladderlist_make_all_active(void);

extern int  ladder_createxptable(const char *xplevelfile, const char *xpcalcfile);
extern void ladder_destroyxptable(void);

extern int ladder_war3_xpdiff(unsigned int winnerlevel, unsigned int looserlevel, int *, int *);
extern int ladder_war3_updatelevel(unsigned int oldlevel, int xp);
extern int ladder_war3_get_min_xp(unsigned int level);
extern int war3_get_maxleveldiff(void);


 extern int war3_ladder_add(t_ladder *ladder, int uid, int xp, int level, t_account *account, unsigned int teamcount,t_clienttag clienttag);
 /* this function adds a user to the ladder and keeps the ladder sorted
  * returns 0 if everything is fine and -1 when error occured */

 extern int war3_ladder_update(t_ladder *ladder, int uid, int xp, int level, t_account *account, unsigned int teamcount);
 /* this functions increases the xp of user with UID uid and corrects ranking
  * returns 0 if everything is fine
  * if user is not yet in ladder, he gets added automatically */

 extern int ladder_get_rank(t_ladder *ladder, int uid, unsigned int teamcount, t_clienttag clienttag);
 /* this function returns the rank of a user with a given uid
  * returns 0 if no such user is found */
 
 extern int ladder_update_all_accounts(void);
 /* write the correct ranking information to all user accounts
  * and cut down ladder size to given limit */
 
 extern int ladders_write_to_file(void);
 /* outputs the ladders into  files - for the guys that wanna make ladder pages */
 
 extern void ladders_init(void);
 /* initialize the ladders */

 extern void ladders_destroy(void);
 /* remove all ladder data from memory */
 
 extern void ladders_load_accounts_to_ladderlists(void);
 /* enters all accounts from accountlist into the ladders */
 
 extern void ladder_reload_conf(void);
 /* reloads relevant parameters from bnetd.conf (xml/std mode for ladder) */
 
 extern t_account * ladder_get_account(t_ladder *ladder,int rank, unsigned int * teamcount, t_clienttag clienttag);
 /* returns the account that is on specified rank in specified ladder. also return teamcount for AT ladder
  * returns NULL if this rank is still vacant */
 
 extern t_ladder * solo_ladder(t_clienttag clienttag);
 extern t_ladder * team_ladder(t_clienttag clienttag);
 extern t_ladder * ffa_ladder(t_clienttag clienttag);
 extern t_ladder * at_ladder(t_clienttag clienttag);
 extern t_ladder * ladder_ar(t_clienttag clienttag, t_ladder_id ladder_id);
 extern t_ladder * ladder_aw(t_clienttag clienttag, t_ladder_id ladder_id);
 extern t_ladder * ladder_ag(t_clienttag clienttag, t_ladder_id ladder_id);
 extern t_ladder * ladder_cr(t_clienttag clienttag, t_ladder_id ladder_id);
 extern t_ladder * ladder_cw(t_clienttag clienttag, t_ladder_id ladder_id);
 extern t_ladder * ladder_cg(t_clienttag clienttag, t_ladder_id ladder_id);

 /* for external clienttag specific reference of the ladders */

 extern int ladder_get_from_ladder(t_binary_ladder_types type, int rank, int * results);
 extern int ladder_put_into_ladder(t_binary_ladder_types type, int * values);

#endif
#endif

 extern char * create_filename(const char * path, const char * filename, const char * ending);
