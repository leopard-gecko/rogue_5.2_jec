/*
 * Contains functions for dealing with things like potions, scrolls,
 * and other items.
 *
 * @(#)things.c	4.26 (Berkeley) 5/18/82
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980, 1981, 1982 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include "rogue.h"
#include "jec/color.h"

bool got_genocide = FALSE;


#define PRBUF_REMAIN(ptr) (PRBUF_SIZE - (size_t)((ptr) - prbuf))

static void
prbuf_append(const char *s)
{
    char *pb = &prbuf[strlen(prbuf)];

    if (pb < prbuf + PRBUF_SIZE)
    {
        snprintf(pb, PRBUF_REMAIN(pb), "%s", s);
        utf8_trim_partial(prbuf);
    }
}

static void
prbuf_appendf(const char *fmt, ...)
{
    va_list ap;
    char *pb = &prbuf[strlen(prbuf)];

    if (pb >= prbuf + PRBUF_SIZE)
        return;
    va_start(ap, fmt);
    vsnprintf(pb, PRBUF_REMAIN(pb), fmt, ap);
    va_end(ap);
    utf8_trim_partial(prbuf);
}

/*
 * inv_name:
 *	Return the name of something as it would appear in an
 *	inventory.
 */
static char *
inv_name_en(THING *obj, bool drop)
{
    prbuf[0] = '\0';
    switch (obj->o_type)
    {
	case SCROLL:
	    if (obj->o_count == 1)
		snprintf(prbuf, PRBUF_SIZE, "A scroll ");
	    else
		snprintf(prbuf, PRBUF_SIZE, "%d scrolls ", obj->o_count);
	    if (s_know[obj->o_which])
		prbuf_appendf("of %s", s_magic[obj->o_which].mi_name);
	    else if (s_guess[obj->o_which])
		prbuf_appendf("called %s", s_guess[obj->o_which]);
	    else
		prbuf_appendf("titled '%s'", s_names[obj->o_which]);
        when POTION:
	    if (p_know[obj->o_which])
	    {
		if (obj->o_count == 1)
		    snprintf(prbuf, PRBUF_SIZE, "A potion ");
		else
		    snprintf(prbuf, PRBUF_SIZE, "%d potions ", obj->o_count);
		prbuf_appendf("of %s(%s)", p_magic[obj->o_which].mi_name,
		    p_colors[obj->o_which]);
	    }
	    else if (p_guess[obj->o_which])
	    {
		if (obj->o_count == 1)
		    snprintf(prbuf, PRBUF_SIZE, "A potion ");
		else
		    snprintf(prbuf, PRBUF_SIZE, "%d potions ", obj->o_count);
		prbuf_appendf("called %s(%s)", p_guess[obj->o_which],
		    p_colors[obj->o_which]);
	    }
	    else if (obj->o_count == 1)
		snprintf(prbuf, PRBUF_SIZE, "A%s %s potion",
		    vowelstr(p_colors[obj->o_which]), p_colors[obj->o_which]);
	    else
		snprintf(prbuf, PRBUF_SIZE, "%d %s potions", obj->o_count,
		    p_colors[obj->o_which]);
	when FOOD:
	    if (obj->o_which == 1)
	    {
		if (obj->o_count == 1)
		    snprintf(prbuf, PRBUF_SIZE, "A%s %s", vowelstr(fruit), fruit);
		else
		    snprintf(prbuf, PRBUF_SIZE, "%d %ss", obj->o_count, fruit);
	    }
	    else
		if (obj->o_count == 1)
		    snprintf(prbuf, PRBUF_SIZE, "Some food");
		else
		    snprintf(prbuf, PRBUF_SIZE, "%d rations of food", obj->o_count);
	when WEAPON:
	    if (obj->o_count > 1)
		snprintf(prbuf, PRBUF_SIZE, "%d ", obj->o_count);
	    else
		snprintf(prbuf, PRBUF_SIZE, "A%s ", vowelstr(w_names[obj->o_which]));
	    if (obj->o_flags & ISKNOW)
		prbuf_appendf("%s %s", num(obj->o_hplus, obj->o_dplus, WEAPON),
		    w_names[obj->o_which]);
	    else
		prbuf_appendf("%s", w_names[obj->o_which]);
	    if (obj->o_count > 1)
		prbuf_append("s");
	when ARMOR:
	    if (obj->o_flags & ISKNOW)
	    {
		snprintf(prbuf, PRBUF_SIZE, "%s %s [",
		    num(a_class[obj->o_which] - obj->o_ac, 0, ARMOR),
		    a_names[obj->o_which]);
		if (!terse)
		    prbuf_append("armor class ");
		prbuf_appendf("%d]", obj->o_ac);
	    }
	    else
		snprintf(prbuf, PRBUF_SIZE, "%s", a_names[obj->o_which]);
	when AMULET:
	    snprintf(prbuf, PRBUF_SIZE, "The Amulet of Yendor");
	when STICK:
	    if (ws_know[obj->o_which])
	    {
		snprintf(prbuf, PRBUF_SIZE, "A%s %s ", vowelstr(ws_type[obj->o_which]),
		    ws_type[obj->o_which]);
		prbuf_appendf("of %s%s(%s)", ws_magic[obj->o_which].mi_name,
		    charge_str(obj), ws_made[obj->o_which]);
	    }
	    else if (ws_guess[obj->o_which])
	    {
		snprintf(prbuf, PRBUF_SIZE, "A%s %s ", vowelstr(ws_type[obj->o_which]),
		    ws_type[obj->o_which]);
		prbuf_appendf("called %s(%s)", ws_guess[obj->o_which],
		    ws_made[obj->o_which]);
	    }
	    else
		snprintf(prbuf, PRBUF_SIZE, "A%s %s %s",
		    vowelstr(ws_made[obj->o_which]), ws_made[obj->o_which],
		    ws_type[obj->o_which]);
        when RING:
	    if (r_know[obj->o_which])
		snprintf(prbuf, PRBUF_SIZE, "A%s ring of %s(%s)", ring_num(obj),
		    r_magic[obj->o_which].mi_name, r_stones[obj->o_which]);
	    else if (r_guess[obj->o_which])
		snprintf(prbuf, PRBUF_SIZE, "A ring called %s(%s)",
		    r_guess[obj->o_which], r_stones[obj->o_which]);
	    else
		snprintf(prbuf, PRBUF_SIZE, "A%s %s ring", vowelstr(r_stones[obj->o_which]),
		    r_stones[obj->o_which]);
	when GOLD:
		snprintf(prbuf, PRBUF_SIZE, "%d pieces of gold", obj->o_goldval);
#ifdef WIZARD
	otherwise:
	    debug("Picked up something funny %s", unctrol(obj->o_type));
	    snprintf(prbuf, PRBUF_SIZE, "Something bizarre %s", unctrol(obj->o_type));
#endif
    }
    if (obj == cur_armor)
	prbuf_append(" (being worn)");
    if (obj == cur_weapon)
	prbuf_append(" (weapon in hand)");
    if (obj == cur_ring[LEFT])
	prbuf_append(" (on left hand)");
    else if (obj == cur_ring[RIGHT])
	prbuf_append(" (on right hand)");
    if (drop && isupper((unsigned char)prbuf[0]))
	prbuf[0] = tolower((unsigned char)prbuf[0]);
    else if (!drop && islower((unsigned char)*prbuf))
	*prbuf = toupper((unsigned char)*prbuf);
    utf8_trim_partial(prbuf);
    return prbuf;
}

