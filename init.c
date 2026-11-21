/*
 * global variable initializaton
 *
 * @(#)init.c	4.16 (Berkeley) 3/30/82
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980, 1981, 1982 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "rogue.h"

/*
 * init_player:
 *	Roll up the rogue
 */
void init_player(void)
{
    register THING *obj;

    pstats = max_stats;
    food_left = HUNGERTIME;
    /*
     * Give the rogue his weaponry.  First a mace.
     */
    obj = new_item();
    obj->o_type = WEAPON;
    obj->o_which = MACE;
    init_weapon(obj, MACE);
    obj->o_hplus = 1;
    obj->o_dplus = 1;
    obj->o_flags |= ISKNOW;
    obj->o_count = 1;
    obj->o_group = 0;
    add_pack(obj, TRUE);
    cur_weapon = obj;
    /*
     * Now a +1 bow
     */
    obj = new_item();
    obj->o_type = WEAPON;
    obj->o_which = BOW;
    init_weapon(obj, BOW);
    obj->o_hplus = 1;
    obj->o_dplus = 0;
    obj->o_count = 1;
    obj->o_group = 0;
    obj->o_flags |= ISKNOW;
    add_pack(obj, TRUE);
    /*
     * Now some arrows
     */
    obj = new_item();
    obj->o_type = WEAPON;
    obj->o_which = ARROW;
    init_weapon(obj, ARROW);
    obj->o_count = rnd(15) + 25;
    obj->o_hplus = obj->o_dplus = 0;
    obj->o_flags |= ISKNOW;
    add_pack(obj, TRUE);
    /*
     * And his suit of armor
     */
    obj = new_item();
    obj->o_type = ARMOR;
    obj->o_which = RING_MAIL;
    obj->o_ac = a_class[RING_MAIL] - 1;
    obj->o_flags |= ISKNOW;
    obj->o_count = 1;
    obj->o_group = 0;
    cur_armor = obj;
    add_pack(obj, TRUE);
    /*
     * Give him some food too
     */
    obj = new_item();
    obj->o_type = FOOD;
    obj->o_count = 1;
    obj->o_which = 0;
    obj->o_group = 0;
    add_pack(obj, TRUE);
}

/*
 * Contains defintions and functions for dealing with things like
 * potions and scrolls
 */

const char *rainbow[NCOLORS] = {
    "amber",
    "aquamarine",
    "black",
    "blue",
    "brown",
    "clear",
    "crimson",
    "cyan",
    "ecru",
    "gold",
    "green",
    "grey",
    "magenta",
    "orange",
    "pink",
    "plaid",
    "purple",
    "red",
    "silver",
    "tan",
    "tangerine",
    "topaz",
    "turquoise",
    "vermilion",
    "violet",
    "white",
    "yellow",
};

const char *sylls[NSYLLS] = {
    "a",   "ab",  "ag",  "aks", "ala", "an",  "ankh","app", "arg", "arze",
    "ash", "ban", "bar", "bat", "bek", "bie", "bin", "bit", "bjor",
    "blu", "bot", "bu",  "byt", "comp","con", "cos", "cre", "dalf",
    "dan", "den", "do",  "e",   "eep", "el",  "eng", "er",  "ere", "erk",
    "esh", "evs", "fa",  "fid", "for", "fri", "fu",  "gan", "gar",
    "glen","gop", "gre", "ha",  "he",  "hyd", "i",   "ing", "ion", "ip",
    "ish", "it",  "ite", "iv",  "jo",  "kho", "kli", "klis","la",  "lech",
    "man", "mar", "me",  "mi",  "mic", "mik", "mon", "mung","mur",
    "nej", "nelg","nep", "ner", "nes", "nes", "nih", "nin", "o",   "od",
    "ood", "org", "orn", "ox",  "oxy", "pay", "pet", "ple", "plu", "po",
    "pot", "prok","re",  "rea", "rhov","ri",  "ro",  "rog", "rok", "rol",
    "sa",  "san", "sat", "see", "sef", "seh", "shu", "ski", "sna",
    "sne", "snik","sno", "so",  "sol", "sri", "sta", "sun", "ta",
    "tab", "tem", "ther","ti",  "tox", "trol","tue", "turs","u",
    "ulk", "um",  "un",  "uni", "ur",  "val", "viv", "vly", "vom", "wah",
    "wed", "werg","wex", "whon","wun", "xo",  "y",   "yot", "yu",
    "zant","zap", "zeb", "zim", "zok", "zon", "zum",
};

