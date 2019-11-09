// Crash 2 levels' entry exporter made by Averso
// made in the evening 08.11.2019 and it shows

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dir.h>
#define CHUNKSIZE 65536
#define KILL 2089250961
#define HELP 2089138798
#define EXPORT 2939723207

//counts entry types, counter[0] is total entry count
static int counter[22];

//functional prototypes, also list of functions excluding main and main1
void generic(unsigned char *buffer, int entrysize,char *lvlid, int st);
void gool(unsigned char *buffer, int entrysize,char *lvlid, int st);
void zone(unsigned char *buffer, int entrysize,char *lvlid, int st);
void model(unsigned char *buffer, int entrysize,char *lvlid, int st);
void countwipe();
void countprint();
void animation(unsigned char *buffer, int entrysize,char *lvlid, int st);
void pathstring(char *finalpath, char *type, int eid, char *lvlid, int st);
void idtofname(char *help, char *id);
void eid_conv(unsigned int m_value, char *eid);
int normal_chunk(unsigned char *buffer);
int texture_chunk(unsigned char *buffer, char *lvlid, int st);
int sound_chunk(unsigned char *buffer);
int wavebank_chunk(unsigned char *buffer);
int chunk_handler(unsigned char *buffer,int chunkid, char *lvlid, int st);
int exprt();
void intro_text();
void print_help();
const unsigned long hash(const char *str);

void generic(unsigned char *buffer, int entrysize,char *lvlid, int st)
{
    FILE *f;
    char cur_type[30];
    int eidint = 0;
    char eid[6];
    char path[50] = "";

    switch (buffer[8])
    {
    case 3:
        strcpy(cur_type,"scenery");
        break;
    case 4:
        strcpy(cur_type,"SLST");
        break;
    case 12:
        strcpy(cur_type,"sound");
        break;
    case 13:
        strcpy(cur_type,"music track");
        break;
    case 14:
        strcpy(cur_type,"instruments");
        break;
    case 15:
        strcpy(cur_type,"VCOL");
        break;
    case 19:
        strcpy(cur_type,"demo");
        break;
    case 20:
        strcpy(cur_type,"speech");
        break;
    case 21:
        strcpy(cur_type,"T21");
        break;
    default:
        break;
    }

    for (int i = 0; i < 4; i++)
        eidint = (256 * eidint) + buffer[7 - i];
    eid_conv(eidint,eid);

    pathstring(path, cur_type, eidint, lvlid, st);

    printf("\t T%2d, %s %s\n",buffer[8], cur_type, eid);
    printf("\t\t\t saved as '%s'\n", path+10);
    f = fopen(path,"wb");
    fwrite(buffer, sizeof(char), entrysize, f);
    fclose(f);
    counter[buffer[8]]++;
}

void gool(unsigned char *buffer, int entrysize,char *lvlid, int st)
{
    FILE *f;
    char cur_type[10];
    int eidint = 0;
    char eid[6];
    char path[50] = "";

    for (int i = 0; i < 4; i++)
        eidint = (256 * eidint) + buffer[7 - i];
    strncpy(cur_type,"GOOL",10);
    eid_conv(eidint,eid);

    pathstring(path, cur_type, eidint, lvlid, st);

    printf("\t T%2d, GOOL \t\t%s\n",buffer[8],eid);
    printf("\t\t\t saved as '%s'\n", path+10);
    f = fopen(path,"wb");
    fwrite(buffer, sizeof(char), entrysize, f);
    fclose(f);
    counter[11]++;
}

void zone(unsigned char *buffer, int entrysize,char *lvlid, int st)
{
    FILE *f;
    char cur_type[10];
    int eidint = 0;
    char eid[6];
    char path[50] = "";

    for (int i = 0; i < 4; i++)
        eidint = (256 * eidint) + buffer[7 - i];
    strncpy(cur_type,"zone",10);
    eid_conv(eidint,eid);

    pathstring(path, cur_type, eidint, lvlid, st);

    printf("\t T%2d, zone \t\t%s\n",buffer[8],eid);
    printf("\t\t\t saved as '%s'\n", path+10);
    f = fopen(path,"wb");
    fwrite(buffer, sizeof(char), entrysize, f);
    fclose(f);
    counter[7]++;
}

