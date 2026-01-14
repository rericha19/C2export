#pragma once
#include "../include.h"

namespace level_alter
{
	void wipe_draw_lists(ELIST& elist);
	void wipe_entities(ELIST& elist);
	void convert_old_dl_override(ELIST& elist);
	void flip_level_x(ELIST& elist, int32_t* chunk_count);
	void flip_level_y(ELIST& elist, int32_t* chunk_count);
	void level_recolor(ELIST& elist);
	void medieval_rain_fix(ELIST& elist);
	void warp_swirl_bs_fix(ELIST& elist);
	void ll_alter(int32_t alter_type);
}