#include "elist.hpp"
#include "entry.hpp"

void ELIST::sort_by_eid()
{
	// cmp_func_eid
	std::sort(begin(), end(), [](ENTRY& a, ENTRY& b)
		{
			return a.m_eid < b.m_eid;
		});
}

void ELIST::sort_by_esize()
{
	// cmp_func_esize
	std::sort(begin(), end(), [](ENTRY& a, ENTRY& b)
		{
			return b.m_esize < a.m_esize;
		});
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
		if (at(middle).m_eid < eid)
			first = middle + 1;
		else if (at(middle).m_eid == eid)
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
			entries.add(ntry.m_eid);
	}
	return entries;
}

void ELIST::assign_primary_chunks_all()
{
	for (auto& curr : *this)
	{
		if (curr.is_normal_chunk_entry() && curr.m_chunk == -1 && curr.m_is_loaded)
		{
			curr.m_chunk = m_chunk_count++;
		}
	}
}

void ELIST::remove_empty_chunks()
{
	int32_t empty_chunk = 0;

	while (1)
	{
		for (int32_t i = m_chunk_border_sounds; i < m_chunk_count; i++)
		{
			bool is_occupied = false;
			empty_chunk = -1;
			for (auto& ntry : *this)
			{
				if (ntry.m_chunk == i)
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
			if (ntry.m_chunk == m_chunk_count - 1)
				ntry.m_chunk = empty_chunk;
		}

		m_chunk_count--;
	}
}

// Specially merges permaloaded entries as they dont require any association.
// Works similarly to the sound chunk one.
// Biggest first sort packing algorithm kinda thing, seems to work pretty ok.
void ELIST::merge_permaloaded()
{
	int32_t start_chunk_count = m_chunk_count;

	ELIST perma_elist{};
	for (auto& ntry : *this)
	{
		if (m_permaloaded.find(ntry.m_eid) == -1)
			continue;
		if (!ntry.is_normal_chunk_entry())
			continue;

		perma_elist.push_back(ntry);
	}

	perma_elist.sort_by_esize();

	// keep putting them into existing chunks if they fit
	int32_t perma_normal_entry_count = perma_elist.count();
	int32_t perma_chunk_count = perma_elist.count();

	std::vector<int32_t> sizes(perma_chunk_count, 0x14);

	// place where fits
	for (int32_t i = 0; i < perma_normal_entry_count; i++)
	{
		for (int32_t j = 0; j < perma_chunk_count; j++)
		{
			if (sizes[j] + 4 + perma_elist[i].m_esize <= CHUNKSIZE)
			{
				perma_elist[i].m_chunk = start_chunk_count + j;
				sizes[j] += 4 + perma_elist[i].m_esize;
				break;
			}
		}
	}

	// copy the changes into the main elist
	for (int32_t i = 0; i < count(); i++)
	{
		for (int32_t j = 0; j < perma_normal_entry_count; j++)
		{
			if (at(i).m_eid == perma_elist[j].m_eid)
				at(i).m_chunk = perma_elist[j].m_chunk;
		}
	}

	// counts used chunks
	int32_t counter = 0;
	for (int32_t i = 0; i < perma_chunk_count; i++)
	{
		if (sizes[i] > 0x14)
			counter = i + 1;
	}

	m_chunk_count = start_chunk_count + counter;
}

void ELIST::print_transitions()
{
	printf("\nTransitions in the level: \n");

	for (auto& ntry : *this)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		uint8_t* item0 = ntry.get_nth_item(0);
		for (int32_t j = 0; j < ntry.get_neighbour_count(); j++)
		{
			uint32_t neighbour_eid = from_u32(item0 + C2_NEIGHBOURS_START + 4 + 4 * j);
			uint32_t neighbour_flg = from_u32(item0 + C2_NEIGHBOURS_START + 4 + 4 * j + 0x20);

			if (neighbour_flg == 0xF || neighbour_flg == 0x1F)
			{
				printf("Zone %s transition (%02x) to zone %s (neighbour %d)\n", ntry.m_ename, neighbour_flg, eid2str(neighbour_eid), j);
			}
		}
	}
}

