#include "macros.h"
// contains main build function that governs the process, as well as semi-misc functions
// necessary or used across various build files

/** \brief
 *  Just returns the model reference of an animation.
 *  Only looks at the first frame of the animation.
 *
 * \param anim uint8_t*           pointer to the data of the searched animation
 * \return uint32_t                 eid of the animation's model reference (as uint32_t)
 */

LIST build_get_models(uint8_t *animation)
{
    LIST models = init_list();

    int32_t item_count = build_item_count(animation);
    for (int32_t i = 0; i < item_count; i++)
        list_add(&models, build_get_model(animation, i));

    return models;
}

uint32_t build_get_model(uint8_t *anim, int32_t item)
{
    int32_t item_off = build_get_nth_item_offset(anim, item);
    return from_u32(anim + item_off + 0x10);
}

uint32_t build_get_zone_track(uint8_t *entry)
{
    int32_t item1off = build_get_nth_item_offset(entry, 0);
    int32_t item2off = build_get_nth_item_offset(entry, 1);
    int32_t item1len = item2off - item1off;

    uint32_t music_entry = from_u32(entry + item1off + C2_MUSIC_REF + item1len - 0x318);
    return music_entry;
}

/** \brief
 *  Used during reading from the folder to prevent duplicate texture chunks.
 *  Searches existing chunks (base level's chunks) and if the texture eid matches
 *  it returns the index of the matching chunk, else returns -1
 *
 * \param textr uint32_t            searched texture chunk eid
 * \param chunks uint8_t**        array of chunks to be searched, chunk is a 64kB array
 * \param index_end int32_t                 end index of the chunk array, aka current chunk count
 * \return int32_t                          index of a chunk that matches search, or -1
 */
int32_t build_get_base_chunk_border(uint32_t textr, uint8_t **chunks, int32_t index_end)
{
    int32_t ret = -1;

    for (int32_t i = 0; i < index_end; i++)
        if (from_u32(chunks[i] + 4) == textr)
            ret = i;

    return ret;
}

/** \brief
 *  Searches the entry list looking for the specified eid.
 *  Binary search, entry list should be sorted by eid (ascending).
 *
 * \param eid uint32_t              searched eid
 * \param elist ENTRY*                  list of entries
 * \param entry_count int32_t               amount of entries
 * \return int32_t                          index of the searched eid or -1
 */
int32_t build_get_index(uint32_t eid, ENTRY *elist, int32_t entry_count)
{
    int32_t first = 0;
    int32_t last = entry_count - 1;
    int32_t middle = (first + last) / 2;

    while (first <= last)
    {
        if (elist[middle].eid < eid)
            first = middle + 1;
        else if (elist[middle].eid == eid)
            return middle;
        else
            last = middle - 1;

        middle = (first + last) / 2;
    }

    return -1;
}

/** \brief
 *  Searches the properties, finds the offset of the slst property,
 *  returns eid of the slst.
 *
 * \param item uint8_t*           camera entity data
 * \return uint32_t                 slst reference or 0 if theres no slst reference property
 */
uint32_t build_get_slst(uint8_t *item)
{
    int32_t offset = build_get_prop_offset(item, ENTITY_PROP_CAM_SLST);

    if (offset)
        return from_u32(item + offset + 4);
    else
        return 0;
}

/** \brief
 *  Returns path length of the input item.
 *
 * \param item uint8_t*           item whose length it returns
 * \return uint32_t                 path length
 */
uint32_t build_get_path_length(uint8_t *item)
{
    int32_t offset = build_get_prop_offset(item, ENTITY_PROP_PATH);

    if (offset)
        return from_u32(item + offset);
    else
        return 0;
}

/** \brief
 *  Gets zone's neighbour count.
 *
 * \param entry uint8_t*          entry data
 * \return int32_t                          neighbour count
 */
int32_t build_get_neighbour_count(uint8_t *entry)
{
    int32_t item1off = build_get_nth_item_offset(entry, 0);
    return entry[item1off + C2_NEIGHBOURS_START];
}

/** \brief
 *  Returns a list of neighbours the entry (zone) has.
 *
 * \param entry uint8_t*         entry data
 * \return LIST                        list containing neighbour eids
 */