const STONE stones[NSTONES] = {
    { "agate",		 25},
    { "alexandrite",	 40},
    { "amethyst",	 50},
    { "carnelian",	 40},
    { "diamond",	300},
    { "emerald",	300},
    { "germanium",	225},
    { "granite",	  5},
    { "garnet",		 50},
    { "jade",		150},
    { "kryptonite",	300},
    { "lapis lazuli",	 50},
    { "moonstone",	 50},
    { "obsidian",	 15},
    { "onyx",		 60},
    { "opal",		200},
    { "pearl",		220},
    { "peridot",	 63},
    { "ruby",		350},
    { "saphire",	285},
    { "stibotantalite",	200},
    { "tiger eye",	 50},
    { "topaz",		 60},
    { "turquoise",	 70},
    { "taaffeite",	300},
    { "zircon",	 	 80},
};

const char *wood[NWOOD] = {
    "avocado wood",
    "balsa",
    "bamboo",
    "banyan",
    "birch",
    "cedar",
    "cherry",
    "cinnibar",
    "cypress",
    "dogwood",
    "driftwood",
    "ebony",
    "elm",
    "eucalyptus",
    "fall",
    "hemlock",
    "holly",
    "ironwood",
    "kukui wood",
    "mahogany",
    "manzanita",
    "maple",
    "oaken",
    "persimmon wood",
    "pecan",
    "pine",
    "poplar",
    "redwood",
    "rosewood",
    "spruce",
    "teak",
    "walnut",
    "zebrawood",
};

const char *metal[NMETAL] = {
    "aluminum",
    "beryllium",
    "bone",
    "brass",
    "bronze",
    "copper",
    "electrum",
    "gold",
    "iron",
    "lead",
    "magnesium",
    "mercury",
    "nickel",
    "pewter",
    "platinum",
    "steel",
    "silver",
    "silicon",
    "tin",
    "titanium",
    "tungsten",
    "zinc",
};


static const char *rainbow_ja[NCOLORS] = {
    "こはく色", "淡い青緑色", "黒色", "青色", "茶色", "透明",
    "深紅色", "水色", "ベージュ色", "金色", "緑色", "灰色",
    "赤紫色", "オレンジ色", "ピンク色", "格子柄", "紫色", "赤色",
    "銀色", "淡い茶色", "みかん色", "淡褐色", "青緑色", "朱色",
    "すみれ色", "白色", "黄色"
};

static const char *sylls_ja[NSYLLS] = {
    "ア", "アブ", "アグ", "アクス", "アラ", "アン", "アンク", "アプ",
    "アーグ", "アーゼ", "アッシュ", "バン", "バー", "バット", "ベク", "ビー",
    "ビン", "ビット", "ビジョー", "ブル", "ボット", "ブ", "バイト", "コンプ",
    "コン", "コス", "クレ", "ダルフ", "ダン", "デン", "ド", "エ",
    "イープ", "エル", "エング", "エー", "エレ", "イーク", "エッシュ", "エブス",
    "ファ", "フィッド", "フォー", "フライ", "フー", "ガン", "ガー", "グレン",
    "ゴップ", "グレ", "ハー", "ヘ", "ヒド", "イ", "イング", "イオン",
    "イップ", "イッシュ", "イット", "アイト", "イブ", "ジョー", "コー", "クリ",
    "クリス", "ラー", "レック", "マン", "マー", "メ", "マイ", "マイク",
    "ミク", "モン", "ムング", "マー", "ネジュ", "ネルグ", "ネップ", "ナー",
    "ネス", "ネス", "ニー", "ニン", "オ", "オッド", "オード", "オーグ",
    "オルン", "オックス", "オキシー", "ペイ", "ペット", "プル", "プルー", "ポ",
    "ポット", "プロック", "リ", "レー", "ロブ", "リー", "ロ", "ログ",
    "ロク", "ロル", "サ", "サン", "サット", "シー", "セフ", "ゼー",
    "シュ", "スキ", "スネイ", "スネ", "スニック", "スノ", "ソ", "ソル",
    "スリ", "スタ", "サン", "タ", "タブ", "テム", "ザー", "ティ",
    "トックス", "トロール", "チュー", "ターズ", "ウ", "ウルク", "ウム", "ウン",
    "ウニ", "アー", "ヴァル", "ヴィヴ", "ヴライ", "ヴォム", "ワー", "ウェド",
    "ワーグ", "ウェックス", "ホン", "ウン", "クソー", "イ", "ヨット", "ユ",
    "ザント", "ザップ", "ゼブ", "ジム", "ゾック", "ゾン", "ズム"
};

