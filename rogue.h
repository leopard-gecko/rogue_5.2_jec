/*
 * Rogue definitions and variable declarations
 *
 * @(#)rogue.h	5.2 (Berkeley) 5/10/82
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980, 1981, 1982 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

/* Prevent duplicate type and structure definitions across the port. */
#ifndef ROGUE_52_JEC_ROGUE_H
#define ROGUE_52_JEC_ROGUE_H


#include "jec/language.h"
#include <sys/stat.h>

#ifndef DEFAULT_SAVEDIR
#define DEFAULT_SAVEDIR ""
#endif

#ifndef DEFAULT_SAVEFILE
#define DEFAULT_SAVEFILE "rogue.save"
#endif

typedef struct { 
    const char *st_name;
    const int   st_value;
} STONE;
   
extern const char *rainbow[];
extern const STONE stones[];
extern const char *sylls[];
extern const char *wood[];
extern const char *metal[];

#define NCOLORS 27
#define NSYLLS  159
#define NSTONES 26
#define NWOOD   33
#define NMETAL  22


#ifndef A_CHARTEXT
#define A_CHARTEXT 0xff
#endif
#define CCHAR(ch) ((char)((ch) & A_CHARTEXT))

/*
 * Maximum number of different things
 */
#define MAXDAEMONS  20
#define MAXROOMS	9
#define MAXTHINGS	9
#define MAXOBJ		9
#define MAXPACK		23
#define MAXTRAPS	10
#define AMULETLEVEL	26
#define	NUMTHINGS	7	/* number of types of things */
#define MAXPASS		13	/* upper limit on number of passages */

/*
 * return values for get functions
 */
#define	NORM	0	/* normal exit */
#define	QUIT	1	/* quit option setting */
#define	MINUS	2	/* back up one option */

/*
 * All the fun defines
 */
#define shint		signed char	/* signed short integer; coordinates use negative sentinels */
#define when		break;case
#define otherwise	break;default
#define until(expr)	while(!(expr))
#define next(ptr)	(*ptr).l_next
#define prev(ptr)	(*ptr).l_prev
#define winat(y,x)	(moat(y,x) != NULL ? moat(y,x)->t_disguise : chat(y,x))
#define DISTANCE(y1, x1, y2, x2) ((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1))
#define ce(a,b)		((a).x == (b).x && (a).y == (b).y)
#define hero		player.t_pos
#define pstats		player.t_stats
#define pack		player.t_pack
#define proom		player.t_room
#define max_hp		player.t_stats.s_maxhp
#define attach(a,b)	_attach(&a,b)
#define detach(a,b)	_detach(&a,b)
#define free_list(a)	_free_list(&a)
#ifndef max
#define max(a,b)	((a) > (b) ? (a) : (b))
#endif
#define on(thing,flag)	(((thing).t_flags & (flag)) != 0)
#undef CTRL
#define CTRL(ch)	(ch & 037)
#define GOLDCALC	(rnd(50 + 10 * level) + 2)
#define ISRING(h,r)	(cur_ring[h] != NULL && cur_ring[h]->o_which == r)
#define ISWEARING(r)	(ISRING(LEFT, r) || ISRING(RIGHT, r))
#define ISMULT(type) 	(type==POTION || type==SCROLL || type==FOOD || type==GOLD)
#define INDEX(y,x)	(((x) << 5) + (y))
#define chat(y,x)	(_level[((x) << 5) + (y)])
#define flat(y,x)	(_flags[((x) << 5) + (y)])
#define moat(y,x)	(_monst[((x) << 5) + (y)])
#define unc(cp)		(cp).y, (cp).x
#ifdef WIZARD
#define debug		if (wizard) msg
#endif

/*
 * Things that appear on the screens
 */
#define PASSAGE		'#'
#define DOOR		'+'
#define FLOOR		'.'
#define PLAYER		'@'
#define TRAP		'^'
#define STAIRS		'%'
#define GOLD		'*'
#define POTION		'!'
#define SCROLL		'?'
#define MAX_SCROLL_SYLLABLES	16
#define MAGIC		'$'
#define FOOD		':'
#define WEAPON		')'
#define ARMOR		']'
#define AMULET		','
#define RING		'='
#define STICK		'/'
#define CALLABLE	-1

