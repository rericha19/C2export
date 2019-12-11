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
    printf("\nCrash 2/3 level entry exporter/importer/reformatter made by Averso.\n");
    printf("If any issue pops up (instructions are unclear or it crashes), DM me @ Averso#5633 (Discord).\n");
    printf("If printing to file is on, its printing into 'log.txt' located in folder of the current export.\n");
    printf("Type \"HELP\" for list of commands and their format. ");
    printf("Commands >ARE NOT< case sensitive.\n");
    printf("You can drag & drop the files and folders to this window instead of copying in the paths\n");
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

    printf("INTRO\n\t prints the intro text\n");

    printf("WIPE\n\t wipes current screen\n");

    printf("KILL\n\t ends the program\n");

    printf("CHANGEPRINT\n\t triggers the print selection\n");

    printf("IMPORT\n\t prompts an import screen thing for import of entries into C2 base level\n");

    printf("EXPORT\n");
    printf("\t exports level's contents with given settings\n");

    printf("RESIZE<G> <X> <Y> <Z> (float) \n");
    printf("\t e.g. 'resize3' 1.25 1 1' - files are from C3 and it gets stretched only on X\n");
    printf("\t parameters are according to games' orientation, Y is vertical and Z depth\n");
    printf("\t changes dimensions of certain entries according to given parameters\n\n");

    printf("EXPORTALL\n");
    printf("\t exports contents of all levels in the with given settings.\n");

    printf("\nError messages:\n");
    printf("[ERROR] error message\n");
    printf("\tan error that could not be handled, the program skipped some action\n");
    printf("[error] error message\n");
    printf("\tan error that was handled\n");
    for (int i = 0; i < 75; i++) printf("-");
    printf("\n\n");
}

void countprint(int counter[22], int portmode, int gamemode, char *temp, int print_en, FILE *flog)
// prints the stats
{
    int i;
    char lcltemp[] = "(fixed)";
    char lcltemp2[10] = "";
    char prefix[22][30] = {"Entries: \t", "Animations: \t", "Models:\t\t", "Sceneries: \t", "SLSTs: \t\t", "Textures: \t", "", \
    "Zones: \t\t",  "", "", "", "GOOL entries: \t", "Sound entries: \t", "Music tracks:\t", "Instruments: \t", "VCOL entries:\t", \
     "", "", "", "Demo entries:\t", "Speech entries:\t", "T21 entries: \t"};

    for (i = 0; i < 22; i++)
        if (counter[i])
        {
            if (portmode && (i == 2 || i == 7 || (i == 11 && portmode && (gamemode == 3)))) strcpy(lcltemp2,lcltemp);
                else strcpy(lcltemp2,"\t");
            sprintf(temp,"%s %s%3d\t",prefix[i],lcltemp2,counter[i]);
            condprint(temp, print_en, flog);
            for (int j = 0; j < (double) counter[i]/6; j++)
            {
                sprintf(temp,"|");
                condprint(temp, print_en, flog);
            }
            sprintf(temp,"\n");
            condprint(temp, print_en, flog);
        }
    sprintf(temp,"\n");
    condprint(temp, print_en, flog);
}


void condprint(char *s, int print_en, FILE *flog)
// conditional print controlled by print_en (print state) - both(3) file(2) here(1) or nowhere(0)
{
    switch (print_en)
    {
    case 3:
        printf("%s",s);
        fprintf(flog,"%s",s);
        break;
    case 1:
        printf("%s",s);
        break;
    case 2:
        fprintf(flog,"%s",s);
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


void countwipe(int counter[22])
// wipes the stats
{
    for (int i = 0; i < 22; i++)
        counter[i] = 0;
}


void askprint(int *print_en)
// pops up the dialog that lets you pick where to write most of the print statements
{
    char dest[4][10] = {"to both", "to file", "here", "nowhere"};
    char pc;
    printf("Where to print status messages? [N - nowhere|F - file|H - here|B - both] (from fastest to slowest)\n");
    scanf(" %c",&pc);
    switch (toupper(pc))
    {
    case 'B':
        *print_en = 3;
        break;
    case 'F':
        *print_en = 2;
        break;
    case 'H':
        *print_en = 1;
        break;
    case 'N':
        *print_en = 0;
        break;
    default:
        *print_en = 0;
        printf("[error] invalid input, defaulting to 'N'\n");
        break;
    }
    printf("Printing %s.\n\n",dest[3 - *print_en]);
}


const unsigned long hash(const char *str)
//changes the input string to a number, i just copied this over
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

int main1()
 //changes a string to hash, this is just for me to find the values of the expected strings
{
    char s[MAX];
    while (1)
    {
        scanf("%s",s);
        printf("%lu\n",hash(s));
    }
    return 0;
}


void eid_conv(unsigned int m_value, char *eid)
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


void make_path(char *finalpath, char *type, int eid, char *lvlid, char *date)
//creates a string thats a save path for the currently processed file
{
    char eidstr[6];
    eid_conv(eid,eidstr);
    int port;

    if (portmode == 1)
       {
        if (gamemode == 2) port = 3;
            else port = 2;
       }
    else port = gamemode;

    if (strcmp(type,"texture") == 0)
        sprintf(finalpath, "C%d_to_C%d\\\\%s\\\\S00000%s\\\\%s %s %d", gamemode, port, date, lvlid, type, eidstr, counter[0]);
    else
        sprintf(finalpath, "C%d_to_C%d\\\\%s\\\\S00000%s\\\\%s %s %d.nsentry", gamemode, port, date, lvlid, type, eidstr, counter[0]);
}
