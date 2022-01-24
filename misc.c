#include "macros.h"


void intro_text()
 //self-explanatory
{
    for (int i = 0; i < 100; i++) printf("*");
    printf("\nCrash 2/3 level entry exporter/importer/reformatter/resizer/rotater made by Averso.\n");
    printf("If any issue pops up (instructions are unclear or it crashes), DM me @ Averso#5633 (Discord).\n");
    printf("Type \"HELP\" for list of commands and their format. Commands >ARE NOT< case sensitive.\n");
    printf("It is recommended to provide valid data as there may be edge cases that are unaccounted for.\n");
    printf("You can drag & drop the files and folders to this window instead of copying in the paths\n");
    printf("Most stuff works only for Crash 2 !!!!\n");
    for (int i = 0; i < 100; i++) printf("*");
    printf("\n\n");
}

void print_help()
//self-explanatory
{
    for (int i = 0; i < 100; i++) printf("-");
    printf("\nCommand list:\n");
    //printf("HELP\n");
    //printf("\t prints a list of commands\n");

    printf("WIPE\n");
    printf("\t wipes current screen\n");

    printf("KILL\n");
    printf("\t ends the program\n");

    //printf("CHANGEPRINT & IMPORT (obsolete)\n");
    //printf("\t  print selection, entry->NSF import (useless)\n");

    printf("BUILD & REBUILD\n");
    printf("\t builds a level from chosen inputs\n");

    printf("EXPORT & EXPORTALL\n");
    printf("\t exports level's contents with given settings (EXPORTALL exports all levels in a folder)\n");

    printf("ENT_RESIZE\n");
    printf("\t converts c3 entity to c2 entity (path adjustment)\n");

    printf("RESIZE <game> <xmult> <ymult> <zmult>\n");
    printf("\t changes dimensions of the zones and scenery according to given parameters (can mess up warps)\n");
    printf("\t e.g. 'resize3' 1.25 1 1' - files are from C3 and it gets stretched only on X\n");

    //printf("ROTATE\n");
    //printf("\t rotates scenery or objects in a zone you specified\n");

    printf("TEXTURE\n");
    printf("\t copies from one texture chunk to another (doesnt include CLUTs)\n");

    printf("SCEN_RECOLOR & TEXTURE_RECOLOR\n");
    printf("\t recolors scenery to the color user specifies (kinda sucks but w/e)\n");

    printf("NSD\n");
    printf("\t prints gool table from the nsd\n");

    printf("EID <EID>\n");
    printf("\t prints the EID in hex form\n");

    printf("PROP\n");
    printf("\t prints a list of properties in an entity\n");

    printf("PROP_REMOVE\n");
    printf("\t removes a property from an entity\n");

    printf("PROP_REPLACE\n");
    printf("\t replaces (or inserts) a property into dest entity with prop from source entity\n");

    printf("LL_ANALYZE\n");
    printf("\t stats about the level, integrity checks\n");

    printf("GEN_SPAWN\n");
    printf("\t NSD spawn generation for input level, zone and entity ID\n");

    printf("A <angle> & TIME\n");
    printf("\t modpack crate rotation, TT value\n");

    printf("ENT_MOVE\n");
    printf("\t moves an entity by chosen amount\n");

    printf("MODEL_REFS & MODEL_REFS_NSF\n");
    printf("\t lists model entry texture references\n");

    printf("\nError messages:\n");
    printf("[ERROR]   error message\n");
    printf("\tan error that could not be handled, the program skipped some action or gave up\n");

    printf("[error]   error message\n");
    printf("\tan error that was handled\n");

    printf("[warning] warning text\n");
    printf("\tjust a warning, something may or may not be wrong\n");
    for (int i = 0; i < 100; i++) printf("-");
    printf("\n");
}

void clrscr()
// wipes the current screen
{
    system("@cls||clear");
}

unsigned int from_u32(unsigned char *data)
// reads a word and returns an integer
{
    const unsigned char *p = data;
    unsigned int result = p[0];
    result |= p[1] << 8;
    result |= p[2] << 16;
    result |= p[3] << 24;
    return result;
}

