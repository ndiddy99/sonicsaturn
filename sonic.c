#include <SEGA_MTH.H>
#include "sprite.h"
#include "scroll.h"
#include "sonic.h"
#include "print.h"
#include "vblank.h"

#define ACCEL (MTH_FIXED(0.046875))
#define DECEL (MTH_FIXED(0.5))
#define TOP_SPEED (MTH_FIXED(6))

#define X_OFFSET (MTH_FIXED(18))
#define Y_OFFSET (MTH_FIXED(20))

#define STANDING_XRADIUS (MTH_FIXED(9))
#define STANDING_YRADIUS (MTH_FIXED(19))

SPRITE_INFO sonic;


void sonic_init() {
    sonic.char_num = 96;
	sonic.x = MTH_FIXED(160);
	sonic.y = MTH_FIXED(100);
    sonic.dx = 0;
    sonic.dy = 0;
	sonic.angle = 0;
	sonic.scale = MTH_FIXED(1);
}

void sonic_move() {
    if (PadData1 & PAD_L) {
        //allow sonic to turn around quickly if he's already moving right
        if (sonic.dx > 0) {
            sonic.dx -= DECEL;
            if (sonic.dx <= 0) {
                sonic.dx = MTH_FIXED(-0.5);
            }
        }
        else if (sonic.dx > -TOP_SPEED) {
            sonic.dx -= ACCEL;
            //only check if we've exceeded the speed cap if we were previously
            //going slower than the speed cap
            if (sonic.dx <= -TOP_SPEED) {
                sonic.dx = -TOP_SPEED;
            }
        }
    }
    if (PadData1 & PAD_R) {
        //allow sonic to turn around quickly if he's already moving left
        if (sonic.dx < 0) {
            sonic.dx += DECEL;
            if (sonic.dx >= 0) {
                sonic.dx = MTH_FIXED(0.5);
            }
        }
        else if (sonic.dx < TOP_SPEED) {
            sonic.dx += ACCEL;
            //only check if we've exceeded the speed cap if we were previously
            //going slower than the speed cap            
            if (sonic.dx >= TOP_SPEED) {
                sonic.dx = TOP_SPEED;
            }
        }
    }
    //if we're not holding either direction, apply friction
    if ((PadData1 & (PAD_L | PAD_R)) == 0) {
        if (sonic.dx < 0) {
            //if we're going left, add speed until it's greater than zero, then
            //set the speed to zero
            sonic.dx += ACCEL;
            if (sonic.dx > 0) {
                sonic.dx = 0;
            }
        }        
        else if (sonic.dx > 0) {
            //if we're going right, subtract speed until it's less than zero, then
            //set the speed to zero
            sonic.dx -= ACCEL;
            if (sonic.dx < 0) {
                sonic.dx = 0;
            }
        }
    }

    if (PadData1 & PAD_U) {
        sonic.dy = MTH_FIXED(-1);
    }
    else if (PadData1 & PAD_D) {
        sonic.dy = MTH_FIXED(1);
    }
    else {
        sonic.dy = 0;
    }
    sonic.x += sonic.dx;
    sonic.y += sonic.dy;
    sonic_collision();
    scroll_set(0, sonic.x - MTH_FIXED(160), sonic.y - MTH_FIXED(100));
    scroll_set(1, scrolls_x[0] >> 1, scrolls_y[0] >> 1);
}

//returns the number of pixels to move up/down at a given location
Sint8 sensor_check(Fixed32 target_x, Fixed32 target_y) {
    Uint8 height = scroll_height(1, target_x, target_y);
    Sint8 weighted_height = height;
    //check above tile
    if (height) {
        Uint8 new_height = scroll_height(1, target_x, target_y - MTH_FIXED(16));
        if (new_height != 0) {
            weighted_height = new_height + 16; //we have to add 16px to the height
        }
    }
    //if tile is empty, get height from below block
    else if (height == 0) {
        height = scroll_height(1, target_x, target_y + MTH_FIXED(16));
        weighted_height = height - 16; //we have to remove 16px from the height
    }
    return weighted_height;
}

void sonic_collision() {
    Fixed32 foot_pos = sonic.y + STANDING_YRADIUS;
    Sint8 height_a = sensor_check(sonic.x - STANDING_XRADIUS, foot_pos);
    Sint8 height_b = sensor_check(sonic.x + STANDING_XRADIUS, foot_pos);
    Sint8 height = height_a > height_b ? height_a : height_b;
    //place sonic's feet at bottom of block
    foot_pos &= 0xfff00000;    
    foot_pos += MTH_FIXED(16);
    //move up by the highest sensor's height
    foot_pos -= (height << 16);
    sonic.y = foot_pos - STANDING_YRADIUS;

    print_num(height_a, 2, 0);
    print_num(height_b, 3, 0);
    print_num(sonic.x >> 16, 4, 0);
    print_num(sonic.y >> 16, 5, 0);
}

void sonic_display() {
    Fixed32 temp_x = sonic.x;
    Fixed32 temp_y = sonic.y;
    sonic.x = MTH_FIXED(160) - X_OFFSET;
    sonic.y = MTH_FIXED(100) - Y_OFFSET;
    sprite_draw(&sonic);
    sonic.x = temp_x;
    sonic.y = temp_y;
}