static char *
inv_name_ja(THING *obj, bool drop)
{
    register char *pb;

    (void) drop;
    prbuf[0] = '\0';
    switch (obj->o_type)
    {
	case SCROLL:
	    if (obj->o_count > 1)
		snprintf(prbuf, PRBUF_SIZE, "%dつの", obj->o_count);
	    pb = &prbuf[strlen(prbuf)];
	    if (s_know[obj->o_which])
		snprintf(pb, PRBUF_REMAIN(pb), "%s巻き物",
		    s_magic[obj->o_which].mi_name);
	    else if (s_guess[obj->o_which])
		snprintf(pb, PRBUF_REMAIN(pb), "『%s』と呼ぶ巻き物",
		    s_guess[obj->o_which]);
	    else
		snprintf(pb, PRBUF_REMAIN(pb), "「%s」という巻き物",
		    s_names[obj->o_which]);
	when POTION:
	    if (p_know[obj->o_which])
	    {
		if (obj->o_count == 1)
		    snprintf(prbuf, PRBUF_SIZE, "%s水薬（%s）",
			p_magic[obj->o_which].mi_name, p_colors[obj->o_which]);
		else
		    snprintf(prbuf, PRBUF_SIZE, "%d個の%s水薬（%s）",
			obj->o_count, p_magic[obj->o_which].mi_name,
			p_colors[obj->o_which]);
	    }
	    else if (p_guess[obj->o_which])
	    {
		if (obj->o_count == 1)
		    snprintf(prbuf, PRBUF_SIZE, "『%s』と呼ぶ水薬（%s）",
			p_guess[obj->o_which], p_colors[obj->o_which]);
		else
		    snprintf(prbuf, PRBUF_SIZE,
			"%d個の『%s』と呼ぶ水薬（%s）", obj->o_count,
			p_guess[obj->o_which], p_colors[obj->o_which]);
	    }
	    else if (obj->o_count == 1)
		snprintf(prbuf, PRBUF_SIZE, "%sの水薬", p_colors[obj->o_which]);
	    else
		snprintf(prbuf, PRBUF_SIZE, "%d個の%sの水薬",
		    obj->o_count, p_colors[obj->o_which]);
	when FOOD:
	    if (obj->o_which == 1)
	    {
		if (obj->o_count == 1)
		    prbuf_append(fruit);
		else
		{
		    prbuf_appendf("%d個の", obj->o_count);
		    prbuf_append(fruit);
		}
	    }
	    else
		if (obj->o_count == 1)
		    snprintf(prbuf, PRBUF_SIZE, "%s", "1袋の食糧");
		else
		    snprintf(prbuf, PRBUF_SIZE, "%d袋の食糧", obj->o_count);
	when WEAPON:
	    if (obj->o_count > 1)
		snprintf(prbuf, PRBUF_SIZE, "%d個の", obj->o_count);
	    pb = &prbuf[strlen(prbuf)];
	    if (obj->o_flags & ISKNOW)
		snprintf(pb, PRBUF_REMAIN(pb), "（%s）%s",
		    num(obj->o_hplus, obj->o_dplus, WEAPON), w_names[obj->o_which]);
	    else
		snprintf(pb, PRBUF_REMAIN(pb), "%s", w_names[obj->o_which]);
	when ARMOR:
	    if (obj->o_flags & ISKNOW)
		snprintf(prbuf, PRBUF_SIZE, "（%s）%s［%s%d］",
		    num(a_class[obj->o_which] - obj->o_ac, 0, ARMOR),
		    a_names[obj->o_which], terse ? "" : "守備 ", obj->o_ac);
	    else
		snprintf(prbuf, PRBUF_SIZE, "%s", a_names[obj->o_which]);
	when AMULET:
	    snprintf(prbuf, PRBUF_SIZE, "%s", "イェンダーの魔除け");
	when STICK:
	    if (ws_know[obj->o_which])
		snprintf(prbuf, PRBUF_SIZE, "%s%s%s（%s）",
		    ws_magic[obj->o_which].mi_name, ws_type[obj->o_which],
		    charge_str(obj), ws_made[obj->o_which]);
	    else if (ws_guess[obj->o_which])
		snprintf(prbuf, PRBUF_SIZE, "『%s』と呼ぶ%s（%s）",
		    ws_guess[obj->o_which], ws_type[obj->o_which],
		    ws_made[obj->o_which]);
	    else
		snprintf(prbuf, PRBUF_SIZE, "%sの%s",
		    ws_made[obj->o_which], ws_type[obj->o_which]);
	when RING:
	    if (r_know[obj->o_which])
		snprintf(prbuf, PRBUF_SIZE, "%s指輪%s（%s）",
		    r_magic[obj->o_which].mi_name, ring_num(obj),
		    r_stones[obj->o_which]);
	    else if (r_guess[obj->o_which])
		snprintf(prbuf, PRBUF_SIZE, "『%s』と呼ぶ指輪（%s）",
		    r_guess[obj->o_which], r_stones[obj->o_which]);
	    else
		snprintf(prbuf, PRBUF_SIZE, "%sの指輪", r_stones[obj->o_which]);
	when GOLD:
		snprintf(prbuf, PRBUF_SIZE, "%d個の金塊", obj->o_goldval);
#ifdef WIZARD
	otherwise:
	    debug(JSTR("変なものを拾った。", "Picked up something funny"));
	    snprintf(prbuf, PRBUF_SIZE, "奇妙なもの %s", unctrol(obj->o_type));
#endif
    }
    if (obj == cur_armor)
	prbuf_append("（着ている）");
    if (obj == cur_weapon)
	prbuf_append("（使っている）");
    if (obj == cur_ring[LEFT])
	prbuf_append("（左手）");
    else if (obj == cur_ring[RIGHT])
	prbuf_append("（右手）");
    utf8_trim_partial(prbuf);
    return prbuf;
}

