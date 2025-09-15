#include "../include.h"
#include "../utils/elist.hpp"
#include "../utils/entry.hpp"

// builds nsd, saves at the specified path.
void ELIST::write_nsd()
{
	auto get_ldat_eid = [](int32_t level_ID)
		{
			char eid[6] = "DATxL";

			const char charset[] =
				"0123456789"
				"abcdefghijklmnopqrstuvwxyz"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"_!";

			eid[3] = charset[level_ID];

			return eid_to_int(eid);
		};

	int32_t real_entry_count = 0;

	uint8_t nsddata[20 * 1024]{};

	// count actual entries (some entries might not have been assigned a chunk for a reason, those are left out)
	for (auto& ntry : *this)
	{
		if (ntry.m_chunk != -1)
		{
			*(int32_t*)(nsddata + C2_NSD_ENTRY_TABLE_OFFSET + 8 * real_entry_count) = ntry.m_chunk * 2 + 1;
			*(int32_t*)(nsddata + C2_NSD_ENTRY_TABLE_OFFSET + 8 * real_entry_count + 4) = ntry.m_eid;
			real_entry_count++;
		}
	}

	// write chunk and real entry count
	*(int32_t*)(nsddata + C2_NSD_CHUNK_COUNT_OFFSET) = m_chunk_count;
	*(int32_t*)(nsddata + C2_NSD_ENTRY_COUNT_OFFSET) = real_entry_count;
	*(uint32_t*)(nsddata + C2_NSD_DATxL_EID) = get_ldat_eid(m_level_ID);

	// write spawn count, level ID
	int32_t real_nsd_size = C2_NSD_ENTRY_TABLE_OFFSET + real_entry_count * 8;
	*(int32_t*)(nsddata + real_nsd_size) = int32_t(m_spawns.size());
	*(int32_t*)(nsddata + real_nsd_size + 8) = m_level_ID;
	real_nsd_size += 0x10;

	// gool table, idr why nsd size gets incremented by 0x1CC
	for (int32_t i = 0; i < 0x40; i++)
		*(int32_t*)(nsddata + real_nsd_size + i * 4) = m_gool_table[i];
	real_nsd_size += 0x1CC;

	// write spawns, assumes camera 'spawns' on the zone's first cam path's first point (first == 0th)
	for (int32_t i = 0; i < int32_t(m_spawns.size()); i++)
	{
		*(int32_t*)(nsddata + real_nsd_size) = m_spawns[i].zone;
		*(int32_t*)(nsddata + real_nsd_size + 0x0C) = m_spawns[i].x << 8;
		*(int32_t*)(nsddata + real_nsd_size + 0x10) = m_spawns[i].y << 8;
		*(int32_t*)(nsddata + real_nsd_size + 0x14) = m_spawns[i].z << 8;
		real_nsd_size += 0x18;
	}

	// write and close nsd
	fwrite(nsddata, 1, real_nsd_size, m_nsd_out.f);
	printf("\nNSD #1 saved as '%s'\n",
		std::filesystem::absolute(m_nsd_out.rem_path).generic_string().c_str());

	if (m_nsd_out2.ok())
	{
		fwrite(nsddata, 1, real_nsd_size, m_nsd_out2.f);
		printf("NSD #2 saved as '%s'\n",
			std::filesystem::absolute(m_nsd_out2.rem_path).generic_string().c_str());
	}
}

// Writes to the already open output nsf files
void ELIST::write_nsf(uint8_t** chunks)
{
	for (int32_t i = 0; i < m_chunk_count; i++)
		fwrite(chunks[i], sizeof(uint8_t), CHUNKSIZE, m_nsf_out.f);

	printf("NSF #1 saved as '%s'\n",
		std::filesystem::absolute(m_nsf_out.rem_path).generic_string().c_str());

	if (m_nsf_out2.ok())
	{
		for (int32_t i = 0; i < m_chunk_count; i++)
			fwrite(chunks[i], sizeof(uint8_t), CHUNKSIZE, m_nsf_out2.f);

		printf("NSF #2 saved as '%s'\n",
			std::filesystem::absolute(m_nsf_out2.rem_path).generic_string().c_str());
	}

	printf("\n");
}