/*
 * Various constants
 */
#define	PASSWD		"mTuZ7WUV9RWkQ"
#define	PASSWD54	"mTsdVv0Uvy4xY"
#define BEARTIME	spread(3)
#define SLEEPTIME	spread(5)
#define HEALTIME	spread(30)
#define HOLDTIME	spread(2)
#define WANDERTIME	spread(70)
#define BEFORE		spread(1)
#define AFTER		spread(2)
#define HUHDURATION	spread(20)
#define SEEDURATION	spread(850)
#define HUNGERTIME	spread(1300)
#define MORETIME	150
#define STOMACHSIZE	2000
#define STARVETIME	850
#define ESCAPE		27
#define LEFT		0
#define RIGHT		1
#define BOLT_LENGTH	6
#define LAMPDIST	3

/*
 * Save against things
 */
#define VS_POISON	00
#define VS_PARALYZATION	00
#define VS_DEATH	00
#define VS_BREATH	02
#define VS_MAGIC	03

/*
 * Various flag bits
 */
/* flags for rooms */
#define ISDARK	0000001		/* room is dark */
#define ISGONE	0000002		/* room is gone (a corridor) */

/* flags for objects */
#define ISCURSED 000001		/* object is cursed */
#define ISKNOW	0000002		/* player knows details about the object */
#define ISMISL	0000004		/* object is a missile type */
#define ISMANY	0000010		/* object comes in groups */

/* flags for creatures */
#define CANHUH	0000001		/* creature can confuse */
#define CANSEE	0000002		/* creature can see invisible creatures */
#define ISBLIND	0000004		/* creature is blind */
#define ISCANC	0000010		/* creature has special qualities cancelled */
#define ISFOUND	0000020		/* creature has been seen (used for objects) */
#define ISGREED	0000040		/* creature runs to protect gold */
#define ISHASTE	0000100		/* creature has been hastened */
#define ISHELD	0000400		/* creature has been held */
#define ISHUH	0001000		/* creature is confused */
#define ISINVIS	0002000		/* creature is invisible */
#define ISMEAN	0004000		/* creature can wake when player enters room */
#define ISREGEN	0010000		/* creature can regenerate */
#define ISRUN	0020000		/* creature is running at the player */
#define SEEMONST 040000		/* hero can detect unseen monsters */
#define ISSLOW	0100000		/* creature has been slowed */

/*
 * Flags for level map
 */
#define F_PASS		0x80		/* is a passageway */
#define F_SEEN		0x40		/* have seen this corridor before */
#define F_DROPPED	0x20		/* object was dropped here */
#define F_LOCKED	0x20		/* door is locked */
#define F_REAL		0x10		/* what you see is what you get */
#define F_PNUM		0x0f		/* passage number mask */
#define F_TMASK		0x07		/* trap number mask */

/*
 * Trap types
 */
#define T_DOOR	00
#define T_ARROW	01
#define T_SLEEP	02
#define T_BEAR	03
#define T_TELEP	04
#define T_DART	05
#define NTRAPS	6

/*
 * Potion types
 */
#define P_CONFUSE	0
#define P_PARALYZE	1
#define P_POISON	2
#define P_STRENGTH	3
#define P_SEEINVIS	4
#define P_HEALING	5
#define P_MFIND		6
#define	P_TFIND 	7
#define	P_RAISE		8
#define P_XHEAL		9
#define P_HASTE		10
#define P_RESTORE	11
#define P_BLIND		12
#define P_NOP		13
#define MAXPOTIONS	14

/*
 * Scroll types
 */
#define S_CONFUSE	0
#define S_MAP		1
#define S_HOLD		2
#define S_SLEEP		3
#define S_ARMOR		4
#define S_IDENT		5
#define S_SCARE		6
#define S_GFIND		7
#define S_TELEP		8
#define S_ENCH		9
#define S_CREATE	10
#define S_REMOVE	11
#define S_AGGR		12
#define S_NOP		13
#define S_GENOCIDE	14
#define MAXSCROLLS	15

/*
 * Weapon types
 */
#define MACE		0
#define SWORD		1
#define BOW		2
#define ARROW		3
#define DAGGER		4
#define TWOSWORD	5
#define DART		6
#define CROSSBOW	7
#define BOLT		8
#define SPEAR		9
#define FLAME		10	/* fake entry for dragon breath (ick) */
#define MAXWEAPONS	10	/* this should equal FLAME */