char *
inv_name(THING *obj, bool drop)
{
    return jrogue_lang_is_japanese ? inv_name_ja(obj, drop) : inv_name_en(obj, drop);
}

/*
 * drop:
 *	Put something down
 */
void drop(void)
{
    register char ch;
    register THING *nobj, *op;

    ch = chat(hero.y, hero.x);
    if (ch != FLOOR && ch != PASSAGE)
    {
	after = FALSE;
	msg(JSTR("そこにはすでに何かが置かれている。", "there is something there already"));
	return;
    }
    if ((op = get_item(JSTR("持ちものを落とす", "drop"), 0)) == NULL)
	return;
    if (!dropcheck(op))
	return;
    /*
     * Take it out of the pack
     */
    if (op->o_count >= 2 && op->o_type != WEAPON)
    {
	op->o_count--;
	nobj = new_item();
	*nobj = *op;
	nobj->o_count = 1;
	op = nobj;
	if (op->o_group != 0)
		inpack++;
    }
    else
	detach(pack, op);
    inpack--;
    /*
     * Link it into the level object list
     */
    attach(lvl_obj, op);
    chat(hero.y, hero.x) = op->o_type;
    flat(hero.y, hero.x) |= F_DROPPED;
    op->o_pos = hero;
    if (op->o_type == AMULET)
	amulet = FALSE;
    msg(JSTR("%sを落とした。", "dropped %s"), inv_name(op, TRUE));
}

