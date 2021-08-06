
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

    // this can be done once instead of every iteration since its the same every time
    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded);                     // merge permaloaded entries' chunks as well as possible

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
    printf("\n");
    if (seed == 0) {
        time_t raw_time;
        time(&raw_time);
        srand(raw_time);
        seed = rand();
        printf("Seed used: %d\n", seed);
    }
    srand(seed);

    // clone elists that store the current iteration and the best iretation
    ENTRY clone_elist[ELIST_DEFAULT_SIZE];
    ENTRY best_elist[ELIST_DEFAULT_SIZE];

    // for keeping track of the best found
    long long int best_max = 9223372036854775807; // max signed 64b int
    unsigned int best_zone = 0;

    int tmp_chunk_count;    // clone chunk count
    char temp[6] = "";
    char temp2[6] = "";

    // this probably has no impact but w/e
    for (int i = 0; i < entry_count; i++) {
        best_elist[i] = elist[i];
    }

    // runs until iter count is reached or until break inside goes off (iter count 0 can make it never stop)
    for (int i = 0; i < iter_count || iter_count == 0; i++) {

        // copy elist into clone elist, copy chunk count to clone chunk count
        for (int j = 0; j < entry_count; j++)
            clone_elist[j] = elist[j];
        tmp_chunk_count = *chunk_count;

        // run slightly random absolute matrix merge on the clone elist
        // idk why assigning primary chunks has to happen here but it didnt build properly when it wasnt done here
        // dumb merge could be probably done at the end but it takes almost no time so idc
        build_assign_primary_chunks_all(clone_elist, entry_count, &tmp_chunk_count);                                               // chunks start off having one entry per chunk
        build_matrix_merge(clone_elist, entry_count, chunk_border_sounds, &tmp_chunk_count, config, permaloaded, 1.0, mult);       // current best algorithm + some random
        build_dumb_merge(clone_elist, chunk_border_sounds, &tmp_chunk_count, entry_count);                                         // jic something didnt get merged it gets merged

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

        printf("Iter %3d, current %I64d (%5s), best %I64d (%5s)", i, curr, eid_conv(payloads.arr[0].zone, temp), best_max, eid_conv(best_zone, temp2));
        if (is_new_best)
            printf(" -- NEW BEST");
        printf("\n");

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
}