static const char *stones_ja[NSTONES] = {
    "めのう", "アレキサンドライト", "紫水晶", "カーネリアン",
    "ダイヤ", "エメラルド", "ゲルマニウム", "御影石",
    "ざくろ石", "ひすい", "クリプトナイト", "るり",
    "月長石", "黒曜石", "オニキス", "オパール", "真珠",
    "ペリドット", "ルビー", "サファイア", "安タンタル石",
    "虎目石", "トパーズ", "トルコ石", "ターフェアイト", "ジルコン"
};

static const char *wood_ja[NWOOD] = {
    "アボカド", "バルサ", "竹", "バニヤン", "白樺", "杉",
    "桜", "辰砂", "糸杉", "ハナミズキ", "流木", "黒檀",
    "楡", "ユーカリ", "堕落", "ツガ", "ヒイラギ", "鉄樹",
    "ククイノキ", "マホガニー", "マンザニータ", "楓", "樫", "柿",
    "ピーカン", "松", "ポプラ", "アメリカ杉", "紫檀", "トウヒ",
    "チーク", "胡桃", "ゼブラウッド"
};

static const char *metal_ja[NMETAL] = {
    "アルミニウム", "ベリリウム", "骨", "真ちゅう", "青銅", "銅",
    "琥珀金", "金", "鉄", "鉛", "マグネシウム", "水銀",
    "ニッケル", "ピューター", "プラチナ", "鋼鉄", "銀", "シリコン",
    "すず", "チタン", "タングステン", "亜鉛"
};

static const char *potion_color_name(int which)
{
    if (which < 0 || which >= NCOLORS)
	which = 0;
    return jrogue_lang_is_japanese ? rainbow_ja[which] : rainbow[which];
}

static const char *scroll_syll_name(int which)
{
    if (which < 0 || which >= NSYLLS)
	which = 0;
    return jrogue_lang_is_japanese ? sylls_ja[which] : sylls[which];
}

static const char *ring_stone_name(int which)
{
    if (which < 0 || which >= NSTONES)
	which = 0;
    return jrogue_lang_is_japanese ? stones_ja[which] : stones[which].st_name;
}

static const char *stick_material_name(bool woodp, int which)
{
    if (woodp)
    {
	if (which < 0 || which >= NWOOD)
	    which = 0;
	return jrogue_lang_is_japanese ? wood_ja[which] : wood[which];
    }
    if (which < 0 || which >= NMETAL)
	which = 0;
    return jrogue_lang_is_japanese ? metal_ja[which] : metal[which];
}

/*
 * init_things
 *	Initialize the probabilities for types of things
 */
void init_things(void)
{
    register struct magic_item *mp;

    for (mp = &things[1]; mp <= &things[NUMTHINGS-1]; mp++)
	mp->mi_prob += (mp-1)->mi_prob;
#ifdef WIZARD
    badcheck("things", things, NUMTHINGS);
#endif
}

/*
 * init_colors:
 *	Initialize the potion color scheme for this time
 */
void init_colors(void)
{
    register int i, j;
    bool used[NCOLORS];

    for (i = 0; i < NCOLORS; i++)
	used[i] = FALSE;
    for (i = 0; i < MAXPOTIONS; i++)
    {
	do
	    j = rnd(NCOLORS);
	until (!used[j]);
	used[j] = TRUE;
	p_color_index[i] = j;
	p_colors[i] = potion_color_name(j);
	p_know[i] = FALSE;
	p_guess[i] = NULL;
	if (i > 0)
	    p_magic[i].mi_prob += p_magic[i-1].mi_prob;
    }
#ifdef WIZARD
    badcheck("potions", p_magic, MAXPOTIONS);
#endif
}

/*
 * init_names:
 *	Generate the names of the various scrolls
 */
#define MAXNAME	40	/* Max bytes in a scroll title, following 3.6/5.4 */

