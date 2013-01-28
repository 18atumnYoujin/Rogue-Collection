//Contains functions for dealing with things like potions, scrolls, and other items.
//things.c     1.4 (AI Design) 12/14/84

#include <stdio.h>
#include <ctype.h>

#include "rogue.h"
#include "things.h"
#include "pack.h"
#include "list.h"
#include "sticks.h"
#include "io.h"
#include "misc.h"
#include "daemons.h"
#include "rings.h"
#include "scrolls.h"
#include "potions.h"
#include "weapons.h"
#include "curses.h"
#include "main.h"
#include "armor.h"
#include "daemon.h"
#include "mach_dep.h"
#include "level.h"
#include "thing.h"
#include "monsters.h"
#include "food.h"

struct MagicItem things[NUMTHINGS] =
{
  {0, 27 }, //potion
  {0, 30 }, //scroll
  {0, 17 }, //food
  {0,  8 }, //weapon
  {0,  8 }, //armor
  {0,  5 }, //ring
  {0,  5 }  //stick
};

#define MAX(a,b,c,d) (a>b?(a>c?(a>d?a:d):(c>d?c:d)):(b>c?(b>d?b:d):(c>d?c:d)))

static int line_cnt = 0;

//init_things: Initialize the probabilities for types of things
void init_things()
{
  struct MagicItem *mp;
  for (mp = &things[1]; mp<=&things[NUMTHINGS-1]; mp++) 
    mp->prob += (mp-1)->prob;
}

//inv_name: Return the name of something as it would appear in an inventory.
char *inv_name(ITEM *obj, bool drop)
{
  int which = obj->which;
  char *pb;

  pb = prbuf;
  switch (obj->type)
  {
  case SCROLL:
    if (obj->count==1) {
      strcpy(pb, "A scroll ");
      pb = &prbuf[9];
    }
    else {
      sprintf(pb, "%d scrolls ", obj->count); 
      pb = &prbuf[strlen(prbuf)];
    }
    if (does_know_scroll(which) || wizard) 
      sprintf(pb, "of %s", get_scroll_name(which));
    else if (*get_scroll_guess(which)) 
      sprintf(pb, "called %s", get_scroll_guess(which));
    else
      chopmsg(pb, "titled '%.17s'", "titled '%s'", get_title(which));
    break;

  case POTION:
    if (obj->count==1) {
      strcpy(pb, "A potion ");
      pb = &prbuf[9];
    }
    else {
      sprintf(pb, "%d potions ", obj->count); 
      pb = &pb[strlen(prbuf)];
    }
    if (does_know_potion(which) || wizard) {
      chopmsg(pb, "of %s", "of %s(%s)", get_potion_name(which), get_color(which));
    }
    else if (*get_potion_guess(which)) {
      chopmsg(pb, "called %s", "called %s(%s)", get_potion_guess(which), get_color(which));
    }
    else if (obj->count==1) 
      sprintf(prbuf, "A%s %s potion", vowelstr(get_color(which)), get_color(which));
    else sprintf(prbuf, "%d %s potions", obj->count, get_color(which));
    break;

  case FOOD:
    if (which==1) 
      if (obj->count==1)
        sprintf(pb, "A%s %s", vowelstr(fruit), fruit); 
      else sprintf(pb, "%d %ss", obj->count, fruit);
    else if (obj->count==1) 
      strcpy(pb, "Some food");
    else sprintf(pb, "%d rations of food", obj->count);
    break;

  case WEAPON:
    if (obj->count>1) 
      sprintf(pb, "%d ", obj->count);
    else
      sprintf(pb, "A%s ", vowelstr(get_weapon_name(which)));
    pb = &prbuf[strlen(prbuf)];
    if (obj->flags&ISKNOW || wizard) 
      sprintf(pb, "%s %s", num(obj->hit_plus, obj->damage_plus, WEAPON), get_weapon_name(which));
    else
      sprintf(pb, "%s", get_weapon_name(which));
    if (obj->count>1) strcat(pb, "s");
    if (obj->enemy && (obj->flags&ISREVEAL || wizard))
    {
      strcat(pb, " of ");
      strcat(pb, get_monster_name(obj->enemy));
      strcat(pb, " slaying");
    }
    break;

  case ARMOR:
    if (obj->flags&ISKNOW || wizard)
      chopmsg(pb, "%s %s", "%s %s [armor class %d]", num(get_default_class(which)-obj->armor_class, 0, ARMOR), get_armor_name(which), -(obj->armor_class-11));
    else
      sprintf(pb, "%s", get_armor_name(which));
    break;

  case AMULET:
    strcpy(pb, "The Amulet of Yendor");
    break;

  case STICK:
    sprintf(pb, "A%s %s ", vowelstr(get_stick_type(which)), get_stick_type(which));
    pb = &prbuf[strlen(prbuf)];
    if (does_know_stick(which) || wizard)
      chopmsg(pb, "of %s%s", "of %s%s(%s)", get_stick_name(which), get_charge_string(obj), get_material(which));
    else if (*get_stick_guess(which))
      chopmsg(pb, "called %s", "called %s(%s)", get_stick_guess(which), get_material(which));
    else
      sprintf(pb = &prbuf[2], "%s %s", get_material(which), get_stick_type(which));
    break;

  case RING:
    if (does_know_ring(which) || wizard)
      chopmsg(pb, "A%s ring of %s", "A%s ring of %s(%s)", ring_num(obj), get_ring_name(which), get_stone(which));
    else if (*get_ring_guess(which)) 
      chopmsg(pb, "A ring called %s", "A ring called %s(%s)", get_ring_guess(which), get_stone(which));
    else 
      sprintf(pb, "A%s %s ring", vowelstr(get_stone(which)), get_stone(which));
    break;

#ifdef DEBUG

  case GOLD:
    sprintf(pb, "Gold at %d,%d", obj->pos.y, obj->pos.x);
    break;

  default:
    debug("Picked up someting bizarre %s", unctrl(obj->type));
    sprintf(pb, "Something bizarre %c(%d)", obj->type, obj->type);
    break;

#endif
  }
  if (obj==cur_armor) 
    strcat(pb, " (being worn)");
  if (obj==cur_weapon) 
    strcat(pb, " (weapon in hand)");
  if (obj==cur_ring[LEFT]) 
    strcat(pb, " (on left hand)");
  else if (obj==cur_ring[RIGHT])
    strcat(pb, " (on right hand)");
  if (drop && isupper(prbuf[0])) 
    prbuf[0] = tolower(prbuf[0]);
  else if (!drop && islower(*prbuf)) 
    *prbuf = toupper(*prbuf);
  return prbuf;
}

