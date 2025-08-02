#include "macros.h"

// text at the start when you open the program or wipe
void intro_text()
{
    for (int32_t i = 0; i < 100; i++)
        printf("*");
    printf("\nCrash 2/3 level entry exporter/importer/reformatter/resizer/rotater made by Averso.\n");
    printf("If any issue pops up (instructions are unclear or it crashes), DM me @ Averso#5633 (Discord).\n");
    printf("Type \"HELP\" for list of commands and their format. Commands >ARE NOT< case sensitive.\n");
    printf("It is recommended to provide valid data as there may be edge cases that are unaccounted for.\n");
    printf("You can drag & drop the files and folders to this window instead of copying in the paths\n");
    printf("Most stuff works only for Crash 2 !!!!\n");
    for (int32_t i = 0; i < 100; i++)
        printf("*");
    printf("\n\n");
}

// help for miscellaneous, modpack-specific and obsolete commands
void print_help2()
{
    for (int32_t i = 0; i < 100; i++)
        printf("-");
    printf("\nMisc/obsolete/modpack command list:\n");

    printf("KILL\n");
    printf("\t ends the program\n");

    printf("PROP & NSF_PROP\n");
    printf("\t print a list of properties in an entity or all occurences of entity in a NSF\n");

    printf("PROP_REMOVE\n");
    printf("\t removes a property from an entity\n");

    printf("PROP_REPLACE\n");
    printf("\t replaces (or inserts) a property into dest entity with prop from source entity\n");

    printf("TEXTURE\n");
    printf("\t copies tiles from one texture chunk to another (doesnt include CLUTs)\n");

    printf("SCEN_RECOLOR\n");
    printf("\t recolor vertex colors to selected hue / brightness\n");

    printf("TEXTURE_RECOLOR\n");
    printf("\t recolor texture clut colors to selected hue / brightness\n");

    printf("WARP_SPAWNS\n");
    printf("\t modpack, used to generate warp room spawns from the nsf\n");

    printf("CHECK_UTIL\n");
    printf("\t modpack, lists regular+dda checks/masks, some levels have hardcoded exceptions (folder)\n");

    printf("LIST_SPECIAL_LL\n");
    printf("\t lists special zone load list entries (for rebuild) located within first item (folder)\n");

    printf("NSD_UTIL\n");
    printf("\t misc gool listing util (folder)\n");

    printf("FOV_UTIL\n");
    printf("\t misc fov listing util (folder)\n");

    printf("DRAW_UTIL\n");
    printf("\t detailed info about draw lists (NSF)\n");

    printf("TPAGE_UTIL\n");
    printf("\t list tpages within files (folder)\n");

    printf("GOOL_UTIL\n");
    printf("\t list gool entries within files (folder)\n");

    printf("A <angle> & TIME\n");
    printf("\t modpack crate rotation, TT value\n");

    printf("GEN_SLST\n");
    printf("\t creates an 'empty' SLST entry for path with chosen length\n");

    printf("ALL_PERMA\n");
    printf("\t list all non-instrument entries and tpages (NSF)\n");

    printf("SCEN_RECOLOR2\n");
    printf("\t modpack, hardcoded values, dont use\n");

    printf("RESIZE <game> <xmult> <ymult> <zmult>\n");
    printf("\t changes dimensions of the zones and scenery according to given parameters (can mess up warps)\n");
    printf("\t e.g. 'resize3' 1.25 1 1' - files are from C3 and it gets stretched only on X\n");

    printf("ROTATE\n");
    printf("\t rotates scenery or objects in a zone you specified\n");

    printf("ENT_RESIZE\n");
    printf("\t converts c3 entity to c2 entity (path adjustment)\n");

    printf("CONV_OLD_DL_OVERRIDE\n");
    printf("\t converts old draw list override to new format (NSF)\n");

    printf("ENT_MOVE\n");
    printf("\t moves an entity by chosen amount\n");

    printf("CHANGEPRINT & IMPORT (obsolete)\n");
    printf("\t print selection, entry->NSF import (pretty much useless)\n");

    printf("FLIP_Y & FLIP_X\n");
    printf("\t flips the level horizontally or vertically\n");

    printf("LEVEL_RECOLOR\n");
    printf("\t recolors all level scenery to given rgb and brightness\n");

    for (int32_t i = 0; i < 100; i++)
        printf("-");
    printf("\n");
}

