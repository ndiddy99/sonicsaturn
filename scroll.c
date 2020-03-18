#include <string.h>
#include <sega_def.h>
#include <sega_mth.h>
#include <sega_scl.h>

#include "cd.h"
#include "graphicrefs.h"
#include "print.h"
#include "scroll.h"
#include "sprite.h"
#include "graphic/cpz.c"

Fixed32 scrolls_x[]  = {0, 0, 0, 0};
Fixed32 scrolls_y[]  = {0, 0, 0, 0};
Sint32 map_tiles_x[] = {0, 0};
Sint32 map_tiles_y[] = {0, 0};
Uint32 copy_modes[]  = {0, 0};
Uint16 *maps[2]; //map locations in WRAM for the scrolling backgrounds
int scroll_xsize = 0;
int scroll_ysize = 0;
Uint32 vram[] = {SCL_VDP2_VRAM_A0, SCL_VDP2_VRAM_A0 + 0x4000, SCL_VDP2_VRAM_B1, SCL_VDP2_VRAM_B1 + 0x4000}; //where in VRAM each tilemap is
#define VRAM_PTR(bg) ((Uint32 *)vram[bg])
//block offset for 8x8 tiles in A1
#define VRAM_A1_OFFSET (0x1000)

/*
 * 0: NBG0 Pattern Name
 * 1: NBG1 Pattern Name
 * 2: NBG2 Pattern Name
 * 3: NBG3 Pattern Name
 * 4: NBG0 Character Pattern
 * 5: NBG1 Character Pattern
 * 6: NBG2 Character Pattern
 * 7: NBG3 Character Pattern
 * C: NBG0 Vertical Scroll Table
 * D: NBG1 Vertical Scroll Table
 * E: CPU Read/Write
 * F: No Access
 */

/*
 Data Type			# Accesses required
 Pattern name data          1
 16-color tiles		  		1
 256-color tiles	  		2
 2048-color tiles	  		4
 32K-color tiles	  		4
 16M-color tiles	  		8
 Vertical scroll data 		1
 */

// There's also numerous read restrictions, see SOA technical bulletin #6 for more information

Uint16	CycleTb[]={
	0x0011,0xeeee,
	0x5555,0x4444,
	0x6677,0xffff,
	0x23ff,0xeeee
};

SclConfig scfg0;
SclConfig scfg1;
SclConfig scfg2;
SclConfig scfg3;

SclLineparam line_param0;
SclLineparam line_param1;

#define BLOCK_MIRROR_NONE (0)
#define BLOCK_MIRROR_HORIZ (1)
#define BLOCK_MIRROR_VERT (2)
#define BLOCK_MIRROR_BOTH (3)
#define TILE_XFLIP (1 << 30)
#define TILE_YFLIP (1 << 31)

Uint32 block_defs[0x300 * 4];
Uint16 chunk_defs[0x100 * 64];
Uint8 level[0x1000];
Uint8 slopes_normal[0x1000];
Uint8 slopes_rotated[0x1000];
Uint8 angles[0x100];
Uint8 collision_indexes_pri[0x300];
Uint8 collision_indexes_sec[0x300];

