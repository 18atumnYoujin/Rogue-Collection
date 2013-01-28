//new_level: Dig and draw a new level
//new_level.c 1.4 (A.I. Design) 12/13/84

#include "rogue.h"
#include "new_leve.h"
#include "monsters.h"
#include "potions.h"
#include "main.h"
#include "list.h"
#include "curses.h"
#include "mach_dep.h"
#include "rooms.h"
#include "things.h"
#include "io.h"
#include "passages.h"
#include "misc.h"
#include "level.h"
#include "thing.h"
#include "pack.h"

#define TREAS_ROOM  20 //one chance in TREAS_ROOM for a treasure room
#define MAXTREAS  10 //maximum number of treasures in a treasure room
#define MINTREAS  2 //minimum number of treasures in a treasure room
#define MAXTRIES  10 //max number of tries to put down a monster

void new_level(int do_implode)
{
  int room, i, ntraps;
  AGENT *monster;
  Coord stairs;

  player.flags &= ~ISHELD; //unhold when you go down just in case
  //Monsters only get displayed when you move so start a level by having the poor guy rest. God forbid he lands next to a monster!
  if (level>max_level) max_level = level;

  //Clean things off from last level
  clear_level();
  //Free up the monsters on the last level
  for (monster = mlist; monster!=NULL; monster = next(monster)) 
    free_item_list(&monster->pack);
  free_agent_list(&mlist);
  //just in case we left some flytraps behind
  f_restor();
  //Throw away stuff left on the previous level (if anything)
  free_item_list(&lvl_obj);
  do_rooms(); //Draw rooms
  if (max_level==1)
  {
    clear();
  }
  if(do_implode) implode();
  status();
  do_passages(); //Draw passages
  no_food++;
  put_things(); //Place objects (if any)
  //Place the staircase down.
  i = 0;
  do
  {
    room = rnd_room();
    rnd_pos(&rooms[room], &stairs);
    if (i++>100) {i = 0; seed = srand2();}
  } while (!isfloor(get_tile(stairs.y, stairs.x)));
  set_tile(stairs.y, stairs.x, STAIRS);
  //Place the traps
  if (rnd(10)<level)
  {
    ntraps = rnd(level/4)+1;
    if (ntraps>MAXTRAPS) ntraps = MAXTRAPS;
    i = ntraps;
    while (i--)
    {
      do
      {
        room = rnd_room();
        rnd_pos(&rooms[room], &stairs);
      } while (!isfloor(get_tile(stairs.y, stairs.x)));
      unset_flag(stairs.y, stairs.x, F_REAL);
      set_flag(stairs.y, stairs.x, rnd(NTRAPS));
    }
  }
  do
  {
    room = rnd_room();
    rnd_pos(&rooms[room], &player.pos);
  } while (!(isfloor(get_tile(player.pos.y, player.pos.x)) && (get_flags(player.pos.y, player.pos.x)&F_REAL) && monster_at(player.pos.y, player.pos.x)==NULL));
  mpos = 0;
  enter_room(&player.pos);
  mvaddch(player.pos.y, player.pos.x, PLAYER);
  oldpos = player.pos;
  oldrp = player.room;
  if (on(player, SEEMONST)) turn_see(FALSE);
}

//rnd_room: Pick a room that is really there
int rnd_room()
{
  int rm;

  do rm = rnd(MAXROOMS); while (!((rooms[rm].flags&ISGONE)==0 || (rooms[rm].flags&ISMAZE)));
  return rm;
}

//put_things: Put potions and scrolls on this level
void put_things()
{
  int i = 0;
  ITEM *cur;
  int rm;
  Coord tp;

  //Once you have found the amulet, the only way to get new stuff is to go down into the dungeon.
  //This is real unfair - I'm going to allow one thing, that way the poor guy will get some food.
  if (had_amulet() && level<max_level) i = MAXOBJ-1;
  else
  {
    //If he is really deep in the dungeon and he hasn't found the amulet yet, put it somewhere on the ground
    //Check this first so if we are out of memory the guy has a hope of getting the amulet
    if (level>=AMULETLEVEL && !had_amulet())
    {
      if ((cur = create_item(AMULET, 0))!=NULL)
      {
        attach_item(&lvl_obj, cur);
        cur->hit_plus = cur->damage_plus = 0;
        cur->damage = cur->throw_damage = "0d0";
        cur->armor_class = 11;
        //Put it somewhere
        do {rm = rnd_room(); rnd_pos(&rooms[rm], &tp);} while (!isfloor(display_character(tp.y, tp.x)));
        set_tile(tp.y, tp.x, AMULET);
        cur->pos = tp;
      }
    }
    //check for treasure rooms, and if so, put it in.
    if (rnd(TREAS_ROOM)==0) treas_room();
  }
  //Do MAXOBJ attempts to put things on a level
  for (; i<MAXOBJ; i++) {
    if (total_items<MAXITEMS && rnd(100)<35)
    {
      //Pick a new object and link it in the list
      cur = new_item();
      attach_item(&lvl_obj, cur);
      //Put it somewhere
      do {rm = rnd_room(); rnd_pos(&rooms[rm], &tp);} while (!isfloor(get_tile(tp.y, tp.x)));
      set_tile(tp.y, tp.x, cur->type);
      cur->pos = tp;
    }
  }
}

//treas_room: Add a treasure room
void treas_room()
{
  int nm;
  ITEM *item;
  AGENT *monster;
  struct Room *room;
  int spots, num_monst;
  Coord monster_pos;

  room = &rooms[rnd_room()];
  spots = (room->size.y-2)*(room->size.x-2)-MINTREAS;
  if (spots>(MAXTREAS-MINTREAS)) spots = (MAXTREAS-MINTREAS);
  num_monst = nm = rnd(spots)+MINTREAS;
  while (nm-- && total_items<MAXITEMS)
  {
    do {
      rnd_pos(room, &monster_pos);
    } while (!isfloor(get_tile(monster_pos.y, monster_pos.x)));
    item = new_item();
    item->pos = monster_pos;
    attach_item(&lvl_obj, item);
    set_tile(monster_pos.y, monster_pos.x, item->type);
  }
  //fill up room with monsters from the next level down
  if ((nm = rnd(spots)+MINTREAS)<num_monst+2) nm = num_monst+2;
  spots = (room->size.y-2)*(room->size.x-2);
  if (nm>spots) nm = spots;
  level++;
  while (nm--)
  {
    for (spots = 0; spots<MAXTRIES; spots++)
    {
      rnd_pos(room, &monster_pos);
      if (isfloor(get_tile(monster_pos.y, monster_pos.x)) && monster_at(monster_pos.y, monster_pos.x)==NULL) break;
    }
    if (spots!=MAXTRIES)
    {
      if ((monster = create_agent())!=NULL)
      {
        new_monster(monster, randmonster(FALSE), &monster_pos);
        if (bailout) debug("treasure rm bailout");
        monster->flags |= ISMEAN; //no sloughers in THIS room
        give_pack(monster);
      }
    }
  }
  level--;
}
