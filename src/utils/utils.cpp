
#include "../include.h"
#include "utils.hpp"
#include "entry.hpp"

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

// Lets the user pick the spawn that's to be used as the primary spawn.
bool SPAWNS::ask_spawn()
{
	if (!size())
		return false;

	sort_spawns();
	int32_t input = 0;

	// lets u pick a spawn point
	printf("Pick a spawn:\n");
	for (uint32_t i = 0; i < size(); i++)
		printf("Spawn %2d:\tZone: %s\n", i + 1, eid2str(at(i).zone));

	scanf("%d", &input);
	input--;
	if (input >= size() || input < 0)
	{
		printf("No such spawn, defaulting to first one\n");
		input = 0;
	}

	swap_spawns(0, input);
	printf("Using spawn %2d: Zone: %s\n", input + 1, eid2str(at(0).zone));
	return true;
}

// Deconstructs the load or draw lists and saves into a convenient struct.
void get_generic_lists(GENERIC_LOAD_LIST& load_list, int32_t prop_code, ENTRY& ntry, int32_t cam_index)
{
	int32_t cam_offset = ntry.get_nth_item_offset(cam_index);
	auto entry = ntry._data();
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
					load_list.push_back(new_list);
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
					load_list.push_back(new_list);
					sub_list_offset += load_list_item_count * 4;
				}
			}
		}
	}
}

LOAD_LIST get_load_lists(ENTRY& entry, int32_t cam_index)
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

DRAW_LIST get_draw_lists(ENTRY& entry, int32_t cam_index)
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

int32_t PROPERTY::count(uint8_t* item)
{
	return from_u32(item + 0xC);
}

PROPERTY PROPERTY::get_full(uint8_t* item, uint32_t prop_code)
{
	PROPERTY prop{};
	int32_t property_count = PROPERTY::count(item);
	uint8_t property_header[8];

	for (int32_t i = 0; i < property_count; i++)
	{
		memcpy(property_header, item + 0x10 + 8 * i, 8);
		if (from_u16(property_header) == prop_code)
		{
			memcpy(prop.header, property_header, 8);
			int32_t next_offset = 0;
			if (i == property_count - 1)
				next_offset = *(uint16_t*)(item);
			else
				next_offset = *(uint16_t*)(item + 0x12 + (i * 8) + 8) + OFFSET;
			int32_t curr_offset = *(uint16_t*)(item + 0x12 + 8 * i) + OFFSET;
			prop.length = next_offset - curr_offset;
			prop.data.resize(prop.length);
			memcpy(prop.data.data(), item + curr_offset, prop.length);
			break;
		}
	}

	return prop;
}

int32_t PROPERTY::get_value(uint8_t* entity, uint32_t prop_code)
{
	int32_t offset = get_offset(entity, prop_code);
	if (offset == 0)
		return -1;

	return from_u32(entity + offset + 4);
}

int32_t PROPERTY::get_offset(uint8_t* item, uint32_t prop_code)
{
	int32_t offset = 0;
	int32_t prop_count = PROPERTY::count(item);
	for (int32_t i = 0; i < prop_count; i++)
		if ((from_u16(item + 0x10 + 8 * i)) == prop_code)
			offset = OFFSET + from_u16(item + 0x10 + 8 * i + 2);

	return offset;
}

// Makes a load/draw list property from the input arrays
PROPERTY PROPERTY::make_list_prop(std::vector<LIST>& list_array, int32_t code)
{
	int32_t cam_length = int32_t(list_array.size());
	int32_t delta_counter = 0;
	int32_t total_length = 0;
	PROPERTY prop;

	// count total length and individual deltas
	for (int32_t i = 0; i < cam_length; i++)
	{
		if (list_array[i].count() == 0)
			continue;

		total_length += list_array[i].count() * 4; // space individual load list items of
		// current sublist will take up
		total_length += 4; // each load list sublist uses 2 bytes for its length
		// and 2 bytes for its index
		delta_counter++;
	}

	// header info
	*(int16_t*)(prop.header) = code;
	if (code == ENTITY_PROP_CAM_LOAD_LIST_A || code == ENTITY_PROP_CAM_LOAD_LIST_B)
	{
		*(int16_t*)(prop.header + 4) = (delta_counter > 1) ? 0x0464 : 0x424;
	}

	if (code == ENTITY_PROP_CAM_DRAW_LIST_A || code == ENTITY_PROP_CAM_DRAW_LIST_B)
	{
		*(int16_t*)(prop.header + 4) = (delta_counter > 1) ? 0x0473 : 0x0433;
	}
	*(int16_t*)(prop.header + 6) = delta_counter;

	prop.length = total_length;
	prop.data.resize(total_length);

	int32_t indexer = 0;
	int32_t offset = 4 * delta_counter;
	for (int32_t i = 0; i < cam_length; i++)
	{
		if (list_array[i].count() == 0)
			continue;

		*(int16_t*)(prop.data.data() + 2 * indexer) = list_array[i].count(); // i-th sublist's length (item count)
		*(int16_t*)(prop.data.data() + 2 * (indexer + delta_counter)) = i;   // i-th sublist's index
		for (int32_t j = 0; j < list_array[i].count(); j++)
			*(int32_t*)(prop.data.data() + offset + 4 * j) = list_array[i][j]; // individual items
		offset += list_array[i].count() * 4;
		indexer++;
	}

	return prop;
}