LIST build_get_neighbours(uint8_t *entry)
{
    int32_t item1off = build_get_nth_item_offset(entry, 0);
    int32_t count = entry[item1off + C2_NEIGHBOURS_START];
    LIST neighbours = init_list();

    for (int32_t k = 0; k < count; k++)
    {
        int32_t neighbour_eid = from_u32(entry + item1off + C2_NEIGHBOURS_START + 4 + 4 * k);
        list_add(&neighbours, neighbour_eid);
    }

    return neighbours;
}
/** \brief
 *  Gets zone's camera entity count.
 *
 * \param entry uint8_t*          entry data
 * \return int32_t                          camera entity count (total count, not camera path count)
 */
int32_t build_get_cam_item_count(uint8_t *entry)
{
    int32_t item1off = build_get_nth_item_offset(entry, 0);
    return entry[item1off + 0x188];
}

/** \brief
 *  Gets zone's regular entity count.
 *
 * \param entry uint8_t*          entry data
 * \return int32_t                          entity count (not including camera entities)
 */
int32_t build_get_entity_count(uint8_t *entry)
{
    int32_t item1off = build_get_nth_item_offset(entry, 0);
    return entry[item1off + 0x18C];
}

int32_t build_item_count(uint8_t *entry)
{
    return from_u32(entry + 0xC);
}

int32_t build_prop_count(uint8_t *item)
{
    return from_u32(item + 0xC);
}

/** \brief
 *  Returns type of an entry.
 *
 * \param entry ENTRY                   entry struct
 * \return int32_t                          -1 if entry does not have data allocated, else real entry type
 */
int32_t build_entry_type(ENTRY entry)
{
    if (entry.data == NULL)
        return -1;
    return *(int32_t *)(entry.data + 8);
}

int32_t build_chunk_type(uint8_t *chunk)
{
    if (chunk == NULL)
        return -1;

    return from_u16(chunk + 0x2);
}

void build_check_item_count(uint8_t *zone, int32_t eid)
{
    int32_t item_count = build_item_count(zone);
    int32_t cam_count = build_get_cam_item_count(zone);
    int32_t entity_count = build_get_entity_count(zone);

    if (item_count != (2 + cam_count + entity_count))
    {
        printf("[warning] %s's item count (%d) doesn't match item counts in the first item (2 + %d + %d)\n",
               eid_conv2(eid), item_count, cam_count, entity_count);
    }
}

DRAW_ITEM build_decode_draw_item(uint32_t value)
{
    DRAW_ITEM draw;
    draw.neighbour_item_index = (value & 0xFF000000) >> 24;
    draw.ID = (value & 0xFFFF00) >> 8;
    draw.neighbour_zone_index = value & 0xFF;
    return draw;
}

/** \brief
 *  Returns value of the specified property. Only works on generic, single-value (4B length 4B value) properties.
 *
 * \param entity uint8_t*         entity data
 * \param prop_code int32_t                 property to return
 * \return int32_t                          property value
 */
int32_t build_get_entity_prop(uint8_t *entity, int32_t prop_code)
{
    int32_t offset = build_get_prop_offset(entity, prop_code);
    if (offset == 0)
        return -1;

    return from_u32(entity + offset + 4);
}

// gets offset to data of a property within an item
int32_t build_get_prop_offset(uint8_t *item, int32_t prop_code)
{
    int32_t offset = 0;
    int32_t prop_count = build_prop_count(item);
    for (int32_t i = 0; i < prop_count; i++)
        if ((from_u16(item + 0x10 + 8 * i)) == prop_code)
            offset = OFFSET + from_u16(item + 0x10 + 8 * i + 2);

    return offset;
}

/** \brief
 *  Just calculates the amount of chunks in the provided nsf file.
 *
 * \param nsf FILE*                     provided nsf
 * \return int32_t                          amount of chunks
 */
int32_t build_get_chunk_count_base(FILE *nsf)
{
    fseek(nsf, 0, SEEK_END);
    int32_t result = ftell(nsf) / CHUNKSIZE;
    rewind(nsf);

    return result;
}