// main help
void print_help()
{
    for (int32_t i = 0; i < 100; i++)
        printf("-");
    printf("\nCommand list:\n");
    printf("HELP & HELP2\n");
    printf("\t print a list of commands\n");

    printf("WIPE\n");
    printf("\t wipes current screen\n");

    printf("BUILD & REBUILD & REBUILD_DL\n");
    printf("\t builds a level from chosen inputs\n");

    printf("LL_ANALYZE\n");
    printf("\t stats about the level, integrity checks\n");

    printf("EXPORT & EXPORTALL\n");
    printf("\t exports level entries (EXPORTALL exports all levels in a folder)\n");

    printf("LEVEL_WIPE_DL & LEVEL_WIPE_ENT\n");
    printf("\t remove draw lists / remove draw lists + entities from level (scrambles the level a bit)\n");

    printf("NSD\n");
    printf("\t prints gool table from the nsd\n");

    printf("EID <EID>\n");
    printf("\t prints the EID in hex form\n");

    printf("GEN_SPAWN\n");
    printf("\t NSD spawn generation for input level, zone and entity ID\n");

    printf("MODEL_REFS & MODEL_REFS_NSF\n");
    printf("\t lists model entry texture references\n");

    printf("ENTITY_USAGE\n");
    printf("\t prints info about type/subtype usage count across nsfs in the folder\n");

    printf("PAYLOAD_INFO\n");
    printf("\t prints info about normal chunk / tpage / entity loading in nsf's cam paths\n");

    printf("\nError messages:\n");
    printf("[ERROR]   error message\n");
    printf("\tan error that could not be handled, the program skipped some action or gave up\n");

    printf("[error]   error message\n");
    printf("\tan error that was handled\n");

    printf("[warning] warning text\n");
    printf("\tjust a warning, something may or may not be wrong\n");
    for (int32_t i = 0; i < 100; i++)
        printf("-");
    printf("\n");
}

// wipes the current screen
void clrscr()
{
    system("@cls||clear");
}

// helper for reading signed 32b
int32_t from_s32(uint8_t *data)
{
    /*
    const uint8_t *p = data;
    int32_t result = p[0];
    result |= p[1] << 8;
    result |= p[2] << 16;
    result |= p[3] << 24;
    return result;*/
    return *(int32_t *)data;
}

// helper for reading unsigned 32b
uint32_t from_u32(uint8_t *data)
{
    /*const uint8_t *p = data;
    uint32_t result = p[0];
    result |= p[1] << 8;
    result |= p[2] << 16;
    result |= p[3] << 24;
    return result;*/
    return *(uint32_t *)data;
}

// helper for reading signed 16b
int32_t from_s16(uint8_t *data)
{
    return *(int16_t *)(data);
}

// helper for reading unsigned 16b
uint32_t from_u16(uint8_t *data)
{
    return *(uint16_t *)data;
}

// helper for reading unsigned 8b
uint32_t from_u8(uint8_t *data)
{
    const uint8_t *p = data;
    uint32_t result = p[0];
    return result;
}

// hashes the input string into a number
uint32_t comm_str_hash(const char *str)
{
    uint32_t comm_str_hash = 5381;
    int32_t c;

    while ((c = *str++))
        comm_str_hash = ((comm_str_hash << 5) + comm_str_hash) + c;
    return comm_str_hash;
}

// converts int32_t eid to string eid
// doesnt copy
const char *eid_conv2(uint32_t value)
{
    return eid_conv(value, NULL);
}

#if COMPILE_WITH_THREADS
pthread_mutex_t g_eidconv_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

// converts int32_t eid to string eid
const char *eid_conv(uint32_t value, char *eid)
{
    #if COMPILE_WITH_THREADS    
    pthread_mutex_lock(&g_eidconv_mutex);
    #endif

    const char charset[] =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "_!";

    static char temp[6] = "abcde";
    temp[0] = charset[(value >> 25) & 0x3F];
    temp[1] = charset[(value >> 19) & 0x3F];
    temp[2] = charset[(value >> 13) & 0x3F];
    temp[3] = charset[(value >> 7) & 0x3F];
    temp[4] = charset[(value >> 1) & 0x3F];
    temp[5] = '\0';

    if (eid == NULL) 
    {
        #if COMPILE_WITH_THREADS
        pthread_mutex_unlock(&g_eidconv_mutex);
        #endif
        return temp;
    }

    memcpy(eid, temp, 6);
    #if COMPILE_WITH_THREADS
    pthread_mutex_unlock(&g_eidconv_mutex);
    #endif
    return eid;
}

