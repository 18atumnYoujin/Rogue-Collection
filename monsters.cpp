//File with various monster functions in it
//monsters.c  1.4 (A.I. Design)       12/14/84

#include "rogue.h"
#include "monsters.h"
#include "daemons.h"
#include "list.h"
#include "main.h"
#include "chase.h"
#include "new_leve.h"
#include "rooms.h"
#include "things.h"
#include "io.h"
#include "misc.h"
#include "daemon.h"
#include "fight.h"
#include "rings.h"
#include "thing.h"
#include "rooms.h"
#include "level.h"

//List of monsters in rough order of vorpalness
static char *lvl_mons =  "K BHISOR LCA NYTWFP GMXVJD";
static char *wand_mons = "KEBHISORZ CAQ YTW PUGM VJ ";

#define ___  1
#define XX  10

//Array containing information on all the various types of monsters
struct Monster
{
  char *name;         //What to call the monster
  int carry;          //Probability of carrying something
  short flags;        //Things about the monster
  struct Stats stats; //Initial stats
};

struct Monster monsters[26] =
{
  // Name           CARRY                  FLAG       str,  exp,lvl,amr, hpt, dmg
  { "aquator",          0,                 IS_MEAN,  { XX,   20,  5,  2, ___, "0d0/0d0"         } },
  { "bat",              0,                  IS_FLY,  { XX,    1,  1,  3, ___, "1d2"             } },
  { "centaur",         15,                       0,  { XX,   25,  4,  4, ___, "1d6/1d6"         } },
  { "dragon",         100,                 IS_MEAN,  { XX, 6800, 10, -1, ___, "1d8/1d8/3d10"    } },
  { "emu",              0,                 IS_MEAN,  { XX,    2,  1,  7, ___, "1d2"             } },
  //NOTE: the damage is %%% so that xstr won't merge this string with others, since it is written on in the program
  { "venus flytrap",    0,                 IS_MEAN,  { XX,   80,  8,  3, ___, "%%%d0"           } },
  { "griffin",         20, IS_MEAN|IS_FLY|IS_REGEN,  { XX, 2000, 13,  2, ___, "4d3/3d5/4d3"     } },
  { "hobgoblin",        0,                 IS_MEAN,  { XX,    3,  1,  5, ___, "1d8"             } },
  { "ice monster",      0,                 IS_MEAN,  { XX,   15,  1,  9, ___, "1d2"             } },
  { "jabberwock",      70,                       0,  { XX, 4000, 15,  6, ___, "2d12/2d4"        } },
  { "kestral",          0,          IS_MEAN|IS_FLY,  { XX,    1,  1,  7, ___, "1d4"             } },
  { "leprechaun",       0,                       0,  { XX,   10,  3,  8, ___, "1d2"             } },
  { "medusa",          40,                 IS_MEAN,  { XX,  200,  8,  2, ___, "3d4/3d4/2d5"     } },
  { "nymph",          100,                       0,  { XX,   37,  3,  9, ___, "0d0"             } },
  { "orc",             15,                IS_GREED,  { XX,    5,  1,  6, ___, "1d8"             } },
  { "phantom",          0,                IS_INVIS,  { XX,  120,  8,  3, ___, "4d4"             } },
  { "quagga",          30,                 IS_MEAN,  { XX,   32,  3,  2, ___, "1d2/1d2/1d4"     } },
  { "rattlesnake",      0,                 IS_MEAN,  { XX,    9,  2,  3, ___, "1d6"             } },
  { "slime",            0,                 IS_MEAN,  { XX,    1,  2,  8, ___, "1d3"             } },
  { "troll",           50,        IS_REGEN|IS_MEAN,  { XX,  120,  6,  4, ___, "1d8/1d8/2d6"     } },
  { "ur-vile",          0,                 IS_MEAN,  { XX,  190,  7, -2, ___, "1d3/1d3/1d3/4d6" } },
  { "vampire",         20,        IS_REGEN|IS_MEAN,  { XX,  350,  8,  1, ___, "1d10"            } },
  { "wraith",           0,                       0,  { XX,   55,  5,  4, ___, "1d6"             } },
  { "xeroc",           30,                       0,  { XX,  100,  7,  7, ___, "3d4"             } },
  { "yeti",            30,                       0,  { XX,   50,  4,  6, ___, "1d6/1d6"         } },
  { "zombie",           0,                 IS_MEAN,  { XX,    6,  2,  8, ___, "1d8"             } }
};

char f_damage[10];

#undef ___
#undef XX

const char* get_monster_name(char monster)
{
  return monsters[monster-'A'].name;
}

int get_monster_carry_prob(char monster)
{
  return monsters[monster-'A'].carry;
}

const char* Agent::get_monster_name() const
{
    return ::get_monster_name(type);
}

int Agent::get_monster_carry_prob() const
{
    return ::get_monster_carry_prob(type);
}


//randmonster: Pick a monster to show up.  The lower the level, the meaner the monster.
char randmonster(bool wander, int level)
{
  int d;
  char *mons;

  mons = wander?wand_mons:lvl_mons;
  do
  {
    int r10 = rnd(5)+rnd(6);

    d = level+(r10-5);
    if (d<1) d = rnd(5)+1;
    if (d>26) d = rnd(5)+22;
  } while (mons[--d]==' ');
  return mons[d];
}

