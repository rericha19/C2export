
#include "../include.h"
#include "entry.hpp"

// entry

// gets zone neighbours
LIST ENTRY::get_neighbours()
{
	LIST neighbours{};
	if (entry_type() != ENTRY_TYPE_ZONE)
		return neighbours;

	auto item0_ptr = get_nth_item(0);
	for (int32_t k = 0; k < neighbour_count(); k++)
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

	LIST models{};
	if (entry_type() != ENTRY_TYPE_ANIM)
		return models;

	for (int32_t i = 0; i < item_count(); i++)
		models.add(get_model_single(_data(), i));

	return models;
}

// returns list of camera links of the specified camera path 
LIST ENTRY::get_links(int32_t item_idx)
{
	LIST links{};
	if (entry_type() != ENTRY_TYPE_ZONE)
		return links;

	auto entry = _data();
	int32_t cam_offset = get_nth_item_offset(item_idx);
	for (int32_t k = 0; k < build_prop_count(entry + cam_offset); k++)
	{
		int32_t code = from_u16(entry + cam_offset + 0x10 + 8 * k);
		int32_t offset = from_u16(entry + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
		int32_t prop_len = from_u16(entry + cam_offset + 0x16 + 8 * k);

		if (code == ENTITY_PROP_CAM_PATH_LINKS)
		{
			if (prop_len == 1)
			{
				int32_t link_count = from_u16(entry + offset);
				links.resize(link_count);
				for (int32_t l = 0; l < link_count; l++)
					links[l] = from_u32(entry + offset + 4 + 4 * l);
			}
			else
			{
				int32_t link_count = max(1, from_u16(entry + offset)) + max(1, from_u16(entry + offset + 2));
				links.resize(link_count);
				for (int32_t l = 0; l < link_count; l++)
					links[l] = from_u32(entry + offset + 0x8 + 4 * l);
			}
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
	LIST special_entries{};
	if (entry_type() != ENTRY_TYPE_ZONE)
		return special_entries;

	auto item0_ptr = get_nth_item(0);
	const uint8_t* metadata_ptr = item0_ptr + C2_SPECIAL_METADATA_OFFSET;
	int32_t special_entry_count = from_u32(metadata_ptr) & SPECIAL_METADATA_MASK_LLCOUNT;

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
		int32_t item = iteration_clone[i];
		int32_t index = elist.get_index(item);
		if (index == -1)
		{
			printf("[error] Zone %s special entry list contains entry %s which is not present.\n",
				ename, eid2str(item));
			special_entries.remove(item);
			continue;
		}

		if (elist[index].entry_type() != ENTRY_TYPE_ANIM)
			continue;

		for (auto& model : elist[index].get_models())
		{
			int32_t model_index = elist.get_index(model);
			if (model_index == -1 || elist[model_index].entry_type() != ENTRY_TYPE_MODEL)
			{
				printf("[ERROR] Zone %s special entry list contains animation %s that uses model %s that is not present or is not a model\n",
					ename, eid2str(item), eid2str(model));
				continue;
			}

			special_entries.add(model);
			build_add_model_textures_to_list(elist[model_index]._data(), &special_entries);
		}
	}

	return special_entries;
}


LIST ENTRY::get_sceneries()
{
	int32_t scen_count = scenery_count();
	int32_t item1off = get_nth_item_offset(0);
	LIST list{};
	for (int32_t i = 0; i < scen_count; i++)
	{
		uint32_t scen = from_u32(_data() + item1off + 0x4 + 0x30 * i);
		list.add(scen);
	}

	return list;
}

// collects list of IDs using draw lists of neighbouring camera paths
LIST ENTRY::get_entities_to_load(ELIST& elist, int32_t* config, LIST& neighbours, int32_t camera_index, int32_t point_index)
{	
	LIST entity_list{};

	int32_t draw_dist = config[CNFG_IDX_LL_DRAW_DIST_VALUE];
	int32_t preloading_flag = config[CNFG_IDX_LL_TRNS_PRLD_FLAG];
	int32_t backwards_penalty = config[CNFG_IDX_LL_BACKWARDS_PENALTY];

	int32_t coord_count;
	int16_t* coords = build_get_path(elist, elist.get_index(eid), camera_index, &coord_count);
	if (!coords)
		return entity_list;

	neighbours.copy_in(get_neighbours());

	std::vector<LIST> draw_list_zone = build_get_complete_draw_list(elist, *this, camera_index, coord_count);
	for (int32_t i = 0; i < coord_count; i++)
		entity_list.copy_in(draw_list_zone[i]);

	LIST links = get_links(camera_index);
	for (int32_t i = 0; i < links.count(); i++)
	{
		int32_t distance = 0;
		CAMERA_LINK link(links[i]);
		if (link.type == 1)
			distance += build_get_distance(coords, point_index, 0, -1, NULL);
		if (link.type == 2)
			distance += build_get_distance(coords, point_index, coord_count - 1, -1, NULL);

		uint32_t eid_offset = from_u32(_data() + 0x10) + 4 + link.zone_index * 4 + C2_NEIGHBOURS_START;
		uint32_t neighbour_eid = from_u32(_data() + eid_offset);
		uint32_t neighbour_flg = from_u32(_data() + eid_offset + 0x20);

		int32_t neigh_idx = elist.get_index(neighbour_eid);
		if (neigh_idx == -1)
			continue;

		// preloading everything check
		if (preloading_flag != PRELOADING_ALL && (neighbour_flg == 0xF || neighbour_flg == 0x1F))
			continue;

		ENTRY& neigh1 = elist[neigh_idx];
		neighbours.copy_in(neigh1.get_neighbours());

		int32_t neighbour_cam_length;
		int16_t* coords2 = build_get_path(elist, neigh_idx, 2 + 3 * link.cam_index, &neighbour_cam_length);
		if (!coords2)
			continue;

		std::vector<LIST> draw_list_neighbour1 = build_get_complete_draw_list(elist, elist[neigh_idx], 2 + 3 * link.cam_index, neighbour_cam_length);

		int32_t draw_dist_w_orientation = draw_dist;
		if (build_is_before(elist, elist.get_index(eid), camera_index / 3, neigh_idx, link.cam_index))
		{
			draw_dist_w_orientation = build_dist_w_penalty(draw_dist, backwards_penalty);
		}

		int32_t point_index2;
		if (link.flag == 1)
		{
			distance += build_get_distance(coords2, 0, neighbour_cam_length - 1, draw_dist_w_orientation - distance, &point_index2);

			for (int32_t j = 0; j < point_index2; j++)
				entity_list.copy_in(draw_list_neighbour1[j]);
		}
		if (link.flag == 2)
		{
			distance += build_get_distance(coords2, neighbour_cam_length - 1, 0, draw_dist_w_orientation - distance, &point_index2);
			for (int32_t j = point_index2; j < neighbour_cam_length - 1; j++)
				entity_list.copy_in(draw_list_neighbour1[j]);
		}

		if (distance >= draw_dist_w_orientation)
			continue;

		LIST layer2 = elist[neigh_idx].get_links(2 + 3 * link.cam_index);
		for (int32_t j = 0; j < layer2.count(); j++)
		{
			CAMERA_LINK link2(layer2[j]);
			uint32_t eid_offset2 = from_u32(elist[neigh_idx]._data() + 0x10) + 4 + link2.zone_index * 4 + C2_NEIGHBOURS_START;
			uint32_t neighbour_eid2 = from_u32(elist[neigh_idx]._data() + eid_offset2);
			uint32_t neighbour_flg2 = from_u32(elist[neigh_idx]._data() + eid_offset2 + 0x20);

			int32_t neigh_idx2 = elist.get_index(neighbour_eid2);
			if (neigh_idx2 == -1)
				continue;

			// preloading everything check
			if (preloading_flag != PRELOADING_ALL && (neighbour_flg2 == 0xF || neighbour_flg2 == 0x1F))
				continue;

			ENTRY& neigh2 = elist[neigh_idx2];
			neighbours.copy_in(neigh2.get_neighbours());

			int32_t neighbour_cam_length2;
			int16_t* coords3 = build_get_path(elist, neigh_idx2, 2 + 3 * link2.cam_index, &neighbour_cam_length2);
			if (!coords3)
				continue;

			std::vector<LIST> draw_list_neighbour2 = build_get_complete_draw_list(elist, elist[neigh_idx2], 2 + 3 * link2.cam_index, neighbour_cam_length2);

			int32_t point_index3;
			draw_dist_w_orientation = draw_dist;
			if (build_is_before(elist, elist.get_index(eid), camera_index / 3, neigh_idx2, link2.cam_index))
			{
				draw_dist_w_orientation = build_dist_w_penalty(draw_dist, backwards_penalty);
			}

			// start to end
			if ((link.type == 2 && link2.type == 2 && link2.flag == 1) || (link.type == 1 && link2.type == 1 && link2.flag == 1))
			{
				build_get_distance(coords3, 0, neighbour_cam_length2 - 1, draw_dist_w_orientation - distance, &point_index3);
				for (int32_t k = 0; k < point_index3; k++)
					entity_list.copy_in(draw_list_neighbour2[k]);
			}

			// end to start
			if ((link.type == 2 && link2.type == 2 && link2.flag == 1) || (link.type == 1 && link2.type == 1 && link2.flag == 2))
			{
				build_get_distance(coords3, neighbour_cam_length2 - 1, 0, draw_dist_w_orientation - distance, &point_index3);
				for (int32_t k = point_index3; k < neighbour_cam_length2; k++)
					entity_list.copy_in(draw_list_neighbour2[k]);
			}
		}
	}

	return entity_list;
}

int32_t ENTRY::get_nth_item_offset(uint8_t* entry, int32_t n)
{	
	return from_u32(entry + 0x10 + 4 * n);
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
	return get_nth_item(2 + cam_item_count() + entity_n);
}

uint8_t* ENTRY::get_nth_main_cam(int32_t cam_n)
{
	return get_nth_item(2 + 3 * cam_n);
}

int32_t ENTRY::entry_type()
{
	if (data.empty())
		return -1;
	return *(int32_t*)(_data() + 8);
}

bool ENTRY::is_normal_chunk_entry()
{
	int32_t type = entry_type();
	return (
		type == ENTRY_TYPE_ANIM ||
		type == ENTRY_TYPE_DEMO ||
		type == ENTRY_TYPE_ZONE ||
		type == ENTRY_TYPE_MODEL ||
		type == ENTRY_TYPE_SCENERY ||
		type == ENTRY_TYPE_SLST ||
		type == ENTRY_TYPE_DEMO ||
		type == ENTRY_TYPE_VCOL ||
		type == ENTRY_TYPE_MIDI ||
		type == ENTRY_TYPE_GOOL ||
		type == ENTRY_TYPE_T21);
}

int32_t ENTRY::item_count(uint8_t* entry)
{
	return from_u32(entry + 0xC);
}

int32_t ENTRY::item_count()
{
	return item_count(_data());
}

int32_t ENTRY::entity_count(uint8_t* entry)
{
	uint8_t* item0 = ENTRY::get_nth_item(entry, 0);
	return from_u32(item0 + C2_ENTITY_COUNT_OFF);
}

int32_t ENTRY::entity_count()
{
	return entity_count(_data());
}

int32_t ENTRY::cam_item_count(uint8_t* entry)
{
	uint8_t* item0 = ENTRY::get_nth_item(entry, 0);
	return from_u32(item0 + C2_CAMERA_COUNT_OFF);
}

int32_t ENTRY::cam_item_count()
{
	return cam_item_count(_data());
}

int32_t ENTRY::neighbour_count(uint8_t* entry)
{
	uint8_t* item0 = ENTRY::get_nth_item(entry, 0);
	return from_u32(item0 + C2_NEIGHBOURS_START);
}

int32_t ENTRY::neighbour_count()
{
	return neighbour_count(_data());
}

int32_t ENTRY::scenery_count(uint8_t* entry)
{
	uint8_t* item0 = ENTRY::get_nth_item(entry, 0);
	return from_u32(item0 + C2_SCEN_OFFSET);
}

int32_t ENTRY::scenery_count()
{
	return scenery_count(_data());
}

bool ENTRY::check_item_count()
{
	int32_t item_cnt = item_count();
	int32_t cam_cnt = cam_item_count();
	int32_t entity_cnt = entity_count();

	if (item_cnt != (2 + cam_cnt + entity_cnt))
	{
		printf("[ERROR] %s's item count (%d) doesn't match item counts in the first item (2 + %d + %d)\n",
			ename, item_cnt, cam_cnt, entity_cnt);
		return false;
	}
	return true;
}

void ENTRY::remove_nth_item(int32_t n)
{
	int32_t item_c = item_count();
	if (item_c < n)
	{
		printf("[Warning] Trying to remove item %d from entry %s (has %d items)!\n", n, ename, item_c);
		return;
	}

	int32_t i, offset;
	int32_t first_item_offset = 0x14 + 4 * item_c;

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
			next_start = esize;

		item_lengths[i] = next_start - get_nth_item_offset(i);
	}

	for (offset = first_item_offset, i = 0; i < item_c;
		offset += item_lengths[i], i++)
	{
		items[i].reset((uint8_t*)try_malloc(item_lengths[i]));
		memcpy(items[i].get(), _data() + offset, item_lengths[i]);
	}

	item_lengths[n] = 0;
	first_item_offset -= 4;

	int32_t new_size = first_item_offset;
	for (int32_t i = 0; i < item_c; i++)
		new_size += item_lengths[i];

	uint8_t* new_data = (uint8_t*)try_malloc(new_size);
	*(int32_t*)(new_data) = MAGIC_ENTRY;
	*(int32_t*)(new_data + 0x4) = eid;
	*(int32_t*)(new_data + 0x8) = *(int32_t*)(_data() + 8);
	*(int32_t*)(new_data + 0xC) = item_c - 1;

	for (offset = first_item_offset, i = 0; i < item_c + 1;
		offset += item_lengths[i], i++)
	{
		if (i == n)
			continue;

		int32_t curr = i;
		if (i > n)
			curr--;
		*(int32_t*)(new_data + 0x10 + curr * 4) = offset;
	}

	for (offset = first_item_offset, i = 0; i < item_c;
		offset += item_lengths[i], i++)
	{
		if (i == n)
			continue;
		memcpy(new_data + offset, items[i].get(), item_lengths[i]);
	}

	data.resize(new_size);
	memcpy(_data(), new_data, new_size);
	esize = new_size;
}

uint32_t ENTRY::get_zone_track()
{
	int32_t item0off = get_nth_item_offset(0);
	int32_t item1off = get_nth_item_offset(1);
	int32_t item0len = item1off - item0off;

	uint32_t music_entry = from_u32(_data() + item0off + C2_MUSIC_REF + item0len - 0x318);
	return music_entry;
}


// entry end
// -------------------------------------------------------------
// elist

void ELIST::sort_by_eid()
{
	// cmp_func_eid
	std::sort(begin(), end(), [](ENTRY& a, ENTRY& b)
		{
			return a.eid < b.eid;
		});
}

void ELIST::sort_by_esize()
{
	// cmp_func_esize
	std::sort(begin(), end(), [](ENTRY& a, ENTRY& b)
		{
			return b.esize < a.esize;
		});
}

// For each gool entry it searches related animations and adds the model entries
// referenced by the animations to the gool entry's relatives.
void ELIST::update_gool_references()
{
	for (auto& ntry : *this)
	{
		if (ntry.entry_type() != ENTRY_TYPE_GOOL)
			continue;

		LIST new_relatives{};
		for (auto& existing_relative : ntry.related)
		{
			int32_t relative_index = get_index(existing_relative);
			if (relative_index == -1)
				continue;

			if (at(relative_index).entry_type() == ENTRY_TYPE_ANIM)
				new_relatives.copy_in(at(relative_index).get_models());
		}

		if (new_relatives.size())
			ntry.related.copy_in(new_relatives);
	}
}

// Searches the entry list looking for the specified eid.
// Binary search, entry list should be sorted by eid (ascending).
int32_t ELIST::get_index(uint32_t eid)
{
	int32_t first = 0;
	int32_t last = int32_t(size() - 1);
	int32_t middle = (first + last) / 2;

	while (first <= last)
	{
		if (at(middle).eid < eid)
			first = middle + 1;
		else if (at(middle).eid == eid)
			return middle;
		else
			last = middle - 1;

		middle = (first + last) / 2;
	}

	return -1;
}

int32_t ELIST::count()
{
	return int32_t(size());
}

// get a list of all normal chunk entries in elist
LIST ELIST::get_normal_entries()
{
	LIST entries{};
	for (auto& ntry : *this)
	{
		if (ntry.is_normal_chunk_entry())
			entries.add(ntry.eid);
	}
	return entries;
}

int32_t ELIST::assign_primary_chunks_all(int32_t chunk_count)
{
	for (auto& curr : *this)
	{
		if (curr.is_normal_chunk_entry() && curr.chunk == -1 && curr.norm_chunk_ent_is_loaded)
		{
			curr.chunk = chunk_count++;
		}
	}
	return chunk_count;
}

int32_t ELIST::remove_empty_chunks(int32_t chunk_min, int32_t chunk_count)
{
	int32_t empty_chunk = 0;

	while (1)
	{
		for (int32_t i = chunk_min; i < chunk_count; i++)
		{
			bool is_occupied = false;
			empty_chunk = -1;
			for (auto& ntry : *this)
			{
				if (ntry.chunk == i)
					is_occupied = true;
			}

			if (!is_occupied)
			{
				empty_chunk = i;
				break;
			}
		}

		if (empty_chunk == -1)
			break;

		for (auto& ntry : *this)
		{
			if (ntry.chunk == chunk_count - 1)
				ntry.chunk = empty_chunk;
		}

		chunk_count--;
	}

	return chunk_count;
}

// Specially merges permaloaded entries as they dont require any association.
// Works similarly to the sound chunk one.
// Biggest first sort packing algorithm kinda thing, seems to work pretty ok.
int32_t ELIST::merge_permaloaded(int32_t chunk_border_sounds, int32_t chunk_count, LIST& permaloaded)
{
	int32_t entry_count = count();
	int32_t start_chunk_count = chunk_count;

	ELIST perma_elist{};
	for (auto& ntry : *this)
	{
		if (permaloaded.find(ntry.eid) == -1)
			continue;
		if (!ntry.is_normal_chunk_entry())
			continue;

		perma_elist.push_back(ntry);
	}

	perma_elist.sort_by_esize();

	// keep putting them into existing chunks if they fit
	int32_t perma_normal_entry_count = perma_elist.count();
	int32_t perma_chunk_count = perma_elist.count();
	int32_t* sizes = (int32_t*)try_malloc(perma_chunk_count * sizeof(int32_t)); // freed here

	for (int32_t i = 0; i < perma_chunk_count; i++)
		sizes[i] = 0x14;

	// place where fits
	for (int32_t i = 0; i < perma_normal_entry_count; i++)
		for (int32_t j = 0; j < perma_chunk_count; j++)
			if (sizes[j] + 4 + perma_elist[i].esize <= CHUNKSIZE)
			{
				perma_elist[i].chunk = start_chunk_count + j;
				sizes[j] += 4 + perma_elist[i].esize;
				break;
			}
	// to edit the array of entries that is actually used
	for (int32_t i = 0; i < entry_count; i++)
		for (int32_t j = 0; j < perma_normal_entry_count; j++)
			if (at(i).eid == perma_elist[j].eid)
				at(i).chunk = perma_elist[j].chunk;

	// counts used chunks
	int32_t counter = 0;
	for (int32_t i = 0; i < perma_chunk_count; i++)
		if (sizes[i] > 0x14)
			counter = i + 1;

	free(sizes);
	return start_chunk_count + counter;
}

void ELIST::print_transitions()
{
	printf("\nTransitions in the level: \n");

	for (auto& ntry : *this)
	{
		if (ntry.entry_type() != ENTRY_TYPE_ZONE)
			continue;

		LIST neighbours = ntry.get_neighbours();
		int32_t item1off = ntry.get_nth_item_offset(0);
		for (int32_t j = 0; j < neighbours.count(); j++)
		{
			uint32_t neighbour_eid = from_u32(ntry._data() + item1off + C2_NEIGHBOURS_START + 4 + 4 * j);
			uint32_t neighbour_flg = from_u32(ntry._data() + item1off + C2_NEIGHBOURS_START + 4 + 4 * j + 0x20);

			if (neighbour_flg == 0xF || neighbour_flg == 0x1F)
			{
				printf("Zone %s transition (%02x) to zone %s (neighbour %d)\n", ntry.ename, neighbour_flg, eid2str(neighbour_eid), j);
			}
		}
	}
}

// Sorts load lists according to the NSD entry table order. I think.
void ELIST::sort_load_lists()
{
	// used to sort load lists
	struct LOAD_LIST_ITEM_UTIL
	{
		int32_t eid;
		int32_t index;
	};

	// sort load list items by elist index
	auto ll_item_sort = [](const LOAD_LIST_ITEM_UTIL& x, const LOAD_LIST_ITEM_UTIL& y)
		{
			return (x.index < y.index);
		};

	// for each zone entry's each camera path it searches properties, if its a load list property it reads the sublists one by one,
	// sorts the load list entries so that the order matches the nsd's entry table's order
	for (auto& ntry : *this)
	{
		if (ntry.entry_type() != ENTRY_TYPE_ZONE)
			continue;

		int32_t cam_count = ntry.cam_item_count() / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			int32_t cam_offset = ntry.get_nth_item_offset(2 + 3 * j);
			for (int32_t k = 0; k < build_prop_count(ntry._data() + cam_offset); k++)
			{
				int32_t code = from_u16(ntry._data() + cam_offset + 0x10 + 8 * k);
				int32_t offset = from_u16(ntry._data() + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
				int32_t list_count = from_u16(ntry._data() + cam_offset + 0x16 + 8 * k);
				if (code == ENTITY_PROP_CAM_LOAD_LIST_A || code == ENTITY_PROP_CAM_LOAD_LIST_B)
				{
					int32_t sub_list_offset = offset + 4 * list_count;
					for (int32_t l = 0; l < list_count; l++)
					{
						int32_t item_count = from_u16(ntry._data() + offset + l * 2);
						std::vector<LOAD_LIST_ITEM_UTIL> item_list{};
						item_list.resize(item_count);
						for (int32_t m = 0; m < item_count; m++)
						{
							item_list[m].eid = from_u32(ntry._data() + sub_list_offset + 4 * m);
							item_list[m].index = get_index(item_list[m].eid);
						}
						std::sort(item_list.begin(), item_list.end(), ll_item_sort);
						for (int32_t m = 0; m < item_count; m++)
							*(uint32_t*)(ntry._data() + sub_list_offset + 4 * m) = item_list[m].eid;
						sub_list_offset += item_count * 4;
					}
				}
			}
		}
	}
}

// checks/prints if there are any entities whose type/subtype has no dependencies.
void ELIST::check_unspecified_entities(DEPENDENCIES& sub_info)
{
	printf("\n");

	LIST considered{};
	for (auto& ntry : *this)
	{
		if (ntry.entry_type() != ENTRY_TYPE_ZONE)
			continue;

		int32_t camitem_count = ntry.cam_item_count();
		int32_t entity_count = ntry.entity_count();

		for (int32_t j = 0; j < entity_count; j++)
		{
			uint8_t* entity = ntry.get_nth_item(2 + camitem_count + j);
			int32_t type = PROPERTY::get_value(entity, ENTITY_PROP_TYPE);
			int32_t subt = PROPERTY::get_value(entity, ENTITY_PROP_SUBTYPE);
			int32_t enID = PROPERTY::get_value(entity, ENTITY_PROP_ID);
			if (type >= 64 || type < 0 || subt < 0)
			{
				printf("[warning] Zone %s entity %2d is invalid! (type %2d subtype %2d)\n", ntry.ename, j, type, subt);
				continue;
			}

			if (considered.find((type << 16) + subt) != -1)
				continue;

			considered.add((type << 16) + subt);

			bool found = false;
			for (auto& dep : sub_info)
			{
				if (dep.subtype == subt && dep.type == type)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				printf("[warning] Entity with type %2d subtype %2d has no dependency list! (e.g. Zone %s entity %2d ID %3d)\n",
					type, subt, ntry.ename, j, enID);
			}
		}

	}
}

void ELIST::check_loaded()
{
	int32_t omit = 0;
	printf("\nOmit normal chunk entries that are never loaded? [0 - include, 1 - omit]\n");
	scanf("%d", &omit);

	for (auto &ntry : *this)
		ntry.norm_chunk_ent_is_loaded = true;

	if (!omit)
	{
		printf("Not omitting\n");
		return;
	}

	printf("Checking for normal chunk entries that are never loaded\n");
	LIST ever_loaded{};
	int32_t entries_skipped = 0;

	// reads all load lists and bluntly adds all items into the list of all loaded entries
	for (auto& ntry: *this)
	{
		if (ntry.entry_type() != ENTRY_TYPE_ZONE)
			continue;

		int32_t cam_count = ntry.cam_item_count() / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			LOAD_LIST ll = get_load_lists(ntry, 2 + 3 * j);
			for (auto& sublist : ll)
				ever_loaded.copy_in(sublist.list);
		}
	}

	for (auto& ntry: *this)
	{
		if (!ntry.is_normal_chunk_entry())
			continue;

		if (ever_loaded.find(ntry.eid) == -1)
		{
			ntry.norm_chunk_ent_is_loaded = false;
			entries_skipped++;
			printf("  %3d. entry %s never loaded, will not be included\n", entries_skipped, ntry.ename);
		}
	}

	printf("Number of normal entries not included: %3d\n", entries_skipped);
}


// elist end