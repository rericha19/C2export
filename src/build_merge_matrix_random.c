#include "macros.h"

#if COMPILE_WITH_THREADS

typedef struct matrix_merge_thread_input_struct
{
    ENTRY *elist;
    int32_t entry_count;
    uint32_t *best_zone_ptr;
    int64_t *best_max_ptr;
    ENTRY *best_elist;
    LIST entrs;
    int32_t **entry_mtrx;
    int32_t *conf;
    int32_t max_pay;
    int32_t iter_cnt;
    int32_t *curr_iter_ptr;
    double mlt;
    int32_t rnd_seed;
    int32_t thread_idx;
    int32_t *running_threads;
    pthread_mutex_t *mutex_running_thr_cnt;
    pthread_mutex_t *mutex_best;
    pthread_mutex_t *mutex_iter;
    int32_t chunk_border_sounds;
    MATRIX_STORED_LLS stored_lls;
    MERGE_WORST_ZONE_INFO *worst_zones_info;
} MTRX_THRD_IN_STR;
#endif // COMPILE_WITH_THREADS

// asking user parameters for the method
void ask_params_matrix(double *mult, int32_t *iter_count, int32_t *seed, int32_t *max_payload_limit)
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
        srand(raw_time);
        *seed = rand();
        printf("Seed used: %d\n", *seed);
    }
    printf("\n");
}