static size_t
scroll_syll_len(int id)
{
    size_t en_len, ja_len;

    if (id < 0 || id >= NSYLLS)
	return 0;
    en_len = strlen(sylls[id]);
    ja_len = strlen(sylls_ja[id]);
    return en_len > ja_len ? en_len : ja_len;
}

static int
scroll_name_count_and_len(int which, size_t *lenp)
{
    int n, id, count;
    bool pending_space;
    size_t title_len, bytes, add;

    title_len = 0;
    bytes = 0;
    count = 0;
    pending_space = FALSE;

    if (which < 0 || which >= MAXSCROLLS ||
	scroll_syll_count[which] <= 0 ||
	scroll_syll_count[which] > MAX_SCROLL_SYLLABLES)
    {
	if (lenp != NULL)
	    *lenp = 0;
	return 0;
    }

    for (n = 0; n < scroll_syll_count[which]; n++)
    {
	id = scroll_syll_ids[which][n];
	if (id < 0 || id >= NSYLLS)
	    continue;
	add = scroll_syll_len(id);
	if (count > 0 && title_len + (pending_space ? 1 : 0) + add > MAXNAME)
	    break;
	if (pending_space)
	{
	    title_len++;
	    bytes++;
	}
	title_len += add;
	bytes += strlen(scroll_syll_name(id));
	count++;
	pending_space = (scroll_syll_breaks[which][n] && n + 1 < scroll_syll_count[which]);
    }

    if (lenp != NULL)
	*lenp = bytes;
    return count;
}

static void
free_scroll_name(int which)
{
    if (s_names[which] != NULL)
    {
	free(s_names[which]);
	s_names[which] = NULL;
    }
}

void
jrogue_rebuild_scroll_names(void)
{
    register int i, n, id, used, limit;
    bool pending_space;
    size_t len;
    char *name, *cp;
    const char *sp;

    for (i = 0; i < MAXSCROLLS; i++)
    {
	limit = scroll_name_count_and_len(i, &len);
	if (limit <= 0)
	    continue;
	name = (char *) malloc((unsigned) len + 1);
	if (name == NULL)
	    continue;
	cp = name;
	used = 0;
	pending_space = FALSE;
	for (n = 0; n < scroll_syll_count[i] && used < limit; n++)
	{
	    id = scroll_syll_ids[i][n];
	    if (id < 0 || id >= NSYLLS)
		continue;
	    if (pending_space)
		*cp++ = ' ';
	    sp = scroll_syll_name(id);
	    while (*sp)
		*cp++ = *sp++;
	    used++;
	    pending_space = (scroll_syll_breaks[i][n] && n + 1 < scroll_syll_count[i]);
	}
	*cp = '\0';
	free_scroll_name(i);
	s_names[i] = name;
    }
}

void init_names(void)
{
    register int nsyl, sid;
    register int i, nwords, n;
    size_t total, add;
    bool broke;

    for (i = 0; i < MAXSCROLLS; i++)
    {
	nwords = rnd(4) + 2;
	total = 0;
	n = 0;
	broke = FALSE;
	while (nwords-- && n < MAX_SCROLL_SYLLABLES && !broke)
	{
	    nsyl = rnd(3) + 1;
	    while (nsyl-- && n < MAX_SCROLL_SYLLABLES)
	    {
		sid = rnd(NSYLLS);
		add = scroll_syll_len(sid);
		if (total + add > MAXNAME)
		{
		    broke = TRUE;
		    break;
		}
		scroll_syll_ids[i][n] = sid;
		scroll_syll_breaks[i][n] = FALSE;
		total += add;
		n++;
	    }
	    if (n > 0 && !broke && nwords > 0 && total + 1 <= MAXNAME)
	    {
		scroll_syll_breaks[i][n - 1] = TRUE;
		total++;
	    }
	}
	scroll_syll_count[i] = n;
	s_know[i] = FALSE;
	s_guess[i] = NULL;
	if (i > 0)
	    s_magic[i].mi_prob += s_magic[i-1].mi_prob;
    }
    jrogue_rebuild_scroll_names();
#ifdef WIZARD
    badcheck("scrolls", s_magic, MAXSCROLLS);
#endif
}

/*
 * init_stones:
 *	Initialize the ring stone setting scheme for this time
 */