void chopmsg(char *s, char *shmsg, char *lnmsg, int arg1, int arg2, int arg3)
{
  sprintf(s, lnmsg, arg1, arg2, arg3);
  if (terse || expert) sprintf(s, shmsg, arg1, arg2, arg3);
}

//drop: Put something down
void drop()
{
  byte ch;
  ITEM *nobj, *op;

  ch = get_tile(player.pos.y, player.pos.x);
  if (ch!=FLOOR && ch!=PASSAGE) {msg("there is something there already"); return;}
  if ((op = get_item("drop", 0))==NULL) return;
  if (!can_drop(op)) return;
  //Take it out of the pack
  if (op->count>=2 && op->type!=WEAPON)
  {
    if ((nobj = create_item(0,0))==NULL) {msg("%sit appears to be stuck in your pack!", noterse("can't drop it, ")); return;}
    op->count--;
    bcopy(*nobj, *op);
    nobj->count = 1;
    op = nobj;
  }
  else detach_item(&player.pack, op);
  //Link it into the level object list
  attach_item(&lvl_obj, op);
  set_tile(player.pos.y, player.pos.x, op->type);
  bcopy(op->pos, player.pos);
  msg("dropped %s", inv_name(op, TRUE));
}

//can_drop: Do special checks for dropping or unweilding|unwearing|unringing
int can_drop(ITEM *op)
{
  if (op==NULL) return TRUE;
  if (op!=cur_armor && op!=cur_weapon && op!=cur_ring[LEFT] && op!=cur_ring[RIGHT]) return TRUE;
  if (op->flags&ISCURSED) {msg("you can't.  It appears to be cursed"); return FALSE;}
  if (op==cur_weapon) cur_weapon = NULL;
  else if (op==cur_armor) {waste_time(); cur_armor = NULL;}
  else
  {
    int hand;

    if (op!=cur_ring[hand = LEFT]) if (op!=cur_ring[hand = RIGHT])
    {
      debug("Candrop called with funny thing");
      return TRUE;
    }
    cur_ring[hand] = NULL;
    switch (op->which)
    {
    case R_ADDSTR: chg_str(-op->ring_level); break;
    case R_SEEINVIS: unsee(); extinguish(unsee); break;
    }
  }
  return TRUE;
}

