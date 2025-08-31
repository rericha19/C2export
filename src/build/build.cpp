
#include "../include.h"
#include "../utils/utils.hpp"
#include "../utils/dist_queue.hpp"
#include "../utils/entry.hpp"

// contains main build function that governs the process, as well as semi-misc functions
// necessary or used across various build files


// Used during reading from the folder to prevent duplicate texture chunks.
// Searches existing chunks (base level's chunks) and if the texture eid matches
// it returns the index of the matching chunk, else returns -1
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
	int32_t offset = PROPERTY::get_offset(item, ENTITY_PROP_CAM_SLST);

	if (offset)
		return from_u32(item + offset + 4);
	else
		return 0;
}

// Returns path length of the input item.
uint32_t build_get_path_length(uint8_t* item)
{
	int32_t offset = PROPERTY::get_offset(item, ENTITY_PROP_PATH);

	if (offset)
		return from_u32(item + offset);
	else
		return 0;
}

int32_t build_prop_count(uint8_t* item)
{
	return from_u32(item + 0xC);
}

int32_t build_chunk_type(uint8_t* chunk)
{
	if (chunk == NULL)
		return -1;

	return from_u16(chunk + 0x2);
}

// calculates the amount of chunks based on the nsf size
int32_t build_get_chunk_count_base(FILE* nsf)
{
	fseek(nsf, 0, SEEK_END);
	int32_t result = ftell(nsf) / CHUNKSIZE;
	rewind(nsf);

	return result;
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
	int32_t chunk_border_sounds = 0;
	int32_t chunk_count = 0;

	uint32_t gool_table[C3_GOOL_TABLE_SIZE]{};
	for (int32_t i = 0; i < C3_GOOL_TABLE_SIZE; i++)
		gool_table[i] = EID_NONE;

	// config:
	int32_t config[] = {
		0, // 0 - unused
		0, // 1 - unused
		1, // 2 - merge type value             0 - per delta   |   1 - weird per point |   2 - per point   set here, used by matrix merge

		0, // 3 - slst distance value              set by user in function build_ask_distances(config); affects load lists
		0, // 4 - neighbour distance value         set by user in function build_ask_distances(config); affects load lists
		0, // 5 - draw list distance value         set by user in function build_ask_distances(config); affects load lists
		0, // 6 - transition pre-load flag         set by user in function build_ask_distances(config); affects load lists
		0, // 7 - backwards penalty value          set by user in function build_ask_dist...;  aff LLs     is 1M times the float value because int32_t, range 0 - 0.5

		0, // 8 - unused
		0, // 9 - unused

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
	bool input_parse_err = build_read_and_parse_rebld(&level_ID, &nsfnew, &nsd, &chunk_count, gool_table, elist, chunks, &spawns, 0, NULL);

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

	if (build_type == BuildType_Rebuild_DL)
	{
		build_ask_draw_distances(config);
		build_remake_draw_lists(elist, config);
	}

	if (config[CNFG_IDX_LL_REMAKE_FLAG] == 1)
	{
		// print for the user, informs them about entity type/subtypes that have no dependency list specified
		elist.check_unspecified_entities(subtype_info);

		// transition info (0xF/0x1F)
		elist.print_transitions();

		// ask user desired distances for various things, eg how much in advance in terms of camera rail distance things get loaded
		// there are some restrictions in terms of path link depth so its not entirely accurate, but it still matters
		build_ask_distances(config);

		// build load lists based on user input and metadata, and already or not yet collected metadata
		printf("\nNumber of permaloaded entries: %d\n\n", permaloaded.count());
		build_remake_load_lists(elist, gool_table, permaloaded, subtype_info, collisions, mus_dep, config);
	}

	elist.check_loaded();

	clock_t time_start = clock();
	int32_t merge_tech_flag = config[CNFG_IDX_MERGE_METHOD_VALUE];
	// call merge function
	switch (merge_tech_flag)
	{
	case 5:
		build_matrix_merge_random_thr_main(elist, chunk_border_sounds, chunk_count, config, permaloaded);
		break;
	case 4:
		build_matrix_merge_random_main(elist, chunk_border_sounds, chunk_count, config, permaloaded, false);
		break;
	default:
		printf("[ERROR] deprecated merge %d\n", merge_tech_flag);
		break;
	}
	printf("\nMerge took %.3fs\n", ((double)clock() - time_start) / CLOCKS_PER_SEC);

	// build and write nsf and nsd file
	build_write_nsd(nsd, nsd2, elist, chunk_count, spawns, gool_table, level_ID);
	elist.sort_load_lists();
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
