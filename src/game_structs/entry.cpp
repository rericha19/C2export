
#include "../include.h"
#include "entry.hpp"
#include <pthread.h>

// gets zone neighbours
LIST ENTRY_2::get_neighbours(uint8_t* entry)
{
	// todo check type
	int32_t item1off = build_get_nth_item_offset(entry, 0);
	int32_t count = entry[item1off + C2_NEIGHBOURS_START];
	LIST neighbours{};

	for (int32_t k = 0; k < count; k++)
	{
		int32_t neighbour_eid = from_u32(entry + item1off + C2_NEIGHBOURS_START + 4 + 4 * k);
		neighbours.add(neighbour_eid);
	}

	return neighbours;
}

// gets models used by an animation
LIST ENTRY_2::get_models(uint8_t* animation)
{
	LIST models{};
	// todo check type

	int32_t item_count = build_item_count(animation);
	for (int32_t i = 0; i < item_count; i++)
		models.add(build_get_model(animation, i));

	return models;
}

// returns list of camera links of the specified camera path 
LIST ENTRY_2::get_links(uint8_t* entry, int32_t item_idx)
{
	// todo check type / cam count

	int32_t link_count = 0;
	int32_t* links = NULL;
	int32_t cam_offset = build_get_nth_item_offset(entry, item_idx);

	for (int32_t k = 0; k < build_prop_count(entry + cam_offset); k++)
	{
		int32_t code = from_u16(entry + cam_offset + 0x10 + 8 * k);
		int32_t offset = from_u16(entry + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
		int32_t prop_len = from_u16(entry + cam_offset + 0x16 + 8 * k);

		if (code == ENTITY_PROP_CAM_PATH_LINKS)
		{
			if (prop_len == 1)
			{
				link_count = from_u16(entry + offset);
				links = (int32_t*)try_malloc(link_count * sizeof(int32_t));
				for (int32_t l = 0; l < link_count; l++)
					links[l] = from_u32(entry + offset + 4 + 4 * l);
			}
			else
			{
				link_count = max(1, from_u16(entry + offset)) + max(1, from_u16(entry + offset + 2));
				links = (int32_t*)try_malloc(link_count * sizeof(int32_t));
				for (int32_t l = 0; l < link_count; l++)
					links[l] = from_u32(entry + offset + 0x8 + 4 * l);
			}
		}
	}

	LIST links_list{};
	for (int32_t i = 0; i < link_count; i++)
		links_list.add(links[i]);

	return links_list;
}

#if COMPILE_WITH_THREADS
pthread_mutex_t g_eidconv_mutex2 = PTHREAD_MUTEX_INITIALIZER;
#endif

// converts uint32_t eid to string eid
std::string ENTRY_2::eid_to_str(uint32_t value)
{
#if COMPILE_WITH_THREADS
	pthread_mutex_lock(&g_eidconv_mutex2);
#endif

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


#if COMPILE_WITH_THREADS
	pthread_mutex_unlock(&g_eidconv_mutex2);
#endif
	return s_buf;
}

// Gets list of special entries in the zone's first item. For more info see function below.
LIST ENTRY_2::get_special_entries(uint8_t* zone)
{
	LIST special_entries{};
	int32_t item1off = from_u32(zone + 0x10);
	uint8_t* metadata_ptr = zone + item1off + C2_SPECIAL_METADATA_OFFSET;
	int32_t special_entry_count = from_u32(metadata_ptr) & SPECIAL_METADATA_MASK_LLCOUNT;

	for (int32_t i = 1; i <= special_entry_count; i++)
	{
		uint32_t entry = from_u32(metadata_ptr + i * 4);
		// *(uint32_t *)(metadata_ptr + i * 4) = 0;
		special_entries.add(entry);
	}

	//*(uint32_t *)metadata_ptr = 0;
	return special_entries;
}

void ELIST::sort_by_eid()
{
	// cmp_func_eid
	std::sort(begin(), end(), [](ENTRY& a, ENTRY& b)
		{
			return a.eid < b.eid;
		});
}

// For each gool entry it searches related animations and adds the model entries
// referenced by the animations to the gool entry's relatives.
void ELIST::update_gool_references()
{
	for (auto& ntry : *this)
	{		
		if (build_entry_type(ntry) != ENTRY_TYPE_GOOL)
			continue;

		LIST new_relatives{};

		for (auto& existing_relative : ntry.related)
		{
			int32_t relative_index = get_index(existing_relative);
			if (relative_index == -1)
				continue;

			if (build_entry_type(at(relative_index)) == ENTRY_TYPE_ANIM)								
				new_relatives.copy_in(ENTRY_2::get_models(at(relative_index)._data()));
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