// conversion of eid from string form to u32int form
uint32_t eid_to_int(char *eid)
{
    uint32_t result = 0;
    const char charset[] =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "_!";

    for (int32_t i = 0; i < 5; i++)
        for (int32_t j = 0; j < 0x40; j++)
            if (charset[j] == eid[i])
                result = (result << 6) + j;

    result = 1 + (result << 1);
    return result;
}

// calculates chunk checksum (CRC)
uint32_t crcChecksum(const uint8_t *data, int32_t size)
{
    uint32_t checksum = 0x12345678;
    for (int32_t i = 0; i < size; i++)
    {
        if (i < CHUNK_CHECKSUM_OFFSET || i >= CHUNK_CHECKSUM_OFFSET + 4)
            checksum += data[i];
        checksum = checksum << 3 | checksum >> 29;
    }
    return checksum;
}

// calculates chunk checksum
uint32_t nsfChecksum(const uint8_t *data)
{
    return crcChecksum(data, CHUNKSIZE);
}

// integer comparison func for qsort
int32_t cmp_func_int(const void *a, const void *b)
{
    return (*(int32_t *)a - *(int32_t *)b);
}

// used to sort payload ladder in descending order
int32_t cmp_func_payload(const void *a, const void *b)
{
    PAYLOAD x = *(PAYLOAD *)a;
    PAYLOAD y = *(PAYLOAD *)b;

    if (y.count != x.count)
        return (y.count - x.count);
    else
        return (x.zone - y.zone);
}

// used in LIST struct
int32_t cmp_func_uint(const void *a, const void *b)
{
    uint32_t x = *(uint32_t *)a;
    uint32_t y = *(uint32_t *)b;

    return (x - y);
}

// for sorting by entry eid
int32_t cmp_func_eid(const void *a, const void *b)
{
    return ((*(ENTRY *)a).eid - (*(ENTRY *)b).eid);
}

// used to sort entries by size
int32_t cmp_func_esize(const void *a, const void *b)
{
    return ((*(ENTRY *)b).esize - (*(ENTRY *)a).esize);
}

/** \brief
 *  List struct init function.
 *
 * \return LIST
 */
LIST init_list()
{
    LIST list;
    list.count = 0;
    list.eids = NULL;

    return list;
}

/** \brief
 *  Spawns object init function.
 *
 * \return SPAWNS
 */
SPAWNS init_spawns()
{
    SPAWNS temp;
    temp.spawn_count = 0;
    temp.spawns = NULL;

    return temp;
}

/** \brief
 *  Binary search in a sorted list.
 *
 * \param list LIST                     list to be searched
 * \param searched uint32_t         searched item
 * \return int32_t                          index the item has or -1 if item wasnt found
 */
int32_t list_find(LIST list, uint32_t searched)
{
    int32_t first = 0;
    int32_t last = list.count - 1;
    int32_t middle = (first + last) / 2;

    while (first <= last)
    {
        if (list.eids[middle] < searched)
            first = middle + 1;
        else if (list.eids[middle] == searched)
            return middle;
        else
            last = middle - 1;

        middle = (first + last) / 2;
    }

    return -1;
}

/** \brief
 *  Adds an item to the list.
 *
 * \param list LIST*                    list to be added into
 * \param eid uint32_t              item to be added
 * \return void
 */
void list_add(LIST *list, uint32_t eid)
{
    if (list_find(*list, eid) != -1)
        return;

    if (list->count)
        list->eids = (uint32_t *)realloc(list->eids, (list->count + 1) * sizeof(uint32_t)); // realloc is slow
    else
        list->eids = (uint32_t *)malloc(sizeof(uint32_t)); // not freed, big issue
    list->eids[list->count] = eid;
    list->count++;
    qsort(list->eids, list->count, sizeof(uint32_t), cmp_func_uint);
}

// Returns 1 if list_a is a subset of list_b, 0 otherwise
int32_t list_is_subset(LIST list_a, LIST list_b)
{
    for (int32_t i = 0; i < list_a.count; i++)
    {
        if (list_find(list_b, list_a.eids[i]) == -1)
            return 0;
    }
    return 1;
}

/** \brief
 *  Removes given item from the list if it exists.
 *
 * \param list LIST*                    list to be removed from
 * \param eid uint32_t              item to be removed
 * \return void
 */
void list_remove(LIST *list, uint32_t eid)
{
    int32_t index = list_find(*list, eid);
    if (index == -1)
        return;

    list->eids[index] = list->eids[list->count - 1];
    list->eids = (uint32_t *)realloc(list->eids, (list->count - 1) * sizeof(uint32_t)); // realloc is slow
    list->count--;
    qsort(list->eids, list->count, sizeof(uint32_t), cmp_func_uint);
}