double randfrom(double min, double max)
{
    double range = (max - min);
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

#if COMPILE_WITH_THREADS
void *build_matrix_merge_random_util(void *args)
{
    char eid1[6] = "";
    char eid2[6] = "";
    bool limit_reached = false;
    bool best_reached = false;

    MTRX_THRD_IN_STR inp_args = *(MTRX_THRD_IN_STR *)args;
    ENTRY clone_elist[ELIST_DEFAULT_SIZE];
    int32_t entry_count = inp_args.entry_count;
    RELATIONS array_repr_untouched = build_transform_matrix(inp_args.entrs, inp_args.entry_mtrx, inp_args.conf, inp_args.elist, entry_count);
    RELATIONS array_representation = build_transform_matrix(inp_args.entrs, inp_args.entry_mtrx, inp_args.conf, inp_args.elist, entry_count);
    srand(inp_args.rnd_seed);
    int32_t iter_count = inp_args.iter_cnt;
    int32_t curr_i;
    int32_t t_id = inp_args.thread_idx;
    double mult = inp_args.mlt;

    while (1)
    {

        // check whether iter limit was reached
        pthread_mutex_lock(inp_args.mutex_iter);
        if (*inp_args.curr_iter_ptr > iter_count && iter_count != 0)
            limit_reached = true;
        pthread_mutex_unlock(inp_args.mutex_iter);

        if (limit_reached)
            break;

        // restore stuff
        for (int32_t j = 0; j < entry_count; j++)
            clone_elist[j] = inp_args.elist[j];
        memcpy(array_representation.relations, array_repr_untouched.relations, array_representation.count * sizeof(RELATION));

        // second half of the matrix merge slightly randomised and ran on clone elist
        for (int32_t j = 0; j < array_representation.count; j++)
            array_representation.relations[j].value = array_repr_untouched.relations[j].value * randfrom(1.0, mult);
        qsort(array_representation.relations, array_representation.count, sizeof(RELATION), relations_cmp);

        // do the merges according to the slightly randomised relation array
        build_matrix_merge_util(array_representation, clone_elist, entry_count, 1.0);

        // get payload ladder for current iteration
        PAYLOADS payloads = build_matrix_get_payload_ladder(inp_args.stored_lls, clone_elist, entry_count, inp_args.chunk_border_sounds, 0);
        qsort(payloads.arr, payloads.count, sizeof(PAYLOAD), cmp_func_payload);

        // how many cam paths' payload it takes into consideration, max 8 but if the level is smaller it does less, also it could do 9 too but 8 is a nicer number
        int32_t src_depth = min(payloads.count, 8);

        // gets current 'score', its multiplied by 100 instead of being bit shifted each iter so its legible in the console
        int64_t curr = 0;
        for (int32_t j = 0; j < src_depth; j++)
        {
            curr = curr * (int64_t)100;
            curr += (int64_t)payloads.arr[j].count;
        }

        pthread_mutex_lock(inp_args.mutex_best);
        bool is_new_best = false;
        // if its better than the previous best it gets stored in best_elist and score and zone are remembered
        if (curr < *inp_args.best_max_ptr)
        {
            for (int32_t j = 0; j < entry_count; j++)
                inp_args.best_elist[j] = clone_elist[j];
            *inp_args.best_max_ptr = curr;
            *inp_args.best_zone_ptr = payloads.arr[0].zone;
            is_new_best = true;
        }

        pthread_mutex_lock(inp_args.mutex_iter);
        build_update_worst_zones_info(inp_args.worst_zones_info, payloads.arr[0].zone, payloads.arr[0].count);
        curr_i = *inp_args.curr_iter_ptr;

        int64_t cr_max = *inp_args.best_max_ptr;
        for (int32_t j = 0; j < src_depth - 1; j++)
            cr_max = cr_max / 100;

        if (cr_max <= inp_args.max_pay)
            best_reached = true;

        if (best_reached)
        {
            if (is_new_best)
                printf("Iter %3d, thr %2d, current %I64d (%5s), best %I64d (%5s) -- DONE\n",
                       curr_i, t_id, curr, eid_conv(payloads.arr[0].zone, eid1), *inp_args.best_max_ptr, eid_conv(*inp_args.best_zone_ptr, eid2));
            else
                printf("Iter %3d, thr %2d, solution found by another thread, thread terminating\n", curr_i, t_id);
        }
        else
        {
            if (is_new_best)
                printf("Iter %3d, thr %2d, current %I64d (%5s), best %I64d (%5s) -- NEW BEST\n",
                       curr_i, t_id, curr, eid_conv(payloads.arr[0].zone, eid1), *inp_args.best_max_ptr, eid_conv(*inp_args.best_zone_ptr, eid2));
            else
                printf("Iter %3d, thr %2d, current %I64d (%5s), best %I64d (%5s)\n",
                       curr_i, t_id, curr, eid_conv(payloads.arr[0].zone, eid1), *inp_args.best_max_ptr, eid_conv(*inp_args.best_zone_ptr, eid2));
        }
        *inp_args.curr_iter_ptr += 1;

        pthread_mutex_unlock(inp_args.mutex_iter);
        pthread_mutex_unlock(inp_args.mutex_best);

        if (best_reached)
            break;
    }

    free(array_representation.relations);
    free(array_repr_untouched.relations);

    pthread_mutex_lock(inp_args.mutex_running_thr_cnt);
    (*inp_args.running_threads)--;
    pthread_mutex_unlock(inp_args.mutex_running_thr_cnt);
    pthread_exit(NULL);
    return (void *)NULL;
}

/** \brief
 *
 * \param elist ENTRY*
 * \param entry_count int32_t
 * \param chunk_border_sounds int32_t
 * \param chunk_count int32_t*
 * \param config int32_t*
 * \param permaloaded LIST
 *
 */
void build_matrix_merge_random_thr_main(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, int32_t *config, LIST permaloaded)
{

    // asking user parameters for the method
    double mult;
    int32_t iter_count, seed, max_payload_limit;
    ask_params_matrix(&mult, &iter_count, &seed, &max_payload_limit);

    // thread count, mutexes
    int32_t t_count = 0;
    int32_t curr_iter = 0;
    int32_t running_threads = 0;
    pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t iter_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t best_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded); // merge permaloaded entries' chunks as well as possible
    build_assign_primary_chunks_all(elist, entry_count, chunk_count);                           // chunks start off having one entry per chunk

    // get occurence matrix
    int32_t permaloaded_include_flag = config[CNFG_IDX_MTRX_PERMA_INC_FLAG];
    LIST entries = build_get_normal_entry_list(elist, entry_count);
    if (permaloaded_include_flag == 0)
        for (int32_t i = 0; i < permaloaded.count; i++)
            list_remove(&entries, permaloaded.eids[i]);

    int32_t **entry_matrix = build_get_occurence_matrix(elist, entry_count, entries, config);

    // clone elists that store the current iteration and the best iretation
    ENTRY best_elist[ELIST_DEFAULT_SIZE];
    // for keeping track of the best found
    int64_t best_max = 9223372036854775807; // max signed 64b int
    uint32_t best_zone = 0;

    MATRIX_STORED_LLS stored_lls = build_matrix_store_lls(elist, entry_count);
    MERGE_WORST_ZONE_INFO wzi;
    wzi.used_count = 0;

    // declare, initialise and create threads, pass them necessary args thru structs
    pthread_t *threads = (pthread_t *)malloc(t_count * sizeof(pthread_t));
    MTRX_THRD_IN_STR *thread_args = (MTRX_THRD_IN_STR *)malloc(t_count * sizeof(MTRX_THRD_IN_STR));

    for (int32_t i = 0; i < t_count; i++)
    {
        thread_args[i].best_elist = best_elist;
        thread_args[i].best_max_ptr = &best_max;
        thread_args[i].best_zone_ptr = &best_zone;
        thread_args[i].elist = elist;
        thread_args[i].entry_count = entry_count;
        thread_args[i].entrs = entries;
        thread_args[i].entry_mtrx = entry_matrix;
        thread_args[i].conf = config;
        thread_args[i].iter_cnt = iter_count;
        thread_args[i].curr_iter_ptr = &curr_iter;
        thread_args[i].mlt = mult;
        thread_args[i].rnd_seed = seed + i;
        thread_args[i].thread_idx = i;
        thread_args[i].running_threads = &running_threads;
        thread_args[i].mutex_running_thr_cnt = &running_mutex;
        thread_args[i].mutex_best = &best_mutex;
        thread_args[i].mutex_iter = &iter_mutex;
        thread_args[i].chunk_border_sounds = chunk_border_sounds;
        thread_args[i].max_pay = max_payload_limit;
        thread_args[i].stored_lls = stored_lls;
        thread_args[i].worst_zones_info = &wzi;

        pthread_mutex_lock(&running_mutex);
        running_threads++;
        pthread_mutex_unlock(&running_mutex);

        int32_t rc = pthread_create(&threads[i], NULL, build_matrix_merge_random_util, (void *)&thread_args[i]);
        if (rc)
            printf("Error while creating %d. thread - err %d\n", i, rc);
    }

    // wait for the threads to stop running
    while (running_threads > 0)
    {
        Sleep(50);
    }

    // copy in the best one
    for (int32_t i = 0; i < entry_count; i++)
    {
        elist[i] = best_elist[i];
        if (elist[i].chunk >= *chunk_count)
            *chunk_count = elist[i].chunk + 1;
    }

    qsort(wzi.infos, wzi.used_count, sizeof(MERGE_WORST_ZONE_INFO_SINGLE), cmp_worst_zone_info);
    printf("\nWorst zone average:\n");
    for (int32_t i = 0; i < wzi.used_count && i < 10; i++)
        printf("\t%s - %4dx, worst-avg payload %4.2f\n", eid_conv2(wzi.infos[i].zone), wzi.infos[i].count, ((double)wzi.infos[i].sum) / wzi.infos[i].count);

    // cleanup
    for (int32_t i = 0; i < stored_lls.count; i++)
        free(stored_lls.stored_lls[i].full_load.eids);
    free(stored_lls.stored_lls);
    for (int32_t i = 0; i < entries.count; i++)
        free(entry_matrix[i]);
    free(entry_matrix);
    free(threads);
    free(thread_args);
    *chunk_count = build_remove_empty_chunks(chunk_border_sounds, *chunk_count, entry_count, elist); // gets rid of empty chunks at the end
    printf("\07");
}
#endif // COMPILE_WITH_THREADS

