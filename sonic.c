#include <SEGA_MTH.H>
#include "sprite.h"
#include "scroll.h"
#include "vblank.h"

#define ACCEL (MTH_FIXED(0.046875))
#define DECEL (MTH_FIXED(0.5))
#define TOP_SPEED (MTH_FIXED(6))

SPRITE_INFO sonic;


void sonic_init() {
    sonic.char_num = 96;
	sonic.x = MTH_FIXED(100);
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
        sonic.dy = MTH_FIXED(-4);
    }
    else if (PadData1 & PAD_D) {
        sonic.dy = MTH_FIXED(4);
    }
    else {
        sonic.dy = 0;
    }
    scroll_move(0, sonic.dx, sonic.dy);
    scroll_move(1, sonic.dx >> 1, sonic.dy >> 1);
}

void sonic_display() {
    sprite_draw(&sonic);
}