/** \brief
 *  Checks whether the entry is meant to be placed into a normal chunk.
 *
 * \param entry ENTRY                   entry to be tested
 * \return int32_t                          1 if it belongs to a normal chunk, else 0
 */
bool build_is_normal_chunk_entry(ENTRY entry)
{
    int32_t type = build_entry_type(entry);
    if (type == ENTRY_TYPE_ANIM ||
        type == ENTRY_TYPE_DEMO ||
        type == ENTRY_TYPE_ZONE ||
        type == ENTRY_TYPE_MODEL ||
        type == ENTRY_TYPE_SCENERY ||
        type == ENTRY_TYPE_SLST ||
        type == ENTRY_TYPE_DEMO ||
        type == ENTRY_TYPE_VCOL ||
        type == ENTRY_TYPE_MIDI ||
        type == ENTRY_TYPE_GOOL ||
        type == ENTRY_TYPE_T21)
        return true;

    return false;
}

void build_cleanup_elist(ENTRY *elist, int32_t entry_count)
{
    for (int32_t i = 0; i < entry_count; i++)
    {
        if (elist[i].data != NULL)
            free(elist[i].data);

        if (elist[i].related != NULL)
            free(elist[i].related);

        if (elist[i].visited != NULL)
            free(elist[i].visited);

        if (elist[i].distances != NULL)
            free(elist[i].distances);
    }

    free(elist);
}

/** \brief
 *  Gets rid of some dynamically allocated stuff and closes files.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param chunks uint8_t**        built chunks
 * \param chunk_count int32_t               chunk count
 * \return void
 */
void build_final_cleanup(ENTRY *elist, int32_t entry_count, uint8_t **chunks, int32_t chunk_count, FILE *nsfnew, FILE *nsd, DEPENDENCIES dep1, DEPENDENCIES dep2)
{

    build_cleanup_elist(elist, entry_count);

    for (int32_t i = 0; i < chunk_count; i++)
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
void build_get_box_count(ENTRY *elist, int32_t entry_count)
{
    int32_t box_counter = 0;
    int32_t nitro_counter = 0;
    int32_t entity_counter = 0;
    for (int32_t i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            int32_t entity_count = build_get_entity_count(elist[i].data);
            int32_t camera_count = build_get_cam_item_count(elist[i].data);
            for (int32_t j = 0; j < entity_count; j++)
            {
                uint8_t *entity = build_get_nth_item(elist[i].data, (2 + camera_count + j));
                int32_t type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
                int32_t subt = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);
                int32_t id = build_get_entity_prop(entity, ENTITY_PROP_ID);

                if (id == -1)
                    continue;

                entity_counter++;

                if ((type >= 34 && type <= 43) &&
                    (subt == 0 || subt == 2 || subt == 3 || subt == 4 || subt == 6 || subt == 8 || subt == 9 ||
                     subt == 10 || subt == 11 || subt == 12 || subt == 17 || subt == 18 || subt == 23 || subt == 25 || subt == 26))
                {
                    box_counter++;
                }

                if ((type >= 34 && type <= 43) && subt == 18)
                {
                    nitro_counter++;
                }
            }
        }
    }
    printf("Box count:      %4d\n", box_counter);
    printf("Nitro count:    %4d\n", nitro_counter);
}

// used to sort load lists and to avoid stuff getting removed before its been added
int32_t cmp_func_loadlist(const void *a, const void *b)
{
    LOAD x = *(LOAD *)a;
    LOAD y = *(LOAD *)b;

    if (x.index != y.index)
        return (x.index - y.index);
    else
        return (x.type - y.type);
}

// used to sort draw list to avoid stuff getting removed before its been added
int32_t cmp_func_drawlist(const void *a, const void *b)
{
    LOAD x = *(LOAD *)a;
    LOAD y = *(LOAD *)b;

    if (x.index != y.index)
        return (x.index - y.index);
    else
        return (y.type - x.type);
}

LOAD_LIST build_get_draw_lists(uint8_t *entry, int32_t cam_index)
{
    LOAD_LIST dl = build_get_lists(ENTITY_PROP_CAM_DRAW_LIST_A, entry, cam_index);
    qsort(dl.array, dl.count, sizeof(LOAD), cmp_func_drawlist);
    return dl;
}