void scroll_init() {
	int i;
	Uint16 BackCol;
	Uint8 *VramWorkP;
	Uint32 *tilemap_ptr;
	Uint16 *lwram_ptr = (Uint16 *)LWRAM;
	SclVramConfig vram_cfg;

	SCL_SetColRamMode(SCL_CRM24_1024);
		SCL_AllocColRam(SCL_NBG0, 64, OFF);
		SCL_SetColRam(SCL_NBG0, 0, 64, (void *)(cpz_pal));
		SCL_AllocColRam(SCL_NBG1, 64, OFF);
		SCL_SetColRam(SCL_NBG1, 0, 64, (void *)(cpz_pal));
		BackCol = 0x0004; //set the background color
	SCL_SetBack(SCL_VDP2_VRAM+0x80000-2,1,&BackCol);

	//---nbg0---
	VramWorkP = (Uint8 *)SCL_VDP2_VRAM_A1;
	cd_load(cpz_name, (void *)LWRAM, 32 * 867);
	memcpy(VramWorkP, (void *)LWRAM, 32 * 867);
	cd_load_nosize("CPZ16.BIN", block_defs);
	cd_load_nosize("CPZ128.BIN", chunk_defs);
	cd_load_nosize("CPZ.LVL", level);
	cd_load_nosize("SLOPESTD.BIN", slopes_normal);
	cd_load_nosize("SLOPEROT.BIN", slopes_rotated);
	cd_load_nosize("ANGLES.BIN", angles);
	cd_load_nosize("CPZINDP.BIN", collision_indexes_pri);
	cd_load_nosize("CPZINDS.BIN", collision_indexes_sec);
	// cd_load(level->bg_far.map_name, (void *)LWRAM, level->bg_far.map_width * level->bg_far.map_height * 2);
	tilemap_ptr = VRAM_PTR(0);
	for (i = 0; i < 64 * 64; i++) {
		tilemap_ptr[i] = VRAM_A1_OFFSET;
	}
	tilemap_ptr = VRAM_PTR(1);
	for (i = 0; i < 64 * 64; i++) {
		tilemap_ptr[i] = VRAM_A1_OFFSET;
	}


	//load initial level segment
	for (i = 0; i < 4; i++) {
		scroll_load_chunk(1, level[0x80 + i], i, 0);
		scroll_load_chunk(1, level[0x180 + i], i, 1);
		scroll_load_chunk(1, level[0x280 + i], i, 2);
	}

	for (i = 0; i < 4; i++) {
		scroll_load_chunk(0, level[0x100 + i], i, 0);
		scroll_load_chunk(0, level[0x200 + i], i, 1);
		scroll_load_chunk(0, level[0x300 + i], i, 2);
	}

	// memcpy(TilemapVram, (void *)LWRAM, level->bg_far.map_width * level->bg_far.map_height * 2);
	// memcpy(TilemapVram, (void *)LWRAM, 0x800);
	// TilemapVram[0] = 2;
	// TilemapVram[1] = 2;

	// TilemapVram = VRAM_PTR(3);
	// memcpy(TilemapVram, data->bg3_tilemap, 0x800);

	//scroll initial configuration
	SCL_InitConfigTb(&scfg0);
		scfg0.dispenbl      = ON;
		scfg0.charsize      = SCL_CHAR_SIZE_1X1;
		scfg0.pnamesize     = SCL_PN2WORD;
		scfg0.platesize     = SCL_PL_SIZE_1X1; //they meant "plane size"
		scfg0.coltype       = SCL_COL_TYPE_16;
		scfg0.datatype      = SCL_CELL;
		// scfg0.dispenbl      = ON;
		// scfg0.charsize      = SCL_CHAR_SIZE_2X2;
		// scfg0.pnamesize     = SCL_PN1WORD;
		// scfg0.flip          = SCL_PN_10BIT;
		// scfg0.platesize     = SCL_PL_SIZE_1X1; //they meant "plane size"
		// scfg0.coltype       = SCL_COL_TYPE_16;
		// scfg0.datatype      = SCL_CELL;
		// scfg0.patnamecontrl = 0x0004; //vram A1 offset		
	for(i=0;i<4;i++)   scfg0.plate_addr[i] = vram[0];
	SCL_SetConfig(SCL_NBG0, &scfg0);

	memcpy((void *)&scfg1, (void *)&scfg0, sizeof(SclConfig));
	scfg1.dispenbl = ON;
	for(i=0;i<4;i++)   scfg1.plate_addr[i] = vram[1];
	SCL_SetConfig(SCL_NBG1, &scfg1);

	memcpy((void *)&scfg2, (void *)&scfg0, sizeof(SclConfig));
	scfg2.dispenbl = OFF;
	scfg2.coltype = SCL_COL_TYPE_256;
	scfg2.patnamecontrl = 0x0008;
	for(i=0;i<4;i++)   scfg2.plate_addr[i] = vram[2];
	SCL_SetConfig(SCL_NBG2, &scfg2);

	memcpy((void *)&scfg3, (void *)&scfg2, sizeof(SclConfig));
	scfg3.dispenbl = OFF;
	for(i=0;i<4;i++)   scfg3.plate_addr[i] = vram[3];
	SCL_SetConfig(SCL_NBG3, &scfg3);
	
	//setup VRAM configuration
	SCL_InitVramConfigTb(&vram_cfg);
		vram_cfg.vramModeA = ON; //separate VRAM A into A0 & A1
		vram_cfg.vramModeB = ON; //separate VRAM B into B0 & B1
	SCL_SetVramConfig(&vram_cfg);

	//setup vram access pattern
	SCL_SetCycleTable(CycleTb);
	 
	SCL_Open(SCL_NBG0);
		SCL_MoveTo(FIXED(0), FIXED(0), 0); //home position
	SCL_Close();
	SCL_Open(SCL_NBG1);
		SCL_MoveTo(FIXED(0), FIXED(0), 0);
	SCL_Close();
	SCL_Open(SCL_NBG2);
		SCL_MoveTo(FIXED(0), FIXED(0), 0);
	SCL_Close();
	SCL_Open(SCL_NBG3);
		SCL_MoveTo(FIXED(0), FIXED(0), 0);
	SCL_Close();
	scroll_scale(0, FIXED(1));
	scroll_scale(1, FIXED(1));
	SCL_SetPriority(SCL_SPR,  7); //set layer priorities
	SCL_SetPriority(SCL_SP1,  7);
	SCL_SetPriority(SCL_NBG0, 6);
	SCL_SetPriority(SCL_NBG1, 5);
	SCL_SetPriority(SCL_NBG2, 4);
	SCL_SetPriority(SCL_NBG3, 3);
	
	// curr_level = level;

	SCL_SetColMixMode(1, SCL_IF_FRONT);
	SCL_SetColMixRate(SCL_SP1, 6);
}


