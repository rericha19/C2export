#include "../include.h"
#include "../utils/payloads.hpp"
#include "../utils/utils.hpp"
#include "../utils/entry.hpp"
#include "../utils/elist.hpp"

// global DSU state
constexpr auto DSU_LENGTH = 2000;
thread_local int32_t g_dsu_parent[DSU_LENGTH];
thread_local int32_t g_dsu_chunk_size[DSU_LENGTH];
thread_local int32_t g_dsu_entry_count[DSU_LENGTH];

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

// repeatedly merge pairs in descending order of their common occurence count
void ELIST::matrix_merge_util(RELATIONS& relations)
{
	// Determine the number of chunks needed for DSU arrays
	int32_t entry_count = count();
	int32_t max_chunk_index = 0;
	for (int32_t i = 0; i < entry_count; i++)
	{
		if (at(i).m_chunk > max_chunk_index)
			max_chunk_index = at(i).m_chunk;
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
		int32_t c_idx = at(i).m_chunk;
		g_dsu_chunk_size[c_idx] += at(i).m_esize + 4;
		g_dsu_entry_count[c_idx]++;
	}

	int32_t iter_count = relations.count;
	for (int32_t x = 0; x < iter_count; x++)
	{
		int32_t chunk_idx1 = at(relations.relations[x].index1).m_chunk;
		int32_t chunk_idx2 = at(relations.relations[x].index2).m_chunk;

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
		at(i).m_chunk = dsu_find_set(at(i).m_chunk);
}

// builds a triangular matrix that contains the count of common load list occurences of i-th and j-th entry
RELATIONS ELIST::get_occurence_array()
{
	// For each pair of entries it increments corresponding triangular matrix tile.
	auto matrix_increment_pair = [](LIST& list, LIST normal_entries, std::vector<std::vector<int32_t>>& entry_matrix, int32_t rating)
		{
			for (int32_t i = 0; i < list.count(); i++)
			{
				for (int32_t j = i + 1; j < list.count(); j++)
				{
					int32_t indexA = normal_entries.find(list[i]);
					int32_t indexB = normal_entries.find(list[j]);

					if (indexA == -1 || indexB == -1)
						continue;

					// matrix is triangular
					if (indexA < indexB)
					{
						int32_t temp = indexA;
						indexA = indexB;
						indexB = temp;
					}

					entry_matrix[indexA][indexB] += rating;
				}
			}
		};

	int32_t ll_polling_type = m_config[LL_Matrix_Polling_Type_CONST];
	LIST normal_entries = get_normal_entries();
	int32_t normal_count = normal_entries.count();

	std::vector<std::vector<int32_t>> entry_matrix(normal_count);
	for (int32_t i = 0; i < normal_count; i++)
		entry_matrix[i].resize(i, 0);

	// for each zone's each camera path gets load list and based on it increments values of common load list occurences of pairs of entries
	for (auto& ntry : *this)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		int32_t cam_count = ntry.get_cam_item_count() / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			LOAD_LIST load_list = ntry.get_load_lists(2 + 3 * j);
			int32_t cam_length = ntry.get_ent_path_len(2 + 3 * j);

			LIST list{};
			switch (ll_polling_type)
			{
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
						if (ll_polling_type == 1)
							matrix_increment_pair(list, normal_entries, entry_matrix, 1);
						if (ll_polling_type == 2)
							matrix_increment_pair(list, normal_entries, entry_matrix, counter);
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
						matrix_increment_pair(list, normal_entries, entry_matrix, 1);
				}
				break;
			}
			default:
				break;
			}
		}
	}

	return matrix_transform_to_array(entry_matrix, normal_entries);
}

RELATIONS ELIST::matrix_transform_to_array(std::vector<std::vector<int32_t>>& entry_matrix, LIST& normal_entries)
{
	int32_t normal_count = normal_entries.count();
	int32_t indexer = 0;
	int32_t rel_counter = (normal_count * (normal_count - 1)) / 2;

	RELATIONS relations{ rel_counter };

	for (int32_t i = 0; i < normal_count; i++)
	{
		for (int32_t j = 0; j < i; j++)
		{
			auto& value = entry_matrix[i][j];
			relations.relations[indexer].value = value;
			relations.relations[indexer].save_value = value;
			relations.relations[indexer].index1 = get_index(normal_entries[i]);
			relations.relations[indexer].index2 = get_index(normal_entries[j]);

			indexer++;
		}
	}

	relations.do_sort();
	return relations;
}

