#pragma once

#include "../include.h"
#include "../utils/utils.hpp"

class ENTITY_PATH : public std::vector<int16_t>
{
public:
	inline int32_t length() { return int32_t(size() / 3); }
};

class ENTRY
{
public:
	uint32_t m_eid = 0;
	char m_ename[6] = "";
	bool m_is_loaded = true;
	bool m_is_tpage = false;
	int32_t m_esize = 0;
	int32_t m_chunk = -1;
	uint32_t m_graph_visited{}; // bitset
	std::vector<int32_t> m_graph_distances{};
	std::vector<uint8_t> m_data{};
	LIST m_related{};

	inline uint8_t* _data() { return m_data.data(); }

	LIST get_neighbours();
	LIST get_models();
	LIST get_links(int32_t item_idx);
	LIST get_special_entries_raw();
	LIST get_special_entries_extended(ELIST& elist);
	LIST get_sceneries();
	LIST get_entities_to_load(ELIST& elist, LIST& neighbours, int32_t camera_index, int32_t point_index);
	LIST get_scenery_textures();
	LIST get_model_textures();
	LIST get_anim_refs(ELIST& elist);
	uint32_t get_nth_slst(int32_t main_cam_idx);
	uint32_t get_zone_track();

	void parse_relatives();
	void parse_spawns(SPAWNS& spawns);

	// Deconstructs the load or draw lists and saves into a convenient struct.
	GENERIC_LOAD_LIST get_generic_lists(int32_t prop_code, int32_t cam_index);
	LOAD_LIST get_load_lists(int32_t cam_index);
	DRAW_LIST get_draw_lists(int32_t cam_index);

	ENTITY_PATH get_ent_path(int32_t item_index);
	int32_t get_ent_path_len(int32_t item_index);
	int32_t get_nth_item_offset(int32_t n);
	uint8_t* get_nth_item(int32_t n);
	uint8_t* get_nth_entity(int32_t entity_n);
	uint8_t* get_nth_main_cam(int32_t cam_n);
	EntryType get_entry_type();
	int32_t get_item_count();
	int32_t get_entity_count();
	int32_t get_cam_item_count();
	int32_t get_neighbour_count();
	int32_t get_scenery_count();
	bool is_normal_chunk_entry();

	bool check_get_item_count();
	void remove_nth_item(int32_t n);
	void replace_nth_item(int32_t item_index, uint8_t* new_item, int32_t new_size);

	static std::string eid2s(uint32_t eid);
	static int32_t get_nth_item_offset(uint8_t* entry, int32_t n);
	static uint8_t* get_nth_item(uint8_t* entry, int32_t n);
	static int32_t get_item_count(uint8_t* entry);
	static int32_t get_entity_count(uint8_t* entry);
	static int32_t get_cam_item_count(uint8_t* entry);
	static int32_t get_neighbour_count(uint8_t* entry);
	static int32_t get_scenery_count(uint8_t* entry);
	static bool is_before(ENTRY& zone, int32_t cam_index, ENTRY& neigh, int32_t neigh_cam_idx);
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
