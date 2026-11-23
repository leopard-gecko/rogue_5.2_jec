/*
 * Read and execute the user commands
 *
 * @(#)command.c	4.31 (Berkeley) 4/6/82
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
#include <stdlib.h>
#include <string.h>
#include "rogue.h"
#include "jec/color.h"
#ifdef _WIN32
#include "windows/rogue_windows.h"
#endif

shint countch, direction, newcount = FALSE;

/*
 * command:
 *	Process the user commands
 */
void command(void)
{
    register char ch;
    register int ntimes = 1;			/* Number of player moves */

    if (on(player, ISHASTE))
	ntimes++;
    /*
     * Let the daemons start up
     */
    do_daemons(BEFORE);
    do_fuses(BEFORE);
    while (ntimes--)
    {
	/*
	 * these are illegal things for the player to be, so if any are
	 * set, someone's been poking in memeory
	 */
	if (on(player, ISSLOW|ISCANC|ISGREED|ISINVIS|ISMEAN|ISREGEN))
	    auto_save(-1);

	look(TRUE);
	if (!running)
	    door_stop = FALSE;
	status();
	lastscore = purse;
	move(hero.y, hero.x);
	if (!((running || count) && jump))
	{
            jrogue_restore_cursor();
#ifdef _WIN32
	    jrogue_windows_refresh(stdscr);	/* Draw without cursor flicker */
#else
	    refresh();			/* Draw screen */
#endif
	    if (running)
		md_delay(ROGUE_ANIMATION_DELAY_MS);
	}
	take = 0;
	after = TRUE;
	/*
	 * Read command or continue run
	 */
#ifdef WIZARD
	if (wizard)
	    noscore = TRUE;
#endif
	if (!no_command)
	{
	    if (running) ch = runch;
	    else if (count) ch = countch;
	    else
	    {
		ch = readchar();
		if (mpos != 0 && !running)	/* Erase message if its there */
		    msg("");
	    }
	}
	else
	    ch = '.';
	if (no_command)
	{
	    if (--no_command == 0)
	    {
		player.t_flags |= ISRUN;
		msg(JSTR("また動けるようになった。", "you can move again"));
	    }
	}
	else
	{
	    /*
	     * check for prefixes
	     */
	    if (isdigit(ch))
	    {
		count = 0;
		newcount = TRUE;
		while (isdigit(ch))
		{
		    count = count * 10 + (ch - '0');
		    ch = readchar();
		}
		countch = ch;
		/*
		 * turn off count for commands which don't make sense
		 * to repeat
		 */
		switch (ch) {
		    case 'h': case 'j': case 'k': case 'l':
		    case 'y': case 'u': case 'b': case 'n':
		    case 'H': case 'J': case 'K': case 'L':
		    case 'Y': case 'U': case 'B': case 'N':
		    case 'q': case 'r': case 's': case 'f':
		    case 't': case 'C': case 'I': case '.':
		    case 'z':
#ifdef WIZARD
		    case CTRL('D'): case CTRL('U'):
#endif
			break;
		    default:
			count = 0;
		}
	    }
	    switch (ch)
	    {
		case 'f':
		    if (!on(player, ISBLIND))
		    {
			door_stop = TRUE;
			firstmove = TRUE;
		    }
		    if (count && !newcount)
			ch = direction;
		    else
			ch = readchar();
		    switch (ch)
		    {
			case 'h': case 'j': case 'k': case 'l':
			case 'y': case 'u': case 'b': case 'n':
			    ch = toupper(ch);
		    }
		    direction = ch;
	    }
	    newcount = FALSE;
	    /*
	     * execute a command
	     */
	    if (count && !running)
		count--;
	    switch (ch)
	    {
		case '!' : shell();
		when 'h' : do_move(0, -1);
		when 'j' : do_move(1, 0);
		when 'k' : do_move(-1, 0);
		when 'l' : do_move(0, 1);
		when 'y' : do_move(-1, -1);
		when 'u' : do_move(-1, 1);
		when 'b' : do_move(1, -1);
		when 'n' : do_move(1, 1);
		when 'H' : do_run('h');
		when 'J' : do_run('j');
		when 'K' : do_run('k');
		when 'L' : do_run('l');
		when 'Y' : do_run('y');
		when 'U' : do_run('u');
		when 'B' : do_run('b');
		when 'N' : do_run('n');
		when 't':
		    if (!get_dir())
			after = FALSE;
		    else
			missile(delta.y, delta.x);
		when 'Q' : after = FALSE; quit(-1);
		when 'i' : after = FALSE; inventory(pack, 0);
		when 'I' : after = FALSE; picky_inven();
		when 'd' : drop();
		when 'q' : quaff();
		when 'r' : read_scroll();
		when 'e' : eat();
		when 'w' : wield();
		when 'W' : wear();
		when 'T' : take_off();
		when 'P' : ring_on();
		when 'R' : ring_off();
		when 'o' : option(); after = FALSE;
		when 'c' : call(); after = FALSE;
		when '>' : after = FALSE; d_level();
		when '<' : after = FALSE; u_level();
		when '?' : after = FALSE; help();
		when '/' : after = FALSE; identify();
		when 's' : search();
		when 'z':
		    if (get_dir())
			do_zap();
		    else
			after = FALSE;
		when 'D': after = FALSE; discovered();
		when CTRL('R') : after = FALSE; msg(huh);
		when CTRL('L') :
		    after = FALSE;
		    clearok(curscr,TRUE);
		    wrefresh(curscr);
		when 'v' :
		    after = FALSE;
		    msg("rogue version %s. (mctesq was here)", release);
		when 'S' : 
		    after = FALSE;
		    if (save_game())
		    {
			move(LINES-1, 0); 
			clrtoeol();
			refresh();
			rogue_endwin();
			rogue_clear_plain_screen();
			exit(0);
		    }
		when '.' : ;			/* Rest command */
		when ' ' : after = FALSE;	/* "Legal" illegal command */
		when '^' :
		    after = FALSE;
		    if (get_dir()) {
			delta.y += hero.y;
			delta.x += hero.x;
			if (chat(delta.y, delta.x) != TRAP)
			    msg(JSTR("わながない。", "no trap there"));
			else
			    msg(tr_name(flat(delta.y, delta.x) & F_TMASK));
		    }
#ifdef WIZARD
		when CTRL('P') :
		    after = FALSE;
		    if (wizard)
		    {
			wizard = FALSE;
			turn_see(TRUE);
			msg(JSTR("もはや、魔法使いではない。", "not wizard any more"));
		    }
		    else
		    {
			if ((wizard = passwd()) != 0)
			{
			    noscore = TRUE;
			    turn_see(FALSE);
			    msg(JSTR("あなたは洞窟（#%d）の中で、突然ケン・アーノルドのように賢くなった。", "you are suddenly as smart as Ken Arnold in dungeon #%d"), dnum);
			}
			else
			    msg(JSTR("残念でした。", "sorry"));
		    }
#endif
		when ESCAPE :	/* Escape */
		    door_stop = FALSE;
		    count = 0;
		    after = FALSE;
		otherwise :
		    after = FALSE;
#ifdef WIZARD
		    if (wizard) switch (ch)
		    {
			case '@' : msg("@ %d,%d", hero.y, hero.x);
			when 'C' : create_obj();
			when CTRL('I') : inventory(lvl_obj, 0);
			when CTRL('W') : whatis(FALSE);
			when CTRL('D') : level++; new_level();
			when CTRL('U') : if (level > 1) level--; new_level();
			when CTRL('F') : show_map();
			when CTRL('T') : teleport();
			when CTRL('E') : msg(JSTR("食べものの残り：%d", "food left: %d"), food_left);
			when CTRL('A') : msg(JSTR("荷物の数：%d", "%d things in your pack"), inpack);
			when CTRL('K') : add_pass();
			when CTRL('X') : turn_see(on(player, SEEMONST));
			when CTRL('N') :
			{
			    register THING *item;

			    if ((item = get_item(JSTR("つえをチャージする", "charge"), STICK)) != NULL)
				item->o_charges = 10000;
			}
			when CTRL('H') :
			{
			    register int i;
			    register THING *obj;

			    for (i = 0; i < 9; i++)
				raise_level();
			    /*
			     * Give the rogue a sword (+1,+1)
			     */
			    obj = new_item();
			    obj->o_type = WEAPON;
			    obj->o_which = TWOSWORD;
			    init_weapon(obj, SWORD);
			    obj->o_hplus = 1;
			    obj->o_dplus = 1;
			    obj->o_count = 1;
			    obj->o_group = 0;
			    add_pack(obj, TRUE);
			    cur_weapon = obj;
			    /*
			     * And his suit of armor
			     */
			    obj = new_item();
			    obj->o_type = ARMOR;
			    obj->o_which = PLATE_MAIL;
			    obj->o_ac = -5;
			    obj->o_flags |= ISKNOW;
			    obj->o_count = 1;
			    obj->o_group = 0;
			    cur_armor = obj;
			    add_pack(obj, TRUE);
			}
			otherwise :
			    illcom(ch);
		    }
		    else
#endif
			illcom(ch);
	    }
	    /*
	     * turn off flags if no longer needed
	     */
	    if (!running)
		door_stop = FALSE;
	}
	/*
	 * If he ran into something to take, let him pick it up.
	 */
	if (take != 0)
	    pick_up(take);
	if (!running)
	    door_stop = FALSE;
	if (!after)
	    ntimes++;
    }
    do_daemons(AFTER);
    do_fuses(AFTER);
    if (ISRING(LEFT, R_SEARCH))
	search();
    else if (ISRING(LEFT, R_TELEPORT) && rnd(50) == 0)
	teleport();
    if (ISRING(RIGHT, R_SEARCH))
	search();
    else if (ISRING(RIGHT, R_TELEPORT) && rnd(50) == 0)
	teleport();
}

