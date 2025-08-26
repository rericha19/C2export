#pragma once
#include <stdint.h>
#include "../game_structs/entry.hpp"

namespace ll_export
{
	void camera_fix(uint8_t* cam, int32_t length);
	void entity_coord_fix(uint8_t* item, int32_t itemlength);
	void gool_patch(ENTRY* gool, int32_t game, bool porting);
	void export_level(int32_t levelid, ELIST& elist, uint8_t** chunks, uint8_t** chunks2, int32_t chunk_count);
	char* make_path(char* dir_path, std::string type, uint32_t eid, int32_t counter);
	void model_patch(ENTRY* model, int32_t game, bool porting);
	void zone_patch(ENTRY* zone, int32_t zonetype, int32_t game, bool porting);
}