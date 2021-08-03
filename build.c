#include "macros.h"
// contains main build function that governs the process, as well as semi-misc functions
// necessary or used across various build files


/** \brief
 *  Merges chunks prioritising highest combined size.
 *  Does not consider anything else.
 *
 * \param elist ENTRY*                  entry list
 * \param chunk_index_start int         start of the range
 * \param chunk_index_end int*          end of the range
 * \param entry_count int               entry count
 * \return void
 */
void build_dumb_merge(ENTRY *elist, int chunk_index_start, int *chunk_index_end, int entry_count) {
    int i, j, k;
    while(1) {
        int merge_happened = 0;
        for (i = chunk_index_start; i < *chunk_index_end; i++) {
            int size1 = 0;
            int count1 = 0;

            for (j = 0; j < entry_count; j++)
                if (elist[j].chunk == i) {
                    size1 += elist[j].esize;
                    count1++;
                }

            int maxsize = 0;
            int maxentry_count = 0;

            for (j = i + 1; j < *chunk_index_end; j++) {
                int size2 = 0;
                int count2 = 0;
                for (k = 0; k < entry_count; k++)
                    if (elist[k].chunk == j) {
                        size2 += elist[k].esize;
                        count2++;
                    }

                if ((size1 + size2 + 4 * count1 + 4 * count2 + 0x14) <= CHUNKSIZE)
                    if (size2 > maxsize) {
                        maxsize = size2;
                        maxentry_count = j;
                    }
            }

            if (maxentry_count) {
                for (j = 0; j < entry_count; j++)
                    if (elist[j].chunk == maxentry_count) elist[j].chunk = i;
                merge_happened++;
            }
        }
        if (!merge_happened) break;
    }

    *chunk_index_end = build_remove_empty_chunks(chunk_index_start, *chunk_index_end, entry_count, elist);
}


/** \brief
 *  Just returns the model reference of an animation.
 *  Only looks at the first frame of the animation.
 *
 * \param anim unsigned char*           pointer to the data of the searched animation
 * \return unsigned int                 eid of the animation's model reference (as unsigned int)
 */


LIST build_get_models(unsigned char* animation) {
    LIST models = init_list();

    int item_count = build_item_count(animation);
    for (int i = 0; i < item_count; i++)
        list_add(&models, build_get_model(animation, i));

    return models;
}

unsigned int build_get_model(unsigned char *anim, int item) {
    int item_off = build_get_nth_item_offset(anim, item);
    return from_u32(anim + item_off + 0x10);
}

unsigned int build_get_zone_track(unsigned char *entry) {
    int item1off = build_get_nth_item_offset(entry, 0);
    int item2off = build_get_nth_item_offset(entry, 1);
    int item1len = item2off - item1off;

    unsigned int music_entry = from_u32(entry + item1off + C2_MUSIC_REF + item1len - 0x318);
    return music_entry;
}


/** \brief
 *  Used during reading from the folder to prevent duplicate texture chunks.
 *  Searches existing chunks (base level's chunks) and if the texture eid matches
 *  it returns the index of the matching chunk, else returns -1
 *
 * \param textr unsigned int            searched texture chunk eid
 * \param chunks unsigned char**        array of chunks to be searched, chunk is a 64kB array
 * \param index_end int                 end index of the chunk array, aka current chunk count
 * \return int                          index of a chunk that matches search, or -1
 */
int build_get_base_chunk_border(unsigned int textr, unsigned char **chunks, int index_end) {
    int i, retrn = -1;

    for (i = 0; i < index_end; i++)
        if (from_u32(chunks[i] + 4) == textr) retrn = i;

    return retrn;
}


/** \brief
 *  Searches the entry list looking for the specified eid.
 *  Binary search, entry list should be sorted by eid (ascending).
 *
 * \param eid unsigned int              searched eid
 * \param elist ENTRY*                  list of entries
 * \param entry_count int               amount of entries
 * \return int                          index of the searched eid or -1
 */
