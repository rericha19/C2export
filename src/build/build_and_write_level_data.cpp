#include "../include.h"
#include "../utils/utils.hpp"
#include "../utils/entry.hpp"

// contains functions responsible for transforming existing entry->chunk assignmened data
// into playable/output nsd/nsf files

// Writes to the already open output nsf file, closes it.
void build_write_nsf(FILE* nsfnew, int32_t chunk_count, uint8_t** chunks, FILE* nsfnew2)
{
	for (int32_t i = 0; i < chunk_count; i++)
		fwrite(chunks[i], sizeof(uint8_t), CHUNKSIZE, nsfnew);
	if (nsfnew2 != NULL)
		for (int32_t i = 0; i < chunk_count; i++)
			fwrite(chunks[i], sizeof(uint8_t), CHUNKSIZE, nsfnew2);
}


//  Builds chunks according to entries' assigned chunks
void build_normal_chunks(ELIST& elist, int32_t chunk_border_sounds, int32_t chunk_count, uint8_t** chunks, int32_t do_end_print)
{
	int32_t entry_count = elist.count();
	int32_t sum = 0;
	// texture, wavebank and sound chunks are already taken care of, thats why it starts after sounds
	for (int32_t i = chunk_border_sounds; i < chunk_count; i++)
	{
		int32_t chunk_no = 2 * i + 1;
		int32_t local_entry_count = 0;
		for (int32_t j = 0; j < entry_count; j++)
			if (elist[j].chunk == i)
				local_entry_count++;

		chunks[i] = (uint8_t*)try_calloc(CHUNKSIZE, sizeof(uint8_t)); // freed by build_main
		// header stuff
		*(uint16_t*)chunks[i] = MAGIC_CHUNK;
		*(uint16_t*)(chunks[i] + 4) = chunk_no;
		*(uint16_t*)(chunks[i] + 8) = local_entry_count;

		std::vector<uint32_t> offsets{};
		offsets.resize(local_entry_count + 2);

		// calculates offsets
		int32_t indexer = 0;
		offsets[0] = 0x10 + (local_entry_count + 1) * 4;

		for (int32_t j = 0; j < entry_count; j++)
		{
			if (elist[j].chunk == i)
			{
				offsets[indexer + 1] = offsets[indexer] + elist[j].esize;
				indexer++;
			}
		}

		// writes offsets
		for (int32_t j = 0; j < local_entry_count + 1; j++)
			*(uint32_t*)(chunks[i] + 0x10 + j * 4) = offsets[j];

		// writes entries
		int32_t curr_offset = offsets[0];
		for (int32_t j = 0; j < entry_count; j++)
		{
			if (elist[j].chunk == i)
			{
				memcpy(chunks[i] + curr_offset, elist[j]._data(), elist[j].esize);
				curr_offset += elist[j].esize;
			}
		}

		sum += curr_offset;                                                          // for avg
		*((uint32_t*)(chunks[i] + CHUNK_CHECKSUM_OFFSET)) = nsfChecksum(chunks[i]); // chunk checksum
	}

	if (do_end_print)
		printf("Average normal chunk portion taken: %.3f%c\n", (100 * (double)sum / (chunk_count - chunk_border_sounds)) / CHUNKSIZE, '%');
}

uint32_t build_get_ldat_eid(int32_t level_ID)
{
	char eid[6] = "DATxL";

	const char charset[] =
		"0123456789"
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"_!";

	eid[3] = charset[level_ID];

	return eid_to_int(eid);
}


// Builds nsd, sorts load lists according to the nsd entry table order.
// Saves at the specified path.
void build_write_nsd(FILE* nsd, FILE* nsd2, ELIST& elist, int32_t chunk_count, SPAWNS spawns, uint32_t* gool_table, int32_t level_ID)
{
	int32_t entry_count = elist.count();
	int32_t real_entry_count = 0;

	// arbitrarily doing 64kB because convenience
	uint8_t* nsddata = (uint8_t*)try_calloc(CHUNKSIZE, 1); // freed here

	// count actual entries (some entries might not have been assigned a chunk for a reason, those are left out)
	for (int32_t i = 0; i < entry_count; i++)
	{
		if (elist[i].chunk != -1)
		{
			*(int32_t*)(nsddata + C2_NSD_ENTRY_TABLE_OFFSET + 8 * real_entry_count) = elist[i].chunk * 2 + 1;
			*(int32_t*)(nsddata + C2_NSD_ENTRY_TABLE_OFFSET + 8 * real_entry_count + 4) = elist[i].eid;
			real_entry_count++;
		}
	}

	// write chunk and real entry count
	*(int32_t*)(nsddata + C2_NSD_CHUNK_COUNT_OFFSET) = chunk_count;
	*(int32_t*)(nsddata + C2_NSD_ENTRY_COUNT_OFFSET) = real_entry_count;
	*(uint32_t*)(nsddata + C2_NSD_DATxL_EID) = build_get_ldat_eid(level_ID);

	// write spawn count, level ID
	int32_t real_nsd_size = C2_NSD_ENTRY_TABLE_OFFSET + real_entry_count * 8;
	*(int32_t*)(nsddata + real_nsd_size) = int32_t(spawns.size() + 1);
	*(int32_t*)(nsddata + real_nsd_size + 8) = level_ID;
	real_nsd_size += 0x10;

	// gool table, idr why nsd size gets incremented by 0x1CC
	for (int32_t i = 0; i < 0x40; i++)
		*(int32_t*)(nsddata + real_nsd_size + i * 4) = gool_table[i];
	real_nsd_size += 0x1CC;

	// write spawns, assumes camera 'spawns' on the zone's first cam path's first point (first == 0th)
	*(int32_t*)(nsddata + real_nsd_size) = spawns[0].zone;
	*(int32_t*)(nsddata + real_nsd_size + 0x0C) = spawns[0].x << 8;
	*(int32_t*)(nsddata + real_nsd_size + 0x10) = spawns[0].y << 8;
	*(int32_t*)(nsddata + real_nsd_size + 0x14) = spawns[0].z << 8;
	real_nsd_size += 0x18;

	for (size_t i = 0; i < spawns.size(); i++)
	{
		*(int32_t*)(nsddata + real_nsd_size) = spawns[i].zone;
		*(int32_t*)(nsddata + real_nsd_size + 0x0C) = spawns[i].x << 8;
		*(int32_t*)(nsddata + real_nsd_size + 0x10) = spawns[i].y << 8;
		*(int32_t*)(nsddata + real_nsd_size + 0x14) = spawns[i].z << 8;
		real_nsd_size += 0x18;
	}

	// write and close nsd
	fwrite(nsddata, 1, real_nsd_size, nsd);
	if (nsd2 != NULL)
		fwrite(nsddata, 1, real_nsd_size, nsd2);
	free(nsddata);
}