void model(unsigned char *buffer, int entrysize,char *lvlid, int st)
{
    FILE *f;
    int i;
    char cur_type[10];
    int eidint = 0;
    char eid[6] = "";
    char path[50] = "";
    int msize = 0;

    for (i = 0; i < 3; i++)
    {
        msize = 256 * buffer[0x28 + 1 + i * 2] + buffer[0x28 + i * 2];
        msize = msize / 8;
        buffer[0x28 + 1 + i * 2] = msize / 256;
        buffer[0x28 + i * 2] = msize % 256;
    }

    for (i = 0; i < 4; i++)
    {
        eidint = (256 * eidint) + buffer[7 - i];
    }


    strncpy(cur_type,"model",10);
    eid_conv(eidint,eid);

    pathstring(path, cur_type, eidint, lvlid, st);

    printf("\t T%2d, model \t\t%s\n",buffer[8],eid);
    printf("\t\t\t saved as '%s'\n", path+10);
    f = fopen(path,"wb");
    fwrite(buffer, sizeof(char), entrysize, f);
    fclose(f);
    counter[2]++;
}

void countwipe()
{
    int i;
    for (i = 0; i < 22; i++)
        counter[i] = 0;
}

void countprint()
{
    int i;
    char prefix[22][30] = {"Entries: \t", "Animations: \t", "Models: \t", "Sceneries: \t", "SLSTs: \t\t", "Textures: \t", "", \
    "Zones: \t\t",  "", "", "", "GOOL entries: \t", "Sound entries: \t", "Music tracks:\t", "Instruments: \t", "VCOL entries:\t", \
     "", "", "", "Demo entries:\t", "Speech entries:\t", "T21 entries: \t"};

    printf("\nMemory freed\n");
    for (i = 0; i < 22; i++)
        if (counter[i])
        {
            printf("%s %3d\t",prefix[i],counter[i]);
            for (int j = 0; j < 75 * (double) counter[i]/counter[0]; j++) printf("|");
            printf("\n");
            if (!i) printf("\n");
        }
}

void animation(unsigned char *buffer, int entrysize, char *lvlid, int st)
// does stuff to animations and saves them
{
    FILE *f;
    char eid[6];
    char *curitem;
    int coord, eidint = 0;
    char path[50] = "";
    char cur_type[10] = "";
    for (int i = 0; i < 4; i++)
        eidint = (256 * eidint) + buffer[7 - i];
    char *cpy;

    cpy = (char *) calloc(entrysize, sizeof(char));
    memcpy(cpy,buffer,entrysize);

    /*for (int j = 0; j < 0xC; j++)
    {
       *curitem = 0;
        for (int i = 0; i < 3; i++)
        {
            if (buffer[1] < 0x80)
            {
                coord = 256 * buffer[1 + 2*i] + buffer[2*i];
                coord = 8 * coord;
                buffer[1 + 2*i] = coord / 256;
                buffer[2 * i]   = coord % 256;
            }
            else
            {
                coord = CHUNKSIZE - 256 * buffer[1 + 2*i] + buffer[2*i];
                coord = 8 * coord;
                coord = CHUNKSIZE - coord;
                buffer[1 + 2*i] = coord / 256;
                buffer[2 * i]   = coord % 256;
            }
        }
    }*/

    strncpy(cur_type,"animation",9);
    pathstring(path, cur_type, eidint, lvlid, st);
    eid_conv(eidint,eid);
    printf("\t T%2d, animation \t%s\n",buffer[8],eid);
    printf("\t\t\t saved as '%s'\n", path+10);
    f = fopen(path,"wb");
    fwrite(buffer, sizeof(char), entrysize, f);
    fclose(f);
    counter[1]++;
}

void pathstring(char *finalpath, char *type, int eid, char *lvlid, int st)
//creates a string thats a path for the currently processed file
{
    char *p_end;
    char eidstr[6];
    char begn[7] = "S00000";
    eid_conv(eid,eidstr);
    char entrynumber[5];

    strncpy(finalpath, begn, 6);
    strncpy(finalpath + 6, lvlid, 2);
    strncpy(finalpath + 8, "\\\\", 2);
    sprintf(entrynumber,"%04d",counter[0]);

    if (st == 0)
    {
        strcpy(finalpath + 10, type);
        p_end = strchr(finalpath,'\0');
        *p_end = ' ';
        strncpy(p_end + 1, eidstr, 5);
        p_end = strchr(finalpath,'\0');
        *p_end = ' ';
        strncpy(p_end + 1, entrynumber,4);
        if (strcmp(type,"texture")) strncpy(p_end + 5, ".nsentry", 8);
    }
    else
    {
        strcpy(finalpath + 10, eidstr);
        p_end = strchr(finalpath, '\0');
        *p_end = ' ';
        strcpy(p_end + 1, type);
        p_end = strchr(finalpath,'\0');
        *p_end = ' ';
        strncpy(p_end + 1, entrynumber,4);
        if (strcmp(type,"texture")) strncpy(p_end + 5, ".nsentry", 8);
    }
}