/*
 * dropcheck:
 *	Do special checks for dropping or unweilding|unwearing|unringing
 */
int dropcheck(THING * op)
{
    if (op == NULL)
	return TRUE;
    if (op != cur_armor && op != cur_weapon
	&& op != cur_ring[LEFT] && op != cur_ring[RIGHT])
	    return TRUE;
    if (op->o_flags & ISCURSED)
    {
	msg(JSTR("それはできない。呪われているようだ。", "you can't.  It appears to be cursed"));
	return FALSE;
    }
    if (op == cur_weapon)
	cur_weapon = NULL;
    else if (op == cur_armor)
    {
	waste_time();
	cur_armor = NULL;
    }
    else
    {
	cur_ring[op == cur_ring[LEFT] ? LEFT : RIGHT] = NULL;
	switch (op->o_which)
	{
	    case R_ADDSTR:
		chg_str(-op->o_ac);
		break;
	    case R_SEEINVIS:
		unsee(0);
		extinguish(unsee);
		break;
	}
    }
    return TRUE;
}

/*
 * new_thing:
 *	Return a new thing
 */
THING * new_thing(void)
{
    register THING *cur;
    register int j, k;

    cur = new_item();
    cur->o_hplus = cur->o_dplus = 0;
    strcpy(cur->o_damage,"0d0");
    strcpy(cur->o_hurldmg,"0d0");
    cur->o_ac = 11;
    cur->o_count = 1;
    cur->o_group = 0;
    cur->o_flags = 0;
    /*
     * Decide what kind of object it will be
     * If we haven't had food for a while, let it be food.
     */
    switch (no_food > 3 ? 2 : pick_one(things, NUMTHINGS))
    {
	case 0:
	    cur->o_type = POTION;
	    cur->o_which = pick_one(p_magic, MAXPOTIONS);
	when 1:
	    cur->o_type = SCROLL;
	    cur->o_which = pick_one(s_magic, MAXSCROLLS);
	    /*
	     * Only one genocide scroll allowed per game, so if it's
	     * the second one, then turn it into a identify scroll
	     */
	    if (cur->o_which == S_GENOCIDE)
		{
		    if (got_genocide)
			cur->o_which = S_IDENT;
		    else
			got_genocide = TRUE;
		}
	when 2:
	    no_food = 0;
	    cur->o_type = FOOD;
	    if (rnd(10) != 0)
		cur->o_which = 0;
	    else
		cur->o_which = 1;
	when 3:
	    cur->o_type = WEAPON;
	    cur->o_which = rnd(MAXWEAPONS);
	    init_weapon(cur, cur->o_which);
	    if ((k = rnd(100)) < 10)
	    {
		cur->o_flags |= ISCURSED;
		cur->o_hplus -= rnd(3) + 1;
	    }
	    else if (k < 15)
		cur->o_hplus += rnd(3) + 1;
	when 4:
	    cur->o_type = ARMOR;
	    for (j = 0, k = rnd(100); j < MAXARMORS; j++)
		if (k < a_chances[j])
		    break;
#ifdef WIZARD
	    if (j == MAXARMORS)
	    {
		debug("Picked a bad armor %d", k);
		j = 0;
	    }
#endif
	    cur->o_which = j;
	    cur->o_ac = a_class[j];
	    if ((k = rnd(100)) < 20)
	    {
		cur->o_flags |= ISCURSED;
		cur->o_ac += rnd(3) + 1;
	    }
	    else if (k < 28)
		cur->o_ac -= rnd(3) + 1;
	when 5:
	    cur->o_type = RING;
	    cur->o_which = pick_one(r_magic, MAXRINGS);
	    switch (cur->o_which)
	    {
		case R_ADDSTR:
		case R_PROTECT:
		case R_ADDHIT:
		case R_ADDDAM:
		    if ((cur->o_ac = rnd(3)) == 0)
		    {
			cur->o_ac = -1;
			cur->o_flags |= ISCURSED;
		    }
		when R_AGGR:
		case R_TELEPORT:
		    cur->o_flags |= ISCURSED;
	    }
	when 6:
	    cur->o_type = STICK;
	    cur->o_which = pick_one(ws_magic, MAXSTICKS);
	    fix_stick(cur);
#ifdef WIZARD
	otherwise:
	    debug("Picked a bad kind of object");
	    wait_for(' ');
#endif
    }
    return cur;
}

