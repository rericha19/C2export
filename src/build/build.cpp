
#include "../include.h"
#include "../utils/utils.hpp"
#include "../utils/dist_queue.hpp"
#include "../game_structs/entry.hpp"

// contains main build function that governs the process, as well as semi-misc functions
// necessary or used across various build files

uint32_t build_get_model(uint8_t* anim, int32_t item)
{
	int32_t item_off = build_get_nth_item_offset(anim, item);
	return from_u32(anim + item_off + 0x10);
}

uint32_t build_get_zone_track(uint8_t* entry)
{
	int32_t item1off = build_get_nth_item_offset(entry, 0);
	int32_t item2off = build_get_nth_item_offset(entry, 1);
	int32_t item1len = item2off - item1off;

	uint32_t music_entry = from_u32(entry + item1off + C2_MUSIC_REF + item1len - 0x318);
	return music_entry;
}

/*
Used during reading from the folder to prevent duplicate texture chunks.
Searches existing chunks (base level's chunks) and if the texture eid matches
it returns the index of the matching chunk, else returns -1
*/
int32_t build_get_base_chunk_border(uint32_t textr, uint8_t** chunks, int32_t index_end)
{
	int32_t ret = -1;

	for (int32_t i = 0; i < index_end; i++)
		if (from_u32(chunks[i] + 4) == textr)
			ret = i;

	return ret;
}


// gets slst from camera item
uint32_t build_get_slst(uint8_t* item)
{
	int32_t offset = build_get_prop_offset(item, ENTITY_PROP_CAM_SLST);

	if (offset)
		return from_u32(item + offset + 4);
	else
		return 0;
}

// Returns path length of the input item.
uint32_t build_get_path_length(uint8_t* item)
{
	int32_t offset = build_get_prop_offset(item, ENTITY_PROP_PATH);

	if (offset)
		return from_u32(item + offset);
	else
		return 0;
}

// Gets a zone's neighbour count.
int32_t build_get_neighbour_count(uint8_t* entry)
{
	int32_t item1off = build_get_nth_item_offset(entry, 0);
	return entry[item1off + C2_NEIGHBOURS_START];
}

// Gets a zone's camera entity count.
int32_t build_get_cam_item_count(uint8_t* entry)
{
	int32_t item1off = build_get_nth_item_offset(entry, 0);
	return entry[item1off + 0x188];
}

// Gets a zone's regular entity count.
int32_t build_get_entity_count(uint8_t* entry)
{
	int32_t item1off = build_get_nth_item_offset(entry, 0);
	return entry[item1off + 0x18C];
}

int32_t build_item_count(uint8_t* entry)
{
	return from_u32(entry + 0xC);
}

int32_t build_prop_count(uint8_t* item)
{
	return from_u32(item + 0xC);
}

// Returns type of an entry.
int32_t build_entry_type(ENTRY entry)
{
	if (entry.data.empty())
		return -1;
	return *(int32_t*)(entry._data() + 8);
}

int32_t build_chunk_type(uint8_t* chunk)
{
	if (chunk == NULL)
		return -1;

	return from_u16(chunk + 0x2);
}

void build_check_item_count(ENTRY& zone)
{
	int32_t item_count = build_item_count(zone._data());
	int32_t cam_count = build_get_cam_item_count(zone._data());
	int32_t entity_count = build_get_entity_count(zone._data());

	if (item_count != (2 + cam_count + entity_count))
	{
		printf("[warning] %s's item count (%d) doesn't match item counts in the first item (2 + %d + %d)\n",
			zone.ename, item_count, cam_count, entity_count);
	}
}

DRAW_ITEM build_decode_draw_item(uint32_t value)
{
	DRAW_ITEM draw;
	draw.neighbour_item_index = (value & 0xFF000000) >> 24;
	draw.ID = (value & 0xFFFF00) >> 8;
	draw.neighbour_zone_index = value & 0xFF;
	return draw;
}

// Returns value of the specified property. Only works on generic, single-value (4B length 4B value) properties.
int32_t build_get_entity_prop(uint8_t* entity, int32_t prop_code)
{
	int32_t offset = build_get_prop_offset(entity, prop_code);
	if (offset == 0)
		return -1;

	return from_u32(entity + offset + 4);
}