void scroll_load_block(int num, int block, int x, int y) {
	Uint32 *tilemap_ptr = VRAM_PTR(num);
	x <<= 1;
	y <<= 1;
	int mirror = (block & 0xc00) >> 10;
	block &= 0x3ff; //isolate the tile index

	if (x < 0 || y < 0) {
		return;
	}

	switch (mirror) {
	case BLOCK_MIRROR_NONE:
		tilemap_ptr[y * 64 + x]           = block_defs[block << 2] | VRAM_A1_OFFSET;
		tilemap_ptr[y * 64 + x + 1]       = block_defs[(block << 2) + 1] | VRAM_A1_OFFSET;
		tilemap_ptr[(y + 1) * 64 + x]     = block_defs[(block << 2) + 2] | VRAM_A1_OFFSET;
		tilemap_ptr[(y + 1) * 64 + x + 1] = block_defs[(block << 2) + 3] | VRAM_A1_OFFSET;
		break;
	
	case BLOCK_MIRROR_HORIZ:
		tilemap_ptr[y * 64 + x]           = (block_defs[(block << 2) + 1] | VRAM_A1_OFFSET) ^ TILE_XFLIP;
		tilemap_ptr[y * 64 + x + 1]       = (block_defs[block << 2] | VRAM_A1_OFFSET) ^ TILE_XFLIP;
		tilemap_ptr[(y + 1) * 64 + x]     = (block_defs[(block << 2) + 3] | VRAM_A1_OFFSET) ^ TILE_XFLIP;
		tilemap_ptr[(y + 1) * 64 + x + 1] = (block_defs[(block << 2) + 2] | VRAM_A1_OFFSET) ^ TILE_XFLIP;
		break;

	case BLOCK_MIRROR_VERT:
		tilemap_ptr[y * 64 + x]           = (block_defs[(block << 2) + 2] | VRAM_A1_OFFSET) ^ TILE_YFLIP;
		tilemap_ptr[y * 64 + x + 1]       = (block_defs[(block << 2) + 3] | VRAM_A1_OFFSET) ^ TILE_YFLIP;
		tilemap_ptr[(y + 1) * 64 + x]     = (block_defs[block << 2] | VRAM_A1_OFFSET) ^ TILE_YFLIP;
		tilemap_ptr[(y + 1) * 64 + x + 1] = (block_defs[(block << 2) + 1] | VRAM_A1_OFFSET) ^ TILE_YFLIP;
		break;
	
	case BLOCK_MIRROR_BOTH:
		tilemap_ptr[y * 64 + x]           = (block_defs[(block << 2) + 3] | VRAM_A1_OFFSET) ^ (TILE_XFLIP | TILE_YFLIP);
		tilemap_ptr[y * 64 + x + 1]       = (block_defs[(block << 2) + 2] | VRAM_A1_OFFSET) ^ (TILE_XFLIP | TILE_YFLIP);
		tilemap_ptr[(y + 1) * 64 + x]     = (block_defs[(block << 2) + 1] | VRAM_A1_OFFSET) ^ (TILE_XFLIP | TILE_YFLIP);
		tilemap_ptr[(y + 1) * 64 + x + 1] = (block_defs[block << 2] | VRAM_A1_OFFSET) ^ (TILE_XFLIP | TILE_YFLIP);
		break;
	}
}

