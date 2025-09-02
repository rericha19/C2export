#include "../include.h"
#include "../utils/payloads.hpp"
#include "../utils/utils.hpp"
#include "../utils/entry.hpp"
#include "../utils/elist.hpp"

typedef struct matrix_merge_thread_input_struct
{
	ELIST* elist;
	uint32_t* best_zone_ptr;
	int64_t* best_max_ptr;
	ELIST* best_elist;
	LIST* entrs;
	int32_t** entry_mtrx;
	int32_t* conf;
	int32_t max_pay;
	int32_t iter_cnt;
	int32_t* curr_iter_ptr;
	double mlt;
	int32_t rnd_seed;
	int32_t thread_idx;
	std::mutex* mutex_best;
	std::mutex* mutex_iter;
	MATRIX_STORED_LLS* stored_lls;
	WORST_ZONE_INFO* worst_zones_info;
} MTRX_THRD_IN_STR;

// asking user parameters for the method
void ask_params_matrix(double* mult, int32_t* iter_count, int32_t* seed, int32_t* max_payload_limit)
{
	printf("\nMax payload limit? (usually 21 or 20)\n");
	scanf("%d", max_payload_limit);
	printf("Max payload limit used: %d\n", *max_payload_limit);
	printf("\n");

	printf("Max iteration count? (0 for no limit)\n");
	scanf("%d", iter_count);
	if (*iter_count < 0)
		*iter_count = 100;
	printf("Max iteration count used: %d\n", *iter_count);
	printf("\n");

	printf("Randomness multiplier? (> 1, use 1.5 if not sure)\n");
	scanf("%lf", mult);
	if (*mult < 1.0)
	{
		*mult = 1.5;
		printf("Invalid multiplier, defaulting to 1.5\n");
	}
	printf("Multiplier used: %.2f\n", *mult);
	printf("\n");

	printf("Seed? (0 for random)\n");
	scanf("%d", seed);
	if (*seed == 0)
	{
		time_t raw_time;
		time(&raw_time);
		srand((uint32_t)raw_time);
		*seed = rand();
		printf("Seed used: %d\n", *seed);
	}
	printf("\n");
}

void build_matrix_merge_random_util(MTRX_THRD_IN_STR inp_args)
{
	bool limit_reached = false;
	bool best_reached = false;

	int32_t entry_count = inp_args.elist->count();

	ELIST clone_elist{};
	clone_elist.resize(entry_count);

	RELATIONS array_repr_untouched = build_transform_matrix(*inp_args.entrs, inp_args.entry_mtrx, *inp_args.elist);
	RELATIONS array_representation = build_transform_matrix(*inp_args.entrs, inp_args.entry_mtrx, *inp_args.elist);
	srand(inp_args.rnd_seed);
	int32_t iter_count = inp_args.iter_cnt;
	int32_t curr_i;
	int32_t t_id = inp_args.thread_idx;
	double mult = inp_args.mlt;

	while (1)
	{
		// check whether iter limit was reached
		inp_args.mutex_iter->lock();
		if (*inp_args.curr_iter_ptr > iter_count && iter_count != 0)
			limit_reached = true;
		inp_args.mutex_iter->unlock();

		if (limit_reached)
			break;

		// restore stuff
		for (int32_t j = 0; j < entry_count; j++)
			clone_elist[j] = inp_args.elist->at(j);
		memcpy(array_representation.relations.get(), array_repr_untouched.relations.get(), array_representation.count * sizeof(RELATION));

		// second half of the matrix merge slightly randomised and ran on clone elist
		for (int32_t j = 0; j < array_representation.count; j++)
			array_representation.relations[j].value = array_repr_untouched.relations[j].value * randfrom(1.0, mult);
		array_representation.do_sort();

		// do the merges according to the slightly randomised relation array
		build_matrix_merge_util(array_representation, clone_elist, 1.0);

		// get payload ladder for current iteration
		PAYLOADS payloads = PAYLOADS::get_payload_ladder(*inp_args.stored_lls, clone_elist, inp_args.elist->m_chunk_border_sounds, false);
		int64_t curr = payloads.calculate_score();

		inp_args.mutex_best->lock();
		bool is_new_best = false;
		// if its better than the previous best it gets stored in best_elist and score and zone are remembered
		if (curr < *inp_args.best_max_ptr)
		{
			for (int32_t j = 0; j < entry_count; j++)
				(*inp_args.best_elist)[j] = clone_elist[j];
			*inp_args.best_max_ptr = curr;
			*inp_args.best_zone_ptr = payloads[0].zone;
			is_new_best = true;
		}

		inp_args.mutex_iter->lock();
		inp_args.worst_zones_info->update(payloads[0]);
		curr_i = *inp_args.curr_iter_ptr;

		int64_t cr_max = *inp_args.best_max_ptr;
		while (cr_max / 100)
			cr_max /= 100;

		if (cr_max <= inp_args.max_pay)
			best_reached = true;

		if (best_reached)
		{
			if (is_new_best)
				printf("Iter %3d, thr %2d, current %lld (%5s), best %lld (%5s) -- DONE\n",
					curr_i, t_id, (long long)curr, eid2str(payloads[0].zone),
					*inp_args.best_max_ptr, eid2str(*inp_args.best_zone_ptr));
			else
				printf("Iter %3d, thr %2d, solution found by another thread, thread terminating\n", curr_i, t_id);
		}
		else
		{
			if (is_new_best)
				printf("Iter %3d, thr %2d, current %lld (%5s), best %lld (%5s) -- NEW BEST\n",
					curr_i, t_id, curr, eid2str(payloads[0].zone),
					*inp_args.best_max_ptr, eid2str(*inp_args.best_zone_ptr));
			else
				printf("Iter %3d, thr %2d, current %lld (%5s), best %lld (%5s)\n",
					curr_i, t_id, curr, eid2str(payloads[0].zone),
					*inp_args.best_max_ptr, eid2str(*inp_args.best_zone_ptr));
		}
		*inp_args.curr_iter_ptr += 1;

		inp_args.mutex_iter->unlock();
		inp_args.mutex_best->unlock();;

		if (best_reached)
			break;
	}
}