/*
 * illcom:
 *	What to do with an illegal command
 */
void illcom(char ch)
{
    save_msg = FALSE;
    count = 0;
    msg(JSTR("'%s' コマンドが違います。", "illegal command '%s'"), unctrol(ch));
    save_msg = TRUE;
}

/*
 * search:
 *	Player gropes about him to find hidden things.
 */
void search(void)
{
    register int y, x;
    register char *fp;
    register int ey, ex;

    if (on(player, ISBLIND))
	return;
    ey = hero.y + 1;
    ex = hero.x + 1;
    for (y = hero.y - 1; y <= ey; y++) 
	for (x = hero.x - 1; x <= ex; x++)
	{
	    if (y == hero.y && x == hero.x)
		continue;
	    fp = &flat(y, x);
	    if (!(*fp & F_REAL))
		switch (chat(y, x))
		{
		    case '|':
		    case '-':
			if (rnd(5) != 0)
			    break;
			chat(y, x) = DOOR;
			*fp |= F_REAL;
			count = running = FALSE;
			break;
		    case FLOOR:
			if (rnd(2) != 0)
			    break;
			chat(y, x) = TRAP;
			*fp |= F_REAL;
			count = running = FALSE;
			msg("%s", tr_name(*fp & F_TMASK));
			break;
		}
	}
}

