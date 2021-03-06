#include <gb/gb.h>
#include <string.h>
#include <rand.h>
#include "tileset.h"
#include "global.h"
#include "enemy_spawner.h"

#include "chunks0.h"
#include "chunks1.h"
#include "chunks2.h"

typedef UINT8 TILE;
typedef TILE CHUNK[64];

typedef struct {
    CHUNK chunk;
    BOOLEAN top_door_flag;
    BOOLEAN left_door_flag;
    BOOLEAN corner;
    UBYTE clutter;
} PREFAB;


UBYTE urand() {
    return (UBYTE) rand();
}

#define NB_TWO_DOORS_PREFABS CHUNK_COUNT2
#define NB_ONE_DOOR_PREFABS CHUNK_COUNT1
#define NB_NO_DOOR_PREFAB CHUNK_COUNT0


static PREFAB buffer;

static UINT8 clutter_budget = 0;
#define MAX_CLUTTER_BUDGET 20

static UINT8 current_room_ptr;

/**
 * @brief Used to stock all the spawners from one chunk
 * 
 */
static ENEMY_SPAWNER spawner_buffer[MAX_ENEMY_NB];
static UINT8 spawner_buffer_ptr = 0;

/**
 * @brief Decompress chunklist[index]
 * 
 * @param chunk_list 
 * @param index 
 * @param output 
 */
static void get_chunk(const UINT8 chunk_list[][33], UBYTE index, CHUNK output) {
    for (UINT8 i = 0; i < 32; i++) {
        UINT8 tile1 = (chunk_list[index][i] >> 4);
        UINT8 tile2 = chunk_list[index][i] & 0x0F;
        if (tile1 < 10) {
            buffer.chunk[i << 1] = tile1;
        } else {
            buffer.chunk[i << 1] = 0;
            spawner_buffer[spawner_buffer_ptr].pos = (i << 1);
            spawner_buffer[spawner_buffer_ptr].id = tile1;
            spawner_buffer_ptr++;
        }
        if (tile2 < 10) {
            buffer.chunk[(i << 1) + 1] = tile2;
        } else {
            buffer.chunk[(i << 1) + 1] = 0;
            spawner_buffer[spawner_buffer_ptr].pos = (i << 1) + 1;
            spawner_buffer[spawner_buffer_ptr].id = tile2;
            spawner_buffer_ptr++;
        }
    }


    UINT8 last = chunk_list[index][32];
    buffer.top_door_flag = (last & 0x80) != 0;
    buffer.left_door_flag = (last & 0x40) != 0;
    buffer.corner = (last & 0x20) != 0;
    buffer.clutter = last & 0x1F;

    clutter_budget -= buffer.clutter;

    memcpy(output, buffer.chunk, sizeof(CHUNK));
}

/**
 * @brief Find the most expensive chunk and return its index + 1
 * 
 * @param chunk_list 
 * @param len 
 * @return UINT8 
 */
static UINT8 find_affordable(const UINT8 chunk_list[][33], UINT8 len) {
    for (UINT8 i = len - 1; i > 0; i--) {
        if ((chunk_list[i][32] & 0x1F) <= clutter_budget)
            return i + 1;
    }
    return 1;
}

static void get_two_doors_chunk(CHUNK output) {
    UINT8 price = find_affordable(CHUNKS2, NB_TWO_DOORS_PREFABS);
    UBYTE index = urand() % price;
    get_chunk(CHUNKS2, index, output);
}

static void get_one_door_chunk(CHUNK output) {
    BOOLEAN two_doors = urand() & 1U;
    if (two_doors) {
        get_two_doors_chunk(output);
    } else {
        UBYTE index = urand() % find_affordable(CHUNKS1, NB_ONE_DOOR_PREFABS);
        get_chunk(CHUNKS1, index, output);
    }
}

static void get_no_door_chunk(CHUNK output) {
    UBYTE nb_doors = (urand() % 3U);
    UBYTE index;
    switch (nb_doors) {
        case 2:
            get_two_doors_chunk(output);
            break;
        case 1:
            get_one_door_chunk(output);
            break;
        default:
            index = urand() % find_affordable(CHUNKS0, NB_NO_DOOR_PREFAB);
            get_chunk(CHUNKS0, index, output);
            break;
    }
}


