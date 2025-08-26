
#include "../include.h"
#include "entry.hpp"

int32_t ENTRY::get_neighbour_count()
{
	const auto item0_ptr = get_nth_item(0);
	return from_u32(item0_ptr + C2_NEIGHBOURS_START);
}

// gets zone neighbours
LIST ENTRY::get_neighbours()
{
	LIST neighbours{};
	if (entry_type() != ENTRY_TYPE_ZONE)
		return neighbours;

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
	LIST models{};
	if (entry_type() != ENTRY_TYPE_ANIM)
		return models;

	for (int32_t i = 0; i < build_item_count(_data()); i++)
		models.add(build_get_model(_data(), i));

	return models;
}

// returns list of camera links of the specified camera path 
LIST ENTRY::get_links(int32_t item_idx)
{
	LIST links{};
	if (entry_type() != ENTRY_TYPE_ZONE)
		return links;

	auto entry = data.data();
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

	auto zone = data.data();
	auto item0_ptr = get_nth_item(0);
	const uint8_t* metadata_ptr = item0_ptr + C2_SPECIAL_METADATA_OFFSET;
	int32_t special_entry_count = from_u32(metadata_ptr) & SPECIAL_METADATA_MASK_LLCOUNT;

	for (int32_t i = 1; i <= special_entry_count; i++)
		special_entries.add(from_u32(metadata_ptr + i * 4));

	return special_entries;
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

int32_t ENTRY::entry_type()
{
	if (data.empty())
		return -1;
	return *(int32_t*)(_data() + 8);
}

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
		if (build_is_normal_chunk_entry(ntry))
			entries.add(ntry.eid);
	}
	return entries;
}