/*
 * Armor types
 */
#define LEATHER		0
#define RING_MAIL	1
#define STUDDED_LEATHER	2
#define SCALE_MAIL	3
#define CHAIN_MAIL	4
#define SPLINT_MAIL	5
#define BANDED_MAIL	6
#define PLATE_MAIL	7
#define MAXARMORS	8

/*
 * Ring types
 */
#define R_PROTECT	0
#define R_ADDSTR	1
#define R_SUSTSTR	2
#define R_SEARCH	3
#define R_SEEINVIS	4
#define R_NOP		5
#define R_AGGR		6
#define R_ADDHIT	7
#define R_ADDDAM	8
#define R_REGEN		9
#define R_DIGEST	10
#define R_TELEPORT	11
#define R_STEALTH	12
#define R_SUSTARM	13
#define MAXRINGS	14

/*
 * Rod/Wand/Staff types
 */

#define WS_LIGHT	0
#define WS_HIT		1
#define WS_ELECT	2
#define WS_FIRE		3
#define WS_COLD		4
#define WS_POLYMORPH	5
#define WS_MISSILE	6
#define WS_HASTE_M	7
#define WS_SLOW_M	8
#define WS_DRAIN	9
#define WS_NOP		10
#define WS_TELAWAY	11
#define WS_TELTO	12
#define WS_CANCEL	13
#define MAXSTICKS	14

extern int r_stone_index[MAXRINGS];
extern int p_color_index[MAXPOTIONS];
extern int ws_material_index[MAXSTICKS];
extern bool ws_material_is_wood[MAXSTICKS];
extern int scroll_syll_count[MAXSCROLLS];
extern int scroll_syll_ids[MAXSCROLLS][MAX_SCROLL_SYLLABLES];
extern bool scroll_syll_breaks[MAXSCROLLS][MAX_SCROLL_SYLLABLES];

/*
 * Now we define the structures and types
 */

/*
 * Help list
 */

struct h_list {
    char h_ch;
    char *h_desc;
};

/*
 * Coordinate data type
 */
typedef struct {
    shint x;
    shint y;
} coord;

/* daemon/fuse data type */

struct delayed_action {
    int d_type;
    int (*d_func)(int);
    int d_arg;
    int d_time;
};

/**/

typedef unsigned int str_t;

/*
 * Stuff about magic items
 */

struct magic_item {
    const char *mi_name;
    shint mi_prob;
    short mi_worth;
};

/*
 * Room structure
 */
struct room {
    coord r_pos;			/* Upper left corner */
    coord r_max;			/* Size of room */
    coord r_gold;			/* Where the gold is */
    int r_goldval;			/* How much the gold is worth */
    short r_flags;			/* Info about the room */
    shint r_nexits;			/* Number of exits */
    coord r_exit[12];			/* Where the exits are */
};

/*
 * Structure describing a fighting being
 */
struct stats {
    str_t s_str;			/* Strength */
    long s_exp;				/* Experience */
    shint s_lvl;			/* Level of mastery */
    shint s_arm;			/* Armor class */
    short s_hpt;			/* Hit points */
    char s_dmg[16];			/* String describing damage done */
    shint s_maxhp;			/* Max hit points */
};

/*
 * Structure for monsters and player
 */
union thing {
    struct {
	union thing *_l_next, *_l_prev;	/* Next pointer in link */
	coord _t_pos;			/* Position */
	bool _t_turn;			/* If slowed, is it a turn to move */
	unsigned char _t_type;		/* What it is */
	char _t_disguise;		/* What mimic looks like */
	char _t_oldch;			/* Character that was where it was */
	coord *_t_dest;			/* Where it is running to */
	short _t_flags;			/* State word */
	struct stats _t_stats;		/* Physical description */
	struct room *_t_room;		/* Current room for thing */
	union thing *_t_pack;		/* What the thing is carrying */
    int  _t_reserved;
    } _t;
    struct {
	union thing *_l_next, *_l_prev;	/* Next pointer in link */
	shint _o_type;			/* What kind of object it is */
	coord _o_pos;			/* Where it lives on the screen */
	char *_o_text;			/* What it says if you read it */
	char _o_launch;			/* What you need to launch it */
	char _o_damage[8];		/* Damage if used like sword */
	char _o_hurldmg[8];		/* Damage if thrown */
	shint _o_count;			/* Count for plural objects */
	shint _o_which;			/* Which object of a type it is */
	shint _o_hplus;			/* Plusses to hit */
	shint _o_dplus;			/* Plusses to damage */
	short _o_ac;			/* Armor class */
	short _o_flags;			/* Information about objects */
	shint _o_group;			/* Group number for this object */
    } _o;
};