int build_get_index(unsigned int eid, ENTRY *elist, int entry_count) {
    int first = 0;
    int last = entry_count - 1;
    int middle = (first + last)/2;

    while (first <= last) {
        if (elist[middle].eid < eid)
            first = middle + 1;
        else if (elist[middle].eid == eid)
            return middle;
        else
            last = middle - 1;

        middle = (first + last)/2;
    }

    return -1;
}

/** \brief
 *  Searches the properties, finds the offset of the slst property,
 *  returns eid of the slst.
 *
 * \param item unsigned char*           camera entity data
 * \return unsigned int                 slst reference or 0 if theres no slst reference property
 */
unsigned int build_get_slst(unsigned char *item) {
    int i, offset = 0;
    for (i = 0; i < build_prop_count(item); i++)
        if ((from_u16(item + 0x10 + 8 * i)) == ENTITY_PROP_CAM_SLST)
            offset = OFFSET + from_u16(item + 0x10 + 8 * i + 2);

    if (offset) return from_u32(item + offset + 4);
        else return 0;
}

/** \brief
 *  Returns path length of the input item.
 *
 * \param item unsigned char*           item whose length it returns
 * \return unsigned int                 path length
 */
unsigned int build_get_path_length(unsigned char *item) {
    int i, offset = 0;
    for (i = 0; i < build_prop_count(item); i++)
        if ((from_u16(item + 0x10 + 8 * i)) == ENTITY_PROP_PATH)
            offset = OFFSET + from_u16(item + 0x10 + 8 * i + 2);

    if (offset) return from_u32(item + offset);
        else return 0;
}

/** \brief
 *  Gets zone's neighbour count.
 *
 * \param entry unsigned char*          entry data
 * \return int                          neighbour count
 */
int build_get_neighbour_count(unsigned char *entry) {
    int item1off = build_get_nth_item_offset(entry, 0);
    return entry[item1off + C2_NEIGHBOURS_START];
}

/** \brief
 *  Returns a list of neighbours the entry (zone) has.
 *
 * \param entry unsigned char*         entry data
 * \return LIST                        list containing neighbour eids
 */
LIST build_get_neighbours(unsigned char *entry) {
    int item1off = build_get_nth_item_offset(entry, 0);
    int count = entry[item1off + C2_NEIGHBOURS_START];

    LIST neighbours = init_list();
    for (int k = 0; k < count; k++) {
        int neighbour_eid = from_u32(entry + item1off + C2_NEIGHBOURS_START + 4 + 4 * k);
        list_add(&neighbours, neighbour_eid);
    }

    return neighbours;
}
/** \brief
 *  Gets zone's camera entity count.
 *
 * \param entry unsigned char*          entry data
 * \return int                          camera entity count (total count, not camera path count)
 */
int build_get_cam_item_count(unsigned char *entry) {
    int item1off = build_get_nth_item_offset(entry, 0);
    return entry[item1off + 0x188];
}


/** \brief
 *  Gets zone's regular entity count.
 *
 * \param entry unsigned char*          entry data
 * \return int                          entity count (not including camera entities)
 */
int build_get_entity_count(unsigned char *entry) {
    int item1off = build_get_nth_item_offset(entry, 0);
    return entry[item1off + 0x18C];
}

int build_item_count(unsigned char *entry) {
    return from_u32(entry + 0xC);
}

int build_prop_count(unsigned char *item) {
    return from_u32(item + 0xC);
}

/** \brief
 *  Returns type of an entry.
 *
 * \param entry ENTRY                   entry struct
 * \return int                          -1 if entry does not have data allocated, else real entry type
 */
int build_entry_type(ENTRY entry) {
    if (entry.data == NULL) return -1;
    return *(int *)(entry.data + 8);
}


