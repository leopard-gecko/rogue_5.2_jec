/*
 * Functions for dealing with problems brought about by weapons
 *
 * @(#)weapons.c	4.14 (Berkeley) 4/6/82
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
#include <stdio.h>
#include "rogue.h"
#include "jec/color.h"

#define NONE 100

static struct init_weps {
    char *iw_dam;	/* Damage when wielded */
    char *iw_hrl;	/* Damage when thrown */
    char iw_launch;	/* Launching weapon */
    int iw_flags;	/* Miscellaneous flags */
} init_dam[MAXWEAPONS] = {
    { "2d4",	"1d3",	NONE, 		0 }, /* Mace */
    { "3d4",	"1d2",	NONE,		0 }, /* Long sword */
    { "1d1",	"1d1",	NONE,		0 }, /* Bow */
    { "1d1",	"2d3",	BOW,		ISMANY|ISMISL }, /* Arrow */
    { "1d6",	"1d4",	NONE,		ISMISL }, /* Dagger */
    { "4d4",	"1d2",	NONE,		0 }, /* 2h sword */
    { "1d1",	"1d3",	NONE,		ISMANY|ISMISL }, /* Dart */
    { "1d1",	"1d1",	NONE,		0 }, /* Crossbow */
    { "1d2",	"2d5",	CROSSBOW,	ISMANY|ISMISL }, /* Crossbow bolt */
    { "2d3",	"1d6",	NONE,		ISMISL }, /* Spear */
};

/*
 * missile:
 *	Fire a missile in a given direction
 */
void missile(int ydelta, int xdelta)
{
    register THING *obj, *nitem;

    /*
     * Get which thing we are hurling
     */
    if ((obj = get_item(JSTR("武器を投げる", "throw"), WEAPON)) == NULL)
	return;
    if (!dropcheck(obj) || is_current(obj))
	return;
    /*
     * Get rid of the thing.  If it is a non-multiple item object, or
     * if it is the last thing, just drop it.  Otherwise, create a new
     * item with a count of one.
     */
    if (obj->o_count < 2)
    {
	detach(pack, obj);
	inpack--;
    }
    else
    {
	obj->o_count--;
	if (obj->o_group == 0)
	    inpack--;
	nitem = new_item();
	*nitem = *obj;
	nitem->o_count = 1;
	obj = nitem;
    }
    do_motion(obj, ydelta, xdelta);
    /*
     * AHA! Here it has hit something.  If it is a wall or a door,
     * or if it misses (combat) the monster, put it on the floor
     */
    if (moat(obj->o_pos.y, obj->o_pos.x) == NULL
	|| !hit_monster(unc(obj->o_pos), obj))
	    fall(obj, TRUE);
}

/*
 * do_motion:
 *	Do the actual motion on the screen done by an object traveling
 *	across the room
 */
void do_motion(THING * obj, int ydelta, int xdelta)
{
    /*
     * Come fly with us ...
     */
    obj->o_pos = hero;
    for (;;)
    {
	register int ch;

	/*
	 * Erase the old one
	 */
	if (!ce(obj->o_pos, hero) && cansee(unc(obj->o_pos)))
	    jrogue_mvaddch(obj->o_pos.y, obj->o_pos.x, chat(obj->o_pos.y, obj->o_pos.x));
	/*
	 * Get the new position
	 */
	obj->o_pos.y += ydelta;
	obj->o_pos.x += xdelta;
	if (step_ok(ch = winat(obj->o_pos.y, obj->o_pos.x)) && ch != DOOR)
	{
	    /*
	     * It hasn't hit anything yet, so display it
	     * If it alright.
	     */
	    if (cansee(unc(obj->o_pos)))
	    {
                if (obj->o_type == '*' && (obj->o_flags & ISMISL))
                    jrogue_mvaddch_projectile(obj->o_pos.y, obj->o_pos.x,
                        obj->o_type, JROGUE_PAIR_PROJECTILE_CYAN);
                else
                    jrogue_mvaddch(obj->o_pos.y, obj->o_pos.x, obj->o_type);
		refresh();
		md_delay(ROGUE_ANIMATION_DELAY_MS);
	    }
	    continue;
	}
	break;
    }
}

/*
 * fall:
 *	Drop an item someplace around here.
 */
