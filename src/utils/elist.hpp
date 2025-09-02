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

	// todo turn into a proper struct or just vars
	// config:
	int32_t m_config[18] = {
		0, // 0 - unused
		0, // 1 - unused
		1, // 2 - merge type value             0 - per delta   |   1 - weird per point |   2 - per point   set here, used by matrix merge

		0, // 3 - slst distance value              set by user in function build_ask_load_distances(config); affects load lists
		0, // 4 - neighbour distance value         set by user in function build_ask_load_distances(config); affects load lists
		0, // 5 - draw list distance value         set by user in function build_ask_load_distances(config); affects load lists
		0, // 6 - transition pre-load flag         set by user in function build_ask_load_distances(config); affects load lists
		0, // 7 - backwards penalty value          set by user in function build_ask_dist...;  aff LLs     is 1M times the float value because int32_t, range 0 - 0.5

		0, // 8 - unused
		0, // 9 - unused

		0, // 10 - load list remake flag        0 - dont remake |   1 - remake load lists                   set by user in build_ask_build_flags
		0, // 11 - merge technique value                                                                    set by user in build_ask_build_flags

		1, // 12 - unused
		1, // 13 - unused

		0, // 14 - draw list gen dist 2D            set by user in build_ask_draw_distances
		0, // 15 - draw list gen dist 3D            set by user in build_ask_draw_distances
		0, // 16 - draw list gen dist 2D vertictal| set by user in build_ask_draw_distances
		0, // 17 - draw list gen angle 3D           set by user in build_ask_draw_distances - allowed angle distance
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
	bool read_entry_config();
	void ask_second_output();
	void ask_draw_distances();
	void ask_load_distances();

	// output
	void write_nsd();
	void write_nsf(uint8_t** chunks);
	void build_normal_chunks(uint8_t** chunks);
	void build_sound_chunks(uint8_t** chunks);
	void build_instrument_chunks( uint8_t** chunks);
};
