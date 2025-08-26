#include "../include.h"


// Assigns primary chunks to all entries before merging
int32_t build_assign_primary_chunks_all(ELIST& elist, int32_t* chunk_count)
{
	int32_t counter = 0;
	for (int32_t i = 0; i < elist.count(); i++)
	{
		if (build_is_normal_chunk_entry(elist[i]) && elist[i].chunk == -1 && elist[i].norm_chunk_ent_is_loaded)
		{
			elist[i].chunk = (*chunk_count)++;
			counter++;
		}
	}
	return counter;
}

// builds a triangular matrix that contains the count of common load list occurences of i-th and j-th entry
int32_t** build_get_occurence_matrix(ELIST& elist, LIST entries, int32_t* config)
{
	int32_t entry_count = int32_t(elist.size());
	int32_t ll_pollin_flag = config[CNFG_IDX_MTRX_LL_POLL_FLAG];

	int32_t** entry_matrix = (int32_t**)try_malloc(entries.count() * sizeof(int32_t*)); // freed by caller
	for (int32_t i = 0; i < entries.count(); i++)
		entry_matrix[i] = (int32_t*)try_calloc((i), sizeof(int32_t)); // freed by caller

	// for each zone's each camera path gets load list and based on it increments values of common load list occurences of pairs of entries
	for (int32_t i = 0; i < entry_count; i++)
	{
		if (elist[i].entry_type() != ENTRY_TYPE_ZONE)
			continue;

		int32_t cam_count = build_get_cam_item_count(elist[i]._data()) / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			LOAD_LIST load_list = get_load_lists(elist[i], 2 + 3 * j);

			uint8_t* cam_item = elist[i].get_nth_item(2 + 3 * j);
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

// Current best chunk merge method based on common load list occurence count of each pair of entries,
// therefore relies on proper and good load lists ideally with delta items.
// After the matrix is created and transformed into a sorted array it attemps to merge.
void build_matrix_merge(ELIST& elist, int32_t chunk_border_sounds, int32_t* chunk_count, int32_t* config, LIST permaloaded, double merge_ratio, double mult)
{
	int32_t entry_count = elist.count();
	int32_t permaloaded_include_flag = config[CNFG_IDX_MTRX_PERMA_INC_FLAG];

	LIST entries = elist.get_normal_entries();
	if (permaloaded_include_flag == 0)
	{
		entries.remove_all(permaloaded);
	}

	int32_t** entry_matrix = build_get_occurence_matrix(elist, entries, config);

	// put the matrix's contents in an array and sort (so the matrix doesnt have to be searched every time)
	// frees the contents of the matrix
	RELATIONS array_representation = build_transform_matrix(entries, entry_matrix, config, elist);
	for (int32_t i = 0; i < entries.count(); i++)
		free(entry_matrix[i]);
	free(entry_matrix);

	if (mult != 1.0)
	{
		int32_t dec_mult = (int32_t)(1000 * mult);
		for (int32_t i = 0; i < array_representation.count; i++)
			array_representation.relations[i].value *= ((double)((rand() % dec_mult)) / 1000);
		qsort(array_representation.relations, array_representation.count, sizeof(RELATION), relations_cmp);
	}

	// do the merges according to the relation array, get rid of holes afterwards
	build_matrix_merge_util(array_representation, elist, merge_ratio);

	free(array_representation.relations);
	*chunk_count = build_remove_empty_chunks(chunk_border_sounds, *chunk_count, elist);
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
void build_matrix_merge_util(RELATIONS relations, ELIST& elist, double merge_ratio)
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
RELATIONS build_transform_matrix(LIST entries, int32_t** entry_matrix, int32_t* config, ELIST& elist)
{
	int32_t relation_array_sort_flag = config[CNFG_IDX_MTRI_REL_SORT_FLAG];
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

	RELATIONS relations;
	relations.count = rel_counter;
	relations.relations = (RELATION*)try_malloc(rel_counter * sizeof(RELATION)); // freed by caller

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

	if (relation_array_sort_flag == 0)
		qsort(relations.relations, relations.count, sizeof(RELATION), relations_cmp); // the 'consistent' one
	if (relation_array_sort_flag == 1)
		qsort(relations.relations, relations.count, sizeof(RELATION), relations_cmp2);

	return relations;
}

// Specially merges permaloaded entries as they dont require any association.
// Works similarly to the sound chunk one.
// Biggest first sort packing algorithm kinda thing, seems to work pretty ok.
int32_t build_permaloaded_merge(ELIST& elist, int32_t chunk_border_sounds, int32_t* chunk_count, LIST permaloaded)
{
	int32_t entry_count = elist.count();
	int32_t start_chunk_count = *chunk_count;
	int32_t perma_normal_entry_count = 0;

	// find all permaloaded entries, add them to a temporary list, sort the list in descending order by size (biggest first)
	for (int32_t i = 0; i < entry_count; i++)
		if (permaloaded.find(elist[i].eid) != -1 && build_is_normal_chunk_entry(elist[i]))
			perma_normal_entry_count++;

	ELIST perma_elist{};
	perma_elist.resize(perma_normal_entry_count);

	int32_t indexer = 0;
	for (int32_t i = 0; i < entry_count; i++)
		if (permaloaded.find(elist[i].eid) != -1 && build_is_normal_chunk_entry(elist[i]))
			perma_elist[indexer++] = elist[i];

	perma_elist.sort_by_esize();

	// keep putting them into existing chunks if they fit
	int32_t perma_chunk_count = perma_normal_entry_count;                        // idrc about optimising this
	int32_t* sizes = (int32_t*)try_malloc(perma_chunk_count * sizeof(int32_t)); // freed here

	for (int32_t i = 0; i < perma_chunk_count; i++)
		sizes[i] = 0x14;

	// place where fits
	for (int32_t i = 0; i < perma_normal_entry_count; i++)
		for (int32_t j = 0; j < perma_chunk_count; j++)
			if (sizes[j] + 4 + perma_elist[i].esize <= CHUNKSIZE)
			{
				perma_elist[i].chunk = start_chunk_count + j;
				sizes[j] += 4 + perma_elist[i].esize;
				break;
			}
	// to edit the array of entries that is actually used
	for (int32_t i = 0; i < entry_count; i++)
		for (int32_t j = 0; j < perma_normal_entry_count; j++)
			if (elist[i].eid == perma_elist[j].eid)
				elist[i].chunk = perma_elist[j].chunk;

	// counts used chunks
	int32_t counter = 0;
	for (int32_t i = 0; i < perma_chunk_count; i++)
		if (sizes[i] > 0x14)
			counter = i + 1;

	*chunk_count = start_chunk_count + counter;
	free(sizes);
	return counter;
}


// If a chunk is empty it gets replaced with the last existing chunk.
// This goes on until no further replacements are done (no chunks are empty).
int32_t build_remove_empty_chunks(int32_t index_start, int32_t index_end, ELIST& elist)
{
	int32_t entry_count = elist.count();
	int32_t empty_chunk = 0;
	while (1)
	{
		for (int32_t i = index_start; i < index_end; i++)
		{
			bool is_occupied = false;
			empty_chunk = -1;
			for (int32_t j = 0; j < entry_count; j++)
				if (elist[j].chunk == i)
					is_occupied = true;

			if (!is_occupied)
			{
				empty_chunk = i;
				break;
			}
		}

		if (empty_chunk == -1)
			break;

		for (int32_t i = 0; i < entry_count; i++)
			if (elist[i].chunk == index_end - 1)
				elist[i].chunk = empty_chunk;

		index_end--;
	}

	return index_end;
}

// used to sort relations that store how much entries are loaded simultaneously, sorts in descending order, also takes total occurence count into consideration (experimental)
int32_t relations_cmp2(const void* a, const void* b)
{
	RELATION x = *(RELATION*)a;
	RELATION y = *(RELATION*)b;

	if (y.value - x.value != 0)
		return y.value - x.value;

	return x.total_occurences - y.total_occurences;
}

// used to sort relations that store how much entries are loaded simultaneously, sorts in descending order
int32_t relations_cmp(const void* a, const void* b)
{
	RELATION x = *(RELATION*)a;
	RELATION y = *(RELATION*)b;

	return y.value - x.value;
}
