#pragma once

#include "../include.h"
#include "../utils/utils.hpp"
#include <string>
#include <cstdint>

class ENTRY
{
public:
	uint32_t eid;
	char ename[6] = "";
	bool norm_chunk_ent_is_loaded = false;
	bool is_tpage = false;
	int32_t esize;
	int32_t chunk;
	std::vector<uint8_t> data{};
	LIST related{};
	uint32_t* distances = NULL;
	bool* visited = NULL;

	inline uint8_t* _data() { return data.data(); }

	LIST get_neighbours();
	LIST get_models();
	LIST get_links(int32_t item_idx);
	LIST get_special_entries_raw();
	LIST get_special_entries_extended(ELIST& elist);
	LIST get_sceneries();
	LIST get_entities_to_load(ELIST& elist, int32_t* config, LIST& neighbours, int32_t camera_index, int32_t point_index);	

	int32_t get_nth_item_offset(int32_t n);
	uint8_t* get_nth_item(int32_t n);
	uint8_t* get_nth_entity(int32_t entity_n);
	uint8_t* get_nth_main_cam(int32_t cam_n);
	int32_t entry_type();
	bool is_normal_chunk_entry();
	int32_t item_count();
	int32_t entity_count();
	int32_t cam_item_count();
	int32_t neighbour_count();
	int32_t scenery_count();
	bool check_item_count();
	void remove_nth_item(int32_t n);
	uint32_t get_zone_track();

	static std::string eid2s(uint32_t eid);
	static int32_t get_nth_item_offset(uint8_t* entry, int32_t n);
	static uint8_t* get_nth_item(uint8_t* entry, int32_t n);
	static int32_t item_count(uint8_t* entry);
	static int32_t entity_count(uint8_t* entry);
	static int32_t cam_item_count(uint8_t* entry);
	static int32_t neighbour_count(uint8_t* entry);
	static int32_t scenery_count(uint8_t* entry);
};

class ELIST : public std::vector<ENTRY>
{
public:
	void sort_by_eid();
	void sort_by_esize();
	void update_gool_references();
	int32_t get_index(uint32_t eid);
	int32_t count();
	LIST get_normal_entries();

	int32_t assign_primary_chunks_all(int32_t chunk_count);
	int32_t remove_empty_chunks(int32_t chunk_border_sounds, int32_t chunk_count);
	int32_t merge_permaloaded(int32_t chunk_obder_sounds, int32_t chunk_count, LIST& permaloaded);
	void print_transitions();
	void sort_load_lists();
	void check_unspecified_entities(DEPENDENCIES& sub_info);
	void check_loaded();
};

// stores a camera path link
struct CAMERA_LINK
{
	uint8_t type;
	uint8_t zone_index;
	uint8_t cam_index;
	uint8_t flag;

	inline CAMERA_LINK(uint32_t link)
	{
		type = (link & 0xFF000000) >> 24;
		zone_index = (link & 0xFF0000) >> 16;
		cam_index = (link & 0xFF00) >> 8;
		flag = link & 0xFF;
	}
};

struct DRAW_ITEM
{
	uint8_t neighbour_zone_index;
	uint16_t ID;
	uint8_t neighbour_item_index;

	inline DRAW_ITEM(uint32_t value)
	{
		neighbour_item_index = (value & 0xFF000000) >> 24;
		ID = (value & 0xFFFF00) >> 8;
		neighbour_zone_index = value & 0xFF;
	}
};