// force camera prop 0x27F ENTITY_PROP_CAM_UPDATE_SCENERY to cover all sceneries
void ELIST::patch_27f_props()
{
	int32_t cams_changed = 0;
	for (auto& ntry : *this)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		uint8_t flag = 0;
		for (int32_t i = 0; i < ntry.get_scenery_count(); i++)
			flag |= 1 << i;

		for (int32_t i = 0; i < ntry.get_cam_item_count() / 3; i++)
		{
			uint8_t* cam = ntry.get_nth_main_cam(i);
			int32_t offset = PROPERTY::get_offset(cam, ENTITY_PROP_CAM_UPDATE_SCENERY);
			if (offset == 0)
			{
				printf("[warning] %s-%d doesnt have 0x27F prop??\n", ntry.m_ename, i);
				continue;
			}

			int32_t len = from_u32(cam + offset);
			bool changed = false;
			for (int32_t j = 0; j < len; j++)
			{
				auto prev_flag = *(cam + offset + 4 + j);
				*(cam + offset + 4 + j) = flag;
				changed |= (prev_flag != flag);
			}
			if (changed)
				cams_changed++;
		}
	}
	printf("Patched 0x27F props in %d cam paths\n", cams_changed);
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
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		int32_t cam_count = ntry.get_cam_item_count() / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			int32_t cam_offset = ntry.get_nth_item_offset(2 + 3 * j);
			for (int32_t k = 0; k < PROPERTY::count(ntry._data() + cam_offset); k++)
			{
				int32_t code = from_u16(ntry._data() + cam_offset + 0x10 + 8 * k);
				if (!(code == ENTITY_PROP_CAM_LOAD_LIST_A || code == ENTITY_PROP_CAM_LOAD_LIST_B))
					continue;

				int32_t offset = from_u16(ntry._data() + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
				int32_t list_count = from_u16(ntry._data() + cam_offset + 0x16 + 8 * k);
				int32_t sub_list_offset = offset + 4 * list_count;

				for (int32_t l = 0; l < list_count; l++)
				{
					int32_t item_count = from_u16(ntry._data() + offset + l * 2);
					std::vector<LOAD_LIST_ITEM_UTIL> item_list(item_count);

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

// checks/prints if there are any entities whose type/subtype has no dependencies.
void ELIST::check_unspecified_entities()
{
	printf("\n");

	LIST considered{};
	for (auto& ntry : *this)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		for (int32_t j = 0; j < ntry.get_entity_count(); j++)
		{
			uint8_t* entity = ntry.get_nth_entity(j);
			int32_t type = PROPERTY::get_value(entity, ENTITY_PROP_TYPE);
			int32_t subt = PROPERTY::get_value(entity, ENTITY_PROP_SUBTYPE);
			int32_t enID = PROPERTY::get_value(entity, ENTITY_PROP_ID);
			if (type >= 64 || type < 0 || subt < 0)
			{
				printf("[warning] Zone %s entity %2d is invalid! (type %2d subtype %2d)\n", ntry.m_ename, j, type, subt);
				continue;
			}

			if (considered.find((type << 16) + subt) != -1)
				continue;

			considered.add((type << 16) + subt);

			bool found = false;
			for (auto& dep : m_subt_deps)
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
					type, subt, ntry.m_ename, j, enID);
			}
		}

	}
}

void ELIST::check_loaded()
{
	int32_t omit = 0;
	printf("\nOmit normal chunk entries that are never loaded? [0 - include, 1 - omit]\n");
	scanf("%d", &omit);

	for (auto& ntry : *this)
		ntry.m_is_loaded = true;

	if (!omit)
	{
		printf("Not omitting\n");
		return;
	}

	printf("Checking for normal chunk entries that are never loaded\n");
	LIST ever_loaded{};
	int32_t entries_skipped = 0;

	// reads all load lists and bluntly adds all items into the list of all loaded entries
	for (auto& ntry : *this)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		int32_t cam_count = ntry.get_cam_item_count() / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			LOAD_LIST ll = ntry.get_load_lists(2 + 3 * j);
			for (auto& sublist : ll)
				ever_loaded.copy_in(sublist.list);
		}
	}

	for (auto& ntry : *this)
	{
		if (!ntry.is_normal_chunk_entry())
			continue;

		if (ever_loaded.find(ntry.m_eid) == -1)
		{
			ntry.m_is_loaded = false;
			entries_skipped++;
			printf("  %3d. entry %s never loaded, will not be included\n", entries_skipped, ntry.m_ename);
		}
	}

	printf("Number of normal entries not included: %3d\n", entries_skipped);
}
