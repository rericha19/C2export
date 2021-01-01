#include "macros.h"


/** \brief
 *  Just returns the model reference of an animation.
 *  Only looks at the first frame of the animation.
 *
 * \param anim unsigned char*           pointer to the data of the searched animation
 * \return unsigned int                 EID of the animation's model reference (as unsigned int)
 */
unsigned int build_get_model(unsigned char *anim) {
    return from_u32(anim + 0x10 + from_u32(anim + 0x10));
}


/** \brief
 *  Used during reading from the folder to prevent duplicate texture chunks.
 *  Searches existing chunks (base level's chunks) and if the texture EID matches
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
 *  Searches the entry list looking for the specified EID.
 *  Binary search, entry list should be sorted by EID (ascending).
 *
 * \param eid unsigned int              searched EID
 * \param elist ENTRY*                  list of entries
 * \param entry_count int               amount of entries
 * \return int                          index of the searched EID or -1
 */
int build_get_index(unsigned int eid, ENTRY *elist, int entry_count) {
    int first = 0;
    int last = entry_count - 1;
    int middle = (first + last)/2;

    while (first <= last) {
        if (elist[middle].EID < eid)
            first = middle + 1;
        else if (elist[middle].EID == eid)
            return middle;
        else
            last = middle - 1;

        middle = (first + last)/2;
    }

    return -1;
}

/** \brief
 *  Searches the properties, finds the offset of the slst property,
 *  returns EID of the slst.
 *
 * \param item unsigned char*           camera entity data
 * \return unsigned int                 slst reference or 0 if theres no slst reference property
 */
unsigned int build_get_slst(unsigned char *item) {
    int i, offset = 0;
    for (i = 0; i < item[0xC]; i++)
        if ((from_u16(item + 0x10 + 8 * i)) == ENTITY_PROP_CAM_SLST)
            offset = 0xC + from_u16(item + 0x10 + 8 * i + 2);

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
    for (i = 0; i < item[0xC]; i++)
        if ((from_u16(item + 0x10 + 8 * i)) == ENTITY_PROP_PATH)
            offset = 0xC + from_u16(item + 0x10 + 8 * i + 2);

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
    int item1off = from_u32(entry + 0x10);
    return entry[item1off + C2_NEIGHBOURS_START];
}

/** \brief
 *  Returns a list of neighbours the entry (zone) has.
 *
 * \param entry unsigned char*         entry data
 * \return LIST                        list containing neighbour eids
 */
LIST build_get_neighbours(unsigned char *entry) {
    int item1off = from_u32(entry + 0x10);
    int count = entry[item1off + C2_NEIGHBOURS_START];

    LIST neighbours = init_list();
    for (int k = 0; k < count; k++) {
        int neighbour_eid = from_u32(entry + item1off + C2_NEIGHBOURS_START + 4 + 4 * k);
        list_insert(&neighbours, neighbour_eid);
    }

    return neighbours;
}
/** \brief
 *  Gets zone's camera entity count.
 *
 * \param entry unsigned char*          entry data
 * \return int                          camera entity count (total count, not camera path count)
 */
int build_get_cam_count(unsigned char *entry) {
    int item1off = from_u32(entry + 0x10);
    return entry[item1off + 0x188];
}


/** \brief
 *  Gets zone's regular entity count.
 *
 * \param entry unsigned char*          entry data
 * \return int                          entity count (not including camera entities)
 */
