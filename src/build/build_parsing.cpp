
// reading input data (nsf/folder), parsing and storing it

#include "../include.h"
#include "../utils/utils.hpp"
#include "../utils/entry.hpp"
#include "../utils/elist.hpp"


// todo move
// todo fix the mess at the start
// parsing input info for rebuilding from a nsf file
bool build_read_and_parse_rebld(ELIST& elist, uint8_t** chunks, bool stats_only, const char* fpath)
{
	File nsf;
	char nsfpath[MAX], fname[MAX + 20];

	if (fpath)
	{
		if (nsf.open(fpath, "rb") == false)
		{
			printf("[ERROR] Could not open selected NSF\n\n");
			return true;
		}
	}
	else
	{
		if (!stats_only)
		{
			printf("Input the path to the level (.nsf) you want to rebuild:\n");
			scanf(" %[^\n]", nsfpath);
			path_fix(nsfpath);

			if (nsf.open(nsfpath, "rb") == false)
			{
				printf("[ERROR] Could not open selected NSF\n\n");
				return true;
			}

			elist.m_level_ID = ask_level_ID();

			*(strrchr(nsfpath, '\\') + 1) = '\0';
			sprintf(fname, "%s\\S00000%02X.NSF", nsfpath, elist.m_level_ID);
			elist.m_nsf_out.open(fname, "wb");
			*(strchr(fname, '\0') - 1) = 'D';
			elist.m_nsd_out.open(fname, "wb");

			if (elist.m_nsd_out.f == NULL || elist.m_nsf_out.f == NULL)
			{
				printf("[ERROR] Could not open default outputs \n");
				return true;
			}
		}
		else
		{
			printf("Input the path to the level (.nsf):\n");
			scanf(" %[^\n]", nsfpath);
			path_fix(nsfpath);

			if (nsf.open(nsfpath, "rb") == false)
			{
				printf("[ERROR] Could not open selected NSF\n\n");
				return true;
			}
		}
	}

	int32_t nsf_chunk_count = chunk_count_base(nsf.f);
	elist.m_chunk_count = 0;

	static uint8_t buffer[CHUNKSIZE]{};
	for (int32_t i = 0; i < nsf_chunk_count; i++)
	{
		fread(buffer, sizeof(uint8_t), CHUNKSIZE, nsf.f);
		uint32_t checksum_calc = nsfChecksum(buffer);
		uint32_t checksum_used = *(uint32_t*)(buffer + CHUNK_CHECKSUM_OFFSET);
		if (checksum_calc != checksum_used)
			printf("Chunk %3d has invalid checksum\n", 2 * i + 1);

		if (chunk_type(buffer) == ChunkType::TEXTURE)
		{
			if (chunks != NULL)
			{
				chunks[elist.m_chunk_count] = (uint8_t*)try_calloc(CHUNKSIZE, sizeof(uint8_t)); // freed by build_main
				memcpy(chunks[elist.m_chunk_count], buffer, CHUNKSIZE);
			}
			ENTRY tpage{};
			tpage.m_is_tpage = true;
			tpage.m_eid = from_u32(buffer + 4);
			strncpy(tpage.m_ename, ENTRY::eid2s(tpage.m_eid).c_str(), 5);
			tpage.m_chunk = elist.m_chunk_count;
			elist.m_chunk_count++;
			elist.push_back(tpage);
			elist.sort_by_eid();
		}
		else
		{
			int32_t chunk_entry_count = from_u32(buffer + 0x8);
			for (int32_t j = 0; j < chunk_entry_count; j++)
			{
				int32_t start_offset = ENTRY::get_nth_item_offset(buffer, j);
				int32_t end_offset = ENTRY::get_nth_item_offset(buffer, j + 1);
				int32_t entry_size = end_offset - start_offset;

				ENTRY ntry{};
				ntry.m_eid = from_u32(buffer + start_offset + 0x4);
				ntry.m_esize = entry_size;
				ntry.m_data.resize(entry_size);
				memcpy(ntry._data(), buffer + start_offset, entry_size);
				strncpy(ntry.m_ename, ENTRY::eid2s(ntry.m_eid).c_str(), 5);

				if (!stats_only || ntry.get_entry_type() == EntryType::Sound)
					ntry.m_chunk = -1;
				else
					ntry.m_chunk = i;

				if (ntry.get_entry_type() == EntryType::Zone)
				{
					if (!ntry.check_get_item_count())
						return false;
					ntry.parse_relatives();
					ntry.parse_spawns(elist.m_spawns);
				}

				if (ntry.get_entry_type() == EntryType::GOOL && ntry.get_item_count() == 6)
				{
					uint8_t* item0 = ntry.get_nth_item(0);
					int32_t gool_type = from_u32(item0 + 0);
					if (gool_type < 0)
					{
						printf("[warning] GOOL entry %s has invalid type specified in the third item (%2d)!\n", ntry.m_ename, gool_type);
						continue;
					}

					if (elist.m_gool_table[gool_type] == EID_NONE)
						elist.m_gool_table[gool_type] = ntry.m_eid;
				}

				elist.push_back(ntry);
				elist.sort_by_eid();
			}
		}
	}

	return false;
}
