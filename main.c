#include <string.h> //memcpy
#define	_SPR2_
#include <sega_spr.h>
#include <sega_scl.h> 
#include <sega_mth.h>
#include <sega_cdc.h>
#include <sega_sys.h>

#include "cd.h"
#include "graphicrefs.h"
#include "scroll.h"
#include "sound.h"
#include "sprite.h"
#include "print.h"
#include "vblank.h"

Uint32 frame = 0;

int main() {
	CdcStat cd_status;

	cd_init();
	sprite_init();
	print_init();
	scroll_init();
	SCL_SetSpriteMode(SCL_TYPE5,SCL_MIX,SCL_SP_WINDOW);
	sound_init();


	// sound_cdda(2);



	while(1) {

		frame++;
		print_num(frame, 10, 10);
		//if the cd drive is opened, return to menu
		CDC_GetPeriStat(&cd_status);
		if ((cd_status.status & 0xF) == CDC_ST_OPEN) {
			SYS_EXECDMP();
		}

		if (PadData1 & PAD_L) {
			scroll_move(0, MTH_FIXED(-4), 0);
			scroll_move(1, MTH_FIXED(-2), 0);
		}

		if (PadData1 & PAD_R) {
			scroll_move(0, MTH_FIXED(4), 0);
			scroll_move(1, MTH_FIXED(2), 0);
		}
		if (PadData1 & PAD_U) {
			scroll_move(0, 0, MTH_FIXED(-4));
			scroll_move(1, 0, MTH_FIXED(-2));
		}
		if (PadData1 & PAD_D) {
			scroll_move(0, 0, MTH_FIXED(4));
			scroll_move(1, 0, MTH_FIXED(2));
		}

		SPR_2OpenCommand(SPR_2DRAW_PRTY_OFF);
			// player_draw();
			// sprite_draw_all();
			print_display();
		SPR_2CloseCommand();

		SCL_DisplayFrame();
	}
	return 0;
}