// gets offset to data of a property within an item
int32_t build_get_prop_offset(uint8_t* item, int32_t prop_code)
{
	int32_t offset = 0;
	int32_t prop_count = build_prop_count(item);
	for (int32_t i = 0; i < prop_count; i++)
		if ((from_u16(item + 0x10 + 8 * i)) == prop_code)
			offset = OFFSET + from_u16(item + 0x10 + 8 * i + 2);

	return offset;
}

// calculates the amount of chunks based on the nsf size
int32_t build_get_chunk_count_base(FILE* nsf)
{
	fseek(nsf, 0, SEEK_END);
	int32_t result = ftell(nsf) / CHUNKSIZE;
	rewind(nsf);

	return result;
}

// Checks whether the entry is meant to be placed into a normal chunk.
bool build_is_normal_chunk_entry(ENTRY entry)
{
	int32_t type = build_entry_type(entry);
	if (type == ENTRY_TYPE_ANIM || type == ENTRY_TYPE_DEMO || type == ENTRY_TYPE_ZONE || type == ENTRY_TYPE_MODEL || type == ENTRY_TYPE_SCENERY || type == ENTRY_TYPE_SLST || type == ENTRY_TYPE_DEMO || type == ENTRY_TYPE_VCOL || type == ENTRY_TYPE_MIDI || type == ENTRY_TYPE_GOOL || type == ENTRY_TYPE_T21)
		return true;

	return false;
}

// some cleanup
void build_final_cleanup(uint8_t** chunks, int32_t chunk_count, FILE* nsfnew, FILE* nsd)
{
	for (int32_t i = 0; i < chunk_count; i++)
		free(chunks[i]);

	if (nsfnew != NULL)
		fclose(nsfnew);

	if (nsd != NULL)
		fclose(nsd);
}

void build_normal_check_loaded(ELIST& elist)
{
	int32_t entry_count = elist.count();
	int32_t omit = 0;
	printf("\nOmit normal chunk entries that are never loaded? [0 - include, 1 - omit]\n");
	scanf("%d", &omit);

	for (int32_t i = 0; i < entry_count; i++)
		elist[i].norm_chunk_ent_is_loaded = true;

	if (!omit)
	{
		printf("Not omitting\n");
		return;
	}

	printf("Checking for normal chunk entries that are never loaded\n");
	LIST ever_loaded{};
	int32_t entries_skipped = 0;

	// reads all load lists and bluntly adds all items into the list of all loaded entries
	for (int32_t i = 0; i < entry_count; i++)
	{
		if (!(build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data.size()))
			continue;

		int32_t cam_count = build_get_cam_item_count(elist[i]._data()) / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			LOAD_LIST ll = get_load_lists(elist[i]._data(), 2 + 3 * j);
			for (auto& sublist : ll)
				ever_loaded.copy_in(sublist.list);
		}
	}

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (!build_is_normal_chunk_entry(elist[i]))
			continue;

		if (ever_loaded.find(elist[i].eid) == -1)
		{
			elist[i].norm_chunk_ent_is_loaded = false;
			entries_skipped++;
			printf("  %3d. entry %s never loaded, will not be included\n", entries_skipped, elist[i].ename);
		}
	}

	printf("Number of normal entries not included: %3d\n", entries_skipped);
}

void build_print_transitions(ELIST& elist)
{
	int32_t entry_count = elist.count();
	printf("\nTransitions in the level: \n");

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE)
			continue;

		LIST neighbours = ENTRY_2::get_neighbours(elist[i]._data());
		int32_t item1off = build_get_nth_item_offset(elist[i]._data(), 0);
		for (int32_t j = 0; j < neighbours.count(); j++)
		{
			uint32_t neighbour_eid = from_u32(elist[i]._data() + item1off + C2_NEIGHBOURS_START + 4 + 4 * j);
			uint32_t neighbour_flg = from_u32(elist[i]._data() + item1off + C2_NEIGHBOURS_START + 4 + 4 * j + 0x20);

			if (neighbour_flg == 0xF || neighbour_flg == 0x1F)
			{
				printf("Zone %s transition (%02x) to zone %s (neighbour %d)\n", elist[i].ename, neighbour_flg, ENTRY_2::eid_to_str(neighbour_eid).c_str(), j);
			}
		}
	}
}


