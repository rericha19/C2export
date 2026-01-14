#include "../include.h"
#include "utils.hpp"
#include "payloads.hpp"
#include "entry.hpp"
#include "elist.hpp"

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
		eid2str(zone), cam_path, page_count(), tpage_count(), entcount);
	printf("\n");
}

//  Calculates the amount of normal chunks loaded by the zone, their list.	
PAYLOAD PAYLOAD::get_payload_single(ELIST& elist, LIST& loaded, uint32_t zone, int32_t chunk_min, bool get_tpages)
{
	std::unordered_set<int32_t> unique_chunks;
	unique_chunks.reserve(30);

	for (int32_t i = 0; i < loaded.count(); i++)
	{
		int32_t elist_index = elist.get_index(loaded[i]);
		auto& e = elist[elist_index];
		int32_t curr_chunk = e.m_chunk;

		if (curr_chunk != -1 && curr_chunk >= chunk_min && !e.m_is_tpage)
		{
			unique_chunks.insert(curr_chunk);
		}
	}

	// copy to payload vector
	PAYLOAD payload{};
	payload.zone = zone;
	payload.chunks.reserve(unique_chunks.size());

	for (int32_t chunk : unique_chunks)
		payload.chunks.push_back(chunk);

	if (!get_tpages)
		return payload;

	std::unordered_set<int32_t> tchunks_set;
	tchunks_set.reserve(10);

	for (int32_t i = 0; i < loaded.count(); i++)
	{
		int32_t elist_index = elist.get_index(loaded[i]);
		auto& e = elist[elist_index];
		int32_t curr_chunk = e.m_eid;

		if (e.m_is_tpage)
			tchunks_set.insert(curr_chunk);
	}

	// copy to payload vector
	for (int32_t chunk : tchunks_set)
		payload.tchunks.push_back(chunk);

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
PAYLOADS PAYLOADS::get_payload_ladder(MATRIX_STORED_LLS& stored_lls, ELIST& elist, int32_t chunk_min, bool get_tpages)
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
PAYLOADS PAYLOADS::get_payload_ladder_ll(ELIST& elist)
{
	auto get_max_draw = [](DRAW_LIST& draw_list) -> int32_t
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

	for (auto& ntry : elist)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		int32_t cam_count = ntry.get_cam_item_count() / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			LOAD_LIST load_list = ntry.get_load_lists(2 + 3 * j);
			DRAW_LIST draw_list = ntry.get_draw_lists(2 + 3 * j);

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

				PAYLOAD payload = PAYLOAD::get_payload_single(elist, list, ntry.m_eid, 0, true);
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

	printf("\nWorst zone averages:\n");
	for (int32_t i = 0; i < int32_t(size()) && i < 10; i++)
		printf("\t%s - %d  -  %4dx, worst-avg payload %4.2f\n",
			eid2str(at(i).zone), at(i).cam_path, at(i).count, ((double)at(i).sum) / at(i).count);
}
