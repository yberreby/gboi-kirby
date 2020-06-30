#include <gb/gb.h>

#include "enemy_monster.h"
#include "dummy.sprites.h"

// Initialize an enemy unit
void init_enemy(ENEMY* unit, UINT8 enemy_sprite_l, UINT8 enemy_sprite_r, UINT8 s_id, UINT8 attack_type, UINT8 damage, UINT8 hp, UINT8 frames_between_attacks) {
	unit->enemy_sprite_l = enemy_sprite_l;
	unit->enemy_sprite_r = enemy_sprite_r;
	unit->sprite_id = s_id;
	unit->attack_type = attack_type;
	unit->max_health = hp;
	unit->health = hp; // Enemies always start full HP.
	unit->damage = damage;
	unit->frames_between_attacks = frames_between_attacks;
	unit->frames_until_next_attack = frames_between_attacks; // There is one full cycle before the enemy starts behaving normally. Might want to be able to configure that.
	unit->dying_animation_state = 0; // Start off alive.
}

// Display enemy unit on-screen at specified x and y coordinates
void display_enemy(ENEMY* unit, UINT8 xpos, UINT8 ypos) {	
	// Initialize left sprite
	set_sprite_tile(unit->sprite_id, unit->enemy_sprite_l);
	
	// Initialize right sprite
	set_sprite_tile(unit->sprite_id + 1, unit->enemy_sprite_r);
	
	// Place the sprites
	move_enemy(unit, xpos, ypos);
}

void move_enemy(ENEMY* unit, UINT8 xpos, UINT8 ypos) {
	// Moves enemy unit to (x,y)
	move_sprite(unit->sprite_id, xpos + X_SPRITE_OFFSET, ypos + Y_SPRITE_OFFSET);
	move_sprite(unit->sprite_id + 1, xpos + X_SPRITE_OFFSET + 8, ypos + Y_SPRITE_OFFSET);
	
	// Store current position unless it's out of range
	if (xpos < X_OFFSCREEN && ypos < Y_OFFSCREEN)
	{
		unit->xpos = xpos;
		unit->ypos = ypos;
	}
}

// Handles all steps of the enemy death sequence
void enemy_death(ENEMY* unit) {
	switch(unit->dying_animation_state) {
		case 1: // Make the enemy disappear temporarily
		case 13:
		case 25:
		case 37:
			move_enemy(unit, X_OFFSCREEN, Y_OFFSCREEN);
			break;
		case 7: // Make the enemy reappear temporarily
		case 19:
		case 31:
		case 43:
			move_enemy(unit, unit->xpos, unit->ypos); // This will work because move_enemy does not update (x, y) if it's equal to (200, 200)
			break;
		case 49: // Enemy disappears, for real this time!
			move_enemy(unit, X_OFFSCREEN, Y_OFFSCREEN);
			// #TODO: release the sprite IDs in the sprite ID pool, once said pool is created
			break;
		default:
			break;
	}
	
	unit->dying_animation_state++;
}

// Enemy unit loses specified amount of HP
void enemy_hp_loss(ENEMY* unit, UINT8 amount) {
	if (amount < unit->health) { // The unit survives, so it just loses the specified amount of HP
		unit->health -= amount;
		// Optional: might want a sound here
	} else { // The unit dies: bring its HP to 0 and then call enemy_death
		unit->health = 0;
		// Make the death animation triggerable in the main program
		unit->dying_animation_state = 1;
	}
	
	// Enable palette swap
	set_sprite_prop(unit->sprite_id, get_sprite_prop(unit->sprite_id) | S_PALETTE);
	set_sprite_prop(unit->sprite_id + 1, get_sprite_prop(unit->sprite_id + 1) | S_PALETTE);
	// Wait 1/30th of a second
	wait_vbl_done();
	wait_vbl_done();
	// Swap to default palette
	set_sprite_prop(unit->sprite_id, get_sprite_prop(unit->sprite_id) & ~S_PALETTE);
	set_sprite_prop(unit->sprite_id + 1, get_sprite_prop(unit->sprite_id + 1) & ~S_PALETTE);
}

// Enemy unit gains specified amount of HP. This is capped to max enemy health.
void enemy_hp_regen(ENEMY* unit, UINT8 amount) {
	if (unit->health) { // Do not heal a dead enemy
		if (unit->health + amount <= unit->max_health) { // Gain intended amount
			unit->health += amount;
		} else { // Cap it to max health 
			unit->health = unit->max_health;
		}
	}
}

// Enemy unit launches its attack #TODO: handle MELEE/PROJECTILE
void enemy_attack(ENEMY* unit) {
	if (unit->health) { // Check: an enemy can only attack if it has nonzero health
		switch (unit->attack_type) {
			case ENEMY_ATTACK_SELF:
				enemy_hp_loss(unit, unit->damage);
				// Optional: might want to load sound!
				break;
			
			// Todo: handle projectile launch and melee attacks. In the meantime, nothing happens.
			case ENEMY_ATTACK_MELEE: // #TODO: detect whether the player's in melee range, then attack it
			case ENEMY_ATTACK_PROJECTILE: // #TODO: can only attack horizontally or vertically
			case ENEMY_ATTACK_INNOCENT:
			default:
				break;
		}
		
		// Reset frame counter until next attack
		unit->frames_until_next_attack = unit->frames_between_attacks;
	}
}