// Builds chunks according to entries' assigned chunks
void ELIST::build_normal_chunks(uint8_t** chunks)
{
	// texture, wavebank and sound chunks are already taken care of, thats why it starts after sounds
	for (int32_t i = m_chunk_border_sounds; i < m_chunk_count; i++)
	{
		int32_t chunk_no = 2 * i + 1;
		int32_t chunk_entry_count = 0;
		for (auto& ntry : *this)
			if (ntry.m_chunk == i)
				chunk_entry_count++;

		chunks[i] = (uint8_t*)try_calloc(CHUNKSIZE, sizeof(uint8_t)); // freed by build_main
		// header stuff
		*(uint16_t*)chunks[i] = MAGIC_CHUNK;
		*(uint16_t*)(chunks[i] + 4) = chunk_no;
		*(uint16_t*)(chunks[i] + 8) = chunk_entry_count;

		std::vector<uint32_t> offsets(chunk_entry_count + 2);

		// calculates offsets
		int32_t indexer = 0;
		offsets[0] = 0x10 + (chunk_entry_count + 1) * 4;

		for (auto& ntry : *this)
		{
			if (ntry.m_chunk == i)
			{
				offsets[indexer + 1] = offsets[indexer] + ntry.m_esize;
				indexer++;
			}
		}

		// writes offsets
		for (int32_t j = 0; j < chunk_entry_count + 1; j++)
			*(uint32_t*)(chunks[i] + 0x10 + j * 4) = offsets[j];

		// writes entries
		int32_t curr_offset = offsets[0];
		for (auto& ntry : *this)
		{
			if (ntry.m_chunk == i)
			{
				memcpy(chunks[i] + curr_offset, ntry._data(), ntry.m_esize);
				curr_offset += ntry.m_esize;
			}
		}

		*((uint32_t*)(chunks[i] + CHUNK_CHECKSUM_OFFSET)) = nsfChecksum(chunks[i]); // chunk checksum
	}
}