void set_xeroc_disguise(AGENT* X)
{
  switch (rnd(get_level() >= AMULETLEVEL ? 9 : 8))
  {
  case 0: X->disguise = GOLD; break;
  case 1: X->disguise = POTION; break;
  case 2: X->disguise = SCROLL; break;
  case 3: X->disguise = STAIRS; break;
  case 4: X->disguise = WEAPON; break;
  case 5: X->disguise = ARMOR; break;
  case 6: X->disguise = RING; break;
  case 7: X->disguise = STICK; break;
  case 8: X->disguise = AMULET; break;
  }
}

//new_monster: Pick a new monster and add it to the list
void new_monster(AGENT *monster, byte type, Coord *position, int level)
{
  int level_add = (level <= AMULETLEVEL) ? 0 : level-AMULETLEVEL;
  const struct Monster* defaults;
  
  defaults = &monsters[type-'A'];
  attach_agent(&mlist, monster);

  monster->type = type;
  monster->disguise = type;
  monster->pos = *position;
  monster->oldch = '@';
  monster->room = roomin(position);
  monster->flags = defaults->flags;
  monster->stats = defaults->stats;
  monster->stats.level += level_add;
  monster->stats.hp = monster->stats.max_hp = roll(monster->stats.level, 8);
  monster->stats.ac -= level_add;
  monster->stats.exp += level_add*10 + exp_add(monster);
  monster->turn = TRUE;
  monster->pack = NULL;

  //todo: remove F,X checks
  if (type=='F') 
    monster->stats.damage = f_damage;
  if (monster->is_mimic()) 
    set_xeroc_disguise(monster);

  if (is_wearing_ring(R_AGGR)) 
    start_run(monster);
}

//f_restor(): restor initial damage string for flytraps
void f_restor()
{
  const struct Monster *monster = &monsters['F'-'A'];
  flytrap_hit = 0;
  strcpy(f_damage, monster->stats.damage);
}

//expadd: Experience to add for this monster's level/hit points
int exp_add(AGENT *monster)
{
  int divisor = (monster->stats.level == 1) ? 8 : 6;
  int value = monster->stats.max_hp / divisor;

  if (monster->stats.level>9) 
    value *= 20;
  else if (monster->stats.level>6) 
    value *= 4;
  
  return value;
}

//wanderer: Create a new wandering monster and aim it at the player
void wanderer()
{
  struct Room *room;
  AGENT *monster;
  Coord cp;

  //can we allocate a new monster
  if ((monster = create_agent())==NULL) return;
  do
  {
    room = rnd_room();
    if (room==player.room) continue;
    rnd_pos(room, &cp);
  } while (!(room!=player.room && step_ok(get_tile_or_monster(cp.y, cp.x))));
  new_monster(monster, randmonster(TRUE, get_level()), &cp, get_level());
  if (bailout) debug("wanderer bailout");
  //debug("started a wandering %s", monsters[tp->type-'A'].m_name);
  start_run(monster);
}

//wake_monster: What to do when the hero steps next to a monster
AGENT *wake_monster(int y, int x)
{
  AGENT *monster;
  struct Room *room;
  int dst;

  if ((monster = monster_at(y, x))==NULL) return monster;
  //Every time he sees mean monster, it might start chasing him
  if (!monster->is_flag_set(IS_RUN) && rnd(3)!=0 && monster->is_flag_set(IS_MEAN) && !monster->is_flag_set(IS_HELD) && !is_wearing_ring(R_STEALTH))
  {
    monster->dest = &player.pos;
    monster->flags |= IS_RUN;
  }
  if (monster->causes_confusion() && !player.is_flag_set(IS_BLIND) && !monster->is_flag_set(IS_FOUND) && !monster->is_flag_set(IS_CANC) && monster->is_flag_set(IS_RUN))
  {
    room = player.room;
    dst = DISTANCE(y, x, player.pos.y, player.pos.x);
    if ((room!=NULL && !(room->flags&IS_DARK)) || dst<LAMP_DIST)
    {
      monster->flags |= IS_FOUND;
      if (!save(VS_MAGIC))
      {
        if (player.is_flag_set(IS_HUH)) lengthen(unconfuse, rnd(20)+HUH_DURATION);
        else fuse(unconfuse, 0, rnd(20)+HUH_DURATION);
        player.flags |= IS_HUH;
        msg("the %s's gaze has confused you", monster->get_monster_name());
      }
    }
  }
  //Let greedy ones guard gold
  if (monster->is_flag_set(IS_GREED) && !monster->is_flag_set(IS_RUN))
  {
    monster->flags = monster->flags|IS_RUN;
    if (player.room->goldval) monster->dest = &player.room->gold;
    else monster->dest = &player.pos;
  }
  return monster;
}

//give_pack: Give a pack to a monster if it deserves one
void give_pack(AGENT *monster)
{
  if (rnd(100) < monster->get_monster_carry_prob()) 
    attach_item(&monster->pack, new_item());
}

//pick_mons: Choose a sort of monster for the enemy of a vorpally enchanted weapon
char pick_monster()
{
  char *cp;
  
  do {
    cp = lvl_mons+strlen(lvl_mons);
    while (--cp>=lvl_mons && rnd(10));
    if (cp<lvl_mons) return 'M';
  } while (*cp == ' ');

  return *cp;
}

//moat(x,y): returns pointer to monster at coordinate. if no monster there return NULL
AGENT *monster_at(int y, int x)
{
  AGENT *monster;
  for (monster = mlist; monster!=NULL; monster = next(monster)) 
    if (monster->pos.x == x && monster->pos.y == y) 
      return monster;
  return NULL;
}