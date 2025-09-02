
#include "../include.h"
#include "../utils/utils.hpp"
#include "../utils/dist_queue.hpp"
#include "../utils/entry.hpp"

// Reads nsf, reads inputs, 
// makes draw lists, makes load lists and draw lists if selected, 
// merges entries into chunks, writes output
void build_main(int32_t build_type)
{
	clock_t time_build_start = clock();

	ELIST elist{};

	//std::vector<std::vector<uint8_t>> chunks{};	
	uint8_t* chunks[CHUNK_LIST_DEFAULT_SIZE];
	// used to keep track of counts and to separate groups of chunks	

	// reading contents of the nsf to be rebuilt and collecting metadata
	bool input_parse_err = build_read_and_parse_rebld(elist, chunks, 0, NULL);
	if (input_parse_err)
	{
		printf("[ERROR] something went wrong during parsing\n");
		return;
	}

	// see whether to save in a secondary folder
	elist.ask_second_output();

	// user picks whether to remake load lists or not, also merge method
	elist.ask_build_flags();

	// let the user pick the spawn
	if (!elist.m_spawns.ask_spawn())
	{
		printf("[ERROR] No spawns found, add one using the usual 'willy' entity or a checkpoint\n\n");
		return;
	}

	// for backwards penalty
	DIST_GRAPH_Q::run_distance_graph(elist, elist.m_spawns[0]);

	// builds instrument and sound chunks, chunk_border_sounds is used to make chunk merging and chunk building more convenient, especially in deprecate methods
	elist.build_instrument_chunks(chunks);
	elist.build_sound_chunks(chunks);

	elist.sort_by_eid();
	// ask user paths to metadata files, store perma/dependencies info
	if (!elist.read_entry_config())
	{
		printf("[ERROR] File could not be opened or a different error occured\n\n");
		return;
	}

	if (build_type == BuildType_Rebuild_DL)
	{
		elist.ask_draw_distances();
		build_remake_draw_lists(elist);
	}

	if (elist.m_config[CNFG_IDX_LL_REMAKE_FLAG] == 1)
	{
		// print for the user, informs them about entity type/subtypes that have no dependency list specified
		elist.check_unspecified_entities();

		// transition info (0xF/0x1F)
		elist.print_transitions();

		// ask user params for load list generation		
		elist.ask_load_distances();

		// build load lists based on user input and metadata, and already or not yet collected metadata
		printf("\nNumber of permaloaded entries: %d\n\n", elist.m_permaloaded.count());
		build_remake_load_lists(elist);
	}

	elist.check_loaded();

	clock_t time_start = clock();
	int32_t merge_tech_flag = elist.m_config[CNFG_IDX_MERGE_METHOD_VALUE];
	// call merge function
	switch (merge_tech_flag)
	{
	case 5:
		build_matrix_merge_random_thr_main(elist);
		break;
	case 4:
		build_matrix_merge_random_main(elist);
		break;
	default:
		printf("[ERROR] unknown/deprecated merge %d\n", merge_tech_flag);
		break;
	}
	printf("\nMerge took %.3fs\n", ((double)clock() - time_start) / CLOCKS_PER_SEC);

	// build and write nsd file
	elist.write_nsd();
	elist.sort_load_lists();
	elist.build_normal_chunks(chunks);
	elist.write_nsf(chunks);

	printf("Build/rebuild took %.3fs\n", ((double)clock() - time_build_start) / CLOCKS_PER_SEC);
	printf("Done. It is recommended to save and patch NSD/NSF with current CrashEdit\n\n");
}
