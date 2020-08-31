#include "macros.h"

void printstatus(int zonetype, int gamemode, int portmode)
// prints current settings
{
    printf("Selected game: Crash %d, porting: %d, zone neighbours (if C2->C3): %d\n\n", gamemode, portmode, zonetype);
}

void intro_text()
 //self-explanatory
{
    for (int i = 0; i < 100; i++) printf("*");
    printf("\nCrash 2/3 level entry exporter/importer/reformatter/resizer/rotater made by Averso.\n");
    printf("If any issue pops up (instructions are unclear or it crashes), DM me @ Averso#5633 (Discord).\n");
    // printf("If printing to file is on, its printing into 'log.txt' located in folder of the current export.\n");
    printf("Type \"HELP\" for list of commands and their format. ");
    printf("Commands >ARE NOT< case sensitive.\n");
    printf("I don't really check much whether the files are valid, so make sure they actually are.\n");
    printf("You can drag & drop the files and folders to this window instead of copying in the paths\n");
    printf("Most stuff works only for Crash 2 !!!!\n");
    for (int i = 0; i < 100; i++) printf("*");
    printf("\n\n");
}

void print_help()
//self-explanatory
{
    printf("\n");
    for (int i = 0; i < 75; i++) printf("-");
    printf("\nCommand list:\n");
    printf("HELP\n\t prints a list of commands\n");

    printf("WIPE\n\t wipes current screen\n");

    printf("KILL\n\t ends the program\n");

    printf("CHANGEPRINT\n\t triggers the print selection\n");

    printf("IMPORT\n\t prompts an import screen thing for import of entries into a C2 level\n");

    printf("PROP\n\t prints a list of properties and values the specified item has\n");

    printf("EXPORT\n");
    printf("\t exports level's contents with given settings\n");

    printf("EXPORTALL\n");
    printf("\t exports contents of all levels in the folder with given settings.\n");

    printf("RESIZE\n");
    printf("\t e.g. 'resize3' 1.25 1 1' - files are from C3 and it gets stretched only on X\n");
    printf("\t parameters are according to games' orientation, Y is vertical and Z depth\n");
    printf("\t changes dimensions of the zones and scenery according to given parameters, messes up warps\n");

    printf("ROTATE\n");
    printf("\t rotates scenery or objects in a zone you specified\n");

    printf("TEXTURE\n");
    printf("\t copies from one texture chunk to another (doesnt include CLUTs)\n");

    printf("BUILD or REBUILD\n");
    printf("\t builds a level from stuff that it asks from you.\n");
    printf("\t somehow rebuild may give slightly different results than build, not inevitably worse though.\n");


    printf("SCEN_RECOLOR\n");
    printf("\t recolors scenery to the color user specifies (kinda sucks but w/e)\n");

    printf("A <value>\n");
    printf("\t for crate rotation shenanigans\n");

    printf("NSD\n");
    printf("\t prints gool table from the nsd\n");

    printf("\nError messages:\n");
    printf("[ERROR] error message\n");
    printf("\tan error that could not be handled, the program skipped some action or gave up\n");

    printf("[error] error message\n");
    printf("\tan error that was handled\n");

    printf("[warning] error message\n");
    printf("\tjust a warning, something may or may not be wrong\n");
    for (int i = 0; i < 75; i++) printf("-");
    printf("\n\n");
}

void countprint(INFO status)
// prints the stats
{
    int i;
    char lcltemp[] = "(fixed)";
    char lcltemp2[10] = "";
    char prefix[22][30] = {"Entries: \t", "Animations: \t", "Models:\t\t", "Sceneries: \t", "SLSTs: \t\t", "Textures: \t", "", \
    "Zones: \t\t",  "", "", "", "GOOL entries: \t", "Sound entries: \t", "Music tracks:\t", "Instruments: \t", "VCOL entries:\t", \
     "", "", "", "Demo entries:\t", "Speech entries:\t", "T21 entries: \t"};

    for (i = 0; i < 22; i++)
        if (status.counter[i])
        {
            if (status.portmode && (i == 2 || i == 7 || (i == 11 && status.portmode && (status.gamemode == 3))))
                strcpy(lcltemp2,lcltemp);
            else strcpy(lcltemp2,"\t");
            sprintf(status.temp,"%s %s%3d\t",prefix[i],lcltemp2,status.counter[i]);
            condprint(status);
            for (int j = 0; j < (double) status.counter[i]/6; j++)
            {
                sprintf(status.temp,"|");
                condprint(status);
            }
            sprintf(status.temp,"\n");
            condprint(status);
        }
    sprintf(status.temp,"\n");
    condprint(status);
}