void build_check_item_count(unsigned char *zone, int eid) {
    int item_count = build_item_count(zone);
    int cam_count = build_get_cam_item_count(zone);
    int entity_count = build_get_entity_count(zone);

    char temp[100] = "";
    if (item_count != (2 + cam_count + entity_count))
        printf("[warning] %s's item count (%d) doesn't match item counts in the first item (2 + %d + %d)\n",
               eid_conv(eid, temp), item_count, cam_count, entity_count);
}

DRAW_ITEM build_decode_draw_item(unsigned int value) {
    DRAW_ITEM temp;

    temp.neighbour_item_index = (value & 0xFF000000) >> 24;
    temp.ID = (value & 0xFFFF00) >> 8;
    temp.neighbour_zone_index = value & 0xFF;
    return temp;
}


/** \brief
 *  Returns value of the specified property. Only works on generic, single-value (4B length 4B value) properties.
 *
 * \param entity unsigned char*         entity data
 * \param prop_code int                 property to return
 * \return int                          property value
 */
int build_get_entity_prop(unsigned char *entity, int prop_code) {
    unsigned int i;
    for (i = 0; i < build_prop_count(entity); i++) {
        int code = from_u16(entity + 0x10 + 8 * i);
        int offset = from_u16(entity + 0x12 + 8 * i) + OFFSET;

        if (code == prop_code)
            return from_u32(entity + offset + 4);
    }

    return -1;
}


/** \brief
 *  Just calculates the amount of chunks in the provided nsf file.
 *
 * \param nsf FILE*                     provided nsf
 * \return int                          amount of chunks
 */
int build_get_chunk_count_base(FILE *nsf) {
    fseek(nsf, 0, SEEK_END);
    int result = ftell(nsf) / CHUNKSIZE;
    rewind(nsf);

    return result;
}

/** \brief
 *  Checks whether the entry is meant to be placed into a normal chunk.
 *
 * \param entry ENTRY                   entry to be tested
 * \return int                          1 if it belongs to a normal chunk, else 0
 */
int build_is_normal_chunk_entry(ENTRY entry) {
    int type = build_entry_type(entry);
    if (type == ENTRY_TYPE_ANIM ||
        type == ENTRY_TYPE_DEMO ||
        type == ENTRY_TYPE_ZONE ||
        type == ENTRY_TYPE_MODEL ||
        type == ENTRY_TYPE_SCENERY ||
        type == ENTRY_TYPE_SLST ||
        type == ENTRY_TYPE_DEMO ||
        type == ENTRY_TYPE_VCOL ||
        type == ENTRY_TYPE_MIDI ||
        type == ENTRY_TYPE_GOOL )
    return 1;

    return 0;
}

void build_cleanup_elist(ENTRY *elist, int entry_count) {
    for (int i = 0; i < entry_count; i++) {
        if (elist[i].data != NULL)
            free(elist[i].data);

        if (elist[i].related != NULL)
            free(elist[i].related);

        if (elist[i].visited != NULL)
            free(elist[i].visited);

        if (elist[i].distances != NULL)
            free(elist[i].distances);
    }
}


/** \brief
 *  Gets rid of some dynamically allocated stuff and closes files.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunks unsigned char**        built chunks
 * \param chunk_count int               chunk count
 * \return void
 */
void build_final_cleanup(ENTRY *elist, int entry_count, unsigned char **chunks, int chunk_count, FILE* nsfnew, FILE* nsd, DEPENDENCIES dep1, DEPENDENCIES dep2) {

    build_cleanup_elist(elist, entry_count);

    for (int i = 0; i < chunk_count; i++)
        free(chunks[i]);

    if (nsfnew != NULL)
        fclose(nsfnew);

    if (nsd != NULL)
        fclose(nsd);

    if (dep1.array != NULL)
        free(dep1.array);

    if (dep2.array != NULL)
        free(dep2.array);
}