///// DIRTY TRANSFORM MATRICES

const UBYTE FLIP_X_MATRIX[64] = {
     7, 6, 5, 4, 3, 2, 1, 0,
    15,14,13,12,11,10, 9, 8,
    23,22,21,20,19,18,17,16,
    31,30,29,28,27,26,25,24,
    39,38,37,36,35,34,33,32,
    47,46,45,44,43,42,41,40,
    55,54,53,52,51,50,49,48,
    63,62,61,60,59,58,57,56
};

const UBYTE FLIP_Y_MATRIX[64] = {
    56,57,58,59,60,61,62,63,
    48,49,50,51,52,53,54,55,
    40,41,42,43,44,45,46,47,
    32,33,34,35,36,37,38,39,
    24,25,26,27,28,29,30,31,
    16,17,18,19,20,21,22,23,
     8, 9,10,11,12,13,14,15,
     0, 1, 2, 3, 4, 5, 6, 7,
};

const UBYTE ROT_LEFT_MATRIX[64] = {
    56,48,40,32,24,16, 8, 0,
    57,49,41,33,25,17, 9, 1,
    58,50,42,34,26,18,10, 2,
    59,51,43,35,27,19,11, 3,
    60,52,44,36,28,20,12, 4,
    61,53,45,37,29,21,13, 5,
    62,54,46,38,30,22,14, 6,
    63,55,47,39,31,23,15, 7
};

const UBYTE ROT_RIGHT_MATRIX[64] = {
     7,15,23,31,39,47,55,63,
     6,14,22,30,38,46,54,62,
     5,13,21,29,37,45,53,61,
     4,12,20,28,36,44,52,60,
     3,11,19,27,35,43,51,59,
     2,10,18,26,34,42,50,58,
     1, 9,17,25,33,41,49,57,
     0, 8,16,24,32,40,48,56,
};

const UBYTE ROT_FULL_MATRIX[64] = {
    63,62,61,60,59,58,57,56,
    55,54,53,52,51,50,49,48,
    47,46,45,44,43,42,41,40,
    39,38,37,36,35,34,33,32,
    31,30,29,28,27,26,25,24,
    23,22,21,20,19,18,17,16,
    15,14,13,12,11,10, 9, 8,
     7, 6, 5, 4, 3, 2, 1, 0
};

const UBYTE TRANSPOSE_MATRIX[64] = {
     0, 8,16,24,32,40,48,56,
     1, 9,17,25,33,41,49,57,
     2,10,18,26,34,42,50,58,
     3,11,19,27,35,43,51,59,
     4,12,20,28,36,44,52,60,
     5,13,21,29,37,45,53,61,
     6,14,22,30,38,46,54,62,
     7,15,23,31,39,47,55,63
};

const UBYTE IDENTITY_MATRIX[64] = {
     0, 1, 2, 3, 4, 5, 6, 7,
     8, 9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,
    24,25,26,27,28,29,30,31,
    32,33,34,35,36,37,38,39,
    40,41,42,43,44,45,46,47,
    48,49,50,51,52,53,54,55,
    56,57,58,59,60,61,62,63
};

static void apply_transformation(const UBYTE matrix[], CHUNK input, CHUNK output) {
    for (UBYTE i = 0; i < 64; i++) {
        output[matrix[i]] = input[i];
    }
    for (UBYTE i = 0; i < spawner_buffer_ptr; i++) {
        spawner_buffer[i].pos = matrix[spawner_buffer[i].pos];
    }
}


// used to build the top left chunk. DO NOT REMOVE
static CHUNK chunk = {0};

// used to build the other chunks. DO NOT REMOVE
static CHUNK tempo = {0};

static void get_top_left_chunk(BOOLEAN left_door, BOOLEAN top_door, CHUNK output) {
    UBYTE nb_doors = left_door + top_door;
    if (nb_doors == 2) {
        get_two_doors_chunk(chunk);
        if (urand() % 2)
            memcpy(output, chunk, sizeof(CHUNK));
        else
            apply_transformation(TRANSPOSE_MATRIX, chunk, output);
    } else if (nb_doors == 1) {
        get_one_door_chunk(chunk);
        if (top_door)
            memcpy(output, chunk, sizeof(CHUNK));
        else   // left_door == true
            apply_transformation(TRANSPOSE_MATRIX, chunk, output);
    } else {
        get_no_door_chunk(chunk);
        if (urand() % 2)
            memcpy(output, chunk, sizeof(CHUNK));
        else
            apply_transformation(TRANSPOSE_MATRIX, chunk, output);
    }
}


