#include "../include.h"
#include "../utils/utils.hpp"
#include "../utils/entry.hpp"

// builds a triangular matrix that contains the count of common load list occurences of i-th and j-th entry
int32_t** build_get_occurence_matrix(ELIST& elist, LIST entries, int32_t* config)
{
	int32_t ll_pollin_flag = config[CNFG_IDX_MTRX_LL_POLL_FLAG];

	int32_t** entry_matrix = (int32_t**)try_malloc(entries.count() * sizeof(int32_t*)); // freed by caller
	for (int32_t i = 0; i < entries.count(); i++)
		entry_matrix[i] = (int32_t*)try_calloc((i), sizeof(int32_t)); // freed by caller

	// for each zone's each camera path gets load list and based on it increments values of common load list occurences of pairs of entries
	for (auto& ntry : elist)
	{
		if (ntry.entry_type() != ENTRY_TYPE_ZONE)
			continue;

		int32_t cam_count = ntry.cam_item_count() / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			LOAD_LIST load_list = get_load_lists(ntry, 2 + 3 * j);

			uint8_t* cam_item = ntry.get_nth_main_cam(j);
			int32_t cam_length = build_get_path_length(cam_item);

			LIST list{};
			switch (ll_pollin_flag)
			{
				// per point (seems to generally be better), but its actually kinda fucky because if its not it gives worse results
				// if it were properly per point, the 'build_increment_common' function would use the 'counter' variable as 4th arg instead of 1
			case 2:
			case 1:
			{
				int32_t sublist_index = 0;
				int32_t counter = 0;
				for (int32_t l = 0; l < cam_length; l++)
				{
					counter++;
					if (load_list[sublist_index].index == l)
					{
						if (load_list[sublist_index].type == 'A')
						{
							list.copy_in(load_list[sublist_index].list);
						}
						if (load_list[sublist_index].type == 'B')
						{
							list.remove_all(load_list[sublist_index].list);
						}

						sublist_index++;
						if (ll_pollin_flag == 1)
							build_increment_common(list, entries, entry_matrix, 1);
						if (ll_pollin_flag == 2)
							build_increment_common(list, entries, entry_matrix, counter);
						counter = 0;
					}
				}
				break;
			}
			// per delta item
			case 0:
			{
				for (auto& sublist : load_list)
				{
					if (sublist.type == 'A')
						list.copy_in(sublist.list);
					else if (sublist.type == 'B')
						list.remove_all(sublist.list);

					if (list.count())
						build_increment_common(list, entries, entry_matrix, 1);
				}
				break;
			}
			default:
				break;
			}
		}
	}
	return entry_matrix;
}

// For each pair of entries it increments corresponding triangular matrix tile.
void build_increment_common(LIST list, LIST entries, int32_t** entry_matrix, int32_t rating)
{
	for (int32_t i = 0; i < list.count(); i++)
		for (int32_t j = i + 1; j < list.count(); j++)
		{
			int32_t indexA = entries.find(list[i]);
			int32_t indexB = entries.find(list[j]);

			// matrix is triangular
			if (indexA < indexB)
			{
				int32_t temp = indexA;
				indexA = indexB;
				indexB = temp;
			}

			// something done goofed, shouldnt happen
			if (indexA == -1 || indexB == -1)
				continue;

			entry_matrix[indexA][indexB] += rating;
		}
}

// global DSU state
thread_local int32_t g_dsu_parent[CHUNK_LIST_DEFAULT_SIZE];
thread_local int32_t g_dsu_chunk_size[CHUNK_LIST_DEFAULT_SIZE];
thread_local int32_t g_dsu_entry_count[CHUNK_LIST_DEFAULT_SIZE];

// Finds the representative of the set containing 'i' with path compression
int32_t dsu_find_set(int32_t i)
{
	if (g_dsu_parent[i] == i)
		return i;

	// Path compression for future lookups
	return g_dsu_parent[i] = dsu_find_set(g_dsu_parent[i]);
}

