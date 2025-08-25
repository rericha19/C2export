
#include "../include.h"
#include "utils.hpp"
#include "payloads.hpp"

// used to store payload information

int32_t PAYLOAD::page_count() const
{
	return int32_t(chunks.size());
}

int32_t PAYLOAD::tpage_count() const
{
	return int32_t(tchunks.size());
}

void PAYLOAD::print_info() const
{
	printf("Zone: %s cam path %d: payload: %3d, textures %2d, entities %2d",
		eid_conv2(zone), cam_path, page_count(), tpage_count(), entcount);
	printf("\n");
}

//  Calculates the amount of normal chunks loaded by the zone, their list.	
PAYLOAD PAYLOAD::get_payload_single(ELIST& elist, LIST& loaded, uint32_t zone, int32_t chunk_min, bool get_tpages)
{
	int32_t chunks[1024] = { 0 };
	int32_t count = 0;
	int32_t curr_chunk;
	bool is_there = false;

	for (int32_t i = 0; i < loaded.count(); i++)
	{
		int32_t elist_index = elist.get_index(loaded[i]);
		curr_chunk = elist[elist_index].chunk;

		is_there = false;
		for (int32_t j = 0; j < count; j++)
			if (chunks[j] == curr_chunk)
				is_there = true;

		if (!is_there && eid_conv2(elist[elist_index].eid)[4] != 'T' && curr_chunk != -1 && curr_chunk >= chunk_min)
		{
			chunks[count] = curr_chunk;
			count++;
		}
	}

	PAYLOAD payload{};
	payload.zone = zone;

	for (int32_t i = 0; i < count; i++)
		payload.chunks.push_back(chunks[i]);

	if (!get_tpages)
		return payload;

	int32_t tchunks[128] = { 0 };
	int32_t tcount = 0;

	for (int32_t i = 0; i < loaded.count(); i++)
	{
		int32_t elist_index = elist.get_index(loaded[i]);
		curr_chunk = elist[elist_index].eid;

		is_there = false;
		for (int32_t j = 0; j < tcount; j++)
			if (tchunks[j] == curr_chunk)
				is_there = true;

		if (!is_there && eid_conv2(elist[elist_index].eid)[4] == 'T' && curr_chunk != -1)
		{
			tchunks[tcount] = curr_chunk;
			tcount++;
		}
	}

	for (int32_t i = 0; i < tcount; i++)
		payload.tchunks.push_back(tchunks[i]);

	return payload;
}



// insert payload into payloads ladder
void PAYLOADS::insert(const PAYLOAD& insertee)
{
	for (int32_t i = 0; i < size(); i++)
	{
		if (at(i).zone != insertee.zone || at(i).cam_path != insertee.cam_path)
			continue;

		if (at(i).page_count() < insertee.page_count())
			at(i).chunks = insertee.chunks;

		if (at(i).tpage_count() < insertee.tpage_count())
			at(i).tchunks = insertee.tchunks;

		if (at(i).entcount < insertee.entcount)
			at(i).entcount = insertee.entcount;

		return;
	}

	push_back(insertee);
}

// get payloads ladder for matrix merge eval
PAYLOADS PAYLOADS::get_payload_ladder(MATRIX_STORED_LLS& stored_lls, ELIST& elist, int32_t chunk_min, int32_t get_tpages)
{
	PAYLOADS payloads{};
	for (auto& curr_ll : stored_lls)
	{
		PAYLOAD payload = PAYLOAD::get_payload_single(elist, curr_ll.full_load, curr_ll.zone, chunk_min, get_tpages);
		payload.cam_path = curr_ll.cam_path;
		payloads.insert(payload);
	}

	std::sort(payloads.begin(), payloads.end(), [](PAYLOAD& x, PAYLOAD& y)
		{
			if (y.page_count() != x.page_count())
				return (y.page_count() < x.page_count());
			else
				return (x.zone < y.zone);
		});

	return payloads;
}

// for matrix merge eval
int64_t PAYLOADS::calculate_score() const
{
	// gets current 'score' for the merge
	int64_t curr = 0;
	for (int32_t j = 0; j < min(int32_t(size()), 8); j++)
	{
		curr = curr * (int64_t)100;
		curr += (int64_t)at(j).page_count();
	}

	return curr;
}

// for ll_analyze
PAYLOADS PAYLOADS::get_payload_ladder_ll(ELIST& elist, int32_t chunk_min)
{
	auto get_max_draw = [](LOAD_LIST& draw_list) -> int32_t
		{
			LIST list{};
			int32_t ecount = 0;

			for (auto& sublist : draw_list)
			{
				if (sublist.type == 'B')
					list.copy_in(sublist.list);
				else if (sublist.type == 'A')
					list.remove_all(sublist.list);

				ecount = max(ecount, list.count());
			}
			return ecount;
		};

	PAYLOADS payloads{};

	for (auto& ntry: elist)
	{
		if (!(build_entry_type(ntry) == ENTRY_TYPE_ZONE && !ntry.data.empty()))
			continue;

		int32_t cam_count = build_get_cam_item_count(ntry._data()) / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{			
			LOAD_LIST load_list = get_load_lists(ntry._data(), 2 + 3 * j);
			DRAW_LIST draw_list = get_draw_lists(ntry._data(), 2 + 3 * j);

			int32_t max_draw = get_max_draw(draw_list);

			LIST list{};
			int32_t idx = -1;
			for (auto& sublist : load_list)
			{
				++idx;
				if (sublist.type == 'A')
					list.copy_in(sublist.list);
				else if (sublist.type == 'B')
					list.remove_all(sublist.list);

				// for simultaneous loads and deloads
				if (idx + 1 != load_list.size())
					if (sublist.type == 'A' && load_list[idx + 1].type == 'B')
						if (sublist.index == load_list[idx + 1].index)
							continue;

				PAYLOAD payload = PAYLOAD::get_payload_single(elist, list, ntry.eid, chunk_min, 1);
				payload.cam_path = j;
				payload.entcount = max_draw;
				payloads.insert(payload);
			}
		}
	}

	std::sort(payloads.begin(), payloads.end(), [](PAYLOAD& x, PAYLOAD& y)
		{

			if (x.zone != y.zone)
				return x.zone < y.zone;
			else
				return x.cam_path < y.cam_path;
		}
	);
	return payloads;
}




void WORST_ZONE_INFO::update(const PAYLOAD& worst)
{
	for (auto& info_s : *this)
	{
		if (info_s.zone == worst.zone && info_s.cam_path == worst.cam_path)
		{
			info_s.count++;
			info_s.sum += worst.page_count();
			return;
		}
	}

	// new zone
	WORST_ZONE_INFO_SINGLE new_info{};
	new_info.zone = worst.zone;
	new_info.cam_path = worst.cam_path;
	new_info.count = 1;
	new_info.sum = worst.page_count();
	push_back(new_info);
}

void WORST_ZONE_INFO::print_summary()
{
	std::sort(begin(), end(), [](WORST_ZONE_INFO_SINGLE a, WORST_ZONE_INFO_SINGLE b)
		{
			return b.count < a.count;
		});

	printf("\nWorst zone average:\n");
	for (int32_t i = 0; i < int32_t(size()) && i < 10; i++)
		printf("\t%s-%d - %4dx, worst-avg payload %4.2f\n",
			eid_conv2(at(i).zone), at(i).cam_path, at(i).count, ((double)at(i).sum) / at(i).count);
}