#define BLOCK_XFLIP (0x400)
#define BLOCK_YFLIP (0x800)

void scroll_load_chunk(int num, int chunk, int x, int y) {
	int count = 0;
	int i, j;
	Uint16 block;
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			block = chunk_defs[chunk * 64 + count++];
			scroll_load_block(num, block & 0xfff, j + (x * 8), i + (y * 8));
		}
	}
}


void scroll_move(int num, Fixed32 x, Fixed32 y) {
	Sint32 curr_tile_x;
	Sint32 curr_tile_y;

	scrolls_x[num] += x;
	scrolls_y[num] += y;
	curr_tile_x = MTH_FixedToInt(scrolls_x[num]) >> 4; //tile size is 16x16
	curr_tile_y = MTH_FixedToInt(scrolls_y[num]) >> 4;
	if (curr_tile_x - map_tiles_x[num] > 0) { //if x value increasing
		copy_modes[num] |= COPY_MODE_RCOL;
	}
	else if (curr_tile_x - map_tiles_x[num] < 0) { //if x value decreasing
		copy_modes[num] |= COPY_MODE_LCOL;
	}
	if (curr_tile_y - map_tiles_y[num] > 0) { //if y value increasing
		copy_modes[num] |= COPY_MODE_BROW;
	}
	else if (curr_tile_y - map_tiles_y[num] < 0) { //if y value decreasing
		copy_modes[num] |= COPY_MODE_TROW;
	}
	map_tiles_x[num] = curr_tile_x;
	map_tiles_y[num] = curr_tile_y;
	//Scroll bitmasks are:
	//NBG0 - (1 << 2)
	//NBG1 - (1 << 3)
	//etc
	SCL_Open(1 << (num + 2));
		SCL_MoveTo(scrolls_x[num], scrolls_y[num], 0);
	SCL_Close();
}

//moves scroll absolutely to coordinates
void scroll_set(int num, Fixed32 x, Fixed32 y) {
	scroll_move(num, x - scrolls_x[num], y - scrolls_y[num]);
}

#define ZOOM_HALF_NBG0 (0x1)
#define ZOOM_QUARTER_NBG0 (0x2)
#define ZOOM_HALF_NBG1 (0x100)
#define ZOOM_QUARTER_NBG1 (0x200)
#define LOW_BYTE (0xFF)
#define HIGH_BYTE (0xFF00)

void scroll_scale(int num, Fixed32 scale) {
	SCL_Open(1 << (num + 2));
		SCL_Scale(scale, scale);
	SCL_Close();
	//reset the configuration byte for the given background
	Scl_n_reg.zoomenbl &= (num == 0 ? HIGH_BYTE : LOW_BYTE);
	if (scale >= FIXED(1)) {
		Scl_n_reg.zoomenbl &= (num == 0 ? ~(ZOOM_HALF_NBG0 | ZOOM_QUARTER_NBG0)
										: ~(ZOOM_HALF_NBG1 | ZOOM_QUARTER_NBG1));
		return;
	}
	else if (scale < FIXED(1) && scale >= FIXED(0.5)) {
		Scl_n_reg.zoomenbl |= (num == 0 ? ZOOM_HALF_NBG0 : ZOOM_HALF_NBG1);
	}
	else {
		Scl_n_reg.zoomenbl |= (num == 0 ? ZOOM_QUARTER_NBG0 : ZOOM_QUARTER_NBG1);
	}
}

//gets the value at the given coordinates for a map
Uint16 scroll_get(int num, int x, int y) {
	Uint8 chunk;

	if (x < 0 || y < 0 || x > 2048 || y > 256) {
		return 0;
	}
	if (num == 0) {
		//foreground and background stored line-by-line
		//and interleaved
		chunk = level[(y / 8) * 0x100 + (x / 8)];
	}
	else {
		//background
		chunk = level[(y / 8) * 0x100 + 0x80 + (x / 8)];
	}
	//each chunk is 64 words, an 8x8 grid.
	return chunk_defs[(chunk * 64) + ((y % 8) * 8) + (x % 8)];
}