// Merges the sets containing 'a' and 'b'
// assumes root_a and root_b are different
void dsu_union_sets(int32_t a, int32_t b)
{
	int32_t root_a = dsu_find_set(a);
	int32_t root_b = dsu_find_set(b);

	// Simple union: merge b's set into a's.
	g_dsu_parent[root_b] = root_a;
	g_dsu_chunk_size[root_a] += g_dsu_chunk_size[root_b];
	g_dsu_entry_count[root_a] += g_dsu_entry_count[root_b];
}


// For each entry pair it finds out what chunk each is in and tries to merge.
// Starts with entries with highest common occurence count.
void build_matrix_merge_util(RELATIONS& relations, ELIST& elist, double merge_ratio)
{
	// Determine the number of chunks needed for DSU arrays
	int32_t entry_count = int32_t(elist.size());
	int32_t max_chunk_index = 0;
	for (int32_t i = 0; i < entry_count; i++)
	{
		if (elist[i].chunk > max_chunk_index)
			max_chunk_index = elist[i].chunk;
	}
	int32_t num_chunks = max_chunk_index + 1;

	// Initialize DSU state and Pre-calculate data
	for (int32_t i = 0; i < num_chunks; i++)
	{
		// Each chunk is its own parent initially
		g_dsu_parent[i] = i;
		g_dsu_chunk_size[i] = 0;
		g_dsu_entry_count[i] = 0;
	}

	for (int32_t i = 0; i < entry_count; i++)
	{
		int32_t c_idx = elist[i].chunk;
		g_dsu_chunk_size[c_idx] += elist[i].esize + 4;
		g_dsu_entry_count[c_idx]++;
	}

	// Optimized Merge Loop
	int32_t iter_count = min((int32_t)(relations.count * merge_ratio), relations.count);
	for (int32_t x = 0; x < iter_count; x++)
	{
		int32_t chunk_idx1 = elist[relations.relations[x].index1].chunk;
		int32_t chunk_idx2 = elist[relations.relations[x].index2].chunk;

		int root1 = dsu_find_set(chunk_idx1);
		int root2 = dsu_find_set(chunk_idx2);

		if (root1 == root2)
			continue;

		int32_t merged_chunk_size = g_dsu_chunk_size[root1] + g_dsu_chunk_size[root2];
		int32_t new_entry_count = g_dsu_entry_count[root1] + g_dsu_entry_count[root2];

		if (merged_chunk_size <= (CHUNKSIZE - 0x14) && new_entry_count <= 125)
			dsu_union_sets(root1, root2);
	}

	// Apply the merges back to the elist
	for (int32_t i = 0; i < entry_count; i++)
	{
		elist[i].chunk = dsu_find_set(elist[i].chunk);
	}
}


// Creates an array representation of the common load list occurence matrix, sorts high to low.
// To prevent having to search the entire matrix every time in the main merge util thing.
RELATIONS build_transform_matrix(LIST& entries, int32_t** entry_matrix, int32_t* config, ELIST& elist)
{
	int32_t zeroval_inc_flag = config[CNFG_IDX_MTRI_ZEROVAL_INC_FLAG];

	int32_t rel_counter = 0;
	int32_t indexer = 0;

	if (zeroval_inc_flag == 0)
	{
		for (int32_t i = 0; i < entries.count(); i++)
			for (int32_t j = 0; j < i; j++)
				if (entry_matrix[i][j] != 0)
					rel_counter++;
	}
	else
	{
		rel_counter = (entries.count() * (entries.count() - 1)) / 2;
	}

	RELATIONS relations{ rel_counter };

	for (int32_t i = 0; i < entries.count(); i++)
	{
		for (int32_t j = 0; j < i; j++)
		{
			if (zeroval_inc_flag == 0 && entry_matrix[i][j] == 0)
				continue;
			relations.relations[indexer].value = entry_matrix[i][j];
			relations.relations[indexer].index1 = elist.get_index(entries[i]);
			relations.relations[indexer].index2 = elist.get_index(entries[j]);

			int32_t sum = 0;
			for (int32_t k = 0; k < i; k++)
				sum += entry_matrix[i][k];
			for (int32_t k = 0; k < j; k++)
				sum += entry_matrix[j][k];
			relations.relations[indexer].total_occurences = sum;

			indexer++;
		}
	}

	relations.do_sort();
	return relations;
}
