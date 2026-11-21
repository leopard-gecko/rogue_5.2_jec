/*
 * Hero movement commands
 *
 * @(#)move.c	4.24 (Berkeley) 5/12/82
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980, 1981, 1982 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <ctype.h>
#include <string.h>
#include "rogue.h"
#include "jec/color.h"

/*
 * Used to hold the new hero position
 */

coord nh;

/*
 * Remember the direction at each square during one continuous run.
 * A passage loop can otherwise keep passgo running indefinitely.
 * One byte per map square covers the eight possible directions.
 */
static unsigned char run_visited[MAXLINES][MAXCOLS];

static unsigned char
run_direction_bit(int ch)
{
    switch (ch)
    {
        case 'h': return 1U << 0;
        case 'j': return 1U << 1;
        case 'k': return 1U << 2;
        case 'l': return 1U << 3;
        case 'y': return 1U << 4;
        case 'u': return 1U << 5;
        case 'b': return 1U << 6;
        case 'n': return 1U << 7;
    }
    return 0;
}

/*
 * do_run:
 *	Start the hero running
 */
void do_run(char ch)
{
    /* A fresh run must not inherit the previous run's visited squares. */
    memset(run_visited, 0, sizeof run_visited);
    running = TRUE;
    after = FALSE;
    runch = ch;
}

/*
 * do_move:
 *	Check to see that a move is legal.  If it is handle the
 * consequences (fighting, picking up, etc.)
 */
void do_move(int dy, int dx)
{
    register char ch, fl;

    firstmove = FALSE;
    if (no_move)
    {
	no_move--;
	msg(JSTR("とらばさみのわなでまだ身動きがとれない。", "you are still stuck in the bear trap"));
	return;
    }
    if (running)
    {
        unsigned char bit = run_direction_bit(runch);

        /* Stop before repeating a square and heading: no extra game turn. */
        if (bit != 0 && hero.y > 0 && hero.y < LINES - 1
            && hero.x >= 0 && hero.x < COLS)
        {
            if (run_visited[hero.y][hero.x] & bit)
            {
                running = FALSE;
                after = FALSE;
                return;
            }
            run_visited[hero.y][hero.x] |= bit;
        }
    }
    /*
     * Do a confused move (maybe)
     */
    if (on(player, ISHUH) && rnd(5) != 0)
    {
	nh = *rndmove(&player);
	if (ce(nh, hero))
	{
	    after = FALSE;
	    running = FALSE;
	    return;
	}
    }
    else
    {
over:
	nh.y = hero.y + dy;
	nh.x = hero.x + dx;
    }

    /*
     * Check if he tried to move off the screen or make an illegal
     * diagonal move, and stop him if he did.
     */
    if (nh.x < 0 || nh.x > COLS-1 || nh.y < 1 || nh.y > LINES - 2)
	goto hit_bound;
    if (!diag_ok(&hero, &nh))
    {
	after = FALSE;
	running = FALSE;
	return;
    }
    if (running && ce(hero, nh))
	after = running = FALSE;
    fl = flat(nh.y, nh.x);
    ch = winat(nh.y, nh.x);
    if (!(fl & F_REAL) && ch == FLOOR)
    {
	chat(nh.y, nh.x) = ch = TRAP;
	flat(nh.y, nh.x) |= F_REAL;
    }
    else
    if (on(player, ISHELD) && ch != 'F')
    {
	msg(JSTR("怪物につかまえられている。", "you are being held"));
	return;
    }
    switch (ch)
    {
	case ' ':
	case '|':
	case '-':
hit_bound:
	    if (passgo && running && (proom->r_flags & ISGONE)
		&& !on(player, ISBLIND))
	    {
		register bool	b1, b2;

		switch (runch)
		{
		    case 'h':
		    case 'l':
			b1 = (((flat(hero.y - 1, hero.x) & F_PASS) || chat(hero.y - 1, hero.x) == DOOR) && hero.y != 1);
			b2 = (((flat(hero.y + 1, hero.x) & F_PASS) || chat(hero.y + 1, hero.x) == DOOR) && hero.y != LINES - 2);
			if (!(b1 ^ b2))
			    break;
			if (b1)
			{
			    runch = 'k';
			    dy = -1;
			}
			else
			{
			    runch = 'j';
			    dy = 1;
			}
			dx = 0;
			turnref();
			goto over;
		    case 'j':
		    case 'k':
			b1 = (((flat(hero.y, hero.x - 1) & F_PASS) || chat(hero.y, hero.x - 1) == DOOR) && hero.x != 0);
			b2 = (((flat(hero.y, hero.x + 1) & F_PASS) || chat(hero.y, hero.x + 1) == DOOR) && hero.x != COLS - 1);
			if (!(b1 ^ b2))
			    break;
			if (b1)
			{
			    runch = 'h';
			    dx = -1;
			}
			else
			{
			    runch = 'l';
			    dx = 1;
			}
			dy = 0;
			turnref();
			goto over;
		}
	    }
	    after = running = FALSE;
	    break;
	case DOOR:
	    running = FALSE;
	    if (flat(hero.y, hero.x) & F_PASS)
		enter_room(&nh);
	    goto move_stuff;
	case TRAP:
	    ch = be_trapped(&nh);
	    if (ch == T_DOOR || ch == T_TELEP)
		return;
	    goto move_stuff;
	case PASSAGE:
	    goto move_stuff;
	case FLOOR:
	    if (!(fl & F_REAL))
		be_trapped(&hero);
	    goto move_stuff;
	default:
	    running = FALSE;
	    if (isupper(ch) || moat(nh.y, nh.x))
		fight(&nh, ch, cur_weapon, FALSE);
	    else
	    {
		running = FALSE;
		if (ch != STAIRS)
		    take = ch;
move_stuff:
		jrogue_mvaddch(hero.y, hero.x, chat(hero.y, hero.x));
		if ((fl & F_PASS) && chat(oldpos.y, oldpos.x) == DOOR)
		    leave_room(&nh);
		hero = nh;
	    }
    }
}