/**
 * @brief Build a top right chunk based on a top left chunk
 * 
 * @param top_door 
 * @param right_door 
 * @param output 
 */
static void get_top_right_chunk(BOOLEAN top_door, BOOLEAN right_door, CHUNK output) {
    get_top_left_chunk(top_door, right_door, tempo);
    apply_transformation(ROT_RIGHT_MATRIX, tempo, output);
}

/**
 * @brief Build a bottom right chunk based on a top left chunk
 * 
 * @param right_door 
 * @param bottom_door 
 * @param output 
 */
static void get_bottom_right_chunk(BOOLEAN right_door, BOOLEAN bottom_door, CHUNK output) {
    get_top_left_chunk(right_door, bottom_door, tempo);
    apply_transformation(ROT_FULL_MATRIX, tempo, output);
}

/**
 * @brief Build a bottom left chunk based on a top left chunk
 * 
 * @param bottom_door 
 * @param left_door 
 * @param output 
 */
static void get_bottom_left_chunk(BOOLEAN bottom_door, BOOLEAN left_door, CHUNK output) {
    get_top_left_chunk(bottom_door, left_door, tempo);
    apply_transformation(ROT_LEFT_MATRIX, tempo, output);
}

/**
 * @brief Save the currently processed chunk in the room
 * 
 * @param chunk 
 * @param room 
 * @param offset 
 */
static void write_chunk_to_small_room(CHUNK chunk, ROOM* room, UBYTE offset) {
    TILE* writer = room->small_tiles + offset;
    TILE* reader = chunk;
    UBYTE i;
    for (i = 0; i < 8; i++) {
        memcpy(writer, reader, 8);
        reader += 8;
        writer += 16;
    }
}

/**
 * @brief Save all the spawner found in the currently processed chunk
 * 
 * @param x_offset final x offset of the chunk
 * @param y_offset final y pffset of the chunk
 */
static void flush_spawn_buffer(UINT8 x_offset, UINT8 y_offset) {
    UINT8 x;
    UINT8 y;
    for (UBYTE i = 0; i < spawner_buffer_ptr; i++) {
        x = spawner_buffer[i].pos & 7;
        y = spawner_buffer[i].pos >> 3;
        push_enemy_spawner(current_room_ptr, (x + x_offset) + ((y + y_offset) << 4), spawner_buffer[i].id);
    }
    spawner_buffer_ptr = 0;
}

static CHUNK chunk_buffer = {0};


#define HAS_DOOR(r,d) ((r)->doors[(d)].keys[0] + (r)->doors[(d)].keys[1] + (r)->doors[(d)].keys[2]) < 3


void gen_room(ROOM* room, UINT8 room_ptr) {
    clutter_budget = MAX_CLUTTER_BUDGET;

    current_room_ptr = room_ptr;

    // top-left, top-right, bottom-left, bottom-right
    BOOLEAN left_door = HAS_DOOR(room, LEFT);
    BOOLEAN top_door = HAS_DOOR(room, UP);
    BOOLEAN right_door = HAS_DOOR(room, RIGHT);
    BOOLEAN bottom_door = HAS_DOOR(room, DOWN);

    get_top_left_chunk(left_door, top_door, chunk_buffer);
    write_chunk_to_small_room(chunk_buffer, room, 0);
    flush_spawn_buffer(0, 0);
    get_top_right_chunk(top_door, right_door, chunk_buffer);
    write_chunk_to_small_room(chunk_buffer, room, 8);
    flush_spawn_buffer(8, 0);
    get_bottom_right_chunk(right_door, bottom_door, chunk_buffer);
    write_chunk_to_small_room(chunk_buffer, room, 136);
    flush_spawn_buffer(8, 8);
    get_bottom_left_chunk(bottom_door, left_door, chunk_buffer);
    write_chunk_to_small_room(chunk_buffer, room, 128);
    flush_spawn_buffer(0, 8);
}