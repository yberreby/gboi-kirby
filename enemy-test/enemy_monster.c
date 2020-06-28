#include <gb/gb.h>

#define ENEMY_ATTACK_PROJECTILE 0
#define ENEMY_ATTACK_MELEE 1
#define ENEMY_ATTACK_SELF 2 // used for teting purposes

struct ENEMY {
	UINT8 enemy_sprite_tl; // enemy top-left sprite
	UINT8 enemy_sprite_tr; // enemy top-right sprite
	UINT8 enemy_sprite_bl; // enemy bottom-left sprite
	UINT8 enemy_sprite_br; // enemy bottom-right sprite
	UINT8 attack_type; // enemy attack type, see constants defined above
	UINT8 max_health; // monster max HP
	UINT8 health; // monster current HP
	UINT8 damage; // attack damage - could be manually changed by boosts to enemy shots
	UINT8 frames_between_attacks; // number of frames between attacks (that is how "attack speed" is controlled)
	// Unhandled at this point:
	// - x/y monster velocity, or whatever way to handle monster speed (to be discussed: how to alter these when a collision occurs? I don't think we want monsters going back and forth along the same straight path all game long)
	// - projectile speed (if we decide that it depends on the monster rather than, for example, on how far the player got in the game)
}

// Temporary background tileset for testing purposes
const UINT8 BKG_TILESET[] = {
    // Tile 00: Blank
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // Tile 01: Block
    0xff, 0x01, 0x81, 0x7f, 0xbd, 0x7f, 0xa5, 0x7b,
    0xa5, 0x7b, 0xbd, 0x63, 0x81, 0x7f, 0xff, 0xff,
};

// Temporary sprites tileset for testing purposes
const UINT8 SPRITE_TILESET[] = {
	// Tile 00: will be temporarily used as a placeholder for the projectile
    0x7e, 0x00, 0x81, 0x7f, 0x81, 0x7f, 0x81, 0x7f,
    0x81, 0x7f, 0x81, 0x7f, 0x81, 0x7f, 0x7e, 0x7e,
    // Tile 01: Cherry - temporary placeholder for the projectile
    0x04, 0x04, 0x04, 0x04, 0x0a, 0x0a, 0x12, 0x12,
    0x66, 0x00, 0x99, 0x77, 0x99, 0x77, 0x66, 0x66,
}

// Temporary tilemap for testing purposes
const UINT8 BKG_TILEMAP[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

// Initialize an enemy unit
void init_enemy(ENEMY *unit, UINT8 enemy_sprite_tl, UINT8 attack_type, UINT8 damage, UINT8 hp, UINT8 fba)
{
	unit->enemy_sprite_tl = enemy_sprite_tl;
	unit->enemy_sprite_tr = enemy_sprite_tr;
	unit->enemy_sprite_bl = enemy_sprite_bl;
	unit->enemy_sprite_br = enemy_sprite_br;
	unit->attack_type = attack_type;
	unit->max_health = hp;
	unit->health = hp;
	unit->damage = damage;
	unit->frames_between_attacks = fba;
}

// #TODO: Display enemy unit on screen at specified x and y coordinates
void display_enemy(ENEMY *unit, UINT8 xpos, UINT ypos)
{
	// TODO
}

// #TODO: Play death sequence (blinking, presumably), then make the enemy disappear
void enemy_death(ENEMY *unit)
{
	// Blinking animation: TODO
	// Enemy disappearance: TODO
}

// Enemy unit loses specified amount of HP
void enemy_hp_loss(ENEMY *unit, UINT8 amount)
{
	if (amount < unit->health) // The unit survives, so it just loses the specified amount of HP
	{
		unit->health -= amount;
	}
	else // The unit dies: bring its HP to 0 and then call enemy_death
	{
		unit->health = 0;
		enemy_death(*unit);
	}
}

// Enemy unit gains specified amount of HP
void enemy_hp_regen(ENEMY *unit, UINT8 amount)
{
	if (unit->health + amount <= unit->max_health) // Gain intended amount
	{
		unit->health += amount;
	}
	else
	{
		unit->health = unit->max_health;
	}
}

// Enemy unit launches its attack #TODO: handle MELEE/PROJECTILE
void enemy_attack(ENEMY *unit)
{
	switch (unit->attack_type)
	{
		case ENEMY_ATTACK_SELF:
		enemy_hp_loss(*unit, unit->damage);
		// Optional: might want to load sound!
		break;
		
		// TODO: handle projectile launch and melee attacks. In the meantime, nothing happens.
		case ENEMY_ATTACK_MELEE: // TODO: detect whether the player's in melee range, then attack it
		case ENEMY_ATTACK_PROJECTILE: // can only attack horizontally or vertically
		default:
		break;
	}
}

/* WIP
void main(void)
{
    set_bkg_data(0, 2, BKG_TILESET);
    set_bkg_tiles(0, 0, 20, 18, BKG_TILEMAP);
	ENEMY basic;
	init_enemy(*basic, 1, 1, 1, 1, ENEMY_ATTACK_SELF, 12, 2, 60);
	SHOW_BKG;
	SHOW_SPRITES;
}
*/