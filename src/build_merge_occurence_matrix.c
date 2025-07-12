#include "macros.h"
// contains functions responsible for the current chunk merging algorithm implementation

/** \brief
 *  Main merge function.
 *  First merges the permaloaded entries, then using waren's matrix merge idea merges other normal chunk entries.
 *  Deprecate payload merge and dumb merge do not matter much, payload merge is for metrics, dumb merge is just
 *  in case some minor possible merge slipped thru the cracks.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param chunk_border_sounds int32_t       index of last sound chunk
 * \param chunk_count int32_t*              chunk count
 * \param config int32_t*                   config
 * \return void
 */
void build_matrix_merge_main(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, int32_t *config, LIST permaloaded)
{
    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded);                    // merge permaloaded entries' chunks as well as possible
    build_assign_primary_chunks_all(elist, entry_count, chunk_count);                                              // chunks start off having one entry per chunk
    build_matrix_merge(elist, entry_count, chunk_border_sounds, chunk_count, config, permaloaded, 1.0, 1.0);       // current best algorithm
    deprecate_build_payload_merge(elist, entry_count, chunk_border_sounds, chunk_count, PAYLOAD_MERGE_STATS_ONLY); // for payload printout, doesnt do much anymore
    build_dumb_merge(elist, chunk_border_sounds, chunk_count, entry_count);                                        // jic something didnt get merged it gets merged
}

void build_matrix_merge_relative_main(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, int32_t *config, LIST permaloaded)
{
    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded);
    build_assign_primary_chunks_all(elist, entry_count, chunk_count);
    build_matrix_merge_relative(elist, entry_count, chunk_border_sounds, chunk_count, config, permaloaded, 1.0);
    deprecate_build_payload_merge(elist, entry_count, chunk_border_sounds, chunk_count, PAYLOAD_MERGE_STATS_ONLY);
    build_dumb_merge(elist, chunk_border_sounds, chunk_count, entry_count);
}

/** \brief
 *  Assigns primary chunks to all entries, merges need them.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param chunk_count int32_t*              chunk count
 * \return int32_t                          number of entries that have been assigned a chunk in this function
 */
int32_t build_assign_primary_chunks_all(ENTRY *elist, int32_t entry_count, int32_t *chunk_count)
{
    int32_t counter = 0;
    for (int32_t i = 0; i < entry_count; i++)
        if (build_is_normal_chunk_entry(elist[i]) && elist[i].chunk == -1 && elist[i].norm_chunk_ent_is_loaded)
        {
            elist[i].chunk = (*chunk_count)++;
            counter++;
        }
    return counter;
}