unsigned int from_u16(unsigned char *data)
// reads a word and returns an integer
{
    const unsigned char *p = data;
    unsigned int result = p[0];
    result |= p[1] << 8;
    return result;
}

unsigned int from_u8(unsigned char *data) {
    const unsigned char *p = data;
    unsigned int result = p[0];
    return result;
}


//changes the input string to a number, i just copied this over from somewhere
unsigned long comm_str_hash(const char *str)
{
    unsigned long comm_str_hash = 5381;
    int c;

    while ((c = *str++))
        comm_str_hash = ((comm_str_hash << 5) + comm_str_hash) + c;
    return comm_str_hash;
}


//converts int eid to string eid
const char* eid_conv(unsigned int m_value, char *eid)
{
    const char charset[] =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "_!";

    char temp[6] = "abcde";
    temp[0] = charset[(m_value >> 25) & 0x3F];
    temp[1] = charset[(m_value >> 19) & 0x3F];
    temp[2] = charset[(m_value >> 13) & 0x3F];
    temp[3] = charset[(m_value >> 7) & 0x3F];
    temp[4] = charset[(m_value >> 1) & 0x3F];
    temp[5] = '\0';

    strncpy(eid, temp, 5);
    return eid;
}

// conversion of eid from string form to u32int form
unsigned int eid_to_int(char *eid)
{
    unsigned int result = 0;
    const char charset[] =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "_!";

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 0x40; j++)
            if (charset[j] == eid[i])
                result = (result << 6) + j;

    result = 1 + (result << 1);
    return result;
}


unsigned int nsfChecksum(const unsigned char *data)
// calculates chunk checksum
{
    unsigned int checksum = 0x12345678;
    for (int i = 0;i < 65536;i++) {
        if (i < 12 || i >= 16)
            checksum += data[i];
        checksum = checksum << 3 | checksum >> 29;
    }
    return checksum;
}


// integer comparison func for qsort
int cmp_func_int(const void *a, const void *b)
{
   return (*(int*) a - *(int*) b);
}


// used to sort payload ladder in descending order
int cmp_func_payload(const void *a, const void *b)
{
    PAYLOAD x = *(PAYLOAD *) a;
    PAYLOAD y = *(PAYLOAD *) b;

    if (y.count != x.count)
        return (y.count - x.count);
    else
        return (x.zone - y.zone);
}

// used in LIST struct
int cmp_func_uint(const void *a, const void *b)
{
    unsigned int x = *(unsigned int*) a;
    unsigned int y = *(unsigned int*) b;

    return (x - y);
}


int cmp_func_eid(const void *a, const void *b)
// by entry eid
{
    return ((*(ENTRY*) a).eid - (*(ENTRY *) b).eid);
}

// used to sort entries by size
int cmp_func_esize(const void *a, const void *b)
{
    return ((*(ENTRY *) b).esize - (*(ENTRY *) a).esize);
}


/** \brief
 *  List struct init function.
 *
 * \return LIST
 */