void init_stones(void)
{
    register int i, j;
    bool used[NSTONES];

    for (i = 0; i < NSTONES; i++)
	used[i] = FALSE;
    for (i = 0; i < MAXRINGS; i++)
    {
	do
	    j = rnd(NSTONES);
	until (!used[j]);
	used[j] = TRUE;
	r_stone_index[i] = j;
	r_stones[i] = ring_stone_name(j);
	r_know[i] = FALSE;
	r_guess[i] = NULL;
	if (i > 0)
	    r_magic[i].mi_prob += r_magic[i-1].mi_prob;
	r_magic[i].mi_worth += stones[j].st_value;
    }
#ifdef WIZARD
    badcheck("rings", r_magic, MAXRINGS);
#endif
}

/*
 * init_materials:
 *	Initialize the construction materials for wands and staffs
 */
void init_materials(void)
{
    register int i, j;
    bool metused[NMETAL], woodused[NWOOD];

    for (i = 0; i < NWOOD; i++)
	woodused[i] = FALSE;
    for (i = 0; i < NMETAL; i++)
	metused[i] = FALSE;
    for (i = 0; i < MAXSTICKS; i++)
    {
	for (;;)
	    if (rnd(2) == 0)
	    {
		j = rnd(NMETAL);
		if (!metused[j])
		{
		    ws_material_is_wood[i] = FALSE;
		    ws_material_index[i] = j;
		    metused[j] = TRUE;
		    break;
		}
	    }
	    else
	    {
		j = rnd(NWOOD);
		if (!woodused[j])
		{
		    ws_material_is_wood[i] = TRUE;
		    ws_material_index[i] = j;
		    woodused[j] = TRUE;
		    break;
		}
	    }
	ws_type[i] = (char *) (ws_material_is_wood[i] ? JSTR("つえ", "staff") : JSTR("魔法の棒", "wand"));
	ws_made[i] = stick_material_name(ws_material_is_wood[i], ws_material_index[i]);
	ws_know[i] = FALSE;
	ws_guess[i] = NULL;
	if (i > 0)
	    ws_magic[i].mi_prob += ws_magic[i-1].mi_prob;
    }
#ifdef WIZARD
    badcheck("sticks", ws_magic, MAXSTICKS);
#endif
}

void
jrogue_rebuild_random_names(void)
{
    register int i;

    for (i = 0; i < MAXPOTIONS; i++)
	p_colors[i] = potion_color_name(p_color_index[i]);
    for (i = 0; i < MAXRINGS; i++)
	r_stones[i] = ring_stone_name(r_stone_index[i]);
    for (i = 0; i < MAXSTICKS; i++)
    {
	ws_type[i] = (char *) (ws_material_is_wood[i] ? JSTR("つえ", "staff") : JSTR("魔法の棒", "wand"));
	ws_made[i] = stick_material_name(ws_material_is_wood[i], ws_material_index[i]);
    }
    jrogue_rebuild_scroll_names();
}