MATRIX_STORED_LLS ELIST::matrix_store_lls()
{
	MATRIX_STORED_LLS stored_stuff{};

	for (auto& ntry : *this)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		int32_t cam_count = ntry.get_cam_item_count() / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			LOAD_LIST load_list = ntry.get_load_lists(2 + 3 * j);

			LIST list{};
			int32_t idx = -1;
			for (auto& sublist : load_list)
			{
				++idx;

				if (sublist.type == 'A')
					list.copy_in(sublist.list);
				else if (sublist.type == 'B')
					list.remove_all(sublist.list);

				// for simultaneous loads and deloads
				if (idx != load_list.size() - 1)
					if (sublist.type == 'A' && load_list[idx + 1].type == 'B')
						if (sublist.index == load_list[idx + 1].index)
							continue;

				if (list.count() > 0)
				{
					MATRIX_STORED_LL stored_load_list{ ntry.m_eid, j };

					for (int32_t z = 0; z < list.count(); z++)
					{
						int32_t index = get_index(list[z]);
						if (index != -1 && at(index).is_normal_chunk_entry())
							stored_load_list.full_load.add(list[z]);
					}

					stored_stuff.push_back(stored_load_list);
				}
			}
		}
	}

	return stored_stuff;
}

// do matrix marge (single thread util function)
void ELIST::matrix_merge_random_util(MTRX_THRD_IN_STR inp_args)
{
	bool limit_reached = false;
	bool goal_reached = false;

	int32_t entry_count = count();

	ELIST clone_elist{};
	clone_elist.resize(entry_count);
	for (int32_t j = 0; j < entry_count; j++)
		clone_elist[j] = at(j);

	// separate array representation for each thread
	RELATIONS array_representation(inp_args.rel_array->count);
	memcpy(array_representation.relations.get(), inp_args.rel_array->relations.get(), array_representation.count * sizeof(RELATION));

	rand_seed(inp_args.rnd_seed);
	int32_t iter_count = m_config[Rebuild_Iteration_Limit];
	int16_t thr_id = inp_args.thread_idx;
	double rand_mult = config_to_double(m_config[Rebuild_Random_Mult_DBL]);

	while (!goal_reached)
	{
		// mutex iter
		{
			std::lock_guard<std::mutex> guard(*inp_args.mutex_iter);
			if (*inp_args.curr_iter_ptr > iter_count && iter_count != 0)
				limit_reached = true;
		}

		if (limit_reached)
			break;

		// restore chunk assignments
		for (int32_t j = 0; j < entry_count; j++)
			clone_elist[j].m_chunk = at(j).m_chunk;

		// second half of the matrix merge slightly randomised and ran on clone elist
		for (int32_t j = 0; j < array_representation.count; j++)
			array_representation.relations[j].value = array_representation.relations[j].save_value * randfrom(1.0, rand_mult);
		array_representation.do_sort();

		// do the merges according to the slightly randomised relation array
		clone_elist.matrix_merge_util(array_representation);

		// get payload ladder for current iteration
		PAYLOADS payloads = PAYLOADS::get_payload_ladder(*inp_args.stored_lls, clone_elist, m_chunk_border_sounds, false);
		int64_t curr_score = payloads.calculate_score();

		// mutex best
		{
			std::lock_guard<std::mutex> guard_best(*inp_args.mutex_best);

			bool is_new_best = false;
			// if its better than the previous best it gets stored in best_elist and score and zone are remembered
			if (curr_score < *inp_args.best_max_ptr)
			{
				for (int32_t j = 0; j < entry_count; j++)
					(*inp_args.best_elist)[j].m_chunk = clone_elist[j].m_chunk;
				*inp_args.best_max_ptr = curr_score;
				*inp_args.best_zone_ptr = payloads[0].zone;
				is_new_best = true;
			}

			// mutex iter
			{
				std::lock_guard<std::mutex> guard(*inp_args.mutex_iter);
				inp_args.worst_zones_info->update(payloads[0]);
				int32_t curr_iter = *inp_args.curr_iter_ptr;

				int64_t cr_max = *inp_args.best_max_ptr;
				while (cr_max / 100) cr_max /= 100;

				if (cr_max <= m_config[Rebuild_Payload_Limit])
					goal_reached = true;

				if (goal_reached && !is_new_best)
				{
					printf("Iter %3d, thr %2d, solution found by another thread, thread terminating\n", curr_iter, thr_id);
				}
				else if (goal_reached || is_new_best || *inp_args.curr_iter_ptr % 20 == 0)
				{
					if (m_config[Rebuild_Thread_Count] > 1)
					{
						printf("Iter %3d, thr %2d, current %lld (%5s), best %lld (%5s) %s\n",
							curr_iter, thr_id, curr_score, eid2str(payloads[0].zone),
							*inp_args.best_max_ptr, eid2str(*inp_args.best_zone_ptr),
							goal_reached ? "-- DONE" : (is_new_best ? "-- NEW BEST" : ""));
					}
					else
					{
						printf("Iter %3d, current %lld (%5s), best %lld (%5s) %s\n",
							curr_iter, curr_score, eid2str(payloads[0].zone),
							*inp_args.best_max_ptr, eid2str(*inp_args.best_zone_ptr),
							goal_reached ? "-- DONE" : (is_new_best ? "-- NEW BEST" : ""));
					}
				}
				*inp_args.curr_iter_ptr += 1;
			} // mutex iter
		} // mutex best
	}
}