void fall(THING * obj, bool pr)
{
    static coord fpos;
    register int index;

    if (fallpos(&obj->o_pos, &fpos, TRUE))
    {
	index = INDEX(fpos.y, fpos.x);
	_level[index] = obj->o_type;
	obj->o_pos = fpos;
	if (cansee(fpos.y, fpos.x))
	{
	    jrogue_mvaddch(fpos.y, fpos.x, obj->o_type);
	    if (_monst[index] != NULL)
		_monst[index]->t_oldch = obj->o_type;
	}
	attach(lvl_obj, obj);
	return;
    }

    if (pr)
		msg(JSTR("%sは地面に落ちると、どこかに消えてしまった。", "the %s vanishes as it hits the ground"), 
			/* BUGFIX: Identification trick */
			(obj->o_type==WEAPON) ? w_names[obj->o_which] : inv_name(obj,TRUE));

	discard(obj);
}

/*
 * init_weapon:
 *	Set up the initial goodies for a weapon
 */
void init_weapon(THING * weap, char type)
{
    register struct init_weps *iwp;

    iwp = &init_dam[(unsigned char)type];
    /* Match the original zero-padded fixed fields without risking a
     * missing terminator when a damage string reaches the field limit. */
    memset(weap->o_damage, 0, sizeof weap->o_damage);
    memset(weap->o_hurldmg, 0, sizeof weap->o_hurldmg);
    snprintf(weap->o_damage, sizeof weap->o_damage, "%s", iwp->iw_dam);
    snprintf(weap->o_hurldmg, sizeof weap->o_hurldmg, "%s", iwp->iw_hrl);
    weap->o_launch = iwp->iw_launch;
    weap->o_flags = iwp->iw_flags;
    if (weap->o_flags & ISMANY)
    {
	weap->o_count = rnd(8) + 8;
	weap->o_group = group++;
    }
    else
	weap->o_count = 1;
}

/*
 * hit_monster:
 *	Does the missile hit the monster?
 */
int hit_monster(int y, int x, THING * obj)
{
    static coord mp;

    mp.y = y;
    mp.x = x;
    return fight(&mp, moat(y, x)->t_type, obj, TRUE);
}

/*
 * num:
 *	Figure out the plus number for armor/weapons
 */
char * num(int n1, int n2, char type)
{
    static char numbuf[10];

    sprintf(numbuf, "%s%d", n1 < 0 ? "" : "+", n1);
    if (type == WEAPON)
	sprintf(&numbuf[strlen(numbuf)], ",%s%d", n2 < 0 ? "" : "+", n2);
    return numbuf;
}

/*
 * wield:
 *	Pull out a certain weapon
 */
void wield(void)
{
    register THING *obj, *oweapon;
    register char *sp;

    oweapon = cur_weapon;
    if (!dropcheck(cur_weapon))
    {
	cur_weapon = oweapon;
	return;
    }
    cur_weapon = oweapon;
    if ((obj = get_item(JSTR("武器を装備する", "wield"), WEAPON)) == NULL)
    {
bad:
	after = FALSE;
	return;
    }

    if (obj->o_type == ARMOR)
    {
	msg(JSTR("よろいを武器にすることはできない。", "you can't wield armor"));
	goto bad;
    }
    if (is_current(obj))
        goto bad;

    sp = inv_name(obj, TRUE);
    cur_weapon = obj;
    if (!terse)
	addmsg(JSTR("武器を", "you are now "));
    msg(JSTR("%s (%c)に持ちかえた。", "wielding %s (%c)"), sp, pack_char(obj));
}

/*
 * fallpos:
 *	Pick a random position around the give (y, x) coordinates
 */
int fallpos(coord * pos, coord * newpos, bool pass)
{
    register int y, x, cnt, ch;

    cnt = 0;
    for (y = pos->y - 1; y <= pos->y + 1; y++)
	for (x = pos->x - 1; x <= pos->x + 1; x++)
	{
	    /*
	     * check to make certain the spot is empty, if it is,
	     * put the object there, set it in the level list
	     * and re-draw the room if he can see it
	     */
	    if (y == hero.y && x == hero.x)
		continue;
	    if (((ch = chat(y, x)) == FLOOR || (pass && ch == PASSAGE))
					&& rnd(++cnt) == 0)
	    {
		newpos->y = y;
		newpos->x = x;
	    }
	}
    return (cnt != 0);
}