/*
 * help:
 *	Give single character help, or the whole mess if he wants it
 */
void help(void)
{
    register const struct h_list *strp = helpstr;
    register char helpch;
    register int cnt;

    msg(JSTR("どのコマンドのヘルプを表示しますか？ ('*'で全て表示): ", "character you want help for (* for all): "));
    jrogue_show_cursor(TRUE);
    helpch = readchar();
    jrogue_restore_cursor();
    mpos = 0;
    /*
     * If its not a *, print the right help string
     * or an error if he typed a funny character.
     */
    if (helpch != '*')
    {
	move(0, 0);
	while (strp->h_ch)
	{
	    if (strp->h_ch == helpch)
	    {
		msg("%s%s", unctrol(strp->h_ch), strp->h_desc);
		break;
	    }
	    strp++;
	}
	if (strp->h_ch != helpch)
	    msg(JSTR("'%s' コマンドが違います。", "unknown character '%s'"), unctrol(helpch));
	return;
    }
    /*
     * Here we print help for everything.
     * Then wait before we return to command mode
     */
    jrogue_color_apply_window(hw);
    wclear(hw);
    cnt = 0;
    while (strp->h_ch)
    {
	mvwaddstr(hw, cnt % 23, cnt > 22 ? 40 : 0, unctrol(strp->h_ch));
	waddstr(hw, strp->h_desc);
	cnt++;
	strp++;
    }
    wmove(hw, LINES-1, 0);
    wprintw(hw, JSTR("--スペースを押してください--", "--Press space to continue--"));
    wrefresh(hw);
    w_wait_for(hw,' ');
    wmove(stdscr, 0, 0);
    wclrtoeol(stdscr);
    touchwin(stdscr);
    clearok(stdscr, TRUE);
    refresh();
}

/*
 * identify:
 *	Tell the player what a certain thing is.
 */
