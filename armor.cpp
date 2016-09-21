//This file contains misc functions for dealing with armor
//@(#)armor.c 1.2 (AI Design) 2/12/84

#include <stdio.h>

#include "rogue.h"
#include "game_state.h"
#include "armor.h"
#include "io.h"
#include "pack.h"
#include "things.h"
#include "daemon.h"
#include "weapons.h"
#include "main.h"
#include "hero.h"

//Names of armor types
const char *a_names[MAXARMORS] =
{
  "leather armor",
  "ring mail",
  "studded leather armor",
  "scale mail",
  "chain mail",
  "splint mail",
  "banded mail",
  "plate mail"
};

//Chance for each armor type
int a_chances[MAXARMORS] =
{
  20,
  35,
  50,
  63,
  75,
  85,
  95,
  100
};

//Armor class for each armor type
int a_class[MAXARMORS] =
{
  8,
  7,
  7,
  6,
  5,
  4,
  4,
  3
};

int get_default_class(int type)
{
  return a_class[type];
}

const char* get_armor_name(int type)
{
  return a_names[type];
}

Item* create_armor()
{
  int j, k;

  for (j = 0, k = rnd(100); j < MAXARMORS; j++){ 
      if (k < a_chances[j]) 
          break; 
  }
  if (j==MAXARMORS) {
      debug("Picked a bad armor %d", k); 
      j = 0;
  }
  int which = j;
  return new Armor(which);
}

//wear: The player wants to wear something, so let him/her put it on.
void wear()
{
  Item *obj;
  char *sp;

  if (get_current_armor()!=NULL)
  {
    msg("you are already wearing some%s.", noterse(".  You'll have to take it off first"));
    counts_as_turn = false;
    return;
  }
  if ((obj = get_item("wear", ARMOR))==NULL) return;
  if (obj->type!=ARMOR) {
      msg("you can't wear that"); 
      return;
  }
  waste_time();
  obj->set_known() ;
  sp = inv_name(obj, true);
  set_current_armor(obj);
  msg("you are now wearing %s", sp);
}

//take_off: Get the armor off of the player's back
void take_off()
{
  Item *obj;

  if ((obj = get_current_armor())==NULL)
  {
    counts_as_turn = false;
    msg("you aren't wearing any armor");
    return;
  }
  if (!can_drop(get_current_armor())) return;
  set_current_armor(NULL);
  msg("you used to be wearing %c) %s", pack_char(obj), inv_name(obj, true));
}

//waste_time: Do nothing but let other things happen
void waste_time()
{
  do_daemons();
  do_fuses();
}

const char* get_inv_name_armor(Item* obj)
{
  char *pb = prbuf;
  int which = obj->which;

  if (obj->is_known() || game->hero().is_wizard())
    chopmsg(pb, "%s %s", "%s %s [armor class %d]", num(get_default_class(which)-obj->get_armor_class(), 0, (char)ARMOR), get_armor_name(which), -(obj->get_armor_class()-11));
  else
    sprintf(pb, "%s", get_armor_name(which));

  return prbuf;
}

Armor::Armor(int which) : 
    Item(ARMOR, which)
{
    armor_class = get_default_class(which);

    int k;
    if ((k = rnd(100))<20) {
        set_cursed();
        armor_class += rnd(3) + 1;
    }
    else if (k<28)
        armor_class -= rnd(3) + 1;
}

Armor::Armor(int which, int ac_mod) :
    Item(ARMOR, which)
{
    if (ac_mod > 0)
        set_cursed();
    armor_class = get_default_class(which) + ac_mod;
}

Item * Armor::Clone() const
{
    return new Armor(*this);
}