/*
 * turnref:
 *	Decide whether to refresh at a passage turning or not
 */
void turnref(void)
{
    register int index;

    index = INDEX(hero.y, hero.x);
    if (!(_flags[index] & F_SEEN))
    {
	if (jump)
	{
	    leaveok(stdscr, TRUE);
	    refresh();
	    leaveok(stdscr, FALSE);
	}
	_flags[index] |= F_SEEN;
    }
}

/*
 * door_open:
 *	Called to illuminate a room.  If it is dark, remove anything
 *	that might move.
 */
void door_open(struct room * rp)
{
    register int j, k;
    register char ch;
    register THING *item;

    if (!(rp->r_flags & ISGONE) && !on(player, ISBLIND))
	for (j = rp->r_pos.y; j < rp->r_pos.y + rp->r_max.y; j++)
	    for (k = rp->r_pos.x; k < rp->r_pos.x + rp->r_max.x; k++)
	    {
		ch = winat(j, k);
		move(j, k);
		if (isupper(ch))
		{
		    item = wake_monster(j, k);
		    if (item->t_oldch == ' ' && !(rp->r_flags & ISDARK)
			&& !on(player, ISBLIND))
			    item->t_oldch = chat(j, k);
		}
	    }
}

/*
 * be_trapped:
 *	The guy stepped on a trap.... Make him pay.
 */
int be_trapped(coord * tc)
{
    register char tr;
    register int index;

    count = running = FALSE;
    index = INDEX(tc->y, tc->x);
    _level[index] = TRAP;
    tr = _flags[index] & F_TMASK;
    switch (tr)
    {
	case T_DOOR:
	    level++;
	    new_level();
	    msg(JSTR("わなの中に落ちた！", "you fell into a trap!"));
	when T_BEAR:
	    no_move += BEARTIME;
	    msg(JSTR("とらばさみのわなにつかまった。", "you are caught in a bear trap"));
	when T_SLEEP:
	    no_command += SLEEPTIME;
	    player.t_flags &= ~ISRUN;
	    msg(JSTR("不思議な白いもやにつつまれ、眠り込んでしまった。", "a strange white mist envelops you and you fall asleep"));
	when T_ARROW:
	    if (swing(pstats.s_lvl-1, pstats.s_arm, 1))
	    {
		pstats.s_hpt -= roll(1, 6);
		if (pstats.s_hpt <= 0)
		{
		    msg(JSTR("矢に撃たれて死んでしまった。", "an arrow killed you"));
		    death('a');
		}
		else
		    msg(JSTR("うわっ！矢に撃たれた。", "oh no! An arrow shot you"));
	    }
	    else
	    {
		register THING *arrow;

		arrow = new_item();
		arrow->o_type = WEAPON;
		arrow->o_which = ARROW;
		init_weapon(arrow, ARROW);
		arrow->o_count = 1;
		arrow->o_pos = hero;
		arrow->o_hplus = arrow->o_dplus = 0;
		fall(arrow, FALSE);
		msg(JSTR("矢が体をかすめていった。", "an arrow shoots past you"));
	    }
	when T_TELEP:
	    teleport();
	    jrogue_mvaddch(tc->y, tc->x, TRAP); /* since the hero's leaving, look()
					    won't put it on for us */
	when T_DART:
	    if (swing(pstats.s_lvl+1, pstats.s_arm, 1))
	    {
		pstats.s_hpt -= roll(1, 4);
		if (pstats.s_hpt <= 0)
		{
		    msg(JSTR("毒の投げ矢が当たり死んでしまった。", "a poisoned dart killed you"));
		    death('d');
		}
		if (!ISWEARING(R_SUSTSTR) && !save(VS_POISON))
		    chg_str(-1);
		msg(JSTR("小さな投げ矢が肩に刺さった。", "a small dart just hit you in the shoulder"));
	    }
	    else
		msg(JSTR("小さな投げ矢が耳をかすめ、どこかに消えてしまった。", "a small dart whizzes by your ear and vanishes"));
    }
    flush_type();
    return tr;
}

/*
 * rndmove:
 *	Move in a random direction if the monster/person is confused
 */
coord * rndmove(THING * who)
{
    register int x, y;
    register char ch;
    register THING *obj;
    static coord ret;  /* what we will be returning */

    y = ret.y = who->t_pos.y + rnd(3) - 1;
    x = ret.x = who->t_pos.x + rnd(3) - 1;
    /*
     * Now check to see if that's a legal move.  If not, don't move.
     * (I.e., bump into the wall or whatever)
     */
    if (y == who->t_pos.y && x == who->t_pos.x)
	return &ret;
    if ((y < 0 || y >= LINES - 1) || (x < 0 || x >= COLS))
	goto bad;
    else if (!diag_ok(&who->t_pos, &ret))
	goto bad;
    else
    {
	ch = winat(y, x);
	if (!step_ok(ch))
	    goto bad;
	if (ch == SCROLL)
	{
	    for (obj = lvl_obj; obj != NULL; obj = next(obj))
		if (y == obj->o_pos.y && x == obj->o_pos.x)
		    break;
	    if (obj != NULL && obj->o_which == S_SCARE)
		goto bad;
	}
    }
    return &ret;

bad:
    ret = who->t_pos;
    return &ret;
}