LOAD_LIST build_get_load_lists(uint8_t *entry, int32_t cam_index)
{
    LOAD_LIST ll = build_get_lists(ENTITY_PROP_CAM_LOAD_LIST_A, entry, cam_index);
    qsort(ll.array, ll.count, sizeof(LOAD), cmp_func_loadlist);
    return ll;
}

/** \brief
 *  Deconstructs the load or draw lists and saves into a convenient struct.
 *
 * \param prop_code int32_t                 first of the two list properties (either 0x13B or 0x208)
 * \param entry uint8_t*          entry data
 * \param cam_offset int32_t                offset of the camera item
 * \return LOAD_LIST                    load or draw list struct
 */
LOAD_LIST build_get_lists(int32_t prop_code, uint8_t *entry, int32_t cam_index)
{
    LOAD_LIST load_list;
    load_list.count = 0;

    int32_t cam_offset = build_get_nth_item_offset(entry, cam_index);
    int32_t prop_count = from_u32(entry + cam_offset + OFFSET);

    for (int32_t k = 0; k < prop_count; k++)
    {
        int32_t code = from_u16(entry + cam_offset + 0x10 + 8 * k);
        int32_t offset = from_u16(entry + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
        int32_t list_count = from_u16(entry + cam_offset + 0x16 + 8 * k);

        int32_t next_offset = (k + 1 < prop_count)
                                  ? (from_u16(entry + cam_offset + 0x12 + 8 * (k + 1)) + OFFSET + cam_offset)
                                  : (from_u16(entry + cam_offset) + cam_offset);
        int32_t prop_length = next_offset - offset;

        if (code == prop_code || code == prop_code + 1)
        {
            int32_t sub_list_offset;
            int32_t load_list_item_count;
            bool condensed_check1 = false;
            bool condensed_check2 = false;

            // i should use 0x40 flag in prop header[4] but my spaghetti doesnt work with it so this thing stays here
            int32_t potentially_condensed_length = from_u16(entry + offset) * 4 * list_count;
            potentially_condensed_length += 2 + 2 * list_count;
            if (from_u16(entry + offset + 2 + 2 * list_count) == 0)
                potentially_condensed_length += 2;
            if (potentially_condensed_length == prop_length && list_count > 1)
                condensed_check1 = true;

            int32_t len = 4 * list_count;
            for (int32_t l = 0; l < list_count; l++)
                len += from_u16(entry + offset + l * 2) * 4;
            if (len != prop_length)
                condensed_check2 = true;

            if (condensed_check1 && condensed_check2)
            {

                load_list_item_count = from_u16(entry + offset);
                uint16_t *indices = (uint16_t *)try_malloc(list_count * sizeof(uint16_t));
                memcpy(indices, entry + offset + 2, list_count * 2);
                sub_list_offset = offset + 2 + 2 * list_count;
                if (sub_list_offset % 4)
                    sub_list_offset += 2;
                for (int32_t l = 0; l < list_count; l++)
                {
                    load_list.array[load_list.count].list_length = load_list_item_count;
                    load_list.array[load_list.count].list = (uint32_t *)try_malloc(load_list_item_count * sizeof(uint32_t));
                    memcpy(load_list.array[load_list.count].list, entry + sub_list_offset, load_list_item_count * sizeof(uint32_t));
                    if (code == prop_code)
                        load_list.array[load_list.count].type = 'A';
                    else
                        load_list.array[load_list.count].type = 'B';
                    load_list.array[load_list.count].index = indices[l];
                    load_list.count++;
                    sub_list_offset += load_list_item_count * 4;
                }
            }
            else
            {
                sub_list_offset = offset + 4 * list_count;
                ;
                for (int32_t l = 0; l < list_count; l++)
                {
                    load_list_item_count = from_u16(entry + offset + l * 2);
                    int32_t index = from_u16(entry + offset + l * 2 + list_count * 2);

                    load_list.array[load_list.count].list_length = load_list_item_count;
                    load_list.array[load_list.count].list = (uint32_t *)try_malloc(load_list_item_count * sizeof(uint32_t));
                    memcpy(load_list.array[load_list.count].list, entry + sub_list_offset, load_list_item_count * sizeof(uint32_t));
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

void build_normal_check_loaded(ENTRY *elist, int32_t entry_count)
{
    int32_t omit = 0;
    printf("\nOmit normal chunk entries that are never loaded? [0 - include, 1 - omit]\n");
    scanf("%d", &omit);

    for (int32_t i = 0; i < entry_count; i++)
        elist[i].norm_chunk_ent_is_loaded = true;

    if (!omit)
    {
        printf("Not omitting\n");
        return;
    }

    printf("Checking for normal chunk entries that are never loaded\n");
    LIST ever_loaded = init_list();
    int32_t entries_skipped = 0;

    // reads all load lists and bluntly adds all items into the list of all loaded entries
    for (int32_t i = 0; i < entry_count; i++)
    {
        if (!(build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL))
            continue;

        int32_t cam_count = build_get_cam_item_count(elist[i].data) / 3;
        for (int32_t j = 0; j < cam_count; j++)
        {
            LOAD_LIST ll = build_get_load_lists(elist[i].data, 2 + 3 * j);
            for (int32_t k = 0; k < ll.count; k++)
                for (int32_t l = 0; l < ll.array[k].list_length; l++)
                    list_add(&ever_loaded, ll.array[k].list[l]);
        }
    }

    for (int32_t i = 0; i < entry_count; i++)
    {
        if (!build_is_normal_chunk_entry(elist[i]))
            continue;

        if (list_find(ever_loaded, elist[i].eid) == -1)
        {
            elist[i].norm_chunk_ent_is_loaded = false;
            entries_skipped++;
            printf("  %3d. entry %s never loaded, will not be included\n", entries_skipped, eid_conv2(elist[i].eid));
        }
    }

    printf("Number of normal entries not included: %3d\n", entries_skipped);
    if (ever_loaded.eids != NULL)
        free(ever_loaded.eids);
}

int32_t cmp_spawns(const void *a, const void *b)
{
    return ((*(SPAWN *)a).zone - (*(SPAWN *)b).zone);
}

void build_print_transitions(ENTRY *elist, int32_t entry_count)
{
    printf("\nTransitions in the level: \n");
    char eid1[6] = "";
    char eid2[6] = "";

    for (int32_t i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE)
            continue;

        LIST neighbours = build_get_neighbours(elist[i].data);
        int32_t item1off = build_get_nth_item_offset(elist[i].data, 0);
        for (int32_t j = 0; j < neighbours.count; j++)
        {
            uint32_t neighbour_eid = from_u32(elist[i].data + item1off + C2_NEIGHBOURS_START + 4 + 4 * j);
            uint32_t neighbour_flg = from_u32(elist[i].data + item1off + C2_NEIGHBOURS_START + 4 + 4 * j + 0x20);

            if (neighbour_flg == 0xF || neighbour_flg == 0x1F)
            {
                printf("Zone %s transition (%02x) to zone %s (neighbour %d)\n", eid_conv(elist[i].eid, eid1), neighbour_flg, eid_conv(neighbour_eid, eid2), j);
            }
        }
    }
}

/** \brief
 *  Reads nsf, reads inputs, makes draw lists, makes load lists and draw lists if selected, merges entries into chunks, writes output.
 *
 * \param build_type            rebuild or rebuild_dl
 * \return void
 */
void build_main(int32_t build_type)
{
    clock_t time_build_start = clock();

    FILE *nsfnew = NULL, *nsd = NULL; // file pointers for input nsf, output nsf (nsfnew) and output nsd
    FILE *nsfnew2 = NULL, *nsd2 = NULL;
    SPAWNS spawns = init_spawns(); // struct with spawns found during reading and parsing of the level data
    ENTRY *elist = (ENTRY *)try_calloc(sizeof(ENTRY), ELIST_DEFAULT_SIZE);

    uint8_t *chunks[CHUNK_LIST_DEFAULT_SIZE];     // array of pointers to potentially built chunks, fixed length cuz lazy
    LIST permaloaded;                             // list containing eids of permaloaded entries provided by the user
    DEPENDENCIES subtype_info = build_init_dep(); // struct containing info about dependencies of certain types and subtypes
    DEPENDENCIES collisions = build_init_dep();   // struct containing info about dependencies of certain collision types
    DEPENDENCIES mus_dep = build_init_dep();
    int32_t level_ID = 0; // level ID, used for naming output files and needed in output nsd

    // used to keep track of counts and to separate groups of chunks
    // int32_t chunk_border_base       = 0;
    int32_t chunk_border_texture = 0;
    int32_t chunk_border_sounds = 0;
    int32_t chunk_count = 0;
    int32_t entry_count_base = 0;
    int32_t entry_count = 0;

    uint32_t gool_table[C2_GOOL_TABLE_SIZE]; // table w/ eids of gool entries, needed for nsd, filled using input entries
    for (int32_t i = 0; i < C2_GOOL_TABLE_SIZE; i++)
        gool_table[i] = EID_NONE;

    // config:
    int32_t config[] = {
        0, // 0 - gool initial merge flag      0 - group       |   1 - one by one                          set here, used by deprecate merges
        0, // 1 - zone initial merge flag      0 - group       |   1 - one by one                          set here, used by deprecate merges
        1, // 2 - merge type value             0 - per delta   |   1 - weird per point |   2 - per point   set here, used by matrix merge

        0, // 3 - slst distance value              set by user in function build_ask_distances(config); affects load lists
        0, // 4 - neighbour distance value         set by user in function build_ask_distances(config); affects load lists
        0, // 5 - draw list distance value         set by user in function build_ask_distances(config); affects load lists
        0, // 6 - transition pre-load flag         set by user in function build_ask_distances(config); affects load lists
        0, // 7 - backwards penalty value          set by user in function build_ask_dist...;  aff LLs     is 1M times the float value because int32_t, range 0 - 0.5

        0, // 8 - relation array sort flag     0 - regular     |   1 - also sort by total occurence count; set here, used by matrix merge (1 is kinda meh)
        0, // 9 - sound entry load list flag   0 - all sounds  |   1 - one sound per sound chunk           set here, affects load lists

        0, // 10 - load list remake flag        0 - dont remake |   1 - remake load lists                   set by user in build_ask_build_flags
        0, // 11 - merge technique value                                                                    set by user in build_ask_build_flags

        1, // 12 - perma inc. in matrix flag    0 - dont include|   1 - do include                          set here, used by matrix merges
        1, // 13 - inc. 0-vals in relarray flag 0 - dont include|   1 - do include                          set here, used by matrix merges

        0, // 14 - draw list gen dist 2D            set by user in build_ask_draw_distances
        0, // 15 - draw list gen dist 3D            set by user in build_ask_draw_distances
        0, // 16 - draw list gen dist 2D vertictal| set by user in build_ask_draw_distances
        0, // 17 - draw list gen angle 3D           set by user in build_ask_draw_distances - allowed angle distance
    };

    // reading contents of the nsf to be rebuilt and collecting metadata
    bool input_parse_err = build_read_and_parse_rebld(&level_ID, &nsfnew, &nsd, &chunk_border_texture, gool_table, elist, &entry_count, chunks, &spawns, 0, NULL);
    chunk_count = chunk_border_texture;

    // end if something went wrong
    if (input_parse_err)
    {
        printf("[ERROR] something went wrong during parsing\n");
        build_final_cleanup(elist, entry_count, chunks, chunk_count, nsfnew, nsd, subtype_info, collisions);
        return;
    }

    // build_get_box_count(elist, entry_count);
    build_try_second_output(&nsfnew2, &nsd2, level_ID);

    // user picks whether to remake load lists or not, also merge method
    build_ask_build_flags(config);
    int32_t load_list_flag = config[CNFG_IDX_LL_REMAKE_FLAG];
    int32_t merge_tech_flag = config[CNFG_IDX_MERGE_METHOD_VALUE];

    qsort(spawns.spawns, spawns.spawn_count, sizeof(SPAWN), cmp_spawns);
    // let the user pick the spawn, according to the spawn determine for each cam path its distance from spawn in terms of path links,
    // which is later used to find out which of 2 paths is in the backwards direction and, where backwards loading penalty should be applied
    // during the load list generation procedure
    if (spawns.spawn_count > 0)
        build_ask_spawn(spawns);
    else
    {
        printf("[ERROR] No spawns found, add one using the usual 'willy' entity or a checkpoint\n\n");
        build_final_cleanup(elist, entry_count, chunks, chunk_count, nsfnew, nsd, subtype_info, collisions);
        if (nsfnew2 != NULL)
        {
            fclose(nsfnew2);
            fclose(nsd2);
        }
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
    if (!build_read_entry_config(&permaloaded, &subtype_info, &collisions, &mus_dep, elist, entry_count, gool_table, config))
    {
        printf("[ERROR] File could not be opened or a different error occured\n\n");
        build_final_cleanup(elist, entry_count, chunks, chunk_count, nsfnew, nsd, subtype_info, collisions);
        if (nsfnew2 != NULL)
        {
            fclose(nsfnew2);
            fclose(nsd2);
        }
        return;
    }

    // debug
    /*for (int32_t i = 0; i < entry_count; i++)
        list_add(&permaloaded, elist[i].eid);*/

    if (build_type == BuildType_Rebuild_DL)
    {
        build_ask_draw_distances(config);
        build_remake_draw_lists(elist, entry_count, config);
    }

    if (load_list_flag == 1)
    {
        // print for the user, informs them about entity type/subtypes that have no dependency list specified
        build_find_unspecified_entities(elist, entry_count, subtype_info);

        // transition info (0xF/0x1F)
        build_print_transitions(elist, entry_count);

        // ask user desired distances for various things, eg how much in advance in terms of camera rail distance things get loaded
        // there are some restrictions in terms of path link depth so its not entirely accurate, but it still matters
        build_ask_distances(config);

        // build load lists based on user input and metadata, and already or not yet collected metadata
        printf("\nNumber of permaloaded entries: %d\n\n", permaloaded.count);
        build_remake_load_lists(elist, entry_count, gool_table, permaloaded, subtype_info, collisions, mus_dep, config);
    }

    build_normal_check_loaded(elist, entry_count);

    clock_t time_start = clock();
    // call merge function
    switch (merge_tech_flag)
    {
    case 5:
#if COMPILE_WITH_THREADS
        build_matrix_merge_random_thr_main(elist, entry_count, chunk_border_sounds, &chunk_count, config, permaloaded);
#else
        printf("This build does not support this method, using method 4 instead\n");
        build_matrix_merge_random_main(elist, entry_count, chunk_border_sounds, &chunk_count, config, permaloaded, true);
#endif // COMPILE_WITH_THREADS
        break;
    case 4:
        build_matrix_merge_random_main(elist, entry_count, chunk_border_sounds, &chunk_count, config, permaloaded, false);
        break;
    default:
        printf("[ERROR] deprecated merge %d\n", merge_tech_flag);
        break;
    }
    printf("\nMerge took %.3fs\n", ((double)clock() - time_start) / CLOCKS_PER_SEC);

    // build and write nsf and nsd file
    build_write_nsd(nsd, nsd2, elist, entry_count, chunk_count, spawns, gool_table, level_ID);
    build_sort_load_lists(elist, entry_count);
    build_normal_chunks(elist, entry_count, chunk_border_sounds, chunk_count, chunks, 1);
    build_write_nsf(nsfnew, elist, entry_count, chunk_border_sounds, chunk_count, chunks, nsfnew2);

    // get rid of at least some dynamically allocated memory, p sure there are leaks all over the place but oh well
    build_final_cleanup(elist, entry_count, chunks, chunk_count, nsfnew, nsd, subtype_info, collisions);
    if (nsfnew2 != NULL)
    {
        fclose(nsfnew2);
        fclose(nsd2);
    }
    printf("Build/rebuild took %.3fs\n", ((double)clock() - time_build_start) / CLOCKS_PER_SEC);
    printf("Done. It is recommended to save NSD & NSF couple times with CrashEdit, e.g. 0.2.135.2 (or higher),\notherwise the level might not work.\n\n");
}
