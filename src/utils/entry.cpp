
#include "../include.h"
#include "entry.hpp"
#include "elist.hpp"

// gets zone neighbours
LIST ENTRY::get_neighbours()
{
	if (get_entry_type() != EntryType::Zone)
		return {};

	LIST neighbours{};
	auto item0_ptr = get_nth_item(0);
	for (int32_t k = 0; k < get_neighbour_count(); k++)
	{
		int32_t neighbour_eid = from_u32(item0_ptr + C2_NEIGHBOURS_START + 4 + 4 * k);
		neighbours.add(neighbour_eid);
	}

	return neighbours;
}

// gets models used by an animation
LIST ENTRY::get_models()
{
	auto get_model_single = [](uint8_t* anim, int32_t item)
		{
			int32_t item_off = ENTRY::get_nth_item_offset(anim, item);
			return from_u32(anim + item_off + 0x10);
		};

	if (get_entry_type() != EntryType::Anim)
		return {};

	LIST models{};
	for (int32_t i = 0; i < get_item_count(); i++)
		models.add(get_model_single(_data(), i));

	return models;
}

// returns list of camera links of the specified camera path 
LIST ENTRY::get_links(int32_t item_idx)
{
	if (get_entry_type() != EntryType::Zone)
		return {};

	LIST links{};
	auto entry = _data();
	uint32_t cam_offset = get_nth_item_offset(item_idx);
	for (int32_t k = 0; k < PROPERTY::count(entry + cam_offset); k++)
	{
		int32_t code = from_u16(entry + cam_offset + 0x10 + 8 * k);
		int32_t offset = from_u16(entry + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
		int32_t prop_len = from_u16(entry + cam_offset + 0x16 + 8 * k);

		if (code != ENTITY_PROP_CAM_PATH_LINKS)
			continue;

		if (prop_len == 1)
		{
			int32_t link_count = from_u16(entry + offset);
			links.resize(link_count);
			for (int32_t l = 0; l < link_count; l++)
				links[l] = from_u32(entry + offset + 4 + 4 * l);
		}
		else
		{
			uint32_t link_count = max(1u, from_u16(entry + offset)) + max(1u, from_u16(entry + offset + 2));
			links.resize(link_count);
			for (uint32_t l = 0; l < link_count; l++)
				links[l] = from_u32(entry + offset + 0x8 + 4 * l);
		}
	}

	return links;
}

// converts uint32_t eid to string eid
std::string ENTRY::eid2s(uint32_t value)
{
	const char charset[] =
		"0123456789"
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"_!";

	std::string s_buf = "abcde\0";
	s_buf[0] = charset[(value >> 25) & 0x3F];
	s_buf[1] = charset[(value >> 19) & 0x3F];
	s_buf[2] = charset[(value >> 13) & 0x3F];
	s_buf[3] = charset[(value >> 7) & 0x3F];
	s_buf[4] = charset[(value >> 1) & 0x3F];
	s_buf[5] = '\0';

	return s_buf;
}

// Gets list of special entries in the zone's first item. For more info see function below.
LIST ENTRY::get_special_entries_raw()
{
	if (get_entry_type() != EntryType::Zone)
		return {};

	const uint8_t* metadata_ptr = get_nth_item(0) + C2_SPECIAL_METADATA_OFFSET;
	int32_t special_entry_count = from_u32(metadata_ptr) & SPECIAL_METADATA_MASK_LLCOUNT;

	LIST special_entries{};
	for (int32_t i = 1; i <= special_entry_count; i++)
		special_entries.add(from_u32(metadata_ptr + i * 4));

	return special_entries;
}

// adds explicitly as well as implicitly added special entries
// implicit are models and textures referenced by animations
LIST ENTRY::get_special_entries_extended(ELIST& elist)
{
	LIST special_entries = get_special_entries_raw();
	LIST iteration_clone = {};
	iteration_clone.copy_in(special_entries);

	for (int32_t i = 0; i < iteration_clone.count(); i++)
	{
		uint32_t item = iteration_clone[i];
		int32_t index = elist.get_index(item);
		if (index == -1)
		{
			printf("[ERROR] Zone %s special entry list contains entry %s which is not present.\n",
				m_ename, eid2str(item));
			special_entries.remove(item);
			continue;
		}

		if (elist[index].get_entry_type() == EntryType::Anim)
			special_entries.copy_in(elist[index].get_anim_refs(elist));
	}

	return special_entries;
}


LIST ENTRY::get_sceneries()
{
	LIST list{};
	for (int32_t i = 0; i < get_scenery_count(); i++)
	{
		uint32_t scen = from_u32(get_nth_item(0) + 0x4 + 0x30 * i);
		list.add(scen);
	}

	return list;
}

// todo maybe make more readable
// collects list of IDs using draw lists of neighbouring camera paths
LIST ENTRY::get_entities_to_load(ELIST& elist, LIST& neighbours, int32_t camera_index, int32_t point_index)
{
	ENTITY_PATH coords = get_ent_path(camera_index);
	if (!coords.length())
		return {};

	LIST entity_list{};

	int32_t draw_dist = elist.m_config[LL_Drawlist_Distance];
	int32_t preloading_flag = elist.m_config[LL_Transition_Preloading_Type];
	double backwards_penalty = config_to_double(elist.m_config[LL_Backwards_Loading_Penalty_DBL]);

	neighbours.copy_in(get_neighbours());

	std::vector<LIST> draw_list_zone = get_expanded_draw_list(camera_index);
	for (int32_t i = 0; i < coords.length(); i++)
		entity_list.copy_in(draw_list_zone[i]);

	LIST links = get_links(camera_index);
	for (int32_t i = 0; i < links.count(); i++)
	{
		int32_t distance = 0;
		CAMERA_LINK link(links[i]);
		if (link.type == 1)
			distance += build_get_distance(coords.data(), point_index, 0, -1, NULL);
		if (link.type == 2)
			distance += build_get_distance(coords.data(), point_index, coords.length() - 1, -1, NULL);

		uint32_t eid_offset = get_nth_item_offset(0) + 4 + link.zone_index * 4 + C2_NEIGHBOURS_START;

		uint32_t neighbour_eid = from_u32(_data() + eid_offset);
		int32_t neigh_idx = elist.get_index(neighbour_eid);
		if (neigh_idx == -1)
			continue;

		// preloading everything check
		uint32_t neighbour_flg = from_u32(_data() + eid_offset + 0x20);
		if (preloading_flag != PRELOADING_ALL && (neighbour_flg == 0xF || neighbour_flg == 0x1F))
			continue;

		neighbours.copy_in(elist[neigh_idx].get_neighbours());

		ENTITY_PATH coords2 = elist[neigh_idx].get_ent_path(2 + 3 * link.cam_index);
		if (!coords2.length())
			continue;

		std::vector<LIST> draw_list_neighbour1 = elist[neigh_idx].get_expanded_draw_list(2 + 3 * link.cam_index);

		int32_t draw_dist_w_orientation = draw_dist;
		if (ENTRY::is_before(*this, camera_index / 3, elist[neigh_idx], link.cam_index))
		{
			draw_dist_w_orientation = distance_with_penalty(draw_dist, backwards_penalty);
		}

		int32_t point_index2;
		if (link.flag == 1)
		{
			distance += build_get_distance(coords2.data(), 0, coords2.length() - 1, draw_dist_w_orientation - distance, &point_index2);
			for (int32_t j = 0; j < point_index2; j++)
				entity_list.copy_in(draw_list_neighbour1[j]);
		}
		if (link.flag == 2)
		{
			distance += build_get_distance(coords2.data(), coords2.length() - 1, 0, draw_dist_w_orientation - distance, &point_index2);
			for (int32_t j = point_index2; j < coords2.length(); j++)
				entity_list.copy_in(draw_list_neighbour1[j]);
		}

		if (distance >= draw_dist_w_orientation)
			continue;

		LIST layer2 = elist[neigh_idx].get_links(2 + 3 * link.cam_index);
		for (auto _l2: layer2)
		{
			CAMERA_LINK link2(_l2);

			uint32_t eid_offset2 = from_u32(elist[neigh_idx]._data() + 0x10) + 4 + link2.zone_index * 4 + C2_NEIGHBOURS_START;			

			uint32_t neighbour_eid2 = from_u32(elist[neigh_idx]._data() + eid_offset2);
			int32_t neigh_idx2 = elist.get_index(neighbour_eid2);
			if (neigh_idx2 == -1)
				continue;

			// preloading everything check
			uint32_t neighbour_flg2 = from_u32(elist[neigh_idx]._data() + eid_offset2 + 0x20);
			if (preloading_flag != PRELOADING_ALL && (neighbour_flg2 == 0xF || neighbour_flg2 == 0x1F))
				continue;
			
			neighbours.copy_in(elist[neigh_idx2].get_neighbours());

			ENTITY_PATH coords3 = elist[neigh_idx2].get_ent_path(2 + 3 * link2.cam_index);
			if (!coords3.length())
				continue;

			std::vector<LIST> draw_list_neighbour2 = elist[neigh_idx2].get_expanded_draw_list(2 + 3 * link2.cam_index);

			int32_t point_index3;
			draw_dist_w_orientation = draw_dist;
			if (ENTRY::is_before(*this, camera_index / 3, elist[neigh_idx2], link2.cam_index))
			{
				draw_dist_w_orientation = distance_with_penalty(draw_dist, backwards_penalty);
			}

			// start to end
			if ((link.type == 2 && link2.type == 2 && link2.flag == 1) || (link.type == 1 && link2.type == 1 && link2.flag == 1))
			{
				build_get_distance(coords3.data(), 0, coords3.length() - 1, draw_dist_w_orientation - distance, &point_index3);
				for (int32_t k = 0; k < point_index3; k++)
					entity_list.copy_in(draw_list_neighbour2[k]);
			}

			// end to start
			if ((link.type == 2 && link2.type == 2 && link2.flag == 1) || (link.type == 1 && link2.type == 1 && link2.flag == 2))
			{
				build_get_distance(coords3.data(), coords3.length() - 1, 0, draw_dist_w_orientation - distance, &point_index3);
				for (int32_t k = point_index3; k < coords3.length(); k++)
					entity_list.copy_in(draw_list_neighbour2[k]);
			}
		}
	}

	return entity_list;
}

LIST ENTRY::get_scenery_textures()
{
	LIST list{};
	uint8_t* item0 = get_nth_item(0);
	int32_t texture_count = from_u32(item0 + 0x28);

	for (int32_t i = 0; i < texture_count; i++)
	{
		uint32_t eid = from_u32(item0 + 0x2C + 4 * i);
		list.add(eid);
	}
	return list;
}

LIST ENTRY::get_model_textures()
{
	LIST list{};
	uint8_t* item0 = get_nth_item(0);
	int32_t tex_count = from_u32(item0 + 0x40);

	for (int32_t i = 0; i < tex_count; i++)
	{
		uint32_t scen_reference = from_u32(item0 + 0xC + 0x4 * i);
		if (scen_reference)
			list.add(scen_reference);
	}
	return list;
}

LIST ENTRY::get_anim_refs(ELIST& elist)
{
	LIST list{};

	for (auto& model : get_models())
	{
		int32_t model_index = elist.get_index(model);
		if (model_index == -1)
		{
			printf("[warning] unknown entry ref in object dependency list, will be skipped:\t %s\n",
				eid2str(model));
			continue;
		}

		list.add(model);
		list.copy_in(elist.at(model_index).get_model_textures());
	}

	return list;
}

void ENTRY::parse_relatives()
{
	int32_t item0off = get_nth_item_offset(0);
	int32_t item1off = get_nth_item_offset(1);
	int32_t item0len = item1off - item0off;

	if (!(item0len == 0x358 || item0len == 0x318))
	{
		printf("[warning] Zone %s item0 size 0x%X ??\n", m_ename, item0len);
		return;
	}

	for (int32_t i = 0; i < get_cam_item_count() / 3; i++)
		m_related.add(get_nth_slst(i));

	m_related.copy_in(get_neighbours());
	m_related.copy_in(get_sceneries());

	uint32_t music_ref = get_zone_track();
	if (music_ref != EID_NONE)
		m_related.add(music_ref);
}

void ENTRY::parse_spawns(SPAWNS& spawns)
{
	auto seek_spawn = [&](int32_t entity_n) -> ENTITY_PATH
		{
			uint8_t* item = get_nth_entity(entity_n);
			int32_t type = PROPERTY::get_value(item, ENTITY_PROP_TYPE);
			int32_t subtype = PROPERTY::get_value(item, ENTITY_PROP_SUBTYPE);
			int32_t coords_offset = PROPERTY::get_offset(item, ENTITY_PROP_PATH);

			if (coords_offset == -1)
				return {};

			if ((type == 0 && subtype == 0) || (type == 34 && subtype == 4))
			{
				ENTITY_PATH coords{};

				for (int32_t i = 0; i < 3; i++)
					coords.push_back((*(int16_t*)(item + coords_offset + 4 + 2 * i)) * 4);
				return coords;
			}

			return {};
		};

	for (int32_t i = 0; i < get_entity_count(); i++)
	{
		ENTITY_PATH coords = seek_spawn(i);
		if (!coords.length())
			continue;

		uint8_t* item1 = get_nth_item(1);
		SPAWN new_spawn{};
		new_spawn.zone = m_eid;
		new_spawn.x = coords[0] + *(int32_t*)(item1);
		new_spawn.y = coords[1] + *(int32_t*)(item1 + 4);
		new_spawn.z = coords[2] + *(int32_t*)(item1 + 8);
		spawns.push_back(new_spawn);
	}
}

// gets slst from camera item
uint32_t ENTRY::get_nth_slst(int32_t main_cam_idx)
{
	uint8_t* item = get_nth_main_cam(main_cam_idx);
	int32_t offset = PROPERTY::get_offset(item, ENTITY_PROP_CAM_SLST);

	if (!offset)
		return 0;

	return from_u32(item + offset + 4);
}

int32_t ENTRY::get_nth_item_offset(uint8_t* entry, int32_t n)
{
	return from_u32(entry + 0x10 + 4 * n);
}

ENTITY_PATH ENTRY::get_ent_path(int32_t item_index)
{
	uint8_t* item = get_nth_item(item_index);
	int32_t offset = PROPERTY::get_offset(item, ENTITY_PROP_PATH);

	if (!offset)
		return {};

	int32_t path_len = from_u32(item + offset);
	ENTITY_PATH coords{};
	coords.resize(3 * path_len);
	memcpy(coords.data(), item + offset + 4, 6 * path_len);
	return coords;
}

int32_t ENTRY::get_ent_path_len(int32_t item_index)
{
	uint8_t* item = get_nth_item(item_index);
	int32_t offset = PROPERTY::get_offset(item, ENTITY_PROP_PATH);

	if (!offset)
		return 0;

	return from_u32(item + offset);
}

// gets offset of the nth item in an entry
int32_t ENTRY::get_nth_item_offset(int32_t n)
{
	return get_nth_item_offset(_data(), n);
}

// get pointer to the nth item in an entry
uint8_t* ENTRY::get_nth_item(uint8_t* entry, int32_t n)
{
	return entry + get_nth_item_offset(entry, n);
}

uint8_t* ENTRY::get_nth_item(int32_t n)
{
	return get_nth_item(_data(), n);
}

uint8_t* ENTRY::get_nth_entity(int32_t entity_n)
{
	return get_nth_item(2 + get_cam_item_count() + entity_n);
}

uint8_t* ENTRY::get_nth_main_cam(int32_t cam_n)
{
	return get_nth_item(2 + 3 * cam_n);
}

EntryType ENTRY::get_entry_type()
{
	if (m_data.empty())
		return EntryType(-1);
	return EntryType(from_s32(_data() + 8));
}

bool ENTRY::is_normal_chunk_entry()
{
	auto type = get_entry_type();
	return (
		type == EntryType::Anim ||
		type == EntryType::Model ||
		type == EntryType::Scenery ||
		type == EntryType::SLST ||
		type == EntryType::Zone ||
		type == EntryType::Demo ||
		type == EntryType::VCOL ||
		type == EntryType::MIDI ||
		type == EntryType::GOOL ||
		type == EntryType::T21);
}

int32_t ENTRY::get_item_count(uint8_t* entry)
{
	return from_u32(entry + 0xC);
}

int32_t ENTRY::get_item_count()
{
	return get_item_count(_data());
}

int32_t ENTRY::get_entity_count(uint8_t* entry)
{
	uint8_t* item0 = ENTRY::get_nth_item(entry, 0);
	return from_u32(item0 + C2_ENTITY_COUNT_OFF);
}

int32_t ENTRY::get_entity_count()
{
	return get_entity_count(_data());
}

int32_t ENTRY::get_cam_item_count(uint8_t* entry)
{
	uint8_t* item0 = ENTRY::get_nth_item(entry, 0);
	return from_u32(item0 + C2_CAMERA_COUNT_OFF);
}

int32_t ENTRY::get_cam_item_count()
{
	return get_cam_item_count(_data());
}

int32_t ENTRY::get_neighbour_count(uint8_t* entry)
{
	uint8_t* item0 = ENTRY::get_nth_item(entry, 0);
	return from_u32(item0 + C2_NEIGHBOURS_START);
}

int32_t ENTRY::get_neighbour_count()
{
	return get_neighbour_count(_data());
}

int32_t ENTRY::get_scenery_count(uint8_t* entry)
{
	uint8_t* item0 = ENTRY::get_nth_item(entry, 0);
	return from_u32(item0 + C2_SCEN_OFFSET);
}

bool ENTRY::is_before(ENTRY& zone, int32_t cam_index, ENTRY& neigh, int32_t neigh_cam_idx)
{
	int32_t distance_neighbour = neigh.m_graph_distances[neigh_cam_idx];
	int32_t distance_current = zone.m_graph_distances[cam_index];

	return (distance_neighbour < distance_current);
}

int32_t ENTRY::get_scenery_count()
{
	return get_scenery_count(_data());
}

bool ENTRY::check_zone_item_count()
{
	int32_t item_cnt = get_item_count();
	int32_t cam_cnt = get_cam_item_count();
	int32_t entity_cnt = get_entity_count();

	if (item_cnt != (2 + cam_cnt + entity_cnt))
	{
		printf("[ERROR] %s's item count (%d) doesn't match item counts in the first item (2 + %d + %d)\n",
			m_ename, item_cnt, cam_cnt, entity_cnt);
		return false;
	}
	return true;
}

void ENTRY::remove_nth_item(int32_t n)
{
	int32_t i, offset;
	int32_t item_c = get_item_count();
	int32_t first_item_off = 0x14 + 4 * item_c;

	if (item_c < n)
	{
		printf("[Warning] Trying to remove item %d from entry %s (has %d items)!\n", n, m_ename, item_c);
		return;
	}

	std::vector<int32_t> item_lengths{};
	std::vector<std::unique_ptr<uint8_t[]>> items{};

	for (i = 0; i < item_c; i++)
	{
		item_lengths.push_back(0);
		items.push_back(nullptr);
	}

	for (int32_t i = 0; i < item_c; i++)
	{
		int32_t next_start = get_nth_item_offset(i + 1);
		if (i == n)
			next_start = m_esize;

		item_lengths[i] = next_start - get_nth_item_offset(i);
	}

	for (offset = first_item_off, i = 0; i < item_c;
		offset += item_lengths[i], i++)
	{
		items[i].reset((uint8_t*)try_malloc(item_lengths[i]));
		memcpy(items[i].get(), _data() + offset, item_lengths[i]);
	}

	item_lengths[n] = 0;
	first_item_off -= 4;

	int32_t new_size = first_item_off;
	for (int32_t i = 0; i < item_c; i++)
		new_size += item_lengths[i];

	m_esize = new_size;
	m_data.resize(new_size);
	uint8_t* new_data = _data();
	*(int32_t*)(new_data) = MAGIC_ENTRY;
	*(int32_t*)(new_data + 0x4) = m_eid;
	*(int32_t*)(new_data + 0x8) = *(int32_t*)(_data() + 8);
	*(int32_t*)(new_data + 0xC) = item_c - 1;

	for (offset = first_item_off, i = 0; i < item_c + 1;
		offset += item_lengths[i], i++)
	{
		if (i == n)
			continue;

		int32_t curr = i;
		if (i > n)
			curr--;
		*(int32_t*)(new_data + 0x10 + curr * 4) = offset;
	}

	for (offset = first_item_off, i = 0; i < item_c;
		offset += item_lengths[i], i++)
	{
		if (i == n)
			continue;
		memcpy(new_data + offset, items[i].get(), item_lengths[i]);
	}
}

// replaces nth item in a zone with an item with specified data and length
void ENTRY::replace_nth_item(int32_t item_index, uint8_t* new_item, int32_t item_size)
{
	int32_t i, offset;
	int32_t item_cnt = get_item_count();
	int32_t first_item_off = 0x14 + 4 * item_cnt;

	std::vector<int32_t> item_lengths{};
	std::vector<std::unique_ptr<uint8_t[]>> items{};

	for (i = 0; i < item_cnt + 1; i++)
	{
		item_lengths.push_back(0);
		items.push_back(nullptr);
	}

	for (int32_t i = 0; i < item_cnt; i++)
		item_lengths[i] = get_nth_item_offset(i + 1) - get_nth_item_offset(i);

	for (offset = first_item_off, i = 0; i < item_cnt; offset += item_lengths[i], i++)
	{
		items[i].reset((uint8_t*)try_malloc(item_lengths[i]));
		memcpy(items[i].get(), _data() + offset, item_lengths[i]);
	}

	item_lengths[item_index] = item_size;
	items[item_index].reset((uint8_t*)try_malloc(item_size));
	memcpy(items[item_index].get(), new_item, item_size);

	int32_t new_size = first_item_off;
	for (int32_t i = 0; i < item_cnt; i++)
		new_size += item_lengths[i];

	m_esize = new_size;
	m_data.resize(new_size);

	uint8_t* new_data = _data();
	*(int32_t*)(new_data) = MAGIC_ENTRY;
	*(int32_t*)(new_data + 0x4) = m_eid;
	*(int32_t*)(new_data + 0x8) = int32_t(EntryType::Zone);
	*(int32_t*)(new_data + 0xC) = item_cnt;

	for (offset = first_item_off, i = 0; i < item_cnt + 1; offset += item_lengths[i], i++)
		*(int32_t*)(new_data + 0x10 + i * 4) = offset;

	for (offset = first_item_off, i = 0; i < item_cnt; offset += item_lengths[i], i++)
		memcpy(new_data + offset, items[i].get(), item_lengths[i]);
}


// alters specified item using the func_arg function, replaces within the entry
void ENTRY::entity_alter(int32_t item_index, std::vector<uint8_t>(func_arg)(uint32_t, std::vector<uint8_t>&, PROPERTY*), int32_t property_code, PROPERTY* prop)
{
	int32_t item_length = get_nth_item_offset(item_index + 1) - get_nth_item_offset(item_index);
	std::vector<uint8_t> item(item_length);
	memcpy(item.data(), _data() + get_nth_item_offset(item_index), item_length);

	item = func_arg(property_code, item, prop);
	replace_nth_item(item_index, item.data(), int32_t(item.size()));
}

uint32_t ENTRY::get_zone_track()
{
	int32_t item0off = get_nth_item_offset(0);
	int32_t item1off = get_nth_item_offset(1);
	int32_t item0len = item1off - item0off;

	uint32_t music_entry = from_u32(_data() + item0off + C2_MUSIC_REF + item0len - 0x318);
	return music_entry;
}


// Deconstructs the load or draw lists
GENERIC_LOAD_LIST ENTRY::get_generic_lists(int32_t prop_code, int32_t item_index)
{
	GENERIC_LOAD_LIST return_list{};
	int32_t cam_offset = get_nth_item_offset(item_index);
	auto entry = _data();
	int32_t prop_count = PROPERTY::count(entry + cam_offset);

	for (int32_t k = 0; k < prop_count; k++)
	{
		int32_t code = from_u16(entry + cam_offset + 0x10 + 8 * k);
		int32_t offset = from_u16(entry + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
		int32_t list_count = from_u16(entry + cam_offset + 0x16 + 8 * k);

		int32_t next_offset = (k + 1 < prop_count)
			? (from_u16(entry + cam_offset + 0x12 + 8 * (k + 1)) + OFFSET + cam_offset)
			: (from_u16(entry + cam_offset) + cam_offset);
		int32_t prop_length = next_offset - offset;

		if (code == prop_code || code == prop_code + 1)
		{
			int32_t sub_list_offset;
			int32_t load_list_item_count;
			bool condensed_check1 = false;
			bool condensed_check2 = false;

			// i should use 0x40 flag in prop header[4] but my spaghetti doesnt work with it so this thing stays here
			int32_t potentially_condensed_length = from_u16(entry + offset) * 4 * list_count;
			potentially_condensed_length += 2 + 2 * list_count;
			if (from_u16(entry + offset + 2 + 2 * list_count) == 0)
				potentially_condensed_length += 2;
			if (potentially_condensed_length == prop_length && list_count > 1)
				condensed_check1 = true;

			int32_t len = 4 * list_count;
			for (int32_t l = 0; l < list_count; l++)
				len += from_u16(entry + offset + l * 2) * 4;
			if (len != prop_length)
				condensed_check2 = true;

			if (condensed_check1 && condensed_check2)
			{
				load_list_item_count = from_u16(entry + offset);
				std::vector<uint16_t> indices(list_count);
				memcpy(indices.data(), entry + offset + 2, list_count * 2);
				sub_list_offset = offset + 2 + 2 * list_count;
				if (sub_list_offset % 4)
					sub_list_offset += 2;
				for (int32_t l = 0; l < list_count; l++)
				{
					LOAD new_list{};
					new_list.type = (code == prop_code) ? 'A' : 'B';
					for (int32_t m = 0; m < load_list_item_count; m++)
						new_list.list.add(from_u32(entry + sub_list_offset + m * 4));
					new_list.index = indices[l];
					return_list.push_back(new_list);
					sub_list_offset += load_list_item_count * 4;
				}
			}
			else
			{
				sub_list_offset = offset + 4 * list_count;
				;
				for (int32_t l = 0; l < list_count; l++)
				{
					load_list_item_count = from_u16(entry + offset + l * 2);
					int32_t index = from_u16(entry + offset + l * 2 + list_count * 2);

					LOAD new_list{};
					new_list.type = (code == prop_code) ? 'A' : 'B';
					new_list.index = index;
					for (int32_t m = 0; m < load_list_item_count; m++)
						new_list.list.add(from_u32(entry + sub_list_offset + m * 4));
					return_list.push_back(new_list);
					sub_list_offset += load_list_item_count * 4;
				}
			}
		}
	}

	return return_list;
}

LOAD_LIST ENTRY::get_load_lists(int32_t item_index)
{
	auto ll_sort = [](LOAD x, LOAD y)
		{
			if (x.index != y.index)
				return (x.index < y.index);
			else
				return (x.type < y.type);
		};

	LOAD_LIST ll = get_generic_lists(ENTITY_PROP_CAM_LOAD_LIST_A, item_index);
	std::sort(ll.begin(), ll.end(), ll_sort);
	return ll;
}

DRAW_LIST ENTRY::get_draw_lists(int32_t item_index)
{
	auto dl_sort = [](LOAD x, LOAD y)
		{
			if (x.index != y.index)
				return (x.index < y.index);
			else
				return (y.type > x.type);
		};

	DRAW_LIST dl = get_generic_lists(ENTITY_PROP_CAM_DRAW_LIST_A, item_index);
	std::sort(dl.begin(), dl.end(), dl_sort);
	return dl;
}

// Checks for whether the current camera isnt trying to load more than 8 textures
void ENTRY::texture_count_check(ELIST& elist, std::vector<LIST>& full_load, int32_t cam_idx)
{
	int32_t max_count = 0;
	uint32_t max_textures[20];

	for (auto& sublist : full_load)
	{
		int32_t texture_count = 0;
		uint32_t textures[20]{};
		for (int32_t l = 0; l < sublist.count(); l++)
		{
			int32_t idx = elist.get_index(sublist[l]);
			if (idx == -1)
				printf("Trying to load invalid entry %s\n", eid2str(sublist[l]));
			else if (elist[idx].m_is_tpage)
				textures[texture_count++] = sublist[l];
		}

		if (texture_count > max_count)
		{
			max_count = texture_count;
			memcpy(max_textures, textures, texture_count * sizeof(uint32_t));
		}
	}

	if (max_count > 8)
	{
		printf("[warning] Zone %s cam path %d trying to load %d textures! \n", m_ename, cam_idx, max_count);

		for (int32_t k = 0; k < max_count; k++)
			printf("\t%s", eid2str(max_textures[k]));
		printf("\n");
	}
}

// gets draw list generation override multiplier to be used immediately
int32_t ENTRY::get_nth_entity_draw_override_mult(int32_t entity_idx)
{
	int32_t draw_mult = PROPERTY::get_value(get_nth_entity(entity_idx), ENTITY_PROP_OVERRIDE_DRAW_MULT);
	if (draw_mult == -1)
		return 100;
	else
		return draw_mult >> 8;
}

// gets index of entity to be drawn, taking draw list generation override index into consideration
std::tuple<bool, int32_t> ENTRY::get_nth_entity_draw_override_ent_idx(int32_t entity_idx)
{
	int32_t pos_override_id = PROPERTY::get_value(get_nth_entity(entity_idx), ENTITY_PROP_OVERRIDE_DRAW_ID);
	if (pos_override_id == -1)
		return { false, -1 };

	pos_override_id = pos_override_id / 0x100;
	for (int32_t o = 0; o < get_entity_count(); o++)
	{
		int32_t ent_id2 = PROPERTY::get_value(get_nth_entity(o), ENTITY_PROP_ID);
		if (ent_id2 == pos_override_id)
		{
			return { true, o };
		}
	}

	return { true, -1 };
}


// reads draw lists of the camera of the zone, returns in a non-delta form.
std::vector<LIST> ENTRY::get_expanded_draw_list(int32_t cam_index)
{
	int32_t cam_length = get_ent_path_len(cam_index);

	std::vector<LIST> draw_list_full(cam_length);
	DRAW_LIST draw_list_delta = get_draw_lists(cam_index);

	LIST list = {};
	int32_t sublist_index = 0;
	for (size_t i = 0; i < size_t(cam_length) && sublist_index < draw_list_delta.size(); i++)
	{
		bool new_read = false;
		auto& sublist = draw_list_delta[sublist_index];
		if (sublist.index == i)
		{
			if (draw_list_delta[sublist_index].type == 'B')
			{
				for (auto& draw_uint : sublist.list)
				{
					DRAW_ITEM draw_item(draw_uint);
					list.add(draw_item.ID);
				}
			}

			if (draw_list_delta[sublist_index].type == 'A')
			{
				for (auto& draw_uint : sublist.list)
				{
					DRAW_ITEM draw_item(draw_uint);
					list.remove(draw_item.ID);
				}
			}

			sublist_index++;
			new_read = true;
		}
		draw_list_full[i].copy_in(list);

		// fixes case when both draw list A and B containsn sublist w/ the same index
		if (new_read)
			i--;
	}

	return draw_list_full;
}