/*
 * pick_one:
 *	Pick an item out of a list of nitems possible magic items
 */
int pick_one(struct magic_item * magic, int nitems)
{
    register struct magic_item *end;
    register int i;
    register struct magic_item *start;

    start = magic;
    for (end = &magic[nitems], i = rnd(100); magic < end; magic++)
	if (i < magic->mi_prob)
	    break;
    if (magic == end)
    {
#ifdef WIZARD
	if (wizard)
	{
	    msg("bad pick_one: %d from %d items", i, nitems);
	    for (magic = start; magic < end; magic++)
		msg("%s: %d%%", magic->mi_name, magic->mi_prob);
	}
#endif
	magic = start;
    }
    return magic - start;
}

/*
 * discovered:
 *	list what the player has discovered in this game of a certain type
 */
static int line_cnt = 0;

static bool newpage = FALSE;

static char lastline[PRBUF_SIZE + 32];

void discovered(void)
{
    register char ch;
    register bool disc_list;

    do {
	disc_list = FALSE;
	/* Preserve the original 5.2 English prompt in non-terse mode.
	 * Japanese keeps its natural word order and omits this prefix. */
	if (!jrogue_lang_is_japanese && !terse)
	    addmsg("for ");
	addmsg(JSTR("どの種類", "what type"));
	if (!terse)
	    addmsg(JSTR("を表示しますか", " of object do you want a list"));
	msg(JSTR("？ ('*'ですべて、ESCでキャンセル)", "? (* for all)"));
	ch = readchar();
	switch (ch)
	{
	    case ESCAPE:
		msg("");
		return;
	    case POTION:
	    case SCROLL:
	    case RING:
	    case STICK:
	    case '*':
		disc_list = TRUE;
		break;
	    default:
		if (terse)
		    msg(JSTR("それは表示できない。", "Not a type"));
		else
		    msg(JSTR("%c、%c、%c、%c から選んでください。", "Please type one of %c%c%c%c (ESCAPE to quit)"), POTION, SCROLL, RING, STICK);
	}
    } while (!disc_list);
    if (ch == '*')
    {
	print_disc(POTION);
	add_line("");
	print_disc(SCROLL);
	add_line("");
	print_disc(RING);
	add_line("");
	print_disc(STICK);
	end_line();
    }
    else
    {
	print_disc(ch);
	end_line();
    }
}

/*
 * print_disc:
 *	Print what we've discovered of type 'type'
 */

#define MAX(a,b,c,d)	(a > b ? (a > c ? (a > d ? a : d) : (c > d ? c : d)) : (b > c ? (b > d ? b : d) : (c > d ? c : d)))

