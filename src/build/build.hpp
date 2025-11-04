
#include "../include.h"
#include "../utils/utils.hpp"
#include "../utils/dist_queue.hpp"

// Reads nsf, reads inputs, 
// makes draw lists, makes load lists and draw lists if selected, 
// merges entries into chunks, writes outputs
inline void build_main(int32_t build_type)
{
	clock_t time_build_start = clock();

	ELIST elist{};
	elist.m_config[Remake_Draw_Lists] = (build_type == BuildType_Rebuild_DL);

	// todo replace with vector of smart pointers?
	//std::vector<std::unique_ptr<uint8_t[]>> chunks(CHUNK_LIST_DEFAULT_SIZE, nullptr);
	uint8_t* chunks[CHUNK_LIST_DEFAULT_SIZE];

	// reading contents of the nsf to be rebuilt and collecting metadata
	bool input_parse_err = elist.read_and_parse_nsf(chunks, 0, NULL);
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

	if (elist.m_config[Remake_Draw_Lists])
	{
		elist.ask_draw_distances();
		elist.print_draw_position_overrides();
		elist.remake_draw_lists();
	}

	if (elist.m_config[Remake_Load_Lists])
	{
		// print for the user, informs them about entity type/subtypes that have no dependency list specified
		elist.check_unspecified_entities();

		// transition info (0xF/0x1F)
		elist.print_transitions();

		// ask user params for load list generation		
		elist.ask_load_distances();

		// build load lists based on user input and metadata, and already or not yet collected metadata
		printf("\nNumber of permaloaded entries: %d\n\n", elist.m_permaloaded.count());
		elist.remake_load_lists();
	}

	// entry omitting stuff
	elist.check_loaded();

	// call merge function
	clock_t time_start = clock();
	switch (elist.m_config[Chunk_Merge_Method])
	{
	case 4:
	case 5:
		elist.matrix_merge_random_main();
		break;
	default:
		printf("[ERROR] unknown/deprecated merge\n");
		break;
	}
	printf("\nMerge took %.3fs\n", ((double)clock() - time_start) / CLOCKS_PER_SEC);

	// build and write nsd file
	elist.write_nsd();
	elist.sort_load_lists();
	elist.build_normal_chunks(chunks);
	elist.write_nsf(chunks);
	elist.write_ll_log();

	printf("\n");
	printf("Build/rebuild took %.3fs\n", ((double)clock() - time_build_start) / CLOCKS_PER_SEC);
	printf("Done. It is recommended to save and patch NSD/NSF with current CrashEdit\n\n");

}
