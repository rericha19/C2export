#include "../include.h"
#include "../utils/elist.hpp"
#include "../utils/entry.hpp"

void ELIST::print_draw_position_overrides()
{
	printf("\nDraw list gen position overrides:\n");

	for (auto& ntry : *this)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		for (int32_t i = 0; i < ntry.get_entity_count(); i++)
		{
			uint8_t* entity = ntry.get_nth_entity(i);
			int32_t ent_id = PROPERTY::get_value(entity, ENTITY_PROP_ID);
			if (ent_id == -1)
				continue;

			int32_t draw_mult = ntry.get_nth_entity_draw_override_mult(i);
			if (draw_mult != 100)
				printf("          %s entity ID %3d uses distance multiplier %.2fx (%d)\n", ntry.m_ename, ent_id, double(draw_mult) / 100, draw_mult);

			auto [present, entity_idx] = ntry.get_nth_entity_draw_override_ent_idx(i);
			if (!present)
				continue;

			int32_t pos_override_id = PROPERTY::get_value(entity, ENTITY_PROP_OVERRIDE_DRAW_ID) >> 8;

			if (entity_idx != -1)
				printf("          %s entity ID %3d uses position from entity %3d\n", ntry.m_ename, ent_id, pos_override_id);
			else
				printf("[warning] %s entity ID %3d cannot use position override %3d\n", ntry.m_ename, ent_id, pos_override_id);
		}
	}
}

void ELIST::write_ll_log()
{
	if (m_config[Remake_Load_Lists] != 2)
		return;

	std::string log_path = std::filesystem::path(m_nsf_out.rem_path).parent_path().generic_string()
		+ "\\load_list_log.json";

	File log_out{};
	if (log_out.open(log_path.c_str(), "w") && log_out.f)
	{
		std::string log_dump = m_load_list_logs.dump(1, '\t');
		fwrite(log_dump.c_str(), 1, log_dump.length(), log_out.f);

		printf("LL log saved as '%s'\n",
			std::filesystem::absolute(log_path).generic_string().c_str());
	}
}

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
void ELIST::write_nsf(CHUNKS& chunks)
{
	for (int32_t i = 0; i < m_chunk_count; i++)
		fwrite(chunks[i].get(), sizeof(uint8_t), CHUNKSIZE, m_nsf_out.f);

	printf("NSF #1 saved as '%s'\n",
		std::filesystem::absolute(m_nsf_out.rem_path).generic_string().c_str());

	if (m_nsf_out2.ok())
	{
		for (int32_t i = 0; i < m_chunk_count; i++)
			fwrite(chunks[i].get(), sizeof(uint8_t), CHUNKSIZE, m_nsf_out2.f);

		printf("NSF #2 saved as '%s'\n",
			std::filesystem::absolute(m_nsf_out2.rem_path).generic_string().c_str());
	}
}

// Builds chunks according to entries' assigned chunks
void ELIST::build_normal_chunks(CHUNKS& chunks)
{
	// texture, wavebank and sound chunks are already taken care of, thats why it starts after sounds
	for (int32_t i = m_chunk_border_sounds; i < m_chunk_count; i++)
	{
		int32_t chunk_no = 2 * i + 1;
		int32_t chunk_entry_count = 0;
		for (auto& ntry : *this)
			if (ntry.m_chunk == i)
				chunk_entry_count++;

		chunks[i].reset((uint8_t*)try_calloc(CHUNKSIZE, sizeof(uint8_t)));
		// header stuff
		*(uint16_t*)(chunks[i].get()) = MAGIC_CHUNK;
		*(uint16_t*)(chunks[i].get() + 4) = chunk_no;
		*(uint16_t*)(chunks[i].get() + 8) = chunk_entry_count;

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
			*(uint32_t*)(chunks[i].get() + 0x10 + j * 4) = offsets[j];

		// writes entries
		int32_t curr_offset = offsets[0];
		for (auto& ntry : *this)
		{
			if (ntry.m_chunk == i)
			{
				memcpy(chunks[i].get() + curr_offset, ntry._data(), ntry.m_esize);
				curr_offset += ntry.m_esize;
			}
		}

		*((uint32_t*)(chunks[i].get() + CHUNK_CHECKSUM_OFFSET)) = nsfChecksum(chunks[i].get()); // chunk checksum
	}
}

// Creates and builds sound chunks, formats accordingly?
void ELIST::build_sound_chunks(CHUNKS& chunks)
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
		chunks[m_chunk_count + i].reset((uint8_t*)try_calloc(CHUNKSIZE, sizeof(uint8_t)));

		for (int32_t j = 0; j < sound_entry_count; j++)
			if (sound_list[j].m_chunk == m_chunk_count + i)
				chunk_entry_count++;

		std::vector<uint32_t> offsets(chunk_entry_count + 2);

		*(uint16_t*)(chunks[m_chunk_count + i].get()) = MAGIC_CHUNK;
		*(uint16_t*)(chunks[m_chunk_count + i].get() + 2) = int32_t(ChunkType::SOUND);
		*(uint16_t*)(chunks[m_chunk_count + i].get() + 4) = chunk_no;
		*(uint16_t*)(chunks[m_chunk_count + i].get() + 8) = chunk_entry_count;

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
			*(uint32_t*)(chunks[m_chunk_count + i].get() + 0x10 + j * 4) = offsets[j];

		indexer = 0;
		for (int32_t j = 0; j < sound_entry_count; j++)
		{
			if (sound_list[j].m_chunk == m_chunk_count + i)
				memcpy(chunks[m_chunk_count + i].get() + offsets[indexer++], sound_list[j]._data(), sound_list[j].m_esize);
		}

		*(uint32_t*)(chunks[m_chunk_count + i].get() + CHUNK_CHECKSUM_OFFSET) = nsfChecksum(chunks[m_chunk_count + i].get());
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
void ELIST::build_instrument_chunks(CHUNKS& chunks)
{
	// wavebank chunks are one entry per chunk, not much to it
	for (auto& ntry : *this)
	{
		if (ntry.get_entry_type() != EntryType::Inst)
			continue;

		ntry.m_chunk = m_chunk_count;
		chunks[m_chunk_count].reset((uint8_t*)try_calloc(CHUNKSIZE, sizeof(uint8_t)));
		*(uint16_t*)(chunks[m_chunk_count].get()) = MAGIC_CHUNK;
		*(uint16_t*)(chunks[m_chunk_count].get() + 2) = int32_t(ChunkType::INSTRUMENT);
		*(uint16_t*)(chunks[m_chunk_count].get() + 4) = 2 * m_chunk_count + 1;

		*(uint32_t*)(chunks[m_chunk_count].get() + 8) = 1;
		*(uint32_t*)(chunks[m_chunk_count].get() + 0x10) = 0x24;
		*(uint32_t*)(chunks[m_chunk_count].get() + 0x14) = 0x24 + ntry.m_esize;

		memcpy(chunks[m_chunk_count].get() + 0x24, ntry._data(), ntry.m_esize);
		*((uint32_t*)(chunks[m_chunk_count].get() + CHUNK_CHECKSUM_OFFSET)) = nsfChecksum(chunks[m_chunk_count].get());
		m_chunk_count++;
	}
}