// dumb thing for snow no or whatever convoluted level its configured for rn
// actually unused at the time
void build_get_box_count(ENTRY *elist, int entry_count) {
    int box_counter = 0;
    int nitro_counter = 0;
    int entity_counter = 0;
    for (int i = 0; i < entry_count; i++) {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE) {
            int entity_count = build_get_entity_count(elist[i].data);
            int camera_count = build_get_cam_item_count(elist[i].data);
            for (int j = 0; j < entity_count; j++) {
                unsigned char *entity = elist[i].data + build_get_nth_item_offset(elist[i].data, (2 + camera_count + j));
                int type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
                int subt = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);
                int id = build_get_entity_prop(entity, ENTITY_PROP_ID);

                if (id == -1)
                    continue;

                entity_counter++;

                if ((type >= 34 && type <= 43) &&
                    (subt == 0 || subt == 2 || subt == 3 || subt == 4 || subt == 6 || subt == 8 || subt == 9 ||
                     subt == 10 || subt == 11 || subt == 18 || subt == 23 || subt == 25 || subt == 26))
                {
                    box_counter++;
                    //printf("Zone: %5s, type: %2d, subtype: %2d, ID: %3d\n", eid_conv(elist[i].eid, temp), type, subt, id);
                }

                if ((type >= 34 && type <= 43) && subt == 18) {
                    nitro_counter++;
                    //printf("NITRO %3d\n", id);
                }
            }
        }
    }
    printf("BOX COUNT:    %3d\n", box_counter);
    printf("NITRO COUNT:  %3d\n", nitro_counter);
}


// used to sort load lists and to avoid stuff getting removed before its been added
int cmp_func_load(const void *a, const void *b)
{
    LOAD x = *(LOAD *) a;
    LOAD y = *(LOAD *) b;

    if (x.index != y.index)
        return (x.index - y.index);
    else
        return (x.type - y.type);
}

// used to sort draw list to avoid stuff getting removed before its been added
int cmp_func_load2(const void *a, const void *b)
{
    LOAD x = *(LOAD *) a;
    LOAD y = *(LOAD *) b;

    if (x.index != y.index)
        return (x.index - y.index);
    else
        return (y.type - x.type);
}

LOAD_LIST build_get_draw_lists(unsigned char *entry, int cam_index) {

    LOAD_LIST temp = build_get_lists(ENTITY_PROP_CAM_DRAW_LIST_A, entry, cam_index);
    qsort(temp.array, temp.count, sizeof(LOAD), cmp_func_load2);
    return temp;
}

LOAD_LIST build_get_load_lists(unsigned char *entry, int cam_index) {

    LOAD_LIST temp = build_get_lists(ENTITY_PROP_CAM_LOAD_LIST_A, entry, cam_index);
    qsort(temp.array, temp.count, sizeof(LOAD), cmp_func_load);
    return temp;
}


/** \brief
 *  Deconstructs the load or draw lists and saves into a convenient struct.
 *
 * \param prop_code int                 first of the two list properties (either 0x13B or 0x208)
 * \param entry unsigned char*          entry data
 * \param cam_offset int                offset of the camera item
 * \return LOAD_LIST                    load or draw list struct
 */