void idtofname(char *help, char *id)
//changes the ID to filaname
{
    help[0] = 'S';
    help[1] = help[2] = help[3] = help[4] = help[5] = '0';
    help[6] = id[0];
    help[7] = id[1];
    help[8] = '.';
    help[9] = 'N';
    help[10]= 'S';
    help[11]= 'F';
    help[12]= '\0';
}

void eid_conv(unsigned int m_value, char *eid)
//converts int eid to string eid
{
    static const char charset[] =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "_!";
    eid[0] = charset[(m_value >> 25) & 0x3F];
    eid[1] = charset[(m_value >> 19) & 0x3F];
    eid[2] = charset[(m_value >> 13) & 0x3F];
    eid[3] = charset[(m_value >> 7) & 0x3F];
    eid[4] = charset[(m_value >> 1) & 0x3F];
    eid[5] = '\0';
}

int normal_chunk(unsigned char *buffer, char *lvlid, int st)
// breaks the normal chunk into entries and saves them one by one
{
    int i;
    int offset_start;
    int offset_end;
    unsigned char *entry;
    char eid[6];
    unsigned int eidnum = 0;

    for (i = 0; i < buffer[8]; i++)
    {
        counter[0]++;
        offset_start = 256 * buffer[0x11 + i * 4] + buffer[0x10 + i * 4];
        offset_end = 256 * buffer[0x15 + i * 4] + buffer[0x14 + i * 4];
        entry = (unsigned char *) calloc(offset_end - offset_start, sizeof(char));
        memcpy(entry,buffer + offset_start, offset_end - offset_start);
        switch (entry[8])
        {
        case 1:
            animation(entry, offset_end - offset_start, lvlid, st);
            break;
        case 2:
            model(entry, offset_end - offset_start, lvlid, st);
            break;
        case 7:
            zone(entry, offset_end - offset_start, lvlid, st);
            break;
        case 11:
            gool(entry, offset_end - offset_start, lvlid, st);
            break;

        case 3:
        case 4:
        case 12:
        case 13:
        case 14:
        case 15:
        case 19:
        case 20:
        case 21:
            generic(entry, offset_end - offset_start, lvlid, st);
            break;
        default:
            eidnum = 0;
            for (int j = 0; j < 4; j++)
                eidnum = (eidnum * 256) + entry[7 - j];
            eid_conv(eidnum,eid);
            printf("\t T%2d, unknown entry\t%s\n",entry[8],eid);
            break;
        }
    }
    return 0;
}

int texture_chunk(unsigned char *buffer, char *lvlid, int st)
// creates a file with the texture chunk, lvlid for file name, st is save type
{
    FILE *f;
    unsigned int eidint = 0;
    char path[50] = "";
    char cur_type[] = "texture";

    for (int i = 0; i < 4; i++)
        eidint = (256 * eidint) + buffer[7 - i];

    pathstring(path, cur_type, eidint, lvlid, st);

    //opening, writing and closing
    f = fopen(path,"wb");
    fwrite(buffer, sizeof(char), CHUNKSIZE, f);
    fclose(f);
    printf("\t saved as '%s'\n",path + 10);
    counter[5]++;
    counter[0]++;
    return 0;
}


int chunk_handler(unsigned char *buffer,int chunkid, char *lvlid, int st)
// receives the current chunk, its id (not ingame id, indexed by 1)
// and lvlid that just gets sent further
{
    char ctypes[7][20]={"Normal","Texture","Proto sound","Sound","Wavebank","Speech","Unknown"};
    printf("%s chunk %3d\n", ctypes[buffer[2]], chunkid*2 +1);
    switch (buffer[2])
    {
    case 0:
    case 3:
    case 4:
    case 5:
            normal_chunk(buffer, lvlid, st);
            break;
    case 1:
            texture_chunk(buffer,lvlid, st);
            break;

            normal_chunk(buffer, lvlid, st);
            break;
            normal_chunk(buffer, lvlid, st);
            break;
    default:
            break;
    }
    return 0;
}