// Creates and builds sound chunks, formats accordingly?
void ELIST::build_sound_chunks(uint8_t** chunks)
{
	auto align_sound = [](int32_t input)
		{
			for (int32_t i = 0; i < 16; i++)
				if ((input + i) % 16 == 8)
					return (input + i);

			return input;
		};

	// merge similar to permaloaded
	std::vector<int32_t> sizes(8, 0x14);
	ELIST sound_list{};
	for (auto& ntry : *this)
	{
		if (ntry.get_entry_type() == EntryType::Sound)
			sound_list.push_back(ntry);
	}

	int32_t sound_entry_count = sound_list.count();
	sound_list.sort_by_esize();

	// put in the first chunk it fits into
	for (int32_t i = 0; i < sound_entry_count; i++)
	{
		for (int32_t j = 0; j < 8; j++)
		{
			if (sizes[j] + 4 + (((sound_list[i].m_esize + 16) >> 4) << 4) <= CHUNKSIZE)
			{
				sound_list[i].m_chunk = m_chunk_count + j;
				sizes[j] += 4 + (((sound_list[i].m_esize + 16) >> 4) << 4);
				break;
			}
		}
	}

	// find out how many actual sound chunks are used
	int32_t snd_chunk_count = 0;
	for (int32_t i = 0; i < sizes.size(); i++)
	{
		if (sizes[i] > 0x14)
			snd_chunk_count = i + 1;
	}

	// load lists like them that way ?
	sound_list.sort_by_eid();

	// build the actual chunks, almost identical to the build_normal_chunks function
	for (int32_t i = 0; i < snd_chunk_count; i++)
	{
		int32_t chunk_entry_count = 0;
		int32_t chunk_no = 2 * (m_chunk_count + i) + 1;
		chunks[m_chunk_count + i] = (uint8_t*)try_calloc(CHUNKSIZE, sizeof(uint8_t)); // freed by build_main

		for (int32_t j = 0; j < sound_entry_count; j++)
			if (sound_list[j].m_chunk == m_chunk_count + i)
				chunk_entry_count++;

		std::vector<uint32_t> offsets(chunk_entry_count + 2);

		*(uint16_t*)chunks[m_chunk_count + i] = MAGIC_CHUNK;
		*(uint16_t*)(chunks[m_chunk_count + i] + 2) = int32_t(ChunkType::SOUND);
		*(uint16_t*)(chunks[m_chunk_count + i] + 4) = chunk_no;
		*(uint16_t*)(chunks[m_chunk_count + i] + 8) = chunk_entry_count;

		int32_t indexer = 0;
		offsets[indexer] = align_sound(0x10 + (chunk_entry_count + 1) * 4);

		for (int32_t j = 0; j < sound_entry_count; j++)
		{
			if (sound_list[j].m_chunk == m_chunk_count + i)
			{
				offsets[indexer + 1] = align_sound(offsets[indexer] + sound_list[j].m_esize);
				indexer++;
			}
		}

		// seems to fix alignment ? hacky
		offsets[indexer] -= 8;

		for (int32_t j = 0; j < chunk_entry_count + 1; j++)
			*(uint32_t*)(chunks[m_chunk_count + i] + 0x10 + j * 4) = offsets[j];

		indexer = 0;
		for (int32_t j = 0; j < sound_entry_count; j++)
		{
			if (sound_list[j].m_chunk == m_chunk_count + i)
				memcpy(chunks[m_chunk_count + i] + offsets[indexer++], sound_list[j]._data(), sound_list[j].m_esize);
		}

		*(uint32_t*)(chunks[m_chunk_count + i] + CHUNK_CHECKSUM_OFFSET) = nsfChecksum(chunks[m_chunk_count + i]);
	}

	// update the chunk assignment in the actual master entry list
	// not sure whether its still necessary
	for (auto& ntry : *this)
	{
		for (int32_t j = 0; j < sound_entry_count; j++)
		{
			if (ntry.m_eid != sound_list[j].m_eid)
				continue;

			ntry.m_chunk = sound_list[j].m_chunk;
		}
	}

	m_chunk_count += snd_chunk_count;
	m_chunk_border_sounds = m_chunk_count;
}

// Creates and builds chunks for the instrument entries according to their format.
void ELIST::build_instrument_chunks(uint8_t** chunks)
{
	// wavebank chunks are one entry per chunk, not much to it
	for (auto& ntry : *this)
	{
		if (ntry.get_entry_type() != EntryType::Inst)
			continue;

		ntry.m_chunk = m_chunk_count;
		chunks[m_chunk_count] = (uint8_t*)try_calloc(CHUNKSIZE, sizeof(uint8_t)); // freed by build_main
		*(uint16_t*)(chunks[m_chunk_count]) = MAGIC_CHUNK;
		*(uint16_t*)(chunks[m_chunk_count] + 2) = int32_t(ChunkType::INSTRUMENT);
		*(uint16_t*)(chunks[m_chunk_count] + 4) = 2 * m_chunk_count + 1;

		*(uint32_t*)(chunks[m_chunk_count] + 8) = 1;
		*(uint32_t*)(chunks[m_chunk_count] + 0x10) = 0x24;
		*(uint32_t*)(chunks[m_chunk_count] + 0x14) = 0x24 + ntry.m_esize;

		memcpy(chunks[m_chunk_count] + 0x24, ntry._data(), ntry.m_esize);
		*((uint32_t*)(chunks[m_chunk_count] + CHUNK_CHECKSUM_OFFSET)) = nsfChecksum(chunks[m_chunk_count]);
		m_chunk_count++;
	}
}