LOAD_LIST build_get_lists(int prop_code, unsigned char *entry, int cam_index) {
    int k, l;
    LOAD_LIST load_list = init_load_list();

    int cam_offset = build_get_nth_item_offset(entry, cam_index);
    int prop_count = from_u32(entry + cam_offset + OFFSET);

    for (k = 0; (unsigned) k < prop_count; k++) {
        int code = from_u16(entry + cam_offset + 0x10 + 8 * k);
        int offset = from_u16(entry + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
        int list_count = from_u16(entry + cam_offset + 0x16 + 8 * k);

        int next_offset = (k + 1 < prop_count)
            ? (from_u16(entry + cam_offset + 0x12 + 8 * (k + 1)) + OFFSET + cam_offset)
            : (from_u16(entry + cam_offset) + cam_offset);
        int prop_length = next_offset - offset;

        if (code == prop_code || code == prop_code + 1) {
            int sub_list_offset;
            int load_list_item_count;
            int condensed_check1 = 0;
            int condensed_check2 = 0;

            // i should use 0x40 flag in prop header[4] but my spaghetti doesnt work with it so this thing stays here
            int potentially_condensed_length = from_u16(entry + offset) * 4 * list_count;
            potentially_condensed_length += 2 + 2 * list_count;
            if (from_u16(entry + offset + 2 + 2 * list_count) == 0)
                potentially_condensed_length += 2;
            if (potentially_condensed_length == prop_length && list_count > 1)
                condensed_check1 = 1;


            int len = 4 * list_count;
            for (l = 0; l < list_count; l++)
                len += from_u16(entry + offset + l * 2) * 4;
            if (len != prop_length)
                condensed_check2 = 1;

            if (condensed_check1 && condensed_check2) {

                load_list_item_count = from_u16(entry + offset);
                unsigned short int *indices = (unsigned short int *) malloc(list_count * sizeof(unsigned short int));
                memcpy(indices, entry + offset + 2, list_count * 2);
                sub_list_offset = offset + 2 + 2 * list_count;
                if (sub_list_offset % 4)
                    sub_list_offset += 2;
                for (l = 0; l < list_count; l++) {
                    load_list.array[load_list.count].list_length = load_list_item_count;
                    load_list.array[load_list.count].list = (unsigned int *)
                            malloc(load_list_item_count * sizeof(unsigned int)); // freed by caller using delete_load_list
                    memcpy(load_list.array[load_list.count].list, entry + sub_list_offset, load_list_item_count * sizeof(unsigned int));
                    if (code == prop_code)
                        load_list.array[load_list.count].type = 'A';
                    else
                        load_list.array[load_list.count].type = 'B';
                    load_list.array[load_list.count].index = indices[l];
                    load_list.count++;
                    sub_list_offset += load_list_item_count * 4;
                }
            } else {
                sub_list_offset = offset + 4 * list_count;;
                for (l = 0; l < list_count; l++) {
                    load_list_item_count = from_u16(entry + offset + l * 2);
                    int index = from_u16(entry + offset + l * 2 + list_count * 2);

                    load_list.array[load_list.count].list_length = load_list_item_count;
                    load_list.array[load_list.count].list = (unsigned int *)
                            malloc(load_list_item_count * sizeof(unsigned int)); // freed by caller using delete_load_list
                    memcpy(load_list.array[load_list.count].list, entry + sub_list_offset, load_list_item_count * sizeof(unsigned int));
                    if (code == prop_code)
                        load_list.array[load_list.count].type = 'A';
                    else
                        load_list.array[load_list.count].type = 'B';
                    load_list.array[load_list.count].index = index;
                    load_list.count++;
                    sub_list_offset += load_list_item_count * 4;
                }
            }
        }
    }

    return load_list;
}


/** \brief
 *  Reads nsf, reads folder, collects relatives, assigns proto chunks, calls some merge functions, makes load lists, makes nsd, makes nsf, end.
 *
 * \param build_rebuild_flag            build or rebuild
 * \return void
 */
void build_main(int build_rebuild_flag) {
    FILE *nsfnew = NULL, *nsd = NULL;               // file pointers for input nsf, output nsf (nsfnew) and output nsd
    SPAWNS spawns = init_spawns();                  // struct with spawns found during reading and parsing of the level data
    ENTRY elist[ELIST_DEFAULT_SIZE];                // array of structs used to store entries, fixed length cuz lazy & struct is small
    unsigned char *chunks[CHUNK_LIST_DEFAULT_SIZE]; // array of pointers to potentially built chunks, fixed length cuz lazy
    LIST permaloaded;                               // list containing eids of permaloaded entries provided by the user
    DEPENDENCIES subtype_info = build_init_dep();   // struct containing info about dependencies of certain types and subtypes
    DEPENDENCIES collisions = build_init_dep();     // struct containing info about dependencies of certain collision types
    DEPENDENCIES mus_dep = build_init_dep();
    int level_ID = 0;                               // level ID, used for naming output files and needed in output nsd

    // used to keep track of counts and to separate groups of chunks
    //int chunk_border_base       = 0;
    int chunk_border_texture    = 0,
        chunk_border_sounds     = 0,
        chunk_count             = 0,
        entry_count_base        = 0,
        entry_count             = 0;

    unsigned int gool_table[C2_GOOL_TABLE_SIZE];                // table w/ eids of gool entries, needed for nsd, filled using input entries
    for (int i = 0; i < C2_GOOL_TABLE_SIZE; i++)
        gool_table[i] = EID_NONE;

    // config:
    int config[] = {
        0,  // 0 - gool initial merge flag      0 - group       |   1 - one by one                          set here, used by deprecate merges
        0,  // 1 - zone initial merge flag      0 - group       |   1 - one by one                          set here, used by deprecate merges
        1,  // 2 - merge type value             0 - per delta   |   1 - weird per point |   2 - per point   set here, used by matrix merge
        0,  // 3 - slst distance value              set by user in function build_ask_distances(config); affects load lists
        0,  // 4 - neighbour distance value         set by user in function build_ask_distances(config); affects load lists
        0,  // 5 - draw list distance value         set by user in function build_ask_distances(config); affects load lists
        0,  // 6 - transition pre-load flag         set by user in function build_ask_distances(config); affects load lists
        0,  // 7 - backwards penalty value          set by user in function build_ask_dist...;  aff LLs     is 1M times the float value because int, range 0 - 0.5
        0,  // 8 - relation array sort flag     0 - regular     |   1 - also sort by total occurence count; set here, used by matrix merge (1 is kinda meh)
        0,  // 9 - sound entry load list flag   0 - all sounds  |   1 - one sound per sound chunk           set here, affects load lists
        0,  //10 - load list remake flag        0 - dont remake |   1 - remake load lists                   set by user in build_ask_build_flags
        0,  //11 - merge technique value                                                                    set by user in build_ask_build_flags
        1,  //12 - perma inc. in matrix flag    0 - dont include|   1 - do include                          set here, used by matrix merges
        1   //13 - inc. 0-vals in relarray flag 0 - dont include|   1 - do include                          set here, used by matrix merges
    };

    int input_parse_rtrn_value = 1;
    // reading contents of the nsf/folder and collecting metadata
    if (build_rebuild_flag == FUNCTION_BUILD)
        input_parse_rtrn_value = build_read_and_parse_build(&level_ID, &nsfnew, &nsd, &chunk_border_texture, gool_table, elist, &entry_count, chunks, &spawns);

    // reading contents of the nsf to be rebuilt and collecting metadata in a matter identical to 'build' procedure
    if (build_rebuild_flag == FUNCTION_REBUILD)
        input_parse_rtrn_value = build_read_and_parse_rebld(&level_ID, &nsfnew, &nsd, &chunk_border_texture, gool_table, elist, &entry_count, chunks, &spawns, 0);

    chunk_count = chunk_border_texture;

    // end if something went wrong
    if (input_parse_rtrn_value) {
        build_final_cleanup(elist, entry_count, chunks, chunk_count, nsfnew, nsd, subtype_info, collisions);
        return;
    }

    //build_get_box_count(elist, entry_count);

    // user picks whether to remake load lists or not, also merge method
    build_ask_build_flags(config);
    int load_list_flag = config[CNFG_IDX_LL_REMAKE_FLAG];
    int merge_tech_flag = config[CNFG_IDX_MERGE_METHOD_VALUE];

    // let the user pick the spawn, according to the spawn determine for each cam path its distance from spawn in terms of path links,
    // which is later used to find out which of 2 paths is in the backwards direction and, where backwards loading penalty should be applied
    // during the load list generation procedure
    if (spawns.spawn_count > 0)
        build_ask_spawn(spawns);
    else {
        printf("[ERROR] No spawns found, add one using the usual 'willy' entity or a checkpoint\n\n");
        build_final_cleanup(elist, entry_count, chunks, chunk_count, nsfnew, nsd, subtype_info, collisions);
        return;
    }
    build_get_distance_graph(elist, entry_count, spawns);

    // gets model references from gools, was useful in a deprecate chunk merging/building algorithm, but might be useful later and barely takes any time so idc
    build_get_model_references(elist, entry_count);
    build_remove_invalid_references(elist, entry_count, entry_count_base);

    // builds instrument and sound chunks, chunk_border_sounds is used to make chunk merging and chunk building more convenient, especially in deprecate methods
    build_instrument_chunks(elist, entry_count, &chunk_count, chunks);
    build_sound_chunks(elist, entry_count, &chunk_count, chunks);
    chunk_border_sounds = chunk_count;

    qsort(elist, entry_count, sizeof(ENTRY), cmp_func_eid);

    // ask user paths to files with permaloaded entries, type/subtype dependencies and collision type dependencies,
    // parse files and store info in permaloaded, subtype_info and collisions structs
    if (!build_read_entry_config(&permaloaded, &subtype_info, &collisions, &mus_dep, elist, entry_count, gool_table, config)) {
        printf("[ERROR] File could not be opened or a different error occured\n\n");
        build_final_cleanup(elist, entry_count, chunks, chunk_count, nsfnew, nsd, subtype_info, collisions);
        return;
    }

    /*for (int i = 0; i < entry_count; i++)
        list_add(&permaloaded, elist[i].eid);*/

    if (load_list_flag == 1) {
        // print for the user, informs them about entity type/subtypes that have no dependency list specified
        build_find_unspecified_entities(elist, entry_count, subtype_info);

        // ask user desired distances for various things, eg how much in advance in terms of camera rail distance things get loaded
        // there are some restrictions in terms of path link depth so its not entirely accurate, but it still matters
        build_ask_distances(config);

        // build load lists based on user input and metadata, and already or not yet collected metadata
        printf("Permaloaded len: %d\n", permaloaded.count);
        build_remake_load_lists(elist, entry_count, gool_table, permaloaded, subtype_info, collisions, mus_dep, config);
    }

    clock_t time_start = clock();
    // call merge function
    switch(merge_tech_flag) {
        case 4:
            build_matrix_merge_random_main(elist, entry_count, chunk_border_sounds, &chunk_count, config, permaloaded);
            break;
        case 3:
            build_merge_state_search_main(elist, entry_count, chunk_border_sounds, &chunk_count, config, permaloaded);
            break;
        case 2:
            deprecate_build_payload_merge_main(elist, entry_count, chunk_border_sounds, &chunk_count, config, permaloaded);
            break;
        case 1:
            build_matrix_merge_relative_main(elist, entry_count, chunk_border_sounds, &chunk_count, config, permaloaded);
            break;
        case 0:
        default:
            build_matrix_merge_main(elist, entry_count, chunk_border_sounds, &chunk_count, config, permaloaded);
            break;
    }
    printf("Merge took %.3fs\n", ((double) clock() - time_start) / CLOCKS_PER_SEC);

    // build and write nsf and nsd file
    build_write_nsd(nsd, elist, entry_count, chunk_count, spawns, gool_table, level_ID);
    build_sort_load_lists(elist, entry_count);
    build_write_nsf(nsfnew, elist, entry_count, chunk_border_sounds, chunk_count, chunks);

    // get rid of at least some dynamically allocated memory, p sure there are leaks all over the place but oh well
    build_final_cleanup(elist, entry_count, chunks, chunk_count, nsfnew, nsd, subtype_info, collisions);
    printf("Done. It is recommended to save NSD & NSF couple times with CrashEdit, e.g. 0.2.135.2 (or higher),\notherwise the level might not work.\n\n");
}
