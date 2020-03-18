#include <SEGA_MTH.H>
#include "sprite.h"
#include "scroll.h"
#include "sonic.h"
#include "print.h"
#include "vblank.h"

// #define ABS(x) (x > 0 ? x : -x)

#define ACCEL (MTH_FIXED(0.046875))
#define DECEL (MTH_FIXED(0.5))
#define TOP_SPEED (MTH_FIXED(6))

#define X_OFFSET (MTH_FIXED(18))
#define Y_OFFSET (MTH_FIXED(20))

#define STANDING_XRADIUS (MTH_FIXED(9))
#define STANDING_YRADIUS (MTH_FIXED(19))

#define SLOPE_RUN (MTH_FIXED(0.125))
#define SLOPE_ROLLUP (MTH_FIXED(0.078125))
#define SLOPE_ROLLDOWN (MTH_FIXED (0.3125))

SPRITE_INFO sonic;
Fixed32 ground_speed;
Fixed32 slope_factor;
Fixed32 angle;

#define MODE_GROUND (2)
#define MODE_RWALL (3)
#define MODE_CEIL (0)
#define MODE_LWALL (1)
int ground_mode;

#define SENSOR_A (1)
#define SENSOR_B (2)
int last_ground_sensor = 0;


void sonic_init() {
    sonic.char_num = 96;
	sonic.x = MTH_FIXED(160);
	sonic.y = MTH_FIXED(100);
    sonic.dx = 0;
    sonic.dy = 0;
    sonic.x_size = MTH_FIXED(32);
    sonic.y_size = MTH_FIXED(40);
	sonic.angle = 0;
	sonic.scale = MTH_FIXED(1);
    ground_speed = 0;
}

void sonic_move() {
    //set angle based on where sonic is touching the ground
    if (last_ground_sensor == SENSOR_A) {
        angle = scroll_angle(1, sonic.x - STANDING_XRADIUS, sonic.y + STANDING_YRADIUS + MTH_FIXED(1));
    }
    else {
        angle = scroll_angle(1, sonic.x + STANDING_XRADIUS, sonic.y + STANDING_YRADIUS + MTH_FIXED(1));
    }
    print_num(angle >> 16, 1, 0);
    
    ground_speed -= MTH_Mul(SLOPE_RUN, MTH_Sin(angle));
    ground_mode = (((angle >> 16) + 225) % 360) / 90;

    if (PadData1 & PAD_L) {
        //allow sonic to turn around quickly if he's already moving right
        if (ground_speed > 0) {
            ground_speed -= DECEL;
            if (ground_speed <= 0) {
                ground_speed = MTH_FIXED(-0.5);
            }
        }
        else if (ground_speed > -TOP_SPEED) {
            ground_speed -= ACCEL;
            //only check if we've exceeded the speed cap if we were previously
            //going slower than the speed cap
            if (ground_speed <= -TOP_SPEED) {
                ground_speed = -TOP_SPEED;
            }
        }
    }
    if (PadData1 & PAD_R) {
        //allow sonic to turn around quickly if he's already moving left
        if (ground_speed < 0) {
            ground_speed += DECEL;
            if (ground_speed >= 0) {
                ground_speed = MTH_FIXED(0.5);
            }
        }
        else if (ground_speed < TOP_SPEED) {
            ground_speed += ACCEL;
            //only check if we've exceeded the speed cap if we were previously
            //going slower than the speed cap            
            if (ground_speed >= TOP_SPEED) {
                ground_speed = TOP_SPEED;
            }
        }
    }
    //if we're not holding either direction, apply friction
    if ((PadData1 & (PAD_L | PAD_R)) == 0) {
        if (ground_speed < 0) {
            //if we're going left, add speed until it's greater than zero, then
            //set the speed to zero
            ground_speed += ACCEL;
            if (ground_speed > 0) {
                ground_speed = 0;
            }
        }        
        else if (ground_speed > 0) {
            //if we're going right, subtract speed until it's less than zero, then
            //set the speed to zero
            ground_speed -= ACCEL;
            if (ground_speed < 0) {
                ground_speed = 0;
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

    sonic.dx = MTH_Mul(ground_speed, MTH_Cos(angle));
    sonic.dy = MTH_Mul(ground_speed, -MTH_Sin(angle));
    sonic.x += sonic.dx;
    sonic.y += sonic.dy;
    //make it look like sonic's running on the slope
    if (ABS(angle) > MTH_FIXED(33)) {
        sonic.angle = -angle;
    }
    else {
        sonic.angle = 0;
    }

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
    Sint8 height;
    if (height_a > height_b) {
        last_ground_sensor = SENSOR_A;
        height = height_a;
    }
    else {
        last_ground_sensor = SENSOR_B;
        height = height_b;
    }
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