void identify(void)
{
    register char ch;
    register const char *str;

    msg(JSTR("どの文字？", "what do you want identified? "));
    jrogue_show_cursor(TRUE);
    ch = readchar();
    jrogue_restore_cursor();
    mpos = 0;
    if (ch == ESCAPE)
    {
	msg("");
	return;
    }
    if (isupper(ch))
	str = monsters[ch-'A'].m_name;
    else switch (ch)
    {
	case '|':
	case '-':
	    str = JSTR("部屋の壁", "wall of a room");
	when GOLD: str = JSTR("金塊", "gold");
	when STAIRS : str = JSTR("隣の階への階段", "a staircase");
	when DOOR: str = JSTR("部屋のドア", "door");
	when FLOOR: str = JSTR("部屋の床", "room floor");
	when PLAYER: str = JSTR("プレイヤー", "you");
	when PASSAGE: str = JSTR("部屋と部屋をむすぶ通路", "passage");
	when TRAP: str = JSTR("隠されたわな", "trap");
	when POTION: str = JSTR("水薬", "potion");
	when SCROLL: str = JSTR("巻き物", "scroll");
	when FOOD: str = JSTR("食糧", "food");
	when WEAPON: str = JSTR("武器", "weapon");
	when ' ' : str = JSTR("硬い岩", "solid rock");
	when ARMOR: str = JSTR("よろい", "armor");
	when AMULET: str = JSTR("イェンダーの魔除け", "the Amulet of Yendor");
	when RING: str = JSTR("指輪", "ring");
	when STICK: str = JSTR("つえと魔法の棒", "wand or staff");
	otherwise: str = JSTR("その文字は使われていない", "unknown character");
    }
    msg(JSTR("'%s'：%s", "'%s': %s"), unctrol(ch), str);
}

/*
 * d_level:
 *	He wants to go down a level
 */
void d_level(void)
{
    if (chat(hero.y, hero.x) != STAIRS)
	msg(JSTR("降りる階段がない。", "I see no way down"));
    else
    {
	level++;
	new_level();
    }
}

/*
 * u_level:
 *	He wants to go up a level
 */
void u_level(void)
{
    if (chat(hero.y, hero.x) == STAIRS)
	if (amulet)
	{
	    level--;
	    if (level == 0)
		total_winner();
	    new_level();
	    msg(JSTR("はらわたをねじ切られるような感じだ。", "you feel a wrenching sensation in your gut"));
	}
	else
	    msg(JSTR("階段は不思議な力でふさがれている。", "your way is magically blocked"));
    else
	msg(JSTR("登る階段がない。", "I see no way up"));
}

/*
 * call:
 *	Allow a user to call a potion, scroll, or ring something
 */
void call(void)
{
    register THING *obj;
    register char **guess;
    const char *elsewise;
    register bool *know;

    obj = get_item(JSTR("持ちものに呼び名をつける", "call"), CALLABLE);
    /*
     * Make certain that it is somethings that we want to wear
     */
    if (obj == NULL)
	return;
    switch (obj->o_type)
    {
	case RING:
	    guess = r_guess;
	    know = r_know;
	    elsewise = (r_guess[obj->o_which] != NULL ?
			r_guess[obj->o_which] : r_stones[obj->o_which]);
	when POTION:
	    guess = p_guess;
	    know = p_know;
	    elsewise = (p_guess[obj->o_which] != NULL ?
			p_guess[obj->o_which] : p_colors[obj->o_which]);
	when SCROLL:
	    guess = s_guess;
	    know = s_know;
	    elsewise = (s_guess[obj->o_which] != NULL ?
			s_guess[obj->o_which] : s_names[obj->o_which]);
	when STICK:
	    guess = ws_guess;
	    know = ws_know;
	    elsewise = (ws_guess[obj->o_which] != NULL ?
			ws_guess[obj->o_which] : ws_made[obj->o_which]);
	otherwise:
	    msg(JSTR("それには呼び名をつけることができない。", "you can't call that anything"));
	    return;
    }
    if (know[obj->o_which])
    {
	msg(JSTR("それはすでに鑑別されている。", "that has already been identified"));
	return;
    }
    if (jrogue_lang_is_japanese)
	msg("現在の呼び名は「%s」。", elsewise);
    else
    {
	if (!terse)
	    addmsg("Was ");
	msg("called \"%s\"", elsewise);
    }
    if (terse)
	msg(JSTR("呼び名は？", "call it: "));
    else
	msg(JSTR("どんな呼び名をつける？", "what do you want to call it? "));
    if (guess[obj->o_which] != NULL)
	free(guess[obj->o_which]);
    utf8_strlcpy(prbuf, elsewise, PRBUF_SIZE);
    if (get_str_size(prbuf, stdscr, PRBUF_SIZE) == NORM)
    {
	guess[obj->o_which] = malloc((unsigned int) strlen(prbuf) + 1);
	utf8_strlcpy(guess[obj->o_which], prbuf, strlen(prbuf) + 1);
    }
}
