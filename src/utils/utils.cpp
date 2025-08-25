
#include "../include.h"
#include "utils.hpp"


int32_t LIST::count() const
{
	return static_cast<int32_t>(size());
}

void LIST::add(uint32_t eid)
{
	if (!empty() && std::find(begin(), end(), eid) != end())
		return;

	push_back(eid);
}

void LIST::remove(uint32_t eid)
{
	if (empty())
		return;

	auto it = std::find(begin(), end(), eid);
	if (it != end())		
		erase(it);
}

int32_t LIST::find(uint32_t eid) const
{
	if (empty())
		return -1;

	auto it = std::find(begin(), end(), eid);
	if (it == end())
		return -1;

	return static_cast<int32_t>(it - begin());

}

bool LIST::is_subset(const LIST& subset, const LIST& superset)
{
	for (const auto& item : subset)
	{
		if (superset.find(item) == -1)
			return false;
	}
	return true;
}

void LIST::copy_in(const LIST& other)
{
	for (auto& eid : other)
	{		
		if (std::find(begin(), end(), eid) == end())
			push_back(eid);
	}
}

void LIST::remove_all(const LIST& other)
{
	for (auto& eid : other)
	{
		auto it = std::find(begin(), end(), eid);
		if (it != end())
			erase(it);
	}
}


void SPAWNS::swap_spawns(int32_t spawnA, int32_t spawnB)
{
	std::swap(at(spawnA), at(spawnB));
}

void SPAWNS::sort_spawns()
{
	auto spsort = [](SPAWN a, SPAWN b)
		{
			return a.zone < b.zone;
		};

	std::sort(begin(), end(), spsort);
}


// Deconstructs the load or draw lists and saves into a convenient struct.
void get_generic_lists(GENERIC_LOAD_LIST& load_list, int32_t prop_code, uint8_t* entry, int32_t cam_index)
{
	int32_t cam_offset = build_get_nth_item_offset(entry, cam_index);
	int32_t prop_count = from_u32(entry + cam_offset + OFFSET);

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
				uint16_t* indices = (uint16_t*)try_malloc(list_count * sizeof(uint16_t));
				memcpy(indices, entry + offset + 2, list_count * 2);
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
					load_list.push_back(new_list);
					sub_list_offset += load_list_item_count * 4;
				}
				free(indices);
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
					load_list.push_back(new_list);
					sub_list_offset += load_list_item_count * 4;
				}
			}
		}
	}
}

LOAD_LIST get_load_lists(uint8_t* entry, int32_t cam_index)
{
	auto ll_sort = [](LOAD x, LOAD y)
		{
			if (x.index != y.index)
				return (x.index < y.index);
			else
				return (x.type < y.type);
		};

	LOAD_LIST ll{};
	get_generic_lists(ll, ENTITY_PROP_CAM_LOAD_LIST_A, entry, cam_index);
	std::sort(ll.begin(), ll.end(), ll_sort);
	return ll;
}

DRAW_LIST get_draw_lists(uint8_t* entry, int32_t cam_index)
{
	auto dl_sort = [](LOAD x, LOAD y)
		{
			if (x.index != y.index)
				return (x.index < y.index);
			else
				return (y.type > x.type);
		};

	DRAW_LIST dl{};
	get_generic_lists(dl, ENTITY_PROP_CAM_DRAW_LIST_A, entry, cam_index);
	std::sort(dl.begin(), dl.end(), dl_sort);
	return dl;
}