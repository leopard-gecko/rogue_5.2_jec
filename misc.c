/*
 * All sorts of miscellaneous routines
 *
 * @(#)misc.c	4.30 (Berkeley) 4/6/82
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980, 1981, 1982 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <curses.h>
#include <ctype.h>
#include <string.h>
#include "rogue.h"
#include "jec/color.h"

/*
 * tr_name:
 *	Print the name of a trap
 */
char * tr_name(char type)
{
    switch (type)
    {
	case T_DOOR:
	    return jrogue_lang_is_japanese ? (terse ? "落とし戸のわな" : "落とし戸のわなを見つけた。") :
		(terse ? "a trapdoor" : "you found a trapdoor");
	case T_BEAR:
	    return jrogue_lang_is_japanese ? (terse ? "とらばさみのわな" : "とらばさみのわなを見つけた。") :
		(terse ? "a beartrap" : "you found a beartrap");
	case T_SLEEP:
	    return jrogue_lang_is_japanese ? (terse ? "眠りガスのわな" : "眠りガスのわなを見つけた。") :
		(terse ? "a sleeping gas trap" : "you found a sleeping gas trap");
	case T_ARROW:
	    return jrogue_lang_is_japanese ? (terse ? "矢のわな" : "矢のわなを見つけた。") :
		(terse ? "an arrow trap" : "you found an arrow trap");
	case T_TELEP:
	    return jrogue_lang_is_japanese ? (terse ? "テレポートのわな" : "テレポートのわなを見つけた。") :
		(terse ? "a teleport trap" : "you found a teleport trap");
	case T_DART:
	    return jrogue_lang_is_japanese ? (terse ? "毒投げ矢のわな" : "毒投げ矢のわなを見つけた。") :
		(terse ? "a dart trap" : "you found a poison dart trap");
    }
    msg(JSTR("変なわな: %d", "wierd trap: %d"), type);
    return NULL;
}

/*
 * look:
 *	A quick glance all around the player
 */
void look(bool wakeup)
{
    register int x, y;
    register unsigned char ch;
    register int index;
    register THING *tp;
    register struct room *rp;
    register int ey, ex;
    register int passcount = 0;
    register char pfl, *fp, pch;
    register int sy, sx, sumhero = 0, diffhero = 0;
    rp = proom;
    if (!ce(oldpos, hero))
    {
	if ((oldrp->r_flags & (ISGONE|ISDARK)) == ISDARK && !on(player,ISBLIND))
	{
	    ey = oldpos.y + 1;
	    ex = oldpos.x + 1;
	    sy = oldpos.y - 1;
	    for (x = oldpos.x - 1; x <= ex; x++)
		for (y = sy; y <= ey; y++)
		{
		    if (y == hero.y && x == hero.x)
			continue;
		    move(y, x);
		    if (CCHAR(inch()) == FLOOR)
			jrogue_addch(' ');
		}
	}
	oldpos = hero;
	oldrp = rp;
    }
    ey = hero.y + 1;
    ex = hero.x + 1;
    sx = hero.x - 1;
    sy = hero.y - 1;
    if (door_stop && !firstmove && running)
    {
	sumhero = hero.y + hero.x;
	diffhero = hero.y - hero.x;
    }
    index = INDEX(hero.y, hero.x);
    pfl = _flags[index];
    pch = _level[index];
    for (y = sy; y <= ey; y++)
	if (y > 0 && y < LINES - 1) for (x = sx; x <= ex; x++)
	{
	    if (x <= 0 || x >= COLS)
		continue;
	    if (!on(player, ISBLIND))
	    {
		if (y == hero.y && x == hero.x)
		    continue;
	    }
	    else if (y != hero.y || x != hero.x)
		continue;

	    index = INDEX(y, x);
	    /*
	     * THIS REPLICATES THE moat() MACRO.  IF MOAT IS CHANGED,
	     * THIS MUST BE CHANGED ALSO
	     */
	    fp = &_flags[index];
	    ch = _level[index];
	    if (pch != DOOR && ch != DOOR)
		{
		    if ((pfl & F_PASS) != (*fp & F_PASS))
			continue;
		    else if ((*fp & F_PASS) && (*fp & F_PNUM) != (pfl & F_PNUM))
			continue;
		}

	    if ((tp = _monst[index]) != NULL)
		{
		    if (on(player, SEEMONST) && on(*tp, ISINVIS))
		    {
			if (door_stop && !firstmove)
			    running = FALSE;
			continue;
		    }
		    else
		    {
		    if (wakeup)
			wake_monster(y, x);
		    if (tp->t_oldch != ' ' ||
			(!(rp->r_flags & ISDARK) && !on(player, ISBLIND)))
			    tp->t_oldch = _level[index];
		    if (see_monst(tp))
			ch = tp->t_disguise;
		}
		}

	    move(y, x);
	    if (ch != CCHAR(inch()))
		jrogue_addch(ch);

	    if (door_stop && !firstmove && running)
	    {
		switch (runch)
		{
		    case 'h':
			if (x == ex)
			    continue;
		    when 'j':
			if (y == sy)
			    continue;
		    when 'k':
			if (y == ey)
			    continue;
		    when 'l':
			if (x == sx)
			    continue;
		    when 'y':
			if ((y + x) - sumhero >= 1)
			    continue;
		    when 'u':
			if ((y - x) - diffhero >= 1)
			    continue;
		    when 'n':
			if ((y + x) - sumhero <= -1)
			    continue;
		    when 'b':
			if ((y - x) - diffhero <= -1)
			    continue;
		}
		switch (ch)
		{
		    case DOOR:
			if (x == hero.x || y == hero.y)
			    running = FALSE;
			break;
		    case PASSAGE:
			if (x == hero.x || y == hero.y)
			    passcount++;
			break;
		    case FLOOR:
		    case '|':
		    case '-':
		    case ' ':
			break;
		    default:
			running = FALSE;
			break;
		}
	    }
	}
    if (door_stop && !firstmove && passcount > 1)
	running = FALSE;
    move(hero.y, hero.x);
    jrogue_addch(PLAYER);
}