Uint8 scroll_height(int primary, Fixed32 x, Fixed32 y) {
	//convert from 16.16 fixed-point pixels to 16x16 tiles
	Uint16 block = scroll_get(0, x >> 20, y >> 20);
	Uint8 slope_num;
	//if block isn't solid, return 0
	// if ((block & 0x000) == 0) {
		// return 0;
	// }
	if (primary) {
		slope_num = collision_indexes_pri[block & 0x3ff];
	}
	else {
		slope_num = collision_indexes_sec[block & 0x3ff];
	}
	if (block & BLOCK_XFLIP) {
		                    //block start index  index within block
		return slopes_normal[(slope_num << 4) + (15 - ((x >> 16) & 0xf))];
	}
	                    //block start index  index within block
	return slopes_normal[(slope_num << 4) + ((x >> 16) & 0xf)];
}

Fixed32 scroll_angle(int primary, Fixed32 x, Fixed32 y) {
	//convert from 16.16 fixed-point pixels to 16x16 tiles
	Uint16 block = scroll_get(0, x >> 20, y >> 20);
	Uint8 slope_num;
	if (primary) {
		slope_num = collision_indexes_pri[block & 0x3ff];
	}
	else {
		slope_num = collision_indexes_sec[block & 0x3ff];
	}
	Uint8 angle = angles[slope_num];
	if (angle == 0xff) {
		//workaround for flat slopes being denoted as 0xff
		return 0;
	}
	//convert angle from its original format to a Fixed32 with degrees (0 to 360)		
	Fixed32 degrees = MTH_Mul(MTH_IntToFixed(256 - angle), MTH_FIXED(1.40625));
	//convert from 0 to 360 to -180 to 180 (what the SBL trig functions take)
	Fixed32 degrees_range = ((degrees + MTH_FIXED(180)) % MTH_FIXED(360)) - MTH_FIXED(180);
	//if block is facing the other way, invert the angle
	if (block & BLOCK_XFLIP) {
		return -degrees_range;
	}
	return degrees_range;
}

void scroll_copy(int num) {
	int i;

	if (copy_modes[num] & COPY_MODE_RCOL) {
		for (i = -2; i < SCREEN_TILES_Y + 2; i++) {
			scroll_load_block(num,
				scroll_get(num, map_tiles_x[num] + SCREEN_TILES_X, i + map_tiles_y[num]),
				((map_tiles_x[num] + SCREEN_TILES_X) % 32),
				(map_tiles_y[num] + i) % 32);
		}
	}
	if (copy_modes[num] & COPY_MODE_LCOL) {
		for (i = -2; i < SCREEN_TILES_Y + 2; i++) {
			scroll_load_block(num,
				scroll_get(num, map_tiles_x[num] - 1, i + map_tiles_y[num]),
				((map_tiles_x[num] - 1) % 32),
				(map_tiles_y[num] + i) % 32);
		}		
	}
	if (copy_modes[num] & COPY_MODE_BROW) {
		for (i = -2; i < SCREEN_TILES_X + 2; i++) {
			scroll_load_block(num,
				scroll_get(num, map_tiles_x[num] + i, map_tiles_y[num] + SCREEN_TILES_Y),
				(map_tiles_x[num] + i) % 32,
				(map_tiles_y[num] + SCREEN_TILES_Y) % 32);
		}
	}
	if (copy_modes[num] & COPY_MODE_TROW) {
		for (i = -2; i < SCREEN_TILES_X + 2; i++) {
			scroll_load_block(num,
				scroll_get(num, map_tiles_x[num] + i, map_tiles_y[num] - 1),
				(map_tiles_x[num] + i) % 32,
				(map_tiles_y[num] - 1) % 32);
		}
	}
	copy_modes[num] = 0;
}

void scroll_linescroll4(int num, Fixed32 scroll_val, int boundary1, int boundary2, int boundary3) {
	SclLineparam *lp = (num == SCROLL_BACKGROUND1 ? &line_param0 : &line_param1);

	for (int i = 0; i < 224; i++) {
		if (i < boundary1) {
			lp->line_tbl[i].h = (scroll_val >> 3);
		}
		else if (i < boundary2) {
			lp->line_tbl[i].h = (scroll_val >> 2);
		}
		else if (i < boundary3) {
			lp->line_tbl[i].h = (scroll_val >> 1);
		}
		else {
			lp->line_tbl[i].h = scroll_val;
		}
	}
	if (num == SCROLL_BACKGROUND1) {
		SCL_Open(SCL_NBG0);
	}
	else {
		SCL_Open(SCL_NBG1);
	}
	SCL_SetLineParam(lp);
	SCL_Close();
}