//new_thing: Return a new thing
ITEM *new_item()
{
  ITEM *item = create_item(0, 0);
  if (!item)
    return NULL; 

  //Decide what kind of object it will be. If we haven't had food for a while, let it be food.
  switch (no_food > 3 ? 2 : pick_one(things, NUMTHINGS))
  {
  case 0:
    init_new_potion(item);
    break;

  case 1:
    init_new_scroll(item);
    break;

  case 2:
    init_new_food(item);
    break;

  case 3:
    init_new_weapon(item);
    break;

  case 4:
    init_new_armor(item);
    break;

  case 5:
    init_new_ring(item);
    break;

  case 6:
    init_new_stick(item);
    break;

  default:
    debug("Picked a bad kind of object");
    wait_for(' ');
    break;
  }
  return item;
}

//pick_one: Pick an item out of a list of nitems possible magic items
int pick_one(struct MagicItem *magic, int nitems)
{
  struct MagicItem *end;
  int i;
  struct MagicItem *start;

  start = magic;
  for (end = &magic[nitems], i = rnd(100); magic<end; magic++) if (i<magic->prob) break;
  if (magic==end)
  {

    if (wizard)
    {
      msg("bad pick_one: %d from %d items", i, nitems);
      for (magic = start; magic<end; magic++) msg("%s: %d%%", magic->name, magic->prob);
    }

    magic = start;
  }
  return magic-start;
}

//discovered: list what the player has discovered in this game of a certain type
void discovered()
{
  print_disc(POTION);
  add_line("", " ", 0);
  print_disc(SCROLL);
  add_line("", " ", 0);
  print_disc(RING);
  add_line("", " ", 0);
  print_disc(STICK);
  end_line("");
}

//print_disc: Print what we've discovered of type 'type'
void print_disc(byte type)
{
  int (*know)(int);
  const char* (*guess)(int);
  int i, maxnum, num_found;
  static ITEM obj;
  static short order[MAX(MAXSCROLLS, MAXPOTIONS, MAXRINGS, MAXSTICKS)];

  switch (type)
  {
  case SCROLL:
    maxnum = MAXSCROLLS; 
    know = does_know_scroll;
    guess = get_scroll_guess;
    break;
  case POTION:
    maxnum = MAXPOTIONS;
    know = does_know_potion;
    guess = get_potion_guess; 
    break;
  case RING:
    maxnum = MAXRINGS;
    know = does_know_ring;
    guess = get_ring_guess;
    break;
  case STICK:
    maxnum = MAXSTICKS;
    know = does_know_stick;
    guess = get_stick_guess;
    break;
  }
  set_order(order, maxnum);
  obj.count = 1;
  obj.flags = 0;
  num_found = 0;
  for (i = 0; i<maxnum; i++) if (know(order[i]) || *guess(order[i]))
  {
    obj.type = type;
    obj.which = order[i];
    add_line("", "%s", inv_name(&obj, FALSE));
    num_found++;
  }
  if (num_found==0) add_line("", nothing(type), 0);
}

//set_order: Set up order for list
void set_order(short *order, int numthings)
{
  int i, r, t;

  for (i = 0; i<numthings; i++) order[i] = i;
  for (i = numthings; i>0; i--)
  {
    r = rnd(i);
    t = order[i-1];
    order[i-1] = order[r];
    order[r] = t;
  }
}

//add_line: Add a line to the list of discoveries
int add_line(char *use, char *fmt, char *arg)
{
  int x, y;
  int retchar = ' ';

  if (line_cnt==0) {wdump(0); clear();}
  if (line_cnt>=LINES-1 || fmt==NULL)
  {
    move(LINES-1, 0);
    if (*use) printw("-Select item to %s. Esc to cancel-", use);
    else addstr("-Press space to continue-");
    do retchar = readchar(); while (retchar!=ESCAPE && retchar!=' ' && (!islower(retchar)));
    clear();
    line_cnt = 0;
  }
  if (fmt!=NULL && !(line_cnt==0 && *fmt=='\0'))
  {
    move(line_cnt, 0);
    printw(fmt, arg);
    getrc(&x, &y);
    //if the line wrapped but nothing was printed on this line you might as well use it for the next item
    if (y!=0) line_cnt = x+1;
  }
  return (retchar);
}

//end_line: End the list of lines
int end_line(char *use)
{
  int retchar;

  retchar = add_line(use, 0, 0);
  wrestor(0);
  line_cnt = 0;
  return (retchar);
}

//nothing: Set up prbuf so that message for "nothing found" is there
char *nothing(byte type)
{
  char *sp, *tystr;

  sprintf(prbuf, "Haven't discovered anything");
  if (terse) sprintf(prbuf, "Nothing");
  sp = &prbuf[strlen(prbuf)];
  switch (type)
  {
  case POTION: tystr = "potion"; break;
  case SCROLL: tystr = "scroll"; break;
  case RING: tystr = "ring"; break;
  case STICK: tystr = "stick"; break;
  }
  sprintf(sp, " about any %ss", tystr);
  return prbuf;
}
