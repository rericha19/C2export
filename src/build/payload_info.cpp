#include "../include.h"
// contains deprecate but not entirely unused implementation of the chunk merging/building algorithm
// mainly payload based decisions and some other older stuff

int32_t build_get_max_draw(LOAD_LIST& draw_list)
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
}

/** \brief
 *  Used by payload method, deprecate.
 *  Creates a payloads object that contains each zone and chunks that zone loads.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param chunk_min int32_t                 used to get rid of invalid chunks/entries
 * \return PAYLOADS                     payloads struct
 */
PAYLOADS build_get_payload_ladder(ENTRY* elist, int32_t entry_count, int32_t chunk_min)
{
	// todo find out where it crashes
	PAYLOADS payloads;
	payloads.count = 0;
	payloads.arr = NULL;	

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (!(build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL))
			continue;

		int32_t cam_count = build_get_cam_item_count(elist[i].data) / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			LOAD_LIST load_list{};
			LOAD_LIST draw_list{};
			build_get_load_lists(load_list, elist[i].data, 2 + 3 * j);
			build_get_draw_lists(draw_list, elist[i].data, 2 + 3 * j);

			int32_t max_draw = build_get_max_draw(draw_list);

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

				PAYLOAD payload = build_get_payload(elist, entry_count, list, elist[i].eid, chunk_min, 1);
				payload.cam_path = j;
				payload.entcount = max_draw;
				build_insert_payload(&payloads, payload);
			}
		}
	}

	return payloads;
}

/** \brief
 *  Adds a payload to a payloads struct, overwrites if its already there.
 *
 * \param payloads PAYLOADS*            payloads struct
 * \param insertee PAYLOAD              payload struct
 * \return void
 */
void build_insert_payload(PAYLOADS* payloads, PAYLOAD insertee)
{
	for (int32_t i = 0; i < payloads->count; i++)
	{
		if (payloads->arr[i].zone == insertee.zone && payloads->arr[i].cam_path == insertee.cam_path)
		{

			if (payloads->arr[i].count < insertee.count)
			{
				payloads->arr[i].count = insertee.count;
				free(payloads->arr[i].chunks);
				payloads->arr[i].chunks = insertee.chunks;
			}

			if (payloads->arr[i].tcount < insertee.tcount)
			{
				payloads->arr[i].tcount = insertee.tcount;
				free(payloads->arr[i].tchunks);
				payloads->arr[i].tchunks = insertee.tchunks;
			}

			if (payloads->arr[i].entcount < insertee.entcount)
				payloads->arr[i].entcount = insertee.entcount;

			return;
		}
	}

	if (payloads->arr == NULL)
		payloads->arr = (PAYLOAD*)try_malloc(1 * sizeof(PAYLOAD));
	else
		payloads->arr = (PAYLOAD*)try_realloc(payloads->arr, (payloads->count + 1) * sizeof(PAYLOAD));
	payloads->arr[payloads->count] = insertee;
	payloads->count++;
}

/** \brief
 *  Prints payload.
 *
 * \param payload PAYLOAD               payload struct
 * \param stopper int32_t                   something dumb
 * \return void
 */
void build_print_payload(PAYLOAD payload)
{
	printf("Zone: %s cam path %d: payload: %3d, textures %2d, entities %2d",
		eid_conv2(payload.zone), payload.cam_path, payload.count, payload.tcount, payload.entcount);
	printf("\n");
}

/** \brief
 *  Calculates the amount of normal chunks loaded by the zone, their list.
 *  Used by a deprecate merge method.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param list LIST                     current load list
 * \param zone uint32_t             current zone
 * \param chunk_min int32_t                 used to weed out sound and instrument entries (nsf structure this program produces is texture - wavebank - sound - normal)
 * \return PAYLOAD                      payload object that contains a list of chunks that are loaded in this zone, their count and the current zone eid
 */
PAYLOAD build_get_payload(ENTRY* elist, int32_t entry_count, LIST list, uint32_t zone, int32_t chunk_min, bool get_tpages)
{
	int32_t chunks[1024] = { 0 };
	int32_t count = 0;
	int32_t curr_chunk;
	bool is_there = false;

	for (int32_t i = 0; i < list.count(); i++)
	{
		int32_t elist_index = build_get_index(list[i], elist, entry_count);
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

	PAYLOAD payload;
	payload.zone = zone;
	payload.count = count;
	payload.chunks = NULL;
	payload.tchunks = NULL;

	if (count)
	{
		payload.chunks = (int32_t*)try_malloc(count * sizeof(int32_t));
		memcpy(payload.chunks, chunks, sizeof(int32_t) * count);
	}

	if (!get_tpages)
		return payload;

	int32_t tchunks[128] = { 0 };
	int32_t tcount = 0;

	for (int32_t i = 0; i < list.count(); i++)
	{
		int32_t elist_index = build_get_index(list[i], elist, entry_count);
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

	payload.tcount = tcount;
	if (tcount)
	{
		payload.tchunks = (int32_t*)try_malloc(tcount * sizeof(int32_t));
		memcpy(payload.tchunks, tchunks, sizeof(int32_t) * tcount);
	}
	return payload;
}