MATRIX_STORED_LLS build_matrix_store_lls(ENTRY *elist, int32_t entry_count)
{
    MATRIX_STORED_LLS stored_stuff;
    stored_stuff.count = 0;
    stored_stuff.stored_lls = NULL;

    for (int32_t i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE || elist[i].data == NULL)
            continue;

        int32_t cam_count = build_get_cam_item_count(elist[i].data) / 3;
        for (int32_t j = 0; j < cam_count; j++)
        {
            LOAD_LIST load_list = build_get_load_lists(elist[i].data, 2 + 3 * j);

            LIST list = init_list();
            for (int32_t l = 0; l < load_list.count; l++)
            {
                if (load_list.array[l].type == 'A')
                    for (int32_t m = 0; m < load_list.array[l].list_length; m++)
                        list_add(&list, load_list.array[l].list[m]);

                if (load_list.array[l].type == 'B')
                    for (int32_t m = 0; m < load_list.array[l].list_length; m++)
                        list_remove(&list, load_list.array[l].list[m]);

                // for simultaneous loads and deloads
                if (l + 1 != load_list.count)
                    if (load_list.array[l].type == 'A' && load_list.array[l + 1].type == 'B')
                        if (load_list.array[l].index == load_list.array[l + 1].index)
                            continue;

                if (list.count > 0)
                {
                    int32_t stored_c = stored_stuff.count;
                    if (stored_c > 0)
                        stored_stuff.stored_lls = (MATRIX_STORED_LL *)realloc(stored_stuff.stored_lls, (stored_c + 1) * sizeof(MATRIX_STORED_LL));
                    else
                        stored_stuff.stored_lls = (MATRIX_STORED_LL *)malloc(sizeof(MATRIX_STORED_LL));

                    LIST new_l = init_list();
                    for (int32_t z = 0; z < list.count; z++)
                    {
                        int32_t index = build_get_index(list.eids[z], elist, entry_count);
                        if (index >= 0 && build_is_normal_chunk_entry(elist[index]))
                            list_add(&new_l, list.eids[z]);
                    }
                    stored_stuff.stored_lls[stored_c].full_load = new_l;
                    stored_stuff.stored_lls[stored_c].zone = elist[i].eid;
                    stored_stuff.stored_lls[stored_c].cam_path = j;
                    stored_stuff.count++;
                }
            }
            delete_load_list(load_list);
        }
    }

    return stored_stuff;
}