int build_get_entity_count(unsigned char *entry) {
    int item1off = from_u32(entry + 0x10);
    return entry[item1off + 0x18C];
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


void build_check_item_count(unsigned char *zone, int EID) {
    int item_count = from_u32(zone + 0xC);
    int cam_count = build_get_cam_count(zone);
    int entity_count = build_get_entity_count(zone);

    char temp[100];
    if (item_count != (2 + cam_count + entity_count))
        printf("[warning] %s's item count (%d) doesn't match item counts in the first item (2 + %d + %d)\n",
               eid_conv(EID, temp), item_count, cam_count, entity_count);
}


/** \brief
 *  Prints entries and their relatives.
 *  Actually unused i think.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \return void
 */
void build_print_relatives(ENTRY *elist, int entry_count) {
    int i, j;
    char temp[100];
    for (i = 0; i < entry_count; i++) {
        printf("%04d %s %02d %d\n", i, eid_conv(elist[i].EID, temp), elist[i].chunk, elist[i].esize);
        if (elist[i].related != NULL) {
            printf("------ %5d\n", elist[i].related[0]);
            for (j = 0; j < (signed) elist[i].related[0]; j++) {
                int relative = elist[i].related[j+1];
                printf("--%2d-- %s %2d %5d\n", j + 1, eid_conv(relative, temp),
                       elist[build_get_index(relative, elist, entry_count)].chunk, elist[build_get_index(relative, elist, entry_count)].esize);
            }
        }
    }
}


/** \brief
 *  Deconstructs the load or draw lists and saves into a convenient struct.
 *
 * \param prop_code int                 first of the two list properties (either 0x13B or 0x208)
 * \param entry unsigned char*          entry data
 * \param cam_offset int                offset of the camera item
 * \return LOAD_LIST                    load or draw list struct
 */
LOAD_LIST build_get_lists(int prop_code, unsigned char *entry, int cam_offset) {
    int k, l;
    LOAD_LIST load_list = init_load_list();
    for (k = 0; (unsigned) k < from_u32(entry + cam_offset + 0xC); k++) {
        int code = from_u16(entry + cam_offset + 0x10 + 8 * k);
        int offset = from_u16(entry + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
        int list_count = from_u16(entry + cam_offset + 0x16 + 8 * k);
        if (code == prop_code || code == prop_code + 1) {
            int sub_list_offset = offset + 4 * list_count;
            int point;
            int load_list_item_count;
            for (l = 0; l < list_count; l++, sub_list_offset += load_list_item_count * 4) {
                load_list_item_count = from_u16(entry + offset + l * 2);
                point = from_u16(entry + offset + l * 2 + list_count * 2);

                load_list.array[load_list.count].list_length = load_list_item_count;
                load_list.array[load_list.count].list = (unsigned int *) malloc(load_list_item_count * sizeof(unsigned int));
                memcpy(load_list.array[load_list.count].list, entry + sub_list_offset, load_list_item_count * sizeof(unsigned int));
                if (code == prop_code)
                    load_list.array[load_list.count].type = 'A';
                else
                    load_list.array[load_list.count].type = 'B';
                load_list.array[load_list.count].index = point;
                (load_list.count)++;
                qsort(load_list.array, load_list.count, sizeof(LOAD), comp);
            }
        }
    }

    // printf("lived\n");
    return load_list;
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
    for (i = 0; i < from_u32(entity + 0xC); i++) {
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


/** \brief
 *  Gets rid of some dynamically allocated stuff and closes files.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunks unsigned char**        built chunks
 * \param chunk_count int               chunk count
 * \return void
 */
void build_final_cleanup(ENTRY *elist, int entry_count, unsigned char **chunks, int chunk_count) {
    for (int i = 0; i < chunk_count; i++)
        free(chunks[i]);

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


// dumb thing for snow no or whatever convoluted level its configured for rn
void build_get_box_count(ENTRY *elist, int entry_count) {
    int counter = 0;
    int nitro_counter = 0;
    for (int i = 0; i < entry_count; i++) {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE) {
            int entity_count = build_get_entity_count(elist[i].data);
            int camera_count = build_get_cam_count(elist[i].data);
            for (int j = 0; j < entity_count; j++) {
                unsigned char *entity = elist[i].data + get_nth_item_offset(elist[i].data, (2 + camera_count + j));
                int type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
                int subt = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);
                int ID = build_get_entity_prop(entity, ENTITY_PROP_ID);

                if ((type >= 34 && type <= 43) &&
                    (subt == 0 || subt == 2 || subt == 3 || subt == 4 || subt == 6 || subt == 8 || subt == 9 ||
                     subt == 10 || subt == 11 || subt == 18 || subt == 23 || subt == 25 || subt == 26))
                /*if (ID != -1)*/
                {
                    counter++;
                    char temp[100];
                    printf("Zone: %5s, type: %2d, subtype: %2d, ID: %3d\n", eid_conv(elist[i].EID, temp), type, subt, ID);
                }

                if ((type >= 34 && type <= 43) && subt == 18) {
                    nitro_counter++;
                    if (type == 43)
                        printf("gotem %d\n", ID);
                    printf("NITRO %3d\n", ID);
                }
            }
        }
    }
    printf("BOX COUNT:   %3d\n", counter);
    printf("NITRO COUNT: %3d\n", nitro_counter);
}


/** \brief
 *  Reads nsf, reads folder, collects relatives, assigns proto chunks, calls some merge functions, makes load lists, makes nsd, makes nsf, end.
 *
 * \param nsfpath char*                 path to the base nsf
 * \param dirpath char*                 path to the folder whose contents are to be added
 * \return void
 */
void build_main(int build_rebuild_flag) {
    char fpaths[FPATH_COUNT][MAX] = {0}, nsfpath[MAX], dirpath[MAX], lcltemp[MAX];      // paths to files, fpaths contains user-input metadata like perma list file
    FILE *nsfnew = NULL, *nsd = NULL;                                                   // file pointers for input nsf, output nsf (nsfnew) and output nsd
    SPAWNS spawns = init_spawns();                                                      // struct with spawns found during reading and parsing of the level data
    ENTRY elist[2500];                                                                  // array of structs used to store entries, static cuz lazy & struct is small
    unsigned char *chunks[1024];                                                        // array of pointers to potentially built chunks, static cuz lazy
    LIST permaloaded;                                                                   // list containing EIDs of permaloaded entries provided by the user
    DEPENDENCIES subtype_info;                                                          // struct containing info about dependencies of certain types and subtypes
    DEPENDENCIES collisions;                                                            // struct containing info about dependencies of certain collision types
    int level_ID = 0;                                                                   // level ID, used for naming output files and needed in output nsd
    int chunk_border_base       = 0,                                                    // used to keep track of counts and to separate groups of chunks
        chunk_border_texture    = 0,
        chunk_border_sounds     = 0,
        chunk_count             = 0,
        entry_count_base        = 0,
        entry_count             = 0;

    unsigned int gool_table[0x40];                                                      // table w/ EIDs of gool entries, needed for nsd, filled using input entries
    for (int i = 0; i < 0x40; i++) gool_table[i] = EID_NONE;

    /* config: */
    // 0 - [gool initial merge flag]    0 - group       |   1 - one by one
    // 1 - [zone initial merge flag]    0 - group       |   1 - one by one
    // 2 - [merge type flag]            0 - per delta   |   1 - per point
    // 3 - [slst distance]              defined by user
    // 4 - [neighbour distance]         defined by user
    // 5 - [draw list distance]         defined by user
    // 6 - transition pre-load flag     defined by user
    // 7 - backwards penalty            defined by user | is 1M times the float value because yes, range 0 - 0.5
    int config[8] = {1, 1, 1, 0, 0, 0, 0, 0};

    // reading contents of the nsf/folder and collecting metadata
    if (build_rebuild_flag == FUNCTION_BUILD) {
        printf("Input the path to the base level (.nsf)[CAN BE A BLANK FILE]:\n");
        scanf(" %[^\n]", nsfpath);
        path_fix(nsfpath);

        printf("\nInput the path to the folder whose contents you want to import:\n");
        scanf(" %[^\n]", dirpath);
        path_fix(dirpath);

        FILE *nsf = NULL;
        if ((nsf = fopen(nsfpath,"rb")) == NULL) {
            printf("[ERROR] Could not open selected NSF\n");
            return;
        }

        DIR *df = NULL;
        if ((df = opendir(dirpath)) == NULL) {
            printf("[ERROR] Could not open selected directory\n");
            fclose(nsf);
            return;
        }

        level_ID = build_ask_ID();

        *(strrchr(nsfpath,'\\') + 1) = '\0';
        sprintf(lcltemp,"%s\\S00000%02X.NSF", nsfpath, level_ID);
        nsfnew = fopen(lcltemp, "wb");
        *(strchr(lcltemp, '\0') - 1) = 'D';
        nsd = fopen(lcltemp, "wb");

        //chunk_border_base = build_get_chunk_count_base(nsf);
        //build_read_nsf(elist, chunk_border_base, chunks, &chunk_border_texture, &entry_count, nsf, gool_table);
        chunk_border_base = 0;
        entry_count_base = entry_count;
        build_read_folder(df, dirpath, chunks, elist, &chunk_border_texture, &entry_count, &spawns, gool_table);
        fclose(nsf);
    }

    if (build_rebuild_flag == FUNCTION_REBUILD) {
        printf("Input the path to the level (.nsf) you want to rebuild:\n");
        scanf(" %[^\n]", nsfpath);
        path_fix(nsfpath);

        FILE *nsf = NULL;
        if ((nsf = fopen(nsfpath, "rb")) == NULL)  {
            printf("[ERROR] Could not open selected NSF\n");
            return;
        }

        level_ID = build_ask_ID();

        *(strrchr(nsfpath,'\\') + 1) = '\0';
        sprintf(lcltemp,"%s\\S00000%02X.NSF", nsfpath, level_ID);
        nsfnew = fopen(lcltemp, "wb");
        *(strchr(lcltemp, '\0') - 1) = 'D';
        nsd = fopen(lcltemp, "wb");

        int nsf_chunk_count = build_get_chunk_count_base(nsf);

        unsigned char buffer[CHUNKSIZE];
        for (int i = 0; i < nsf_chunk_count; i++) {
            fread(buffer, sizeof(unsigned char), CHUNKSIZE, nsf);
            int chunk_entry_count = from_u32(buffer + 0x8);
            if (from_u16(buffer + 0x2) == CHUNK_TYPE_TEXTURE) {
                chunks[chunk_border_texture] = (unsigned char *) calloc(CHUNKSIZE, sizeof(unsigned char));
                memcpy(chunks[chunk_border_texture], buffer, CHUNKSIZE);
                elist[entry_count].EID = from_u32(buffer + 4);
                elist[entry_count].chunk = chunk_border_texture;
                elist[entry_count].data = NULL;
                elist[entry_count].related = NULL;
                entry_count++;
                chunk_border_texture++;
            }
            else
            for (int j = 0; j < chunk_entry_count; j++) {
                int start_offset = get_nth_item_offset(buffer, j);
                int end_offset = get_nth_item_offset(buffer, j + 1);
                int entry_size = end_offset - start_offset;

                elist[entry_count].chunk = -1;
                elist[entry_count].EID = from_u32(buffer + start_offset + 0x4);
                elist[entry_count].esize = entry_size;
                elist[entry_count].related = NULL;
                elist[entry_count].data = (unsigned char *) malloc(entry_size);
                memcpy(elist[entry_count].data, buffer + start_offset, entry_size);

                if (build_entry_type(elist[entry_count]) == ENTRY_TYPE_ZONE)
                    build_check_item_count(elist[entry_count].data, elist[entry_count].EID);
                if (build_entry_type(elist[entry_count]) == ENTRY_TYPE_ZONE)
                    elist[entry_count].related = build_get_zone_relatives(elist[entry_count].data, &spawns);
                if (build_entry_type(elist[entry_count]) == ENTRY_TYPE_GOOL && from_u32(elist[entry_count].data + 0xC) == 6)
                    elist[entry_count].related = build_get_gool_relatives(elist[entry_count].data, entry_size);

                if (build_entry_type(elist[entry_count]) == ENTRY_TYPE_GOOL && *(elist[entry_count].data + 8) > 3) {
                    int item1_offset = *(int *)(elist[entry_count].data + 0x10);
                    int gool_type = *(int*)(elist[entry_count].data + item1_offset);
                    char temp[100];
                    if (gool_type > 63 || gool_type < 0) {
                        printf("[warning] GOOL entry %s has invalid type specified in the third item (%2d)!\n", eid_conv(elist[entry_count].EID, temp), gool_type);
                        continue;
                    }
                    gool_table[gool_type] = elist[entry_count].EID;
                }

                entry_count++;
                qsort(elist, entry_count, sizeof(ENTRY), cmp_entry_eid);
            }
        }
        fclose(nsf);
    }

    chunk_count = chunk_border_texture;


    // let the user pick the spawn, according to the spawn determine for each cam path its distance from spawn in terms of path links,
    // which is later used to find out which of 2 paths is in the backwards direction and, where backwards loading penalty should be applied
    // during the load list generation procedure
    build_ask_spawn(spawns);
    build_get_distance_graph(elist, entry_count, spawns);

    // gets model references from gools, was useful in a deprecate chunk merging/building algorithm, but might be useful later and barely takes any time so idc
    build_get_model_references(elist, entry_count);
    build_remove_invalid_references(elist, entry_count, entry_count_base);
    qsort(elist, entry_count, sizeof(ENTRY), cmp_entry_eid);    // not sure why this is here anymore, might not matter, but its just one qsort so eh

    // builds instrument and sound chunks, chunk_border_sounds is used to make chunk merging and chunk building more convenient, especially in deprecate methods
    build_instrument_chunks(elist, entry_count, &chunk_count, chunks);
    build_sound_chunks(elist, entry_count, &chunk_count, chunks);
    chunk_border_sounds = chunk_count;

    // ask user paths to files with permaloaded entries, type/subtype dependencies and collision type dependencies
    // end if something went wrong
    build_ask_list_paths(fpaths);
    if (!build_read_entry_config(&permaloaded, &subtype_info, &collisions, fpaths, elist, entry_count, gool_table)) {
        printf("File could not be opened or a different error occured\n");
        fclose(nsfnew);
        fclose(nsd);
        return;
    }

    // ask user desired distances for various things aka how much in advance in terms of camera rail distance things get loaded
    // there are some restrictions in terms of path link depth so its not entirely accurate, but it still matters
    build_ask_distances(config);

    // build load lists based on user input and metadata, and already or not yet collected metadata
    build_make_load_lists(elist, entry_count, gool_table, permaloaded, subtype_info, collisions, config);

    // call main merge function
    build_merge_main(elist, entry_count, chunk_border_sounds, &chunk_count, config, permaloaded);

    // build and write nsf and nsd file
    build_write_nsd(nsd, elist, entry_count, chunk_count, spawns, gool_table, level_ID);
    build_write_nsf(nsfnew, elist, entry_count, chunk_border_sounds, chunk_count, chunks);

    // get rid of at least some dynamically allocated memory, p sure there are leaks all over the place but oh well
    build_final_cleanup(elist, entry_count, chunks, chunk_count);
}