void condprint(INFO status)
// conditional print controlled by print_en (print state) - both(3) file(2) here(1) or nowhere(0)
{
    switch (status.print_en)
    {
    case 3:
        printf("%s",status.temp);
        fprintf(status.flog,"%s",status.temp);
        break;
    case 1:
        printf("%s",status.temp);
        break;
    case 2:
        fprintf(status.flog,"%s",status.temp);
        break;
    default:
        break;
    }
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


void countwipe(INFO *status)
// wipes the stats
{
    for (int i = 0; i < 22; i++)
        status->counter[i] = 0;
}


void askprint(INFO *status)
// pops up the dialog that lets you pick where to write most of the print statements
{
    char dest[4][10] = {"to both", "to file", "here", "nowhere"};
    char pc;
    printf("Where to print status messages? [N - nowhere|F - file|H - here|B - both] (from fastest to slowest)[broken rn, doesnt write to file]\n");
    scanf(" %c",&pc);
    switch (toupper(pc))
    {
    case 'B':
        status->print_en = 3;
        break;
    case 'F':
        status->print_en = 2;
        break;
    case 'H':
        status->print_en = 1;
        break;
    case 'N':
        status->print_en = 0;
        break;
    default:
        status->print_en = 0;
        printf("[error] invalid input, defaulting to 'N'\n");
        break;
    }
    printf("Printing %s.\n\n",dest[3 - status->print_en]);
}


unsigned long hash(const char *str)
//changes the input string to a number, i just copied this over
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

int hash_main()
 //changes a string to hash, this is just for me to find the values of the expected strings
{
    char s[MAX];

    scanf("%s",s);
    printf("%lu\n",hash(s));

    return 0;
}

void swap_ints(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

char* eid_conv(unsigned int m_value, char *eid)
//converts int eid to string eid
{
    static const char charset[] =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "_!";
    *eid = charset[(m_value >> 25) & 0x3F];
    eid[1] = charset[(m_value >> 19) & 0x3F];
    eid[2] = charset[(m_value >> 13) & 0x3F];
    eid[3] = charset[(m_value >> 7) & 0x3F];
    eid[4] = charset[(m_value >> 1) & 0x3F];
    eid[5] = '\0';
    return eid;
}

unsigned int eid_to_int(char *eid)
{
    unsigned int result = 0;
    static const char charset[] =
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


void make_path(char *finalpath, char *type, int eid, char *lvlid, char *date, INFO status)
//creates a string thats a save path for the currently processed file
{
    char eidstr[6];
    eid_conv(eid,eidstr);
    int port;

    if (status.portmode == 1)
       {
        if (status.gamemode == 2) port = 3;
            else port = 2;
       }
    else port = status.gamemode;

    if (strcmp(type,"texture") == 0)
        sprintf(finalpath, "C%d_to_C%d\\\\%s\\\\S00000%s\\\\%s %s %d", status.gamemode, port, date, lvlid, type, eidstr, status.counter[0]);
    else
        sprintf(finalpath, "C%d_to_C%d\\\\%s\\\\S00000%s\\\\%s %s %d.nsentry", status.gamemode, port, date, lvlid, type, eidstr, status.counter[0]);
}

void askmode(int *zonetype, INFO *status)
// gets the info about the game and what to do with it
{
    char c;
    int help;
    printf("Which game are the files from? [2/3]\n");
    printf("Change to other game's format? [Y/N]\n");
    scanf("%d %c",&help, &c);
    c = toupper(c);

    status->gamemode = help;

    if (status->gamemode > 3 || status->gamemode < 2)
    {
    	printf("[error] invalid game, defaulting to Crash 2\n");
    	status->gamemode = 2;
    }

    if (c == 'Y')
		status->portmode = 1;
    else
		{
		if (c == 'N')
			status->portmode = 0;
        else
            {
                printf("[error] invalid portmode, defaulting to 0 (not changing format)\n");
                status->portmode = 0;
            }
    	}

    if (status->gamemode == 2 && status->portmode == 1)
    {
        printf("How many neighbours should the exported files' zones have? [8/16]\n");
        scanf("%d",zonetype);
        if (!(*zonetype == 8 || *zonetype == 16))
        {
            printf("[error] invalid neighbour count, defaulting to 8\n");
            *zonetype = 8;
        }
    }
    printstatus(*zonetype,status->gamemode,status->portmode);
}

long long int fact(int n)
{
    long long int result = 1;
    for (int i = 1; i <= n; i++)
        result *= i;

    return result;
}

int cmpfunc(const void *a, const void *b)
{
   return (*(int*) a - *(int*) b);
}

int comp(const void *a, const void *b)
{
    LOAD x = *(LOAD *) a;
    LOAD y = *(LOAD *) b;

    return (x.index - y.index);
}

int comp2(const void *a, const void *b)
{
    LOAD x = *(LOAD *) a;
    LOAD y = *(LOAD *) b;

    if (x.index != y.index)
        return (x.index - y.index);
    else
        return (y.type - x.type);
}

int pay_cmp(const void *a, const void *b)
{
    PAYLOAD x = *(PAYLOAD *) a;
    PAYLOAD y = *(PAYLOAD *) b;

    return (y.count - x.count);
}

int list_comp(const void *a, const void *b)
{
    unsigned int x = *(unsigned int*) a;
    unsigned int y = *(unsigned int*) b;

    return (x - y);
}

int load_list_sort(const void *a, const void *b)
{
    ITEM x = *(ITEM *) a;
    ITEM y = *(ITEM *) b;

    return (x.index - y.index);
}


void combinationUtil(int arr[], int data[], int start, int end, int index, int r, int ***res, int *counter)
{
    if (index == r)
    {
        *res = (int **) realloc(*res, (*counter + 1) * sizeof(int *));
        (*res)[*counter] = (int *) malloc(r * sizeof(int));
        memcpy((*res)[*counter], data, r * sizeof(int));
        (*counter)++;
        return;
    }

    for (int i = start; i <= end && end - i + 1 >= r - index; i++)
    {
        data[index] = arr[i];
        combinationUtil(arr, data, i+1, end, index+1, r, res, counter);
    }
}

void getCombinations(int arr[], int n, int r, int ***res, int *counter)
{
    int data[r];
    combinationUtil(arr, data, 0, n - 1, 0, r, res, counter);
}

int to_enum(const void *a)
{
    ENTRY x = *((ENTRY *) a);
    int result = 0;

    if (x.data == NULL) return 0;

    switch(*(int *)(x.data + 0x8))
    {
        case 0xB:
            result += 1;
            break;
        case 0x2:
            result += 2;
            break;
        case 0x1:
            result += 3;
            break;
        case 0x7:
            result += 4;
            break;
        case 0x4:
            result += 5;
            break;
        case 0x3:
            result += 6;
            break;
        default:
            result += MAX;
            break;
    }

    return result;
}

int cmp_entry(const void *a, const void *b)
// by entry type
{
    int x = to_enum(a);
    int y = to_enum(b);

    if (x != y) return (x - y);

    return ((*(ENTRY*) a).EID - (*(ENTRY *) b).EID);
}

int cmp_entry_eid(const void *a, const void *b)
// by entry eid
{
    return ((*(ENTRY*) a).EID - (*(ENTRY *) b).EID);
}

int cmp_entry_size(const void *a, const void *b)
{
    return ((*(ENTRY *) b).esize - (*(ENTRY *) a).esize);
}

int snd_cmp(const void *a, const void *b)
{
    ENTRY x = *(ENTRY *) a;
    ENTRY y = *(ENTRY *) b;

    if (build_entry_type(x) == ENTRY_TYPE_SOUND && build_entry_type(y) == ENTRY_TYPE_SOUND)
    {
        if (x.chunk == y.chunk)
            return y.esize - x.esize;
        else
            return y.chunk - x.chunk;
    }
    else
        return 0;

}

int relations_cmp(const void *a, const void *b)
{
    RELATION x = *(RELATION *) a;
    RELATION y = *(RELATION *) b;

    return y.value - x.value;
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
    if (list->count)
        list->eids = (unsigned int *) realloc(list->eids, (list->count + 1) * sizeof(unsigned int));
    else
        list->eids = (unsigned int *) malloc(sizeof(unsigned int));
    list->eids[list->count] = eid;
    list->count++;
    qsort(list->eids, list->count, sizeof(unsigned int), list_comp);
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
    list->eids = (unsigned int *) realloc(list->eids, (list->count - 1) * sizeof(unsigned int));
    list->count--;
    qsort(list->eids, list->count, sizeof(unsigned int), list_comp);
}

/** \brief
 *  Adds an item to the list if it's not there yet.
 *
 * \param list LIST*                    list to be inserted into
 * \param eid unsigned int              item to be inserted
 * \return void
 */
void list_insert(LIST *list, unsigned int eid){
    if (list_find(*list, eid) == -1)
        list_add(list, eid);
}


void list_copy_in(LIST *destination, LIST source)
{
    for (int i = 0; i < source.count; i++)
        list_insert(destination, source.eids[i]);
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

int point_distance_3D(short int x1, short int x2, short int y1, short int y2, short int z1, short int z2)
{
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2));
}

LINK int_to_link(unsigned int link)
{
    LINK result;
    result.type = (link & 0xFF000000) >> 24;
    result.zone_index = (link & 0xFF0000) >> 16;
    result.cam_index = (link & 0xFF00) >> 8;
    result.flag = link & 0xFF;

    return result;
}

void delete_load_list(LOAD_LIST load_list)
{
    for (int i = 0; i < load_list.count; i++)
        free(load_list.array[i].list);
}

void path_fix(char *fpath)
{
    if (fpath[0]=='\"')
    {
        strcpy(fpath,fpath + 1);
        *(strchr(fpath,'\0')-1) = '\0';
    }
}