/*
// should really exist but im irresponsible
void list_free(LIST list) {
    free(list.eids);
}*/

/** \brief
 *  Copies contents of the 'source' list into 'destination' list.
 *
 * \param destination LIST*             list to copy into
 * \param source LIST                   list to copy from
 * \return void
 */
void list_copy_in(LIST *destination, LIST source)
{
    for (int32_t i = 0; i < source.count; i++)
        list_add(destination, source.eids[i]);
}

/** \brief
 *  Inits load list struct.
 *
 * \return LOAD_LIST
 */
LOAD_LIST init_load_list()
{
    LOAD_LIST temp;
    temp.count = 0;

    return temp;
}

// calculates distance of two 3D points
int32_t point_distance_3D(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t z1, int16_t z2)
{
    return (int32_t)sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2));
}

// misc convertion thing
CAMERA_LINK int_to_link(uint32_t link)
{
    CAMERA_LINK result;
    result.type = (link & 0xFF000000) >> 24;
    result.zone_index = (link & 0xFF0000) >> 16;
    result.cam_index = (link & 0xFF00) >> 8;
    result.flag = link & 0xFF;

    return result;
}

// misc cleanup thing
void delete_load_list(LOAD_LIST load_list)
{
    for (int32_t i = 0; i < load_list.count; i++)
        free(load_list.array[i].list);
}

// fixes path string supplied by the user, if it starts with "
void path_fix(char *fpath)
{
    if (fpath[0] == '\"')
    {
        strcpy(fpath, fpath + 1);
        *(strchr(fpath, '\0') - 1) = '\0';
    }
}

// following 3 are used for determining each camera path's distance relative to spawn, used to determine whether cam path is 'before' another one
DIST_GRAPH_Q graph_init()
{
    DIST_GRAPH_Q queue;
    queue.add_index = 0;
    queue.pop_index = 0;
    for (int32_t i = 0; i < QUEUE_ITEM_COUNT; i++)
    {
        queue.zone_indices[i] = -1;
        queue.camera_indices[i] = -1;
    }

    return queue;
}

// graph add new (for checking what zone comes 'first' in the level)
void graph_add(DIST_GRAPH_Q *graph, ENTRY *elist, int32_t zone_index, int32_t camera_index)
{
    int32_t n = graph->add_index;
    graph->zone_indices[n] = zone_index;
    graph->camera_indices[n] = camera_index;
    (graph->add_index)++;

    elist[zone_index].distances[camera_index] = n;
    elist[zone_index].visited[camera_index] = 1;
}

// graph pop out (for checking what zone comes 'first' in the level)
void graph_pop(DIST_GRAPH_Q *graph, int32_t *zone_index, int32_t *cam_index)
{
    int32_t n = graph->pop_index;
    *zone_index = graph->zone_indices[n];
    *cam_index = graph->camera_indices[n];
    (graph->pop_index)++;
}

// gets offset of the nth item in an entry
int32_t build_get_nth_item_offset(uint8_t *entry, int32_t n)
{
    return from_u32(entry + 0x10 + 4 * n);
}

// gets pointer to the nth item in an entry
uint8_t *build_get_nth_item(uint8_t *entry, int32_t n)
{
    return entry + build_get_nth_item_offset(entry, n);
}

// for reading txt files
// copied from stackoverflow
int32_t getdelim(char **linep, int32_t *n, int32_t delim, FILE *fp)
{
    int32_t ch;
    int32_t i = 0;
    if (!linep || !n || !fp)
    {
        errno = EINVAL;
        return -1;
    }
    if (*linep == NULL)
    {
        if (NULL == (*linep = malloc(*n = 128)))
        {
            *n = 0;
            errno = ENOMEM;
            return -1;
        }
    }
    while ((ch = fgetc(fp)) != EOF)
    {
        if (i + 1 >= *n)
        {
            char *temp = realloc(*linep, *n + 128);
            if (!temp)
            {
                errno = ENOMEM;
                return -1;
            }
            *n += 128;
            *linep = temp;
        }
        (*linep)[i++] = ch;
        if (ch == delim)
            break;
    }
    (*linep)[i] = '\0';
    return !i && ch == EOF ? -1 : i;
}

// for reading text files
int32_t getline(char **linep, int32_t *n, FILE *fp)
{
    return getdelim(linep, n, '\n', fp);
}