typedef union thing THING;

#define l_next		_t._l_next
#define l_prev		_t._l_prev
#define t_pos		_t._t_pos
#define t_turn		_t._t_turn
#define t_type		_t._t_type
#define t_disguise	_t._t_disguise
#define t_oldch		_t._t_oldch
#define t_dest		_t._t_dest
#define t_flags		_t._t_flags
#define t_stats		_t._t_stats
#define t_pack		_t._t_pack
#define t_room		_t._t_room
#define t_reserved      _t._t_reserved
#define o_type		_o._o_type
#define o_pos		_o._o_pos
#define o_text		_o._o_text
#define o_launch	_o._o_launch
#define o_damage	_o._o_damage
#define o_hurldmg	_o._o_hurldmg
#define o_count		_o._o_count
#define o_which		_o._o_which
#define o_hplus		_o._o_hplus
#define o_dplus		_o._o_dplus
#define o_ac		_o._o_ac
#define o_charges	o_ac
#define o_goldval	o_ac
#define o_flags		_o._o_flags
#define o_group		_o._o_group
#define o_reserved      _o._o_reserved

/*
 * Array containing information on all the various types of mosnters
 */
struct monster {
    const char *m_name;			/* What to call the monster */
    const shint m_carry;			/* Probability of carrying something */
    const short m_flags;			/* Things about the monster */
    struct stats m_stats;		/* Initial stats */
};

/*
 * External variables
 */

extern struct delayed_action d_list[20];

extern THING	*_monst[], *cur_armor, *cur_ring[], *cur_weapon,
		*lvl_obj, *mlist, player;

extern coord	delta, oldpos;

extern struct h_list	helpstr[];

extern struct room	*oldrp, passages[], rooms[];

extern struct stats	max_stats;

extern struct monster	monsters[];

extern struct magic_item	p_magic[], r_magic[], s_magic[],
				things[], ws_magic[];

/* Function prototypes are declared below. */