/*
 * find_obj:
 *	Find the unclaimed object at y, x
 */
THING * find_obj(int y, int x)
{
    register THING *op;

    for (op = lvl_obj; op != NULL; op = next(op))
    {
	if (op->o_pos.y == y && op->o_pos.x == x)
		return op;
    }
#ifdef WIZARD
    snprintf(prbuf, PRBUF_SIZE, JSTR("物ではない %d,%d", "Non-object %d,%d"), y, x);
    utf8_trim_partial(prbuf);
    debug(prbuf);
#endif
    return NULL;
}

/*
 * eat:
 *	She wants to eat something, so let her try
 */
void eat(void)
{
    register THING *obj;

    if ((obj = get_item(JSTR("食べ物をたべる", "eat"), FOOD)) == NULL)
	return;
    if (obj->o_type != FOOD)
    {
	if (!terse)
	    msg(JSTR("うげっ、そんなものを食べたら病気になってしまう。", "ugh, you would get ill if you ate that"));
	else
	    msg(JSTR("それは食べられない。", "that's Inedible!"));
	return;
    }
    inpack--;
    
    if (food_left < 0)
	food_left = 0;
    if ((food_left += HUNGERTIME - 200 + rnd(400)) > STOMACHSIZE)
	food_left = STOMACHSIZE;
    hungry_state = 0;
    if (obj == cur_weapon)
	cur_weapon = NULL;
    if (obj->o_which == 1)
	msg(JSTR("ああ、美味しい%sだった。", "my, that was a yummy %s"), fruit);
    else
	if (rnd(100) > 70)
	{
	    pstats.s_exp++;
	    msg(JSTR("げげっ、ひどい味だ。", "yuk, this food tastes awful"));
	    check_level();
	}
	else
        {
	    msg(JSTR("うーむ、おいしかった。", "yum, that tasted good"));
        }

    /* Consuming one item is independent of the taste message above. */
    if (--obj->o_count < 1)
    {
	detach(pack, obj);
	discard(obj);
    }
}

/*
 * chg_str:
 *	Used to modify the playes strength.  It keeps track of the
 *	highest it has been, just in case
 */
void chg_str(int amt)
{
    str_t comp;

    if (amt == 0)
	return;
    add_str(&pstats.s_str, amt);
    comp = pstats.s_str;
    if (ISRING(LEFT, R_ADDSTR))
	add_str(&comp, -cur_ring[LEFT]->o_ac);
    if (ISRING(RIGHT, R_ADDSTR))
	add_str(&comp, -cur_ring[RIGHT]->o_ac);
    if (comp > max_stats.s_str)
	max_stats.s_str = comp;
}