void build_matrix_merge_random_thr_main(ELIST& elist)
{
	// asking user parameters for the method
	double mult;
	int32_t iter_count, seed, max_payload_limit;
	ask_params_matrix(&mult, &iter_count, &seed, &max_payload_limit);

	// thread count, mutexes
	int32_t t_count = 0;
	int32_t curr_iter = 0;
	std::mutex running_mutex{};
	std::mutex iter_mutex{};
	std::mutex best_mutex{};

	printf("Thread count? (8 is default, 128 max)\n");
	scanf("%d", &t_count);
	if (t_count < 1)
		t_count = 8;
	if (t_count > 128)
		t_count = 128;
	printf("Thread count: %d\n\n", t_count);
	iter_count -= t_count;

	// first half of matrix merge method, unchanged
	// this can be done once instead of every iteration since its the same every time
	elist.merge_permaloaded();				// merge permaloaded entries' chunks as well as possible
	elist.assign_primary_chunks_all();		// chunks start off having one entry per chunk

	// get occurence matrix
	LIST entries = elist.get_normal_entries();

	int32_t entry_count = elist.count();
	int32_t** entry_matrix = build_get_occurence_matrix(elist, entries);

	// clone elists that store the current iteration and the best iretation
	ELIST best_elist{};
	best_elist.resize(entry_count);

	// for keeping track of the best found
	int64_t best_max = 9223372036854775807; // max signed 64b int
	uint32_t best_zone = 0;

	MATRIX_STORED_LLS stored_lls = build_matrix_store_lls(elist);
	WORST_ZONE_INFO wzi{};

	// declare, initialise and create threads, pass them necessary args thru structs	
	std::vector<std::thread> threads{};

	for (int32_t i = 0; i < t_count; i++)
	{
		MTRX_THRD_IN_STR thr_arg{};
		thr_arg.best_elist = &best_elist;
		thr_arg.best_max_ptr = &best_max;
		thr_arg.best_zone_ptr = &best_zone;
		thr_arg.elist = &elist;
		thr_arg.entrs = &entries;
		thr_arg.entry_mtrx = entry_matrix;
		thr_arg.conf = elist.m_config;
		thr_arg.iter_cnt = iter_count;
		thr_arg.curr_iter_ptr = &curr_iter;
		thr_arg.mlt = mult;
		thr_arg.rnd_seed = seed + i;
		thr_arg.thread_idx = i;
		thr_arg.mutex_best = &best_mutex;
		thr_arg.mutex_iter = &iter_mutex;
		thr_arg.max_pay = max_payload_limit;
		thr_arg.stored_lls = &stored_lls;
		thr_arg.worst_zones_info = &wzi;

		threads.emplace_back(build_matrix_merge_random_util, thr_arg);
	}

	// wait for the threads to stop running
	for (auto& t : threads)
	{
		t.join();
	}

	// copy in the best one
	for (int32_t i = 0; i < entry_count; i++)
	{
		elist[i] = best_elist[i];
		if (elist[i].m_chunk >= elist.m_chunk_count)
			elist.m_chunk_count = elist[i].m_chunk + 1;
	}

	wzi.print_summary();

	// cleanup
	for (int32_t i = 0; i < entries.count(); i++)
		free(entry_matrix[i]);
	free(entry_matrix);
	elist.remove_empty_chunks();
	printf("\a"); // bell
}