LIST init_list(){
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
SPAWNS init_spawns(){
    SPAWNS temp;
    temp.spawn_count = 0;
    temp.spawns = NULL;

    return temp;
}


/** \brief
 *  Binary search in a sorted list.
 *
 * \param list LIST                     list to be searched
 * \param searched unsigned int         searched item
 * \return int                          index the item has or -1 if item wasnt found
 */
int list_find(LIST list, unsigned int searched)
{
    int first = 0;
    int last = list.count - 1;
    int middle = (first + last)/2;

    while (first <= last)
    {
        if (list.eids[middle] < searched)
            first = middle + 1;
        else if (list.eids[middle] == searched)
            return middle;
        else
            last = middle - 1;

        middle = (first + last)/2;
    }

    return -1;
}


/** \brief
 *  Adds an item to the list.
 *
 * \param list LIST*                    list to be added into
 * \param eid unsigned int              item to be added
 * \return void
 */
void list_add(LIST *list, unsigned int eid)
{
    if (list_find(*list, eid) != -1)
        return;

    if (list->count)
        list->eids = (unsigned int *) realloc(list->eids, (list->count + 1) * sizeof(unsigned int));     // realloc is slow
    else
        list->eids = (unsigned int *) malloc(sizeof(unsigned int));         // not freed, big issue
    list->eids[list->count] = eid;
    list->count++;
    qsort(list->eids, list->count, sizeof(unsigned int), cmp_func_uint);
}

/** \brief
 *  Removes given item from the list if it exists.
 *
 * \param list LIST*                    list to be removed from
 * \param eid unsigned int              item to be removed
 * \return void
 */
void list_remove(LIST *list, unsigned int eid)
{
    int index = list_find(*list, eid);
    if (index == -1) return;

    list->eids[index] = list->eids[list->count - 1];
    list->eids = (unsigned int *) realloc(list->eids, (list->count - 1) * sizeof(unsigned int));    // realloc is slow
    list->count--;
    qsort(list->eids, list->count, sizeof(unsigned int), cmp_func_uint);
}

/*
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
    for (int i = 0; i < source.count; i++)
        list_add(destination, source.eids[i]);
}

/** \brief
 *  Inits load list struct.
 *
 * \return LOAD_LIST
 */
LOAD_LIST init_load_list(){
    LOAD_LIST temp;
    temp.count = 0;

    return temp;
}

// calculates distance of two 3D points
int point_distance_3D(short int x1, short int x2, short int y1, short int y2, short int z1, short int z2)
{
    return (int) sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2));
}

// misc convertion thing
CAMERA_LINK int_to_link(unsigned int link)
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
    for (int i = 0; i < load_list.count; i++)
        free(load_list.array[i].list);
}

// fixes path string supplied by the user, if it starts with "
void path_fix(char *fpath)
{
    if (fpath[0]=='\"')
    {
        strcpy(fpath,fpath + 1);
        *(strchr(fpath,'\0')-1) = '\0';
    }
}

// following 3 are used for determining each camera path's distance relative to spawn, used to determine whether cam path is 'before' another one
DIST_GRAPH_Q graph_init()
{
    DIST_GRAPH_Q queue;
    queue.add_index = 0;
    queue.pop_index = 0;
    for (int i = 0; i < QUEUE_ITEM_COUNT; i++) {
        queue.zone_indices[i] = -1;
        queue.camera_indices[i] = -1;
    }

    return queue;
}

void graph_add(DIST_GRAPH_Q *graph, ENTRY *elist, int zone_index, int camera_index)
{
    int n = graph->add_index;
    graph->zone_indices[n] = zone_index;
    graph->camera_indices[n] = camera_index;
    (graph->add_index)++;

    elist[zone_index].distances[camera_index] = n;
    elist[zone_index].visited[camera_index] = 1;
}


void graph_pop(DIST_GRAPH_Q *graph, int *zone_index, int *cam_index)
{
    int n = graph->pop_index;
    *zone_index = graph->zone_indices[n];
    *cam_index = graph->camera_indices[n];
    (graph->pop_index)++;
}

int build_get_nth_item_offset(unsigned char *entry, int n) {
    return from_u32(entry + 0x10 + 4 * n);
}

// copied from stackoverflow
int getdelim(char **linep, int *n, int delim, FILE *fp){
    int ch;
    int i = 0;
    if(!linep || !n || !fp){
        errno = EINVAL;
        return -1;
    }
    if(*linep == NULL){
        if(NULL==(*linep = malloc(*n=128))){
            *n = 0;
            errno = ENOMEM;
            return -1;
        }
    }
    while((ch = fgetc(fp)) != EOF){
        if(i + 1 >= *n){
            char *temp = realloc(*linep, *n + 128);
            if(!temp){
                errno = ENOMEM;
                return -1;
            }
            *n += 128;
            *linep = temp;
        }
        (*linep)[i++] = ch;
        if(ch == delim)
            break;
    }
    (*linep)[i] = '\0';
    return !i && ch == EOF ? -1 : i;
}


int getline(char **linep, int *n, FILE *fp){
    return getdelim(linep, n, '\n', fp);
}



DEPENDENCIES build_init_dep() {
    DEPENDENCIES dep;
    dep.array = NULL;
    dep.count = 0;

    return dep;
}
