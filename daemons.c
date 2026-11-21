/*
 * All the daemon and fuse functions are in here
 *
 * @(#)daemons.c	4.10 (Berkeley) 4/6/82
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980, 1981, 1982 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include "rogue.h"
#include "jec/color.h"

int between = 0;

/*
 * doctor:
 *	A healing daemon that restors hit points after rest
 */
int doctor(int ignored)
{
    (void)ignored;
    register int lv, ohp;

    lv = pstats.s_lvl;
    ohp = pstats.s_hpt;
    quiet++;
    if (lv < 8)
    {
	if (quiet + (lv << 1) > 20)
	    pstats.s_hpt++;
    }
    else
	if (quiet >= 3)
	    pstats.s_hpt += rnd(lv - 7) + 1;
    if (ISRING(LEFT, R_REGEN))
	pstats.s_hpt++;
    if (ISRING(RIGHT, R_REGEN))
	pstats.s_hpt++;
    if (ohp != pstats.s_hpt)
    {
	if (pstats.s_hpt > max_hp)
	    pstats.s_hpt = max_hp;
	quiet = 0;
    }
    return 0;
}

/*
 * Swander:
 *	Called when it is time to start rolling for wandering monsters
 */
int swander(int ignored)
{
    (void)ignored;
    rogue_daemon(rollwand, 0, BEFORE);
    return 0;
}

/*
 * rollwand:
 *	Called to roll to see if a wandering monster starts up
 */
int rollwand(int ignored)
{
    (void)ignored;
    if (++between >= 4)
    {
	if (roll(1, 6) == 4)
	{
	    wanderer();
	    kill_daemon(rollwand);
	    fuse(swander, 0, WANDERTIME, BEFORE);
	}
	between = 0;
    }
    return 0;
}

/*
 * unconfuse:
 *	Release the poor player from his confusion
 */
int unconfuse(int ignored)
{
    (void)ignored;
    player.t_flags &= ~ISHUH;
    msg(JSTR("ようやく混乱がおさまってきた。", "you feel less confused now"));
    return 0;
}

/*
 * unsee:
 *	Turn off the ability to see invisible
 */
int unsee(int ignored)
{
    (void)ignored;
    register THING *th;

    for (th = mlist; th != NULL; th = next(th))
	if (on(*th, ISINVIS) && see_monst(th))
	{
	    move(th->t_pos.y, th->t_pos.x);
	    jrogue_addch(th->t_oldch);
	}
    player.t_flags &= ~CANSEE;
    return 0;
}

/*
 * sight:
 *	He gets his sight back
 */
int sight(int ignored)
{
    (void)ignored;
    if (on(player, ISBLIND))
    {
	extinguish(sight);
	player.t_flags &= ~ISBLIND;
	if (!(proom->r_flags & ISGONE))
	    enter_room(&hero);
	msg(JSTR("暗闇のとばりが上がってゆく。", "the veil of darkness lifts"));
    }
    return 0;
}

/*
 * nohaste:
 *	End the hasting
 */
int nohaste(int ignored)
{
    (void)ignored;
    player.t_flags &= ~ISHASTE;
    msg(JSTR("素早くなる薬の効き目がなくなった。", "you feel yourself slowing down"));
    return 0;
}

/*
 * stomach:
 *	Digest the hero's food
 */
int stomach(int ignored)
{
    (void)ignored;
    register int oldfood;

    if (food_left <= 0)
    {
	if (food_left-- < -STARVETIME)
	    death('s');
	/*
	 * the hero is fainting
	 */
	if (no_command || rnd(5) != 0)
	    return 0;
	no_command += rnd(8) + 4;
	player.t_flags &= ~ISRUN;
	running = FALSE;
	count = 0;
	hungry_state = 3;
	if (!terse)
	    addmsg(JSTR("食べていないので体が弱りきっている。", "you feel too weak from lack of food.  "));
	msg(JSTR("気を失った。", "You faint"));
    }
    else
    {
	oldfood = food_left;
	food_left -= ring_eat(LEFT) + ring_eat(RIGHT) + 1 - amulet;

	if (food_left < MORETIME && oldfood >= MORETIME)
	{
	    hungry_state = 2;
	    msg(JSTR("そろそろ力がなくなってきた。", "you are starting to feel weak"));
	}
	else if (food_left < 2 * MORETIME && oldfood >= 2 * MORETIME)
	{
	    hungry_state = 1;
	    if (!terse)
		msg(JSTR("そろそろ空腹になってきた。", "you are starting to get hungry"));
	    else
		msg(JSTR("空腹になってきた。", "getting hungry"));
	}
    }
    return 0;
}