/*
 * add_str:
 *	Perform the actual add, checking upper and lower bound limits
 */
void add_str(str_t * sp, int amt)
{
    if ((*sp += amt) < 3)
	*sp = 3;
    else if (*sp > 31)
	*sp = 31;
}

/*
 * add_haste:
 *	Add a haste to the player
 */
int add_haste(bool potion)
{
    if (on(player, ISHASTE))
    {
	no_command += rnd(8);
	player.t_flags &= ~ISRUN;
	extinguish(nohaste);
	player.t_flags &= ~ISHASTE;
	msg(JSTR("消耗して気絶した。", "you faint from exhaustion"));
	return FALSE;
    }
    else
    {
	player.t_flags |= ISHASTE;
	if (potion)
	    fuse(nohaste, 0, rnd(4)+4, AFTER);
	return TRUE;
    }
}

/*
 * aggravate:
 *	Aggravate all the monsters on this level
 */
void aggravate(void)
{
    register THING *mi;

    for (mi = mlist; mi != NULL; mi = next(mi))
	runto(&mi->t_pos, &hero);
}

/*
 * vowelstr:
 *      For printfs: if string starts with a vowel, return "n" for an
 *	"an".
 */
const char *vowelstr(const char *str)
{
    switch (*str)
    {
	case 'a': case 'A':
	case 'e': case 'E':
	case 'i': case 'I':
	case 'o': case 'O':
	case 'u': case 'U':
	    return "n";
	default:
	    return "";
    }
}

/* 
 * is_current:
 *	See if the object is one of the currently used items
 */
int is_current(THING * obj)
{
    if (obj == NULL)
	return FALSE;
    if (obj == cur_armor || obj == cur_weapon || obj == cur_ring[LEFT]
	|| obj == cur_ring[RIGHT])
    {
	if (!terse)
	    addmsg(JSTR("それは", "That's already "));
	msg(JSTR("もう使っている。", "in use"));
	return TRUE;
    }
    return FALSE;
}

/*
 * get_dir:
 *      Set up the direction co_ordinate for use in varios "prefix"
 *	commands
 */
int get_dir(void)
{
    register char *prompt;
    register bool gotit;

    if (!terse)
	msg(prompt = JSTR("どちらの方角？", "which direction? "));
    else
	prompt = JSTR("方角は？", "direction: ");
    do
    {
	gotit = TRUE;
	switch (readchar())
	{
	    case 'h': case'H': delta.y =  0; delta.x = -1;
	    when 'j': case'J': delta.y =  1; delta.x =  0;
	    when 'k': case'K': delta.y = -1; delta.x =  0;
	    when 'l': case'L': delta.y =  0; delta.x =  1;
	    when 'y': case'Y': delta.y = -1; delta.x = -1;
	    when 'u': case'U': delta.y = -1; delta.x =  1;
	    when 'b': case'B': delta.y =  1; delta.x = -1;
	    when 'n': case'N': delta.y =  1; delta.x =  1;
	    when ESCAPE: return FALSE;
	    otherwise:
		mpos = 0;
		msg(prompt);
		gotit = FALSE;
	}
    } until (gotit);
    if (on(player, ISHUH) && rnd(5) == 0)
	do
	{
	    delta.y = rnd(3) - 1;
	    delta.x = rnd(3) - 1;
	} while (delta.y == 0 && delta.x == 0);
    mpos = 0;
    return TRUE;
}

/*
 * sign:
 *	Return the sign of the number
 */
int sign(int nm)
{
    if (nm < 0)
	return -1;
    else
	return (nm > 0);
}

/*
 * spread:
 *	Give a spread around a given number (+/- 10%)
 */
int spread(int nm)
{
    return nm - nm / 10 + rnd(nm / 5);
}

/*
 * call_it:
 *	Call an object something after use.
 */
void call_it(bool know, char * * guess)
{
    if (know && *guess)
    {
	free(*guess);
	*guess = NULL;
    }
    else if (!know && askme && *guess == NULL)
    {
	msg(terse ? JSTR("呼び名は？", "call it: ") : JSTR("どんな呼び名をつける？", "what do you want to call it? "));
	prbuf[0] = '\0';
	if (get_str_size(prbuf, stdscr, PRBUF_SIZE) == NORM)
	{
	    *guess = malloc((unsigned int) strlen(prbuf) + 1);
	    utf8_strlcpy(*guess, prbuf, strlen(prbuf) + 1);
	}
    }
}