MATRIX_STORED_LLS build_matrix_store_lls(ELIST& elist)
{
	MATRIX_STORED_LLS stored_stuff{};
	int32_t entry_count = int32_t(elist.size());

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (elist[i].get_entry_type() != EntryType::Zone)
			continue;

		int32_t cam_count = elist[i].get_cam_item_count() / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			LOAD_LIST load_list = get_load_lists(elist[i], 2 + 3 * j);

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
					MATRIX_STORED_LL stored_load_list{ elist[i].m_eid, j };

					for (int32_t z = 0; z < list.count(); z++)
					{
						int32_t index = elist.get_index(list[z]);
						if (index != -1 && elist[index].is_normal_chunk_entry())
							stored_load_list.full_load.add(list[z]);
					}

					stored_stuff.push_back(stored_load_list);
				}
			}
		}
	}

	return stored_stuff;
}


void build_matrix_merge_random_main(ELIST& elist)
{
	int32_t entry_count = elist.count();

	double mult;
	int32_t iter_count, seed, max_payload_limit;
	ask_params_matrix(&mult, &iter_count, &seed, &max_payload_limit);
	srand(seed);

	// for keeping track of the best found
	int64_t best_max = 9223372036854775807; // max signed 64b int32_t
	uint32_t best_zone = 0;

	// clone elists that store the current iteration and the best iretation
	ELIST clone_elist{};
	ELIST best_elist{};
	clone_elist.resize(entry_count);
	best_elist.resize(entry_count);

	// first half of matrix merge method, unchanged
	// this can be done once instead of every iteration since its the same every time
	elist.merge_permaloaded();				// merge permaloaded entries' chunks as well as possible
	elist.assign_primary_chunks_all();      // chunks start off having one entry per chunk

	// get occurence matrix and convert to the relation array (not doing this each iteration makes it way faster)
	LIST entries = elist.get_normal_entries();

	int32_t** entry_matrix = build_get_occurence_matrix(elist, entries);

	// put the matrix's contents in an array and sort (so the matrix doesnt have to be searched every time)
	RELATIONS array_repr_untouched = build_transform_matrix(entries, entry_matrix, elist);
	RELATIONS array_representation = build_transform_matrix(entries, entry_matrix, elist);
	// matrix no longer used
	for (int32_t i = 0; i < entries.count(); i++)
		free(entry_matrix[i]);
	free(entry_matrix);

	MATRIX_STORED_LLS stored_lls = build_matrix_store_lls(elist);
	WORST_ZONE_INFO wzi{};

	// runs until iter count is reached or until break inside goes off (iter count 0 can make it never stop)
	for (int32_t i = 0; i < iter_count || iter_count == 0; i++)
	{
		// restore clone elist
		for (int32_t j = 0; j < entry_count; j++)
			clone_elist[j] = elist[j];

		// restore relations array so it can be tweaked again
		memcpy(array_representation.relations.get(), array_repr_untouched.relations.get(), array_representation.count * sizeof(RELATION));

		// second half of the matrix merge slightly randomised and ran on clone elist
		for (int32_t j = 0; j < array_representation.count; j++)
			array_representation.relations[j].value = array_repr_untouched.relations[j].value * randfrom(1.0, mult);
		array_representation.do_sort();

		// do the merges according to the slightly randomised relation array
		build_matrix_merge_util(array_representation, clone_elist, 1.0);

		// get payload ladder for current iteration
		PAYLOADS payloads = PAYLOADS::get_payload_ladder(stored_lls, clone_elist, elist.m_chunk_border_sounds, false);
		int64_t curr = payloads.calculate_score();
		wzi.update(payloads[0]);

		bool is_new_best = false;
		// if its better than the previous best it gets stored in best_elist and score and zone are remembered
		if (curr < best_max)
		{
			// copy clone elist into best elist
			for (int32_t j = 0; j < entry_count; j++)
				best_elist[j] = clone_elist[j];

			best_max = curr;
			best_zone = payloads[0].zone;
			is_new_best = true;
		}

		if (!is_new_best)
			printf("Iter %3d, current %lld (%5s), best %lld (%5s)\n",
				i, curr, eid2str(payloads[0].zone), best_max, eid2str(best_zone));
		else
			printf("Iter %3d, current %lld (%5s), best %lld (%5s) -- NEW BEST\n",
				i, curr, eid2str(payloads[0].zone), best_max, eid2str(best_zone));

		// if the worst payload zone has the same or lower payload than user specified max, it stops since its good enough
		if (payloads[0].page_count() <= max_payload_limit)
			break;
	}

	// copy best_elist into the real elist used by build_main, correct chunk_count
	for (int32_t i = 0; i < entry_count; i++)
	{
		elist[i] = best_elist[i];
		if (elist[i].m_chunk >= elist.m_chunk_count)
			elist.m_chunk_count = elist[i].m_chunk + 1;
	}

	wzi.print_summary();
	elist.remove_empty_chunks();
	printf("\a"); // bell
}