int exprt()
// does the thing yea
{
    countwipe();
    FILE *file;     //the NSF thats to be exported
    int zonetype, numbytes, i, savetype = 0; //zonetype - what type the zones will be once exported, 8 or 16 neighbour ones, numbytes - size of file, i - iteration
    char sid[3];    //level id, but as its read,
    char fname[13];    //name of the NSF
    unsigned char *buffer, chunk[CHUNKSIZE];   //array where the level data is stored
    char path[9];

    //reading and conversion of the input, opening of the file
    scanf("%d",&zonetype);
    scanf("%s",sid);
    scanf("%d",&savetype);
    idtofname(fname,sid);

    if ((file = fopen(fname,"rb")) == NULL)
    {
        printf("[ERROR] File not found or could not be opened\n\n");
        return 0;
    }

    //checks the zonetype input, fixes if invalid
    if (!(zonetype == 8 || zonetype == 16))
    {
        printf("[error] Invalid zone type, defaulting to 8\n");
        zonetype = 8;
    }

    if (savetype > 1 || savetype < 0)
    {
        printf("[error] Invalid save type, defaulting to 0\n");
        zonetype = 0;
    }

    //makes the directory
    strncpy(path,"S00000",6);
    strncpy(path + 6, sid, 2);
    mkdir(path);

    //reading the file
    fseek(file, 0, SEEK_END);
    numbytes = ftell(file);
    rewind(file);
    buffer = (unsigned char *) calloc(numbytes, sizeof(char));
    fread(buffer, sizeof(char), numbytes, file);
    printf("The NSF has %d pages/chunks\n",(numbytes / CHUNKSIZE)*2 - 1);

    //hands the chunks to chunk_handler one by one
    for (i = 0; i < (numbytes/CHUNKSIZE); i++)
    {
        memcpy(chunk,buffer + i*CHUNKSIZE, CHUNKSIZE);
        chunk_handler(chunk,i,sid,savetype);
    }

    //closing and freeing memory
    free(buffer);
    countprint();
    if (fclose(file) == EOF)
    {
        printf("[ERROR] file could not be closed\n");
        return 2;
    }
    printf("\n");
    return 1;
}

void intro_text()
 //self-explanatory
{
    printf("Crash 2 level entry exporter for Crash 2 to Crash 3 porting, made by Averso.\n");
    printf("Type \"HELP\" for list of commands and their format.\n\n");
}

void print_help()
//self-explanatory
{
    printf("\n");
    for (int i = 0; i < 75; i++) printf("-");
    printf("\nCommands list:\n");
    printf("HELP\n\t prints a list of commands\n");
    printf("KILL\n\t ends the program\n");
    printf("EXPORT [zone type] [level ID] [save type] \n \
\t exports level's contents\n");
    printf("\t e.g. \"EXPORT 8 1E 1\"\n");
    printf("   [zone type]\t 8 for 8-neighbour zones, 16 for 16-neighbour zones\n");
    printf("   [level ID]\t ID of the level whose contents will be exported,\n");
    printf("\t\t the NSF has to be in the same directory as this program\n");
    printf("   [save type]\t how the exported entries get named\n");
    printf("\t\t 0 to save as entrytype_EID, 1 to save as EID_entrytype\n");

    printf("\nError messages:\n");
    printf("[ERROR] error message\n");
    printf("\tan error that could not be handled, the program stopped\n");
    printf("[error] error message\n");
    printf("\tan error that was handled\n");
    for  (int i = 0; i < 75; i++) printf("-");
    printf("\n\n");
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
    char s[100];
    while (1)
    {
        scanf("%s",s);
        printf("%lu\n",hash(s));
    }
    return 0;
}

int main()
 //actual main
{
    intro_text();
    char p_command[100];
    while (1)
    {
        scanf("%s",p_command);

        switch(hash(p_command))
        {
        case KILL:
            return 0;
        case HELP:
            print_help();
            break;
        case EXPORT:
            exprt();
            printf("\n");
            break;
        default:
            printf("[ERROR] '%s' is not a valid command.\n", p_command);
        }
    }
}