void print_disc(char type)
{
    register bool *know = NULL;
    register char **guess = NULL;
    register int i, maxnum = 0, num_found;
    static THING obj;
    static short order[MAX(MAXSCROLLS, MAXPOTIONS, MAXRINGS, MAXSTICKS)];

    switch (type)
    {
	case SCROLL:
	    maxnum = MAXSCROLLS;
	    know = s_know;
	    guess = s_guess;
	    break;
	case POTION:
	    maxnum = MAXPOTIONS;
	    know = p_know;
	    guess = p_guess;
	    break;
	case RING:
	    maxnum = MAXRINGS;
	    know = r_know;
	    guess = r_guess;
	    break;
	case STICK:
	    maxnum = MAXSTICKS;
	    know = ws_know;
	    guess = ws_guess;
	    break;
    }
    set_order(order, maxnum);
    obj.o_count = 1;
    obj.o_flags = 0;
    num_found = 0;
    for (i = 0; i < maxnum; i++)
	if (know[order[i]] || guess[order[i]])
	{
	    obj.o_type = type;
	    obj.o_which = order[i];
	    add_line("%s", inv_name(&obj, FALSE));
	    num_found++;
	}
    if (num_found == 0)
	add_line(nothing(type));
}

/*
 * set_order:
 *	Set up order for list
 */
void set_order(short * order, int numthings)
{
    register int i, r, t;

    for (i = 0; i< numthings; i++)
	order[i] = i;

    for (i = numthings; i > 0; i--)
    {
	r = rnd(i);
	t = order[i - 1];
	order[i - 1] = order[r];
	order[r] = t;
    }
}

/*
 * add_line:
 *	Add a line to the list of discoveries
 */
/* VARARGS1 */
void add_line(const char *fmt, ...)
{
    va_list ap;
    char linebuf[PRBUF_SIZE + 32];
    char clipbuf[PRBUF_SIZE + 32];

    if (line_cnt == 0)
    {
        jrogue_color_apply_window(hw);
	wclear(hw);
	if (slow_invent)
	    mpos = 0;
    }

    if (fmt != NULL)
    {
	va_start(ap, fmt);
	vsnprintf(linebuf, sizeof linebuf, fmt, ap);
	va_end(ap);
	linebuf[sizeof linebuf - 1] = '\0';
	utf8_trim_partial(linebuf);
    }
    else
	linebuf[0] = '\0';

    if (slow_invent)
    {
	if (fmt != NULL && linebuf[0] != '\0')
	    msg("%s", linebuf);
	line_cnt++;
    }
    else
    {
	if (line_cnt >= LINES - 1 || fmt == NULL)
	{
	    mvwaddstr(hw, LINES - 1, 0, JSTR("--スペースを押してください--", "--Press space to continue--"));
	    wrefresh(hw);
	    w_wait_for(hw,' ');
	    clearok(curscr, TRUE);
	    wclear(hw);
            touchwin(stdscr);
	    newpage = TRUE;
	    line_cnt = 0;
	}
	if (fmt != NULL && !(line_cnt == 0 && linebuf[0] == '\0'))
	{
	    utf8_display_truncate(clipbuf, linebuf, COLS - 1, sizeof clipbuf);
	    mvwaddstr(hw, line_cnt++, 0, clipbuf);
	    utf8_strlcpy(lastline, linebuf, sizeof lastline);
	}
    }
}

/*
 * end_line:
 *	End the list of lines
 */
void end_line(void)
{
    if (!slow_invent)
    {
	if (line_cnt == 1 && !newpage)
	{
	    mpos = 0;
	    msg("%s", lastline);
	}
	else
	    add_line(NULL);
    }
    line_cnt = 0;
    newpage = FALSE;
}

/*
 * nothing:
 *	Set up prbuf so that message for "nothing found" is there
 */
char * nothing(char type)
{
    register char *sp;
    register char *tystr = NULL;

    if (terse)
	snprintf(prbuf, PRBUF_SIZE, "%s", JSTR("（知っている", "Nothing"));
    else
	snprintf(prbuf, PRBUF_SIZE, "%s", JSTR("（これまでに明らかにされた", "Haven't discovered anything"));
    if (type != '*')
    {
	sp = &prbuf[strlen(prbuf)];
	switch (type)
	{
	    case POTION: tystr = JSTR("水薬", "potion");
	    when SCROLL: tystr = JSTR("巻き物", "scroll");
	    when RING: tystr = JSTR("指輪", "ring");
	    when STICK: tystr = JSTR("つえや魔法の棒", "stick");
	}
	snprintf(sp, PRBUF_REMAIN(sp), JSTR("%sはない）", " about any %ss"), tystr);
    }
    utf8_trim_partial(prbuf);
    return prbuf;
}
