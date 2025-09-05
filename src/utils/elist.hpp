#pragma once
#include "../include.h"
#include "../utils/utils.hpp"

class ELIST : public std::vector<ENTRY>
{
public:
	ELIST()
	{
		for (int32_t i = 0; i < C3_GOOL_TABLE_SIZE; i++)
			m_gool_table[i] = EID_NONE;
	}

	// file pointers for output files
	File m_nsf_out;
	File m_nsd_out;
	File m_nsf_out2;
	File m_nsd_out2;

	int32_t m_chunk_count = 0;
	int32_t m_chunk_border_sounds = 0;

	int32_t m_level_ID = 0;				// level ID, for naming output files, needed in output nsd
	LIST m_permaloaded{};					// eids of permaloaded entries provided by the user
	DEPENDENCIES m_subt_deps{};			// dependencies for certain types and subtypes
	DEPENDENCIES m_coll_deps{};			// dependencies for certain collision types
	DEPENDENCIES m_musi_deps{};			// dependencies for zones using certain music track 
	uint32_t m_gool_table[C3_GOOL_TABLE_SIZE]{};
	SPAWNS m_spawns{};

	RebuildConfig m_config =
	{
		{LL_Matrix_Polling_Type_CONST, 1}, // const

		{Remake_Draw_Lists, 0},
		{Remake_Load_Lists, 0},
		{Chunk_Merge_Method, 0},

		{LL_SLST_Distance, 0},
		{LL_Neighbour_Distance, 0},
		{LL_Drawlist_Distance, 0},
		{LL_Transition_Preloading_Type, 0},
		{LL_Backwards_Loading_Penalty_DBL, 0},

		{DL_Distance_Cap_X, 0},
		{DL_Distance_Cap_Y, 0},
		{DL_Distance_Cap_XZ, 0},
		{DL_Distance_Cap_Angle3D, 0},

		{Rebuild_Payload_Limit, 0},
		{Rebuild_Iteration_Limit, 0},
		{Rebuild_Random_Mult_DBL, 0},
		{Rebuild_Base_Seed, 0},
		{Rebuild_Thread_Count, 0},
	};

	void sort_by_eid();
	void sort_by_esize();
	int32_t get_index(uint32_t eid);
	int32_t count();
	LIST get_normal_entries();

	void assign_primary_chunks_all();
	void remove_empty_chunks();
	void merge_permaloaded();
	void print_transitions();
	void sort_load_lists();
	void check_unspecified_entities();
	void check_loaded();

	// input
	void ask_build_flags();
	void ask_build_paths(char* perma_path, char* subt_deps_path, char* coll_deps_path, char* mus_deps_path);
	bool read_entry_config();
	void ask_second_output();
	void ask_draw_distances();
	void ask_load_distances();
	void ask_params_matrix();
	bool read_and_parse_nsf(uint8_t** chunks, bool stats_only, const char* fpath);

	// load list and draw list gen
	void print_draw_position_overrides();
	void add_neighbour_coll_dependencies(std::vector<LIST>& full_list, ENTRY& ntry);
	void remake_draw_lists();
	void draw_list_gen_handle_neighbour(std::vector<LIST>& full_draw, ENTRY& curr, int32_t cam_idx, ENTRY& neighbour, int32_t neigh_ref_idx);
	LIST get_types_subtypes_from_ids(LIST& entity_ids, LIST& neighbours)

	// matrix merge
	void matrix_merge_util(RELATIONS& relations);
	RELATIONS get_occurence_array();
	RELATIONS matrix_transform_to_array(std::vector<std::vector<int32_t>>& entry_matrix, LIST& normal_entries);
	MATRIX_STORED_LLS matrix_store_lls();
	void matrix_merge_random_util(MTRX_THRD_IN_STR inp_args);
	void matrix_merge_random_main();

	// output
	void write_nsd();
	void write_nsf(uint8_t** chunks);
	void build_normal_chunks(uint8_t** chunks);
	void build_sound_chunks(uint8_t** chunks);
	void build_instrument_chunks(uint8_t** chunks);
};
