
#include "macros.h"

/** \brief
 *
 * \param elist ENTRY*
 * \param entry_count int
 * \param chunk_border_sounds int
 * \param chunk_count int*
 * \param config int*
 * \param permaloaded LIST
 *
 */
void build_matrix_merge_random_main(ENTRY *elist, int entry_count, int chunk_border_sounds, int *chunk_count, int* config, LIST permaloaded) {

    // asking user parameters for the method
    double mult = 1.5;
    int iter_count;
    int seed = 0;
    int max_payload_limit;

    printf("\nMax payload limit? (usually 21 or 20)\n");
    scanf("%d", &max_payload_limit);
    printf("\n");

    printf("Max iteration count? (0 for no limit)\n");
    scanf("%d", &iter_count);
    printf("\n");
    if (iter_count < 0)
        iter_count = 100;

    printf("Randomness multiplier? (> 1, use 1.5 if not sure)\n");
    scanf("%lf", &mult);
    printf("\n");

    printf("Seed? (0 for random)\n");
    scanf("%d", &seed);
    if (seed == 0) {
        time_t raw_time;
        time(&raw_time);
        srand(raw_time);
        seed = rand();
        printf("Seed used: %d\n", seed);
    }
    printf("\n");
    srand(seed);

    // clone elists that store the current iteration and the best iretation
    ENTRY clone_elist[ELIST_DEFAULT_SIZE];
    ENTRY best_elist[ELIST_DEFAULT_SIZE];

    // for keeping track of the best found
    long long int best_max = 9223372036854775807; // max signed 64b int
    unsigned int best_zone = 0;

    char temp[6] = "";
    char temp2[6] = "";

    // this probably has no impact but w/e
    for (int i = 0; i < entry_count; i++)
        best_elist[i] = elist[i];

    // first half of matrix merge method, unchanged
    // this can be done once instead of every iteration since its the same every time
    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded);                     // merge permaloaded entries' chunks as well as possible
    build_assign_primary_chunks_all(elist, entry_count, chunk_count);                                               // chunks start off having one entry per chunk

    // get occurence matrix and convert to the relation array (not doing this each iteration makes it way faster)
    int permaloaded_include_flag = config[CNFG_IDX_MTRX_PERMA_INC_FLAG];
    LIST entries = build_get_normal_entry_list(elist, entry_count);
    if (permaloaded_include_flag == 0)
        for (int i = 0; i < permaloaded.count; i++)
            list_remove(&entries, permaloaded.eids[i]);

    int **entry_matrix = build_get_occurence_matrix(elist, entry_count, entries, config);

    // put the matrix's contents in an array and sort (so the matrix doesnt have to be searched every time)
    RELATIONS array_repr_untouched = build_transform_matrix(entries, entry_matrix, config);
    RELATIONS array_representation = build_transform_matrix(entries, entry_matrix, config);
    // matrix no longer used
    for (int i = 0; i < entries.count; i++)
        free(entry_matrix[i]);
    free(entry_matrix);

    // runs until iter count is reached or until break inside goes off (iter count 0 can make it never stop)
    for (int i = 0; i < iter_count || iter_count == 0; i++) {

        // copy elist into clone elist
        for (int j = 0; j < entry_count; j++)
            clone_elist[j] = elist[j];

        // restore relations array so it can be tweaked again
        memcpy(array_representation.relations, array_repr_untouched.relations, array_representation.count * sizeof(RELATION));

        // second half of the matrix merge slightly randomised and ran on clone elist
        int dec_mult = (int) (1000 * mult);
        for (int i = 0; i < array_representation.count; i++)
            array_representation.relations[i].value = ((array_repr_untouched.relations[i].value * (double) ((rand() % dec_mult))) / 1000);
        qsort(array_representation.relations, array_representation.count, sizeof(RELATION), relations_cmp);

        // do the merges according to the relation array, get rid of holes afterwards
        build_matrix_merge_util(array_representation, clone_elist, entry_count, entries, 1.0);

        // get payload ladder for current iteration
        PAYLOADS payloads = deprecate_build_get_payload_ladder(clone_elist, entry_count, chunk_border_sounds);
        qsort(payloads.arr, payloads.count, sizeof(PAYLOAD), cmp_func_payload);

        // how many cam paths' payload it takes into consideration, max 8 but if the level is smaller it does less, also it could do 9 too but 8 is a nicer number
        int src_depth = min(payloads.count, 8);

        // gets current 'score', its multiplied by 100 instead of being bit shifted each iter so its legible in the console
        long long int curr = 0;
        for (int i = 0; i < src_depth; i++) {
            curr = curr * (long long int) 100;
            curr += (long long int) payloads.arr[i].count;
        }

        int is_new_best = 0;
        // if its better than the previous best it gets stored in best_elist and score and zone are remembered
        if (curr < best_max) {
            for (int j = 0; j < entry_count; j++)
                best_elist[j] = clone_elist[j];
            best_max = curr;
            best_zone = payloads.arr[0].zone;
            is_new_best = 1;
        }

        if (!is_new_best)
            printf("Iter %3d, current %I64d (%5s), best %I64d (%5s)\n", i, curr, eid_conv(payloads.arr[0].zone, temp), best_max, eid_conv(best_zone, temp2));
        else
            printf("Iter %3d, current %I64d (%5s), best %I64d (%5s) -- NEW BEST\n", i, curr, eid_conv(payloads.arr[0].zone, temp), best_max, eid_conv(best_zone, temp2));

        // if the worst payload zone has the same or lower payload than user specified max, it stops since its good enough
        if (payloads.arr[0].count <= max_payload_limit)
            break;
    }

    // copy best_elist into the real elist used by build_main, correct chunk_count
    for (int i = 0; i < entry_count; i++) {
        elist[i] = best_elist[i];
        if (elist[i].chunk >= *chunk_count)
            *chunk_count = elist[i].chunk + 1;
    }

    free(array_representation.relations);
    free(array_repr_untouched.relations);

    *chunk_count = build_remove_empty_chunks(chunk_border_sounds, *chunk_count, entry_count, elist);   // gets rid of empty chunks at the end
}