// builds a triangular matrix that contains the count of common load list occurences of i-th and j-th entry
int32_t **build_get_occurence_matrix(ENTRY *elist, int32_t entry_count, LIST entries, int32_t *config)
{
    int32_t ll_pollin_flag = config[CNFG_IDX_MTRX_LL_POLL_FLAG];

    int32_t i, j, l, m;
    int32_t **entry_matrix = (int32_t **)malloc(entries.count * sizeof(int32_t *)); // freed by caller
    for (i = 0; i < entries.count; i++)
        entry_matrix[i] = (int32_t *)calloc((i), sizeof(int32_t)); // freed by caller

    // for each zone's each camera path gets load list and based on it increments values of common load list occurences of pairs of entries
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL)
        {
            int32_t cam_count = build_get_cam_item_count(elist[i].data) / 3;
            for (j = 0; j < cam_count; j++)
            {

                LOAD_LIST load_list = build_get_load_lists(elist[i].data, 2 + 3 * j);

                int32_t cam_offset = build_get_nth_item_offset(elist[i].data, 2 + 3 * j);
                int32_t cam_length = build_get_path_length(elist[i].data + cam_offset);

                LIST list = init_list();
                switch (ll_pollin_flag)
                {
                // per point (seems to generally be better), but its actually kinda fucky because if its not it gives worse results
                // if it were properly per point, the 'build_increment_common' function would use the 'counter' variable as 4th arg instead of 1
                case 2:
                case 1:
                {
                    int32_t sublist_index = 0;
                    int32_t counter = 0;
                    for (l = 0; l < cam_length; l++)
                    {
                        counter++;
                        if (load_list.array[sublist_index].index == l)
                        {
                            if (load_list.array[sublist_index].type == 'A')
                                for (m = 0; m < load_list.array[sublist_index].list_length; m++)
                                    list_add(&list, load_list.array[sublist_index].list[m]);

                            if (load_list.array[sublist_index].type == 'B')
                                for (m = 0; m < load_list.array[sublist_index].list_length; m++)
                                    list_remove(&list, load_list.array[sublist_index].list[m]);

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
                    for (l = 0; l < load_list.count; l++)
                    {
                        if (load_list.array[l].type == 'A')
                            for (m = 0; m < load_list.array[l].list_length; m++)
                                list_add(&list, load_list.array[l].list[m]);

                        if (load_list.array[l].type == 'B')
                            for (m = 0; m < load_list.array[l].list_length; m++)
                                list_remove(&list, load_list.array[l].list[m]);

                        if (list.count)
                            build_increment_common(list, entries, entry_matrix, 1);
                    }
                    break;
                }
                default:
                    break;
                }

                delete_load_list(load_list);
            }
        }
    return entry_matrix;
}

LIST build_get_normal_entry_list(ENTRY *elist, int32_t entry_count)
{

    LIST entries = init_list();
    // add all normal chunk entries to a new temporary array
    for (int32_t i = 0; i < entry_count; i++)
    {
        switch (build_entry_type(elist[i]))
        {
        case -1:
        case 5:
        case 6:
        case 12:
        case 14:
        case 20:
        case 21:
            continue;
        default:;
        }

        list_add(&entries, elist[i].eid);
    }
    return entries;
}

/** \brief
 *  Current best chunk merge method based on common load list occurence count of each pair of entries,
 *  therefore relies on proper and good load lists ideally with delta items.
 *  After the matrix is created and transformed into a sorted array it attemps to merge.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param chunk_border_sounds int32_t       unused
 * \param chunk_count int32_t*              chunk count
 * \param config int32_t*                   config values
 * \param permaloaded LIST              list of permaloaded entries
 * \param merge_ratio double            portion of entries to merge (used by premerges of method 3)
 * \param mult double                   max multiplier
 * \return void
 */
void build_matrix_merge(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, int32_t *config, LIST permaloaded, double merge_ratio, double mult)
{

    int32_t permaloaded_include_flag = config[CNFG_IDX_MTRX_PERMA_INC_FLAG];

    LIST entries = build_get_normal_entry_list(elist, entry_count);
    if (permaloaded_include_flag == 0)
        for (int32_t i = 0; i < permaloaded.count; i++)
            list_remove(&entries, permaloaded.eids[i]);

    int32_t **entry_matrix = build_get_occurence_matrix(elist, entry_count, entries, config);

    // put the matrix's contents in an array and sort (so the matrix doesnt have to be searched every time)
    // frees the contents of the matrix
    RELATIONS array_representation = build_transform_matrix(entries, entry_matrix, config, elist, entry_count);
    for (int32_t i = 0; i < entries.count; i++)
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
    build_matrix_merge_util(array_representation, elist, entry_count, entries, merge_ratio);

    free(array_representation.relations);
    *chunk_count = build_remove_empty_chunks(chunk_border_sounds, *chunk_count, entry_count, elist);
}

void build_matrix_merge_relative(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, int32_t *config, LIST permaloaded, double merge_ratio)
{

    int32_t permaloaded_include_flag = config[CNFG_IDX_MTRX_PERMA_INC_FLAG];

    LIST entries = build_get_normal_entry_list(elist, entry_count);
    if (permaloaded_include_flag == 0)
        for (int32_t i = 0; i < permaloaded.count; i++)
            list_remove(&entries, permaloaded.eids[i]);

    int32_t **entry_matrix = build_get_occurence_matrix(elist, entry_count, entries, config);

    int32_t *total_occurences = (int32_t *)malloc(entries.count * sizeof(int32_t)); // freed here
    for (int32_t i = 0; i < entries.count; i++)
    {
        int32_t temp_occurences = 0;
        for (int32_t j = 0; j < i; j++)
            temp_occurences += entry_matrix[i][j];
        for (int32_t j = i + 1; j < entries.count; j++)
            temp_occurences += entry_matrix[j][i];
        total_occurences[i] = temp_occurences;
    }

    int32_t **entry_matrix_relative = (int32_t **)malloc(entries.count * sizeof(int32_t *)); // freed here
    for (int32_t i = 0; i < entries.count; i++)
        entry_matrix_relative[i] = (int32_t *)calloc((i), sizeof(int32_t *)); // freed here

    // need int32_t in other functions, thats why its converted from float to int32_t
    for (int32_t i = 0; i < entries.count; i++)
    {
        for (int32_t j = 0; j < i; j++)
        {
            if (total_occurences[i] && total_occurences[j])
                entry_matrix_relative[i][j] = (int32_t)(100000000 * ((double)entry_matrix[i][j] / total_occurences[i] + (double)entry_matrix[i][j] / total_occurences[j]));
            else
                entry_matrix_relative[i][j] = 0;
        }
    }

    RELATIONS array_representation = build_transform_matrix(entries, entry_matrix_relative, config, elist, entry_count);
    for (int32_t i = 0; i < entries.count; i++)
    {
        free(entry_matrix[i]);
        free(entry_matrix_relative[i]);
    }
    free(entry_matrix);
    free(entry_matrix_relative);

    // do the merges according to the relation array, get rid of holes afterwards
    build_matrix_merge_util(array_representation, elist, entry_count, entries, merge_ratio);

    free(total_occurences);
    free(array_representation.relations);
    *chunk_count = build_remove_empty_chunks(chunk_border_sounds, *chunk_count, entry_count, elist);
}

/** \brief
 *  For each pair of entries it increments corresponding triangular matrix tile.
 *
 * \param list LIST                     current load list
 * \param entries LIST                  list of valid normal chunk entries
 * \param entry_matrix int32_t**            triangle matrix that contains amount of common load list occurences of entries[i], entries[j] on each tile [i][j]
 * \param rating int32_t                    increment value (used to properly consider all camera points without doing it for every single point)
 * \return void
 */
void build_increment_common(LIST list, LIST entries, int32_t **entry_matrix, int32_t rating)
{
    for (int32_t i = 0; i < list.count; i++)
        for (int32_t j = i + 1; j < list.count; j++)
        {
            int32_t indexA = list_find(entries, list.eids[i]);
            int32_t indexB = list_find(entries, list.eids[j]);

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

/** \brief
 *  For each entry pair it finds out what chunk each is in and attempts to merge.
 *  Starts with entries with highest common occurence count.
 *
 * \param relations RELATIONS           array form of the common load list matrix sorted high to low
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param entries LIST                  valid entries list
 * \param merge_ratio double            what portion of entries is to be merged
 * \return void
 */
void build_matrix_merge_util(RELATIONS relations, ENTRY *elist, int32_t entry_count, LIST entries, double merge_ratio)
{
    // for each relation it calculates size of the chunk that would be created by the merge
    // if the size is ok it merges, there are empty chunks afterwards that get cleaned up by a function called
    // after this function

    int32_t iter_count = min((int32_t)(relations.count * merge_ratio), relations.count);

    for (int32_t x = 0; x < iter_count; x++)
    {
        // awkward index shenanishans because entries in the matrix and later relation array werent indexed the same way
        // elist is, so theres the need to get index of the elist entry and then that one's chunk

        int32_t index1 = relations.relations[x].index1;
        int32_t index2 = relations.relations[x].index2;

        int32_t chunk_index1 = elist[index1].chunk;
        int32_t chunk_index2 = elist[index2].chunk;

        int32_t merged_chunk_size = 0x14;
        int32_t new_entry_count = 0;
        for (int32_t i = 0; i < entry_count; i++)
            if (elist[i].chunk == chunk_index1 || elist[i].chunk == chunk_index2)
            {
                merged_chunk_size += elist[i].esize + 4;
                new_entry_count++;
            }

        if (merged_chunk_size <= CHUNKSIZE && new_entry_count <= 125)
            for (int32_t i = 0; i < entry_count; i++)
                if (elist[i].chunk == chunk_index2)
                    elist[i].chunk = chunk_index1;
    }
}

/** \brief
 *  Creates an array representation of the common load list occurence matrix, sorts high to low.
 *  To prevent having to search the entire matrix every time in the main merge util thing.
 *
 * \param entries LIST                  valid entries list
 * \param entry_matrix int32_t**            matrix that contains common load list occurences
 * \return RELATIONS
 */
RELATIONS build_transform_matrix(LIST entries, int32_t **entry_matrix, int32_t *config, ENTRY *elist, int32_t entry_count)
{

    int32_t relation_array_sort_flag = config[CNFG_IDX_MTRI_REL_SORT_FLAG];
    int32_t zeroval_inc_flag = config[CNFG_IDX_MTRI_ZEROVAL_INC_FLAG];

    int32_t rel_counter = 0, i, j, indexer = 0;

    if (zeroval_inc_flag == 0)
    {
        for (i = 0; i < entries.count; i++)
            for (j = 0; j < i; j++)
                if (entry_matrix[i][j] != 0)
                    rel_counter++;
    }
    else
    {
        rel_counter = (entries.count * (entries.count - 1)) / 2;
    }

    RELATIONS relations;
    relations.count = rel_counter;
    relations.relations = (RELATION *)malloc(rel_counter * sizeof(RELATION)); // freed by caller

    for (i = 0; i < entries.count; i++)
        for (j = 0; j < i; j++)
        {
            if (zeroval_inc_flag == 0 && entry_matrix[i][j] == 0)
                continue;
            relations.relations[indexer].value = entry_matrix[i][j];
            relations.relations[indexer].index1 = build_get_index(entries.eids[i], elist, entry_count);
            relations.relations[indexer].index2 = build_get_index(entries.eids[j], elist, entry_count);

            // experimental
            int32_t temp = 0;
            for (int32_t k = 0; k < i; k++)
                temp += entry_matrix[i][k];
            for (int32_t k = 0; k < j; k++)
                temp += entry_matrix[j][k];
            relations.relations[indexer].total_occurences = temp;

            indexer++;
        }

    if (relation_array_sort_flag == 0)
        qsort(relations.relations, relations.count, sizeof(RELATION), relations_cmp); // the 'consistent' one
    if (relation_array_sort_flag == 1)
        qsort(relations.relations, relations.count, sizeof(RELATION), relations_cmp2);

    return relations;
}

/** \brief
 *  Specially merges permaloaded entries as they dont require any association.
 *  Works similarly to the sound chunk one.
 *  Biggest first sort packing algorithm kinda thing, seems to work pretty ok.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param chunk_border_sounds int32_t       index from which normal chunks start
 * \param chunk_count int32_t*              current chunk count
 * \param permaloaded LIST              list with permaloaded entries
 * \return int32_t                          permaloaded chunk count
 */
int32_t build_permaloaded_merge(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, LIST permaloaded)
{
    int32_t i, j;
    int32_t temp_count = *chunk_count;
    int32_t perma_normal_entry_count = 0;

    // find all permaloaded entries, add them to a temporary list, sort the list in descending order by size (biggest first)
    for (i = 0; i < entry_count; i++)
        if (list_find(permaloaded, elist[i].eid) != -1 && build_is_normal_chunk_entry(elist[i]))
            perma_normal_entry_count++;

    ENTRY *perma_elist = (ENTRY *)malloc(perma_normal_entry_count * sizeof(ENTRY)); // freed here
    int32_t indexer = 0;
    for (i = 0; i < entry_count; i++)
        if (list_find(permaloaded, elist[i].eid) != -1 && build_is_normal_chunk_entry(elist[i]))
            perma_elist[indexer++] = elist[i];

    qsort(perma_elist, perma_normal_entry_count, sizeof(ENTRY), cmp_func_esize);

    // keep putting them into existing chunks if they fit
    int32_t perma_chunk_count = perma_normal_entry_count;            // idrc about optimising this
    int32_t *sizes = (int32_t *)malloc(perma_chunk_count * sizeof(int32_t)); // freed here
    for (i = 0; i < perma_chunk_count; i++)
        sizes[i] = 0x14;

    // place where fits
    for (i = 0; i < perma_normal_entry_count; i++)
        for (j = 0; j < perma_chunk_count; j++)
            if (sizes[j] + 4 + perma_elist[i].esize <= CHUNKSIZE)
            {
                perma_elist[i].chunk = temp_count + j;
                sizes[j] += 4 + perma_elist[i].esize;
                break;
            }
    // to edit the array of entries that is actually used
    for (i = 0; i < entry_count; i++)
        for (j = 0; j < perma_normal_entry_count; j++)
            if (elist[i].eid == perma_elist[j].eid)
                elist[i].chunk = perma_elist[j].chunk;

    // counts used chunks
    int32_t counter = 0;
    for (i = 0; i < perma_chunk_count; i++)
        if (sizes[i] > 0x14)
            counter = i + 1;

    *chunk_count = temp_count + counter;
    free(perma_elist);
    free(sizes);
    return counter;
}

/** \brief
 *  If a chunk is empty it gets replaced with the last existing chunk.
 *  This goes on until no further replacements are done (no chunks are empty).
 *
 * \param index_start int32_t               start of the range of chunks to be fixed
 * \param index_end int32_t                 end of the range of chunks to be fixed (not in the range anymore)
 * \param entry_count int32_t               current amount of entries
 * \param entry_list ENTRY*             entry list
 * \return int32_t                          new chunk count
 */
int32_t build_remove_empty_chunks(int32_t index_start, int32_t index_end, int32_t entry_count, ENTRY *entry_list)
{
    int32_t i, j, is_occupied = 0, empty_chunk = 0;
    while (1)
    {
        for (i = index_start; i < index_end; i++)
        {
            is_occupied = 0;
            empty_chunk = -1;
            for (j = 0; j < entry_count; j++)
                if (entry_list[j].chunk == i)
                    is_occupied++;

            if (!is_occupied)
            {
                empty_chunk = i;
                break;
            }
        }

        if (empty_chunk == -1)
            break;

        for (i = 0; i < entry_count; i++)
            if (entry_list[i].chunk == index_end - 1)
                entry_list[i].chunk = empty_chunk;

        index_end--;
    }

    return index_end;
}

// used to sort relations that store how much entries are loaded simultaneously, sorts in descending order, also takes total occurence count into consideration (experimental)
int32_t relations_cmp2(const void *a, const void *b)
{
    RELATION x = *(RELATION *)a;
    RELATION y = *(RELATION *)b;

    if (y.value - x.value != 0)
        return y.value - x.value;

    return x.total_occurences - y.total_occurences;
}

// used to sort relations that store how much entries are loaded simultaneously, sorts in descending order
int32_t relations_cmp(const void *a, const void *b)
{
    RELATION x = *(RELATION *)a;
    RELATION y = *(RELATION *)b;

    return y.value - x.value;
}