/* Modern prototypes added for Clang/macOS builds. */
#include <stdarg.h>
void wear(void);
void take_off(void);
void waste_time(void);
int runners(int ignored);
int do_chase(THING * th);
int see_monst(THING * mp);
void runto(coord * runner, coord * spot);
int chase(THING * tp, coord * ee);
struct room * roomin(coord * cp);
int diag_ok(coord * sp, coord * ep);
int cansee(int y, int x);
coord * find_dest(THING * tp);
void command(void);
void illcom(char ch);
void search(void);
void help(void);
void identify(void);
void d_level(void);
void u_level(void);
void call(void);
struct delayed_action * d_slot(void);
struct delayed_action *find_slot(int (*func)(int));
void rogue_daemon(int (*func)(int), int arg, int type);
void kill_daemon(int (*func)(int));
void do_daemons(int flag);
void fuse(int (*func)(int), int arg, int time, int type);
void lengthen(int (*func)(int), int xtime);
void extinguish(int (*func)(int));
void do_fuses(int flag);
int doctor(int ignored);
int swander(int ignored);
int rollwand(int ignored);
int unconfuse(int ignored);
int unsee(int ignored);
int sight(int ignored);
int nohaste(int ignored);
int stomach(int ignored);
int fight(coord * mp, char mn, THING * weap, bool thrown);
int attack(THING * mp);
int swing(int at_lvl, int op_arm, int wplus);
void check_level(void);
int roll_em(THING * thatt, THING * thdef, THING * weap, bool hurl);
char *prname(const char *who, bool upper);
void hit(const char *er, const char *ee);
void miss(const char *er, const char *ee);
int save_throw(int which, THING * tp);
int add_dam(str_t str);
int save(int which);
int str_plus(str_t str);
void raise_level(void);
void thunk(THING *weap, const char *mname);
void bounce(THING *weap, const char *mname);
void remove_monster(coord * mp, THING * tp, bool waskill);
int is_magic(THING * obj);
void killed(THING * tp, bool pr);
void init_player(void);
void init_things(void);
void init_colors(void);
void init_names(void);
void init_stones(void);
void init_materials(void);
void badcheck(char *name, struct magic_item *magic, int bound);
int rogue_display_width(const char *s);
void utf8_trim_partial(char *s);
void utf8_display_truncate(char *dst, const char *src, int maxwidth, int dstsize);
void msg(const char *fmt, ...);
void addmsg(const char *fmt, ...);
void endmsg(void);
void doadd(const char *fmt, va_list ap);
int step_ok(int ch);
int readcharw(WINDOW * win);
int readchar(void);
/* Cursor visibility is not saved; input prompts temporarily reveal it. */
extern bool show_cursor;
void jrogue_show_cursor(int show);
void jrogue_restore_cursor(void);
char * unctrol(char ch);
void status(void);
/* Repaint cached status text when presentation attributes change. */
void jrogue_status_redraw(void);
void wait_for(char ch);
void w_wait_for(WINDOW * win, char ch);
void show_win(WINDOW * scr, char * message);
void _detach(THING ** list, THING * item);
void _attach(THING ** list, THING * item);
void _free_list(THING ** ptr);
void discard(THING * item);
THING * new_item(void);
void init_check(void);
void jrogue_rebuild_scroll_names(void);
void jrogue_rebuild_random_names(void);
void open_score(void);
void setup(void);
void start_score(void);
int issymlink(char * sp);
int too_much(void);
int author(void);
void checkout(int s);
void chmsg(const char *fmt, ...);
int lock_sc(void);
void unlock_sc(void);
void flush_type(void);
int main(int argc, char ** argv, char ** envp);
void crash_exit(int sig);
void endit(int a);
void fatal(char * s);
int rnd(int range);
int roll(int number, int sides);
void tstp(int a);
void playit(void);
void quit(int a);
void leave(int sig);
void shell(void);
void md_init(void);
void md_putchar(int c);
void md_raw_standout(void);
void md_raw_standend(void);
/* Portable file operations; filenames are UTF-8 on every platform. */
int md_file_stat(const char *path, struct stat *result);
FILE *md_save_fopen(const char *path);
int md_file_open(const char *path, int flags, int mode);
int md_score_try_lock(int score_fd);
void md_score_unlock(int score_fd);
int md_unlink_open_file(char *file, int inf);
int md_unlink(char *file);
FILE * md_fdopen(int fd, char *mode);
int md_fileno(FILE *fp);
void md_normaluser(void);
int md_getuid(void);
char * md_getusername(int uid);
char * md_gethomedir(void);
#define ROGUE_ANIMATION_DELAY_MS 24
#define ROGUE_STARTUP_DELAY_MS 1000
void md_delay(int ms);
char * md_getshell(void);
void md_ignore_signals(void);
int md_shellescape(void);
char * md_crypt(char *key, char *salt);
char * md_getpass(char *prompt);
unsigned long int md_ntohl(unsigned long int x);
unsigned long int md_htonl(unsigned long int x);
int md_ucount(void);
int md_getloadavg(double *avg);
int md_erasechar(void);
int md_killchar(void);
int reread(void);
void unread(int c);
int md_readchar(WINDOW *win);
char * tr_name(char type);
void look(bool wakeup);
THING * find_obj(int y, int x);
void eat(void);
void chg_str(int amt);
void add_str(str_t * sp, int amt);
int add_haste(bool potion);
void aggravate(void);
const char *vowelstr(const char *str);
int is_current(THING * obj);
int get_dir(void);
int sign(int nm);
int spread(int nm);
void call_it(bool know, char ** guess);
int randmonster(bool wander);
void new_monster(THING * tp, char type, coord * cp);
int exp_add(THING * tp);
void wanderer(void);
THING * wake_monster(int y, int x);
void genocide(void);
void give_pack(THING * tp);
void do_run(char ch);
void do_move(int dy, int dx);
void turnref(void);
void door_open(struct room *rp);
int be_trapped(coord * tc);
coord * rndmove(THING * who);
void new_level(void);
int rnd_room(void);
void put_things(void);
void treas_room(void);
void option(void);
void put_bool(void *opt);
void put_str(void *opt);
int get_bool(void *opt, WINDOW *win);
int get_str(void *opt, WINDOW *win);
int get_str_size(void *opt, WINDOW *win, int bufsiz);
int get_num(short * opt, WINDOW * win);
void parse_opts(char * str);
void strucpy(char * s1, char * s2, int len);
void utf8_strlcpy(char *dst, const char *src, int dstsize);
void update_mdest(THING * obj);
void add_pack(THING * obj, bool silent);
int inventory(THING * list, int type);
void pick_up(char ch);
void picky_inven(void);
THING * get_item(char * purpose, int type);
int pack_char(THING * obj);
void money(int value);
void do_passages(void);
void conn(int r1, int r2);
void door(struct room *rm, coord *cp);
void add_pass(void);
void passnum(void);
void numpass(int y, int x);
void quaff(void);
void invis_on(void);
int turn_see(int turn_off);
void ring_on(void);
void ring_off(void);
int gethand(void);
int ring_eat(int hand);
char * ring_num(THING * obj);
void rogue_clear_plain_screen(void);
void score(int amount, int flags, char monst);
void death(char monst);
void total_winner(void);
char * killname(char monst, bool doart);
void do_rooms(void);
void draw_room(struct room *rp);
void vert(struct room *rp, int startx);
void horiz(struct room *rp, int starty);
void rnd_pos(struct room *rp, coord *cp);
void enter_room(coord * cp);
void leave_room(coord * cp);
int save_game(void);
void auto_save(int sig);
void save_file(FILE * savef);
int restore(char * file, char ** envp);
void encwrite(void * starta, unsigned size, FILE * outf);
int encread(void * starta, unsigned size, int inf);
void read_scroll(void);
void *get_list_item(THING *l, int i);
int find_list_ptr(THING *l, void *ptr);
int list_size(THING *l);
int rs_write(FILE *savef, void *ptr, int size);
int rs_write_char(FILE *savef, char c);
int rs_write_boolean(FILE *savef, bool c);
int rs_write_booleans(FILE *savef, bool *c, int count);
int rs_write_shint(FILE *savef, shint c);
int rs_write_short(FILE *savef, short c);
int rs_write_shorts(FILE *savef, short *c, int count);
int rs_write_ushort(FILE *savef, unsigned short c);
int rs_write_int(FILE *savef, int c);
int rs_write_ints(FILE *savef, int *c, int count);
int rs_write_uint(FILE *savef, unsigned int c);
int rs_write_long(FILE *savef, long c);
int rs_write_longs(FILE *savef, long *c, int count);
int rs_write_ulong(FILE *savef, unsigned long c);
int rs_write_ulongs(FILE *savef, unsigned long *c, int count);
int rs_write_string(FILE *savef, char *s);
int rs_write_string_index(FILE *savef, const char *master[], int max, const char *str);
int rs_write_strings(FILE *savef, char *s[], int count);
int rs_read(int inf, void *ptr, int size);
int rs_read_char(int inf, char *c);
int rs_read_uchar(int inf, unsigned char *c);
int rs_read_boolean(int inf, bool *i);
int rs_read_booleans(int inf, bool *i, int count);
int rs_read_shint(int inf, shint *i);
int rs_read_short(int inf, short *i);
int rs_read_shorts(int inf, short *i, int count);
int rs_read_ushort(int inf, unsigned short *i);
int rs_read_int(int inf, int *i);
int rs_read_ints(int inf, int *i, int count);
int rs_read_uint(int inf, unsigned int *i);
int rs_read_long(int inf, long *i);
int rs_read_longs(int inf, long *i, int count);
int rs_read_ulong(int inf, unsigned long *i);
int rs_read_ulongs(int inf, unsigned long *i, int count);
int rs_read_string(int inf, char *s, int max);
int rs_read_new_string(int inf, char **s);
int rs_read_string_index(int inf, const char *master[], int maxindex, const char **str);
int rs_read_strings(int inf, char **s, int count, int max);
int rs_read_new_strings(int inf, char **s, int count);
int rs_write_str_t(FILE *savef, str_t st);
int rs_read_str_t(int inf, str_t *st);
int rs_write_coord(FILE *savef, coord c);
int rs_read_coord(int inf, coord *c);
void rs_write_window(FILE *savef, WINDOW *win);
int rs_read_window(int inf, WINDOW *win);
int rs_write_daemons(FILE *savef, struct delayed_action *d_list, int count);
int rs_read_daemons(int inf, struct delayed_action *d_list, int count);
int rs_write_magic_items(FILE *savef, struct magic_item *i, int count);
int rs_read_magic_items(int inf, struct magic_item *mi, int count);
int rs_write_room(FILE *savef, struct room *r);
int rs_write_rooms(FILE *savef, struct room r[], int count);
int rs_read_room(int inf, struct room *r);
int rs_read_rooms(int inf, struct room *r, int count);
int rs_write_room_reference(FILE *savef, struct room *rp);
int rs_read_room_reference(int inf, struct room **rp);
int rs_write_stats(FILE *savef, struct stats *s);
int rs_read_stats(int inf, struct stats *s);
int rs_write_object(FILE *savef, THING *o);
int rs_read_object(int inf, THING *o);
int rs_write_object_list(FILE *savef, THING *l);
int rs_read_object_list(int inf, THING **list);
int rs_write_object_reference(FILE *savef, THING *list, THING *item);
int rs_read_object_reference(int inf, THING *list, THING **item);
int find_thing_coord(THING *monlist, coord *c);
int find_room_coord(struct room *rmlist, coord *c, int n);
int find_object_coord(THING *objlist, coord *c);
int rs_write_thing(FILE *savef, THING *t);
void rs_fix_thing(THING *t);
void rs_fix_thing_list(THING *list);
int rs_read_thing(int inf, THING *t);
int rs_write_thing_list(FILE *savef, THING *l);
int rs_read_thing_list(int inf, THING **list);
int rs_write_monsters(FILE *savef, struct monster *m, int count);
int rs_read_monsters(int inf, struct monster *m, int count);
int rs_write_scrolls(FILE *savef);
int rs_read_scrolls(int inf);
int rs_write_potions(FILE *savef);
int rs_read_potions(int inf);
int rs_write_rings(FILE *savef);
int rs_read_rings(int inf);
int rs_write_sticks(FILE *savef);
int rs_read_sticks(int inf);
int rs_write_thing_reference(FILE *savef, THING *list, THING *item);
int rs_read_thing_reference(int inf, THING *list, THING **item);
int rs_write_thing_references(FILE *savef, THING *list, THING *items[], int count);
int rs_read_thing_references(int inf, THING *list, THING *items[], int count);
int rs_save_file(FILE *savef);
int rs_restore_file(int inf);
void fix_stick(THING * cur);
void do_zap(void);
void drain(void);
void fire_bolt(coord * start, coord * dir, char * name);
char * charge_str(THING * obj);
char * inv_name(THING * obj, bool drop);
void drop(void);
int dropcheck(THING * op);
THING * new_thing(void);
int pick_one(struct magic_item *magic, int nitems);
void discovered(void);
void print_disc(char type);
void set_order(short * order, int numthings);
void add_line(const char *fmt, ...);
void end_line(void);
char * nothing(char type);
void missile(int ydelta, int xdelta);
void do_motion(THING * obj, int ydelta, int xdelta);
void fall(THING * obj, bool pr);
void init_weapon(THING * weap, char type);
int hit_monster(int y, int x, THING * obj);
char * num(int n1, int n2, char type);
void wield(void);
int fallpos(coord * pos, coord * newpos, bool pass);
void whatis(bool insist);
void create_obj(void);
int teleport(void);
int passwd(void);
int wizard_check_password(const char *buf);
void show_map(void);
int ascii_to_bin(char ch);
void des_init(void);
void setup_salt(int salt);
int des_setkey(const unsigned char *key);
int do_des(unsigned int l_in, unsigned int r_in, unsigned int *l_out, unsigned int *r_out, int count);
int des_cipher(const unsigned char *in, unsigned char *out, int salt, int count);
char * xcrypt(const char *key, const char *setting);

#include "extern.h"

#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

#endif /* ROGUE_52_JEC_ROGUE_H */