// do random matrix merge, potentially threaded
void ELIST::matrix_merge_random_main()
{
	// asking user parameters for the method	
	ask_params_matrix();

	// thread count, mutexes
	int32_t curr_iter = 0;
	std::mutex iter_mutex{};
	std::mutex best_mutex{};

	// first half of matrix merge method, unchanged
	// this can be done once instead of every iteration since its the same every time
	merge_permaloaded();				// merge permaloaded entries' chunks as well as possible
	assign_primary_chunks_all();		// chunks start off having one entry per chunk

	// get occurence matrix
	int32_t entry_count = count();
	RELATIONS relation_array = get_occurence_array();

	// clone elists that store the current iteration and the best iretation
	ELIST best_elist{};
	best_elist.resize(entry_count);

	// for keeping track of the best found
	int64_t best_max = INT64_MAX;
	uint32_t best_zone = 0;

	MATRIX_STORED_LLS stored_lls = matrix_store_lls();
	WORST_ZONE_INFO wzi{};

	MTRX_THRD_IN_STR thr_arg{};
	thr_arg.best_elist = &best_elist;
	thr_arg.best_max_ptr = &best_max;
	thr_arg.best_zone_ptr = &best_zone;
	thr_arg.rel_array = &relation_array;
	thr_arg.curr_iter_ptr = &curr_iter;
	thr_arg.rnd_seed = m_config[Rebuild_Base_Seed];
	thr_arg.thread_idx = 0;
	thr_arg.mutex_best = &best_mutex;
	thr_arg.mutex_iter = &iter_mutex;
	thr_arg.stored_lls = &stored_lls;
	thr_arg.worst_zones_info = &wzi;

	if (m_config[Rebuild_Thread_Count] <= 1)
	{
		// one thread can just run directly
		matrix_merge_random_util(thr_arg);
	}
	else
	{
		std::vector<std::thread> threads{};
		for (int32_t i = 0; i < m_config[Rebuild_Thread_Count]; i++)
		{
			thr_arg.thread_idx++;
			thr_arg.rnd_seed++;
			threads.emplace_back([this, thr_arg] { matrix_merge_random_util(thr_arg); });
		}

		// wait for the threads to stop running
		for (auto& t : threads)
			t.join();
	}

	// copy in the best one
	for (int32_t i = 0; i < entry_count; i++)
	{
		at(i).m_chunk = best_elist[i].m_chunk;
		if (at(i).m_chunk >= m_chunk_count)
			m_chunk_count = at(i).m_chunk + 1;
	}

	wzi.print_summary();
	remove_empty_chunks();
	printf("\a"); // bell
}
