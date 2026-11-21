/*
 * Defines for things used in mach_dep.c
 *
 * @(#)extern.h	4.3 (Berkeley) 4/2/82
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980, 1981, 1982 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#ifndef ROGUE_52_JEC_EXTERN_H
#define ROGUE_52_JEC_EXTERN_H


/*
 * Don't change the constants, since they are used for sizes in many
 * places in the program.
 */

#define MAXSTR		80	/* maximum length of strings */
#define PRBUF_SIZE	512	/* display/message scratch buffer */
#define MSGHIST_SIZE	512	/* last message buffer for Ctrl-R */
#define PATHSTR		1024	/* path and save-file buffer */
#define MAXLINES	32	/* maximum number of screen lines used */
#define MAXCOLS		80	/* maximum number of screen columns used */

#define RN		(((seed = seed*11109+13849) >> 16) & 0xffff)

/*
 * Now all the global variables
 */

extern bool	after, amulet, askme, door_stop, fight_flush,
		firstmove, in_shell, jump, noscore, p_know[], passgo,
		playing, r_know[], running, s_know[], save_msg,
		slow_invent, terse, wizard, ws_know[];


extern const char *p_colors[], *r_stones[], *w_names[], 
                  *a_names[], *ws_made[];
extern char	_flags[], _level[], file_name[], fruit[],
		home[], huh[], outbuf[], *p_guess[],
		prbuf[], *r_guess[], *release, runch,
		*s_guess[], *s_names[], take, whoami[],
		*ws_guess[], *ws_type[];

extern int	a_chances[], a_class[], count, dnum, food_left,
		fung_hit, fd, group, hungry_state, inpack, lastscore,
		level, max_level, mpos, no_command, no_food, no_move,
		ntraps, purse, quiet, total;

extern long	seed;

extern WINDOW	*hw;

/* Function prototypes are declared in rogue.h. */

extern coord ch_ret;
extern shint countch;
extern shint direction;
extern shint newcount;
extern int   between;
extern int   num_checks;
extern char  lvl_mons[27];
extern char  wand_mons[27];
extern coord nh;
extern bool  got_genocide;

/* Machine-dependent function prototypes live in rogue.h. */

#endif /* ROGUE_52_JEC_EXTERN_H */