void
jrogue_apply_language(void)
{
    register int i;
    static const char *mon_ja[26] = {
	"巨大アリ", "こうもり", "ケンタウロス", "ドラゴン", "浮遊する目",
	"菫の大キノコ", "ノーム", "ホブゴブリン", "見えざる追手",
	"ジャッカル", "コボルド", "レプラコーン", "ミミック", "ニンフ",
	"オーク", "紫の巨虫", "小悪魔", "さびの怪物", "へび",
	"トロル", "褐色の大あご虫", "ヴァンパイア", "死霊", "ゾーン",
	"イエティ", "ゾンビ"
    };
    static const char *mon_en[26] = {
	"giant ant", "bat", "centaur", "dragon", "floating eye",
	"violet fungi", "gnome", "hobgoblin", "invisible stalker",
	"jackal", "kobold", "leprechaun", "mimic", "nymph",
	"orc", "purple worm", "quasit", "rust monster", "snake",
	"troll", "umber hulk", "vampire", "wraith", "xorn",
	"yeti", "zombie"
    };
    static const char *scroll_ja[MAXSCROLLS] = {
	/* S_CONFUSE  */ "怪物を混乱させる",
	/* S_MAP      */ "魔法の地図の",
	/* S_HOLD     */ "怪物を封じこめる",
	/* S_SLEEP    */ "眠りにおちる",
	/* S_ARMOR    */ "よろいに魔法をかける",
	/* S_IDENT    */ "持ちものの種類がわかる",
	/* S_SCARE    */ "怪物を怖がらせる",
	/* S_GFIND    */ "金塊を見つける",
	/* S_TELEP    */ "テレポートする",
	/* S_ENCH     */ "武器に魔法をかける",
	/* S_CREATE   */ "怪物を作りだす",
	/* S_REMOVE   */ "のろいを解く",
	/* S_AGGR     */ "怪物を怒らせる",
	/* S_NOP      */ "白紙の",
	/* S_GENOCIDE */ "怪物を根絶やしにする"
    };
    static const char *scroll_en[MAXSCROLLS] = {
	/* S_CONFUSE  */ "monster confusion",
	/* S_MAP      */ "magic mapping",
	/* S_HOLD     */ "hold monster",
	/* S_SLEEP    */ "sleep",
	/* S_ARMOR    */ "enchant armor",
	/* S_IDENT    */ "identify",
	/* S_SCARE    */ "scare monster",
	/* S_GFIND    */ "gold detection",
	/* S_TELEP    */ "teleportation",
	/* S_ENCH     */ "enchant weapon",
	/* S_CREATE   */ "create monster",
	/* S_REMOVE   */ "remove curse",
	/* S_AGGR     */ "aggravate monsters",
	/* S_NOP      */ "blank paper",
	/* S_GENOCIDE */ "genocide"
    };
    static const char *potion_ja[MAXPOTIONS] = {
	"頭が混乱する", "体がしびれる", "毒の", "強さが増す",
	"見えないものが見える", "傷を治す", "怪物探知の",
	"魔法探知の", "レベルが上がる", "とてもよく傷を治す",
	"素早くなる", "強さが元にもどる", "目が見えなくなる", "のどを潤す"
    };
    static const char *potion_en[MAXPOTIONS] = {
	"confusion", "paralysis", "poison", "gain strength",
	"see invisible", "healing", "monster detection", "magic detection",
	"raise level", "extra healing", "haste self", "restore strength",
	"blindness", "thirst quenching"
    };
    static const char *ring_ja[MAXRINGS] = {
	"守りの", "強さが増す", "強さが減らない", "探索の",
	"見えないものが見える", "飾りものの", "怪物を怒らせる",
	"機敏になる", "ダメージを増やす", "再生の", "消化が遅くなる",
	"テレポートする", "身を隠す", "よろいが丈夫になる"
    };
    static const char *ring_en[MAXRINGS] = {
	"protection", "add strength", "sustain strength", "searching",
	"see invisible", "adornment", "aggravate monster", "dexterity",
	"increase damage", "regeneration", "slow digestion", "teleportation",
	"stealth", "maintain armor"
    };
    static const char *stick_ja[MAXSTICKS] = {
	"灯りの", "打撃の", "いなずまの", "火の", "冷気の",
	"変身させる", "魔法のミサイルの", "怪物を素早くする",
	"怪物を遅くする", "いのちを削る", "役に立たない",
	"怪物を遠くに飛ばす", "怪物を引き寄せる", "魔力を封じる"
    };
    static const char *stick_en[MAXSTICKS] = {
	"light", "striking", "lightning", "fire", "cold", "polymorph",
	"magic missile", "haste monster", "slow monster", "drain life",
	"nothing", "teleport away", "teleport to", "cancellation"
    };
    static const char *armor_ja[MAXARMORS] = {
	"革のよろい", "鉄環のよろい", "鋲つき革のよろい", "鱗のよろい",
	"鎖のよろい", "鉄片のよろい", "帯金のよろい", "全身のよろい"
    };
    static const char *armor_en[MAXARMORS] = {
	"leather armor", "ring mail", "studded leather armor", "scale mail",
	"chain mail", "splint mail", "banded mail", "plate mail"
    };
    static const char *weapon_ja[MAXWEAPONS] = {
	"つちほこ", "長い剣", "短弓", "矢", "短剣", "両手持ちの剣",
	"投げ矢", "クロスボウ", "クロスボウの矢", "やり"
    };
    static const char *weapon_en[MAXWEAPONS] = {
	"mace", "long sword", "short bow", "arrow", "dagger", "two handed sword",
	"dart", "crossbow", "crossbow bolt", "spear"
    };
    static const char *help_ja[] = {
	"	ヘルプを表示する", "	画面の文字が表すものを表示", "	左に進む",
	"	下に進む", "	上に進む", "	右に進む", "	左上に進む",
	"	右上に進む", "	左下に進む", "	右下に進む", "	左に走る",
	"	下に走る", "	上に走る", "	右に走る", "	左上に走る",
	"	右上に走る", "	左下に走る", "	右下に走る", "<方角>	物を投げる",
	"<方角>	何かを見つけるまで進む", "<方角>	つえを振りかざす",
	"<方角>	わなの種類を調べる", "	わなと隠しとびらを探す",
	"	階段を降りる", "	階段を登る", "	しばらく休む", "	持ちものを見る",
	"	１つの持ちものを見る", "	水薬を飲む", "	巻き物を読む", "	食べ物を食べる",
	"	武器を手に持つ", "	よろいを着る", "	よろいを脱ぐ", "	指輪をはめる",
	"	指輪をはずす", "	持ちものを落とす", "	持ちものに呼び名をつける",
	"	今まで何を発見したかを表示", "	オプションの表示・設定", "	画面を描き直す",
	"	最後のメッセージを再表示する", "	コマンドを中止する", "	シェルに抜ける",
	"	ゲームをセーブして終了する", "	ゲームを終了する"
    };
    static const char *help_en[] = {
	"	prints help", "	identify object", "	left", "	down", "	up",
	"	right", "	up & left", "	up & right", "	down & left",
	"	down & right", "	run left", "	run down", "	run up", "	run right",
	"	run up & left", "	run up & right", "	run down & left", "	run down & right",
	"<dir>	throw something", "<dir>	forward until find something",
	"<dir>	zap a wand in a direction", "<dir>	identify trap type",
	"	search for trap/secret door", "	go down a staircase", "	go up a staircase",
	"	rest for a while", "	inventory", "	inventory single item", "	quaff potion",
	"	read paper", "	eat food", "	wield a weapon", "	wear armor",
	"	take armor off", "	put on ring", "	remove ring", "	drop object",
	"	call object", "	recall what's been discovered", "	examine/set options",
	"	redraw screen", "	repeat last message", "	cancel command", "	shell escape",
	"	save game", "	quit"
    };

    for (i = 0; i < 26; i++)
	monsters[i].m_name = (char *) jrogue_str(mon_ja[i], mon_en[i]);
    for (i = 0; i < MAXSCROLLS; i++)
	s_magic[i].mi_name = jrogue_str(scroll_ja[i], scroll_en[i]);
    for (i = 0; i < MAXPOTIONS; i++)
	p_magic[i].mi_name = jrogue_str(potion_ja[i], potion_en[i]);
    for (i = 0; i < MAXRINGS; i++)
	r_magic[i].mi_name = jrogue_str(ring_ja[i], ring_en[i]);
    for (i = 0; i < MAXSTICKS; i++)
	ws_magic[i].mi_name = jrogue_str(stick_ja[i], stick_en[i]);
    for (i = 0; i < MAXARMORS; i++)
	a_names[i] = jrogue_str(armor_ja[i], armor_en[i]);
    for (i = 0; i < MAXWEAPONS; i++)
	w_names[i] = jrogue_str(weapon_ja[i], weapon_en[i]);
    for (i = 0; helpstr[i].h_ch; i++)
	helpstr[i].h_desc = (char *) jrogue_str(help_ja[i], help_en[i]);
    if (fruit[0] == '\0' || strcmp(fruit, "slime-mold") == 0 || strcmp(fruit, "ネバネバかび") == 0)
	utf8_strlcpy(fruit, JSTR("ネバネバかび", "slime-mold"), MAXSTR);

    jrogue_apply_options_language();
    jrogue_apply_rip_language();
    jrogue_rebuild_random_names();
}

#ifdef WIZARD
/*
 * badcheck:
 *	Check to see if a series of probabilities sums to 100
 */
void badcheck(char * name, struct magic_item * magic, int bound)
{
    register struct magic_item *end;

    if (magic[bound - 1].mi_prob == 100)
	return;
    printf("\nBad percentages for %s:\n", name);
    for (end = &magic[bound]; magic < end; magic++)
	printf("%3d%% %s\n", magic->mi_prob, magic->mi_name);
    printf("[hit RETURN to continue]");
    fflush(stdout);
    while (getchar() != '\n')
	continue;
}
#endif