PAYLOADS build_matrix_get_payload_ladder(MATRIX_STORED_LLS stored_lls, ENTRY *elist, int32_t entry_count, int32_t chunk_min, int32_t get_tpages)
{
    PAYLOADS payloads;
    payloads.arr = NULL;
    payloads.count = 0;
    for (int32_t i = 0; i < stored_lls.count; i++)
    {
        MATRIX_STORED_LL curr_ll = stored_lls.stored_lls[i];
        PAYLOAD payload = build_get_payload(elist, entry_count, curr_ll.full_load, curr_ll.zone, chunk_min, get_tpages);
        payload.cam_path = curr_ll.cam_path;
        build_insert_payload(&payloads, payload);
    }
    return payloads;
}

/** \brief
 *
 * \param elist ENTRY*
 * \param entry_count int32_t
 * \param chunk_border_sounds int32_t
 * \param chunk_count int32_t*
 * \param config int32_t*
 * \param permaloaded LIST
 *
 */
void build_matrix_merge_random_main(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, int32_t *config, LIST permaloaded)
{
    char eid1[6] = "";
    char eid2[6] = "";

    double mult;
    int32_t iter_count, seed, max_payload_limit;
    ask_params_matrix(&mult, &iter_count, &seed, &max_payload_limit);
    srand(seed);

    // for keeping track of the best found
    int64_t best_max = 9223372036854775807; // max signed 64b int32_t
    uint32_t best_zone = 0;

    // clone elists that store the current iteration and the best iretation
    ENTRY clone_elist[ELIST_DEFAULT_SIZE];
    ENTRY best_elist[ELIST_DEFAULT_SIZE];

    // first half of matrix merge method, unchanged
    // this can be done once instead of every iteration since its the same every time
    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded); // merge permaloaded entries' chunks as well as possible
    build_assign_primary_chunks_all(elist, entry_count, chunk_count);                           // chunks start off having one entry per chunk

    // get occurence matrix and convert to the relation array (not doing this each iteration makes it way faster)
    int32_t permaloaded_include_flag = config[CNFG_IDX_MTRX_PERMA_INC_FLAG];
    LIST entries = build_get_normal_entry_list(elist, entry_count);
    if (permaloaded_include_flag == 0)
        for (int32_t i = 0; i < permaloaded.count; i++)
            list_remove(&entries, permaloaded.eids[i]);

    int32_t **entry_matrix = build_get_occurence_matrix(elist, entry_count, entries, config);

    // put the matrix's contents in an array and sort (so the matrix doesnt have to be searched every time)
    RELATIONS array_repr_untouched = build_transform_matrix(entries, entry_matrix, config, elist, entry_count);
    RELATIONS array_representation = build_transform_matrix(entries, entry_matrix, config, elist, entry_count);
    // matrix no longer used
    for (int32_t i = 0; i < entries.count; i++)
        free(entry_matrix[i]);
    free(entry_matrix);

    MATRIX_STORED_LLS stored_lls = build_matrix_store_lls(elist, entry_count);
    MERGE_WORST_ZONE_INFO wzi;
    wzi.used_count = 0;

    // runs until iter count is reached or until break inside goes off (iter count 0 can make it never stop)
    for (int32_t i = 0; i < iter_count || iter_count == 0; i++)
    {
        // copy elist into clone elist
        for (int32_t j = 0; j < entry_count; j++)
            clone_elist[j] = elist[j];

        // restore relations array so it can be tweaked again
        memcpy(array_representation.relations, array_repr_untouched.relations, array_representation.count * sizeof(RELATION));

        // second half of the matrix merge slightly randomised and ran on clone elist
        for (int32_t j = 0; j < array_representation.count; j++)
            array_representation.relations[j].value = array_repr_untouched.relations[j].value * randfrom(1.0, mult);
        qsort(array_representation.relations, array_representation.count, sizeof(RELATION), relations_cmp);

        // do the merges according to the slightly randomised relation array
        build_matrix_merge_util(array_representation, clone_elist, entry_count, 1.0);

        // get payload ladder for current iteration
        PAYLOADS payloads = build_matrix_get_payload_ladder(stored_lls, clone_elist, entry_count, chunk_border_sounds, 0);
        qsort(payloads.arr, payloads.count, sizeof(PAYLOAD), cmp_func_payload);
        build_update_worst_zones_info(&wzi, payloads.arr[0].zone, payloads.arr[0].count);

        // how many cam paths' payload it takes into consideration, max 8 but if the level is smaller it does less, also it could do 9 too but 8 is a nicer number
        int32_t src_depth = min(payloads.count, 8);

        // gets current 'score', its multiplied by 100 instead of being bit shifted each iter so its legible in the console
        int64_t curr = 0;
        for (int32_t j = 0; j < src_depth; j++)
        {
            curr = curr * (int64_t)100;
            curr += (int64_t)payloads.arr[j].count;
        }

        bool is_new_best = false;
        // if its better than the previous best it gets stored in best_elist and score and zone are remembered
        if (curr < best_max)
        {
            for (int32_t j = 0; j < entry_count; j++)
                best_elist[j] = clone_elist[j];
            best_max = curr;
            best_zone = payloads.arr[0].zone;
            is_new_best = true;
        }

        if (!is_new_best)
            printf("Iter %3d, current %I64d (%5s), best %I64d (%5s)\n",
                   i, curr, eid_conv(payloads.arr[0].zone, eid1), best_max, eid_conv(best_zone, eid2));
        else
            printf("Iter %3d, current %I64d (%5s), best %I64d (%5s) -- NEW BEST\n",
                   i, curr, eid_conv(payloads.arr[0].zone, eid1), best_max, eid_conv(best_zone, eid2));

        // if the worst payload zone has the same or lower payload than user specified max, it stops since its good enough
        if (payloads.arr[0].count <= max_payload_limit)
            break;
    }

    // copy best_elist into the real elist used by build_main, correct chunk_count
    for (int32_t i = 0; i < entry_count; i++)
    {
        elist[i] = best_elist[i];
        if (elist[i].chunk >= *chunk_count)
            *chunk_count = elist[i].chunk + 1;
    }

    qsort(wzi.infos, wzi.used_count, sizeof(MERGE_WORST_ZONE_INFO_SINGLE), cmp_worst_zone_info);
    printf("\nWorst zone average:\n");
    for (int32_t i = 0; i < wzi.used_count && i < 10; i++)
        printf("\t%s - %4dx, worst-avg payload %4.2f\n", eid_conv2(wzi.infos[i].zone), wzi.infos[i].count, ((double)wzi.infos[i].sum) / wzi.infos[i].count);

    for (int32_t i = 0; i < stored_lls.count; i++)
        free(stored_lls.stored_lls[i].full_load.eids);

    free(stored_lls.stored_lls);
    free(array_representation.relations);
    free(array_repr_untouched.relations);
    *chunk_count = build_remove_empty_chunks(chunk_border_sounds, *chunk_count, entry_count, elist); // gets rid of empty chunks at the end
    printf("\07");
}