// Reads nsf, reads inputs, 
// makes draw lists, makes load lists and draw lists if selected, 
// merges entries into chunks, writes output
void build_main(int32_t build_type)
{
	clock_t time_build_start = clock();

	FILE* nsfnew = NULL, * nsd = NULL;	// file pointers for input nsf, output nsf (nsfnew) and output nsd
	FILE* nsfnew2 = NULL, * nsd2 = NULL;
	SPAWNS spawns{};		// struct with spawns found during reading and parsing of the level data
	ELIST elist{};	

	uint8_t* chunks[CHUNK_LIST_DEFAULT_SIZE];	// array of pointers to potentially built chunks, fixed length cuz lazy
	LIST permaloaded;							// list containing eids of permaloaded entries provided by the user
	DEPENDENCIES subtype_info{};				// struct containing info about dependencies of certain types and subtypes
	DEPENDENCIES collisions{};					// struct containing info about dependencies of certain collision types
	DEPENDENCIES mus_dep{};
	int32_t level_ID = 0; // level ID, used for naming output files and needed in output nsd

	// used to keep track of counts and to separate groups of chunks	
	int32_t chunk_border_texture = 0;
	int32_t chunk_border_sounds = 0;
	int32_t chunk_count = 0;
	int32_t entry_count = 0;

	uint32_t gool_table[C3_GOOL_TABLE_SIZE]; // table w/ eids of gool entries, needed for nsd, filled using input entries
	for (int32_t i = 0; i < C3_GOOL_TABLE_SIZE; i++)
		gool_table[i] = EID_NONE;

	// config:
	int32_t config[] = {
		0, // 0 - gool initial merge flag      0 - group       |   1 - one by one                          set here, used by deprecate merges
		0, // 1 - zone initial merge flag      0 - group       |   1 - one by one                          set here, used by deprecate merges
		1, // 2 - merge type value             0 - per delta   |   1 - weird per point |   2 - per point   set here, used by matrix merge

		0, // 3 - slst distance value              set by user in function build_ask_distances(config); affects load lists
		0, // 4 - neighbour distance value         set by user in function build_ask_distances(config); affects load lists
		0, // 5 - draw list distance value         set by user in function build_ask_distances(config); affects load lists
		0, // 6 - transition pre-load flag         set by user in function build_ask_distances(config); affects load lists
		0, // 7 - backwards penalty value          set by user in function build_ask_dist...;  aff LLs     is 1M times the float value because int32_t, range 0 - 0.5

		0, // 8 - relation array sort flag     0 - regular     |   1 - also sort by total occurence count; set here, used by matrix merge (1 is kinda meh)
		0, // 9 - sound entry load list flag   0 - all sounds  |   1 - one sound per sound chunk           set here, affects load lists

		0, // 10 - load list remake flag        0 - dont remake |   1 - remake load lists                   set by user in build_ask_build_flags
		0, // 11 - merge technique value                                                                    set by user in build_ask_build_flags

		1, // 12 - perma inc. in matrix flag    0 - dont include|   1 - do include                          set here, used by matrix merges
		1, // 13 - inc. 0-vals in relarray flag 0 - dont include|   1 - do include                          set here, used by matrix merges

		0, // 14 - draw list gen dist 2D            set by user in build_ask_draw_distances
		0, // 15 - draw list gen dist 3D            set by user in build_ask_draw_distances
		0, // 16 - draw list gen dist 2D vertictal| set by user in build_ask_draw_distances
		0, // 17 - draw list gen angle 3D           set by user in build_ask_draw_distances - allowed angle distance
	};

	// reading contents of the nsf to be rebuilt and collecting metadata
	bool input_parse_err = build_read_and_parse_rebld(&level_ID, &nsfnew, &nsd, &chunk_border_texture, gool_table, elist, chunks, &spawns, 0, NULL);
	chunk_count = chunk_border_texture;

	// end if something went wrong
	if (input_parse_err)
	{
		printf("[ERROR] something went wrong during parsing\n");
		build_final_cleanup(chunks, chunk_count, nsfnew, nsd);
		return;
	}

	build_try_second_output(&nsfnew2, &nsd2, level_ID);

	// user picks whether to remake load lists or not, also merge method
	build_ask_build_flags(config);
	int32_t load_list_flag = config[CNFG_IDX_LL_REMAKE_FLAG];
	int32_t merge_tech_flag = config[CNFG_IDX_MERGE_METHOD_VALUE];

	spawns.sort_spawns();	
	// let the user pick the spawn, according to the spawn determine for each cam path its distance from spawn in terms of path links,
	// which is later used to find out which of 2 paths is in the backwards direction and, where backwards loading penalty should be applied
	// during the load list generation procedure
	if (spawns.size() > 0)
		build_ask_spawn(spawns);
	else
	{
		printf("[ERROR] No spawns found, add one using the usual 'willy' entity or a checkpoint\n\n");
		build_final_cleanup(chunks, chunk_count, nsfnew, nsd);
		if (nsfnew2 != NULL)
		{
			fclose(nsfnew2);
			fclose(nsd2);
		}
		return;
	}

	DIST_GRAPH_Q::run_distance_graph(elist, spawns[0]);

	// gets model references from gools, was useful in a deprecate chunk merging/building algorithm, but might be useful later and barely takes any time so idc
	elist.update_gool_references();

	// builds instrument and sound chunks, chunk_border_sounds is used to make chunk merging and chunk building more convenient, especially in deprecate methods
	build_instrument_chunks(elist, &chunk_count, chunks);
	build_sound_chunks(elist, &chunk_count, chunks);
	chunk_border_sounds = chunk_count;

	elist.sort_by_eid();
	// ask user paths to files with permaloaded entries, type/subtype dependencies and collision type dependencies,
	// parse files and store info in permaloaded, subtype_info and collisions structs
	if (!build_read_entry_config(permaloaded, subtype_info, collisions, mus_dep, elist, gool_table, config))
	{
		printf("[ERROR] File could not be opened or a different error occured\n\n");
		build_final_cleanup(chunks, chunk_count, nsfnew, nsd);
		if (nsfnew2 != NULL)
		{
			fclose(nsfnew2);
			fclose(nsd2);
		}
		return;
	}

	// debug
	/*for (int32_t i = 0; i < entry_count; i++)
		list_add(&permaloaded, elist[i].eid);*/

	if (build_type == BuildType_Rebuild_DL)
	{
		build_ask_draw_distances(config);
		build_remake_draw_lists(elist, config);
	}

	if (load_list_flag == 1)
	{
		// print for the user, informs them about entity type/subtypes that have no dependency list specified
		build_find_unspecified_entities(elist, subtype_info);

		// transition info (0xF/0x1F)
		build_print_transitions(elist);

		// ask user desired distances for various things, eg how much in advance in terms of camera rail distance things get loaded
		// there are some restrictions in terms of path link depth so its not entirely accurate, but it still matters
		build_ask_distances(config);

		// build load lists based on user input and metadata, and already or not yet collected metadata
		printf("\nNumber of permaloaded entries: %d\n\n", permaloaded.count());
		build_remake_load_lists(elist, gool_table, permaloaded, subtype_info, collisions, mus_dep, config);
	}

	build_normal_check_loaded(elist);

	clock_t time_start = clock();
	// call merge function
	switch (merge_tech_flag)
	{
	case 5:
#if COMPILE_WITH_THREADS
		build_matrix_merge_random_thr_main(elist, chunk_border_sounds, &chunk_count, config, permaloaded);
#else
		printf("This build does not support this method, using method 4 instead\n");
		build_matrix_merge_random_main(elist, chunk_border_sounds, &chunk_count, config, permaloaded, true);
#endif // COMPILE_WITH_THREADS
		break;
	case 4:
		build_matrix_merge_random_main(elist, chunk_border_sounds, &chunk_count, config, permaloaded, false);
		break;
	default:
		printf("[ERROR] deprecated merge %d\n", merge_tech_flag);
		break;
	}
	printf("\nMerge took %.3fs\n", ((double)clock() - time_start) / CLOCKS_PER_SEC);

	// build and write nsf and nsd file
	build_write_nsd(nsd, nsd2, elist, chunk_count, spawns, gool_table, level_ID);
	build_sort_load_lists(elist);
	build_normal_chunks(elist, chunk_border_sounds, chunk_count, chunks, 1);
	build_write_nsf(nsfnew, chunk_count, chunks, nsfnew2);

	// get rid of at least some dynamically allocated memory, p sure there are leaks all over the place but oh well
	build_final_cleanup(chunks, chunk_count, nsfnew, nsd);
	if (nsfnew2 != NULL)
	{
		fclose(nsfnew2);
		fclose(nsd2);
	}
	printf("Build/rebuild took %.3fs\n", ((double)clock() - time_build_start) / CLOCKS_PER_SEC);
	printf("Done. It is recommended to save NSD & NSF couple times with CrashEdit, e.g. 0.2.135.2 (or higher),\notherwise the level might not work.\n\n");
}
