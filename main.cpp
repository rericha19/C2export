// Crash 2 levels' entry exporter made by Averso
// half the stuff barely works so dont touch it much

/*
In main loop it waits for a command thats hashed and etc and called, before that it fetches the current time and keeps it until the next command.
There are couple commands that can get called, help and info and stuff arent really interesting, just print some helping messages for at least some UX.

The actual commands are EXPORT, EXPORTALL and IMPORT.
EXPORTALL and EXPORT are fundamentally the same command, both hand file(s) to the "exprt" function.
EXPORTALL hands them one by one from a folder thats specified by the user, theres some basic check on whether its a .NSF file.

"exprt" function opens a file with a level, creates some folders and then chunk by chunk hands the level to "chunk_handler" function, then prints stuff and ends.
"chunk_handler" decides what kind of a chunk it got, either a texture one or not, then hands it to "texture_chunk" or "normal_chunk".

"texture_chunk" increases given entry counter and saves it to given location, no changes made to it at all
"normal_chunk" reads the chunk entry by entry and depending on the entry type hands the entry to another functions, generic or specific

"generic" receives a generic entry and just saves it with a certain name, no changes made to it at all
"zone", "gool", "model" and "animation" make changes based on gamemode, zonemode and portmode to change the files to a given format if needed

IMPORT
stuffs all files from a chosen folder into a chosen .nsf
*/

// TODO
// animations when porting
// t11s when porting C2 TO C3
// write a function for chunk saving
// fix scenery change
// rewrite all times you read big endian stuff, make a from_u16 function, use from_u32 more

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dir.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>

// idk why i made any of these macros now that i think about it, maybe to get rid of magic numbers,
// but there's still a ton of magic numbers everywhere
#define CHUNKSIZE 65536
#define BYTE 256
#define MAX 200

// more dumb things
#define C2_NEIGHBOURS_END 0x1B4
#define C2_NEIGHBOURS_FLAGS_END 0x1D4

// commands
#define KILL 2089250961u
#define HELP 2089138798u
#define EXPORT 2939723207u
#define EXPORTALL 1522383616u
#define CHANGEPRINT 2239644728u
#define CHANGEMODE 588358864u
#define STATUS 3482341513u
#define WIPE 2089682330u
#define IMPORT 3083219648u
#define INTRO 223621809
#define RESIZE 3426052343

int counter[22];    // counts entry types, counter[0] is total entry count
int print_en = 2;   // variable for storing printing status 0 - nowhere, 1 - screen, 2 - file, 3 - both
FILE *flog = NULL;  // file where the logs are exported if file print is enabled
char temp[MAX]= ""; // strings are written into this before being printed by condprint
int gamemode = 2;   // 2 for C2, 3 for C3
int portmode = 1;   // 0 for normal export, 1 for porting
unsigned int anim[1024][3];  // field one is model, field 3 is animation, field 2 is original animation when c3->c2
unsigned int animrefcount;   // count of animation references when porting c3 to c2

//functional prototypes, also list of functions excluding main and main1
void resize_chunk_handler(unsigned char *chunk);
void resize_entity(unsigned char *item, int itemsize, double scale[3]);
void resize_zone(int fsize, unsigned char *buffer, double scale[3]);
void resize_level(FILE *fpath, char *path, double scale[3], char *time);
void resize_folder(DIR *df, char *path, double scale[3], char *time);
void resize_scenery(int fsize, unsigned char *buffer, double scale[3]);
void resize_main(char *time);
void chunksave(unsigned char *chunk, int *index, int *curr_off, int *curr_chunk, FILE *fnew, int offsets[]);
int camfix(unsigned char *cam, int length);
void entitycoordfix(unsigned char *item, int itemlength);
unsigned int from_u32(unsigned char *data);
void scenery(unsigned char *buffer, int entrysize,char *lvlid, char *date);
unsigned int nsfChecksum(const unsigned char *data);
int filelister(char *path, FILE *base);
int import(char *time);
void printstatus(int zonetype);
void clrscr();
void askmode(int *zonetype);
void condprint(char *s);
void askprint();
void generic_entry(unsigned char *buffer, int entrysize,char *lvlid, char *date);
void gool(unsigned char *buffer, int entrysize,char *lvlid, char *date);
void zone(unsigned char *buffer, int entrysize,char *lvlid, char *date, int zonetype);
void model(unsigned char *buffer, int entrysize,char *lvlid, char *date);
void countwipe();
void countprint();
void animation(unsigned char *buffer, int entrysize,char *lvlid, char *date);
void pathstring(char *finalpath, char *type, int eid, char *lvlid, char *date);
void eid_conv(unsigned int m_value, char *eid);
int normal_chunk(unsigned char *buffer, char *lvlid, char *date, int zonetype);
int texture_chunk(unsigned char *buffer, char *lvlid, char *date);
int sound_chunk(unsigned char *buffer);
int wavebank_chunk(unsigned char *buffer);
int chunk_handler(unsigned char *buffer,int chunkid, char *lvlid, char *date, int zonetype);
int exprt(int zone, char *fpath, char *date);
void intro_text();
void print_help();
const unsigned long hash(const char *str);


void resize_main(char *time)
{
    FILE *level = NULL;
    char path[MAX] = "";
    double scale[3];
    int check = 1;
    DIR *df = NULL;

    scanf("%d %lf %lf %lf",&gamemode,&scale[0],&scale[1],&scale[2]);
    if (gamemode != 2 && gamemode != 3)
    {
        printf("[error] invalid gamemode, defaulting to 2");
        gamemode = 2;
    }
    printf("Input the path to the directory or level whose contents you want to export:\n");
    scanf(" %[^\n]",path);

    if (path[0]=='\"')
    {
        strcpy(path,path+1);
        *(strchr(path,'\0')-1) = '\0';
    }

    if ((df = opendir(path)) != NULL)  // opendir returns NULL if couldn't open directory
        resize_folder(df,path,scale,time);
    else if ((level = fopen(path,"rb")) != NULL)
        resize_level(level,path,scale,time);
    else
    {
        printf("Couldn't open\n");
        check--;
    }

    fclose(level);
    closedir(df);
    if (check) printf("\nEntries' dimensions resized\n\n");
    return ;
}

void resize_level(FILE *level, char *filepath, double scale[3], char *time)
{
    FILE *filenew;
    char *help, lcltemp[MAX];
    char buffer[CHUNKSIZE];
    int i,chunkcount;

    help = strrchr(filepath,'\\');
    *help = '\0';
    help = help + 1;
    sprintf(lcltemp,"%s\\%s_%s",filepath,time,help);

    filenew = fopen(lcltemp,"wb");
    fseek(level,0,SEEK_END);
    chunkcount = ftell(level) / CHUNKSIZE;
    rewind(level);

    printf("\n\n%d\n",chunkcount);

    for (i = 0; i < chunkcount; i++)
    {
        fread(buffer, sizeof(unsigned char), CHUNKSIZE, level);
        resize_chunk_handler(buffer);
        fwrite(buffer, sizeof(unsigned char), CHUNKSIZE, filenew);
    }
    fclose(filenew);
}

void resize_chunk_handler(unsigned char *chunk)
{

}

void resize_folder(DIR *df, char *path, double scale[3], char *time)
{
    struct dirent *de;
    char lcltemp[6] = "", help[MAX] = "";
    unsigned char *buffer = NULL;
    FILE *file, *filenew;
    int filesize;

    sprintf(help,"%s\\%s",path,time);
    mkdir(help);

    while ((de = readdir(df)) != NULL)
    {
        if (de->d_name[0] != '.')
        {
            char fpath[200] = "";
            sprintf(fpath,"%s\\%s",path,de->d_name);
            strncpy(lcltemp,de->d_name,5);
            if (buffer != NULL) free(buffer);

            if (!strcmp("scene",lcltemp))
            {
                sprintf(temp,"%s\n",de->d_name);
                condprint(temp);
                file = fopen(fpath,"rb");
                fseek(file,0,SEEK_END);
                filesize = ftell(file);
                rewind(file);
                buffer = (unsigned char *) calloc(sizeof(unsigned char), filesize);
                fread(buffer,sizeof(unsigned char),filesize,file);
                resize_scenery(filesize,buffer,scale);
                sprintf(help,"%s\\%s\\%s",path,time,strrchr(fpath,'\\')+1);
                filenew = fopen(help,"wb");
                fwrite(buffer,sizeof(unsigned char),filesize,filenew);
                fclose(file);
                fclose(filenew);
            }
            if (!strcmp("zone ",lcltemp))
            {
                sprintf(temp,"%s\n",de->d_name);
                condprint(temp);
                file = fopen(fpath,"rb");
                fseek(file,0,SEEK_END);
                filesize = ftell(file);
                rewind(file);
                buffer = (unsigned char *) calloc(sizeof(unsigned char), filesize);
                fread(buffer,sizeof(unsigned char),filesize,file);
                resize_zone(filesize,buffer,scale);
                sprintf(help,"%s\\%s\\%s",path,time,strrchr(fpath,'\\')+1);
                filenew = fopen(help,"wb");
                fwrite(buffer,sizeof(unsigned char),filesize,filenew);
                fclose(file);
                fclose(filenew);
            }
        }
    }
}

void resize_zone(int fsize, unsigned char *buffer, double scale[3])
{
    int i, itemcount, coord;
    itemcount = buffer[0xC];
    int itemoffs[itemcount];

    for (i = 0; i < itemcount; i++)
        itemoffs[i] = 256 * buffer[0x11 + i*4] + buffer[0x10 + i*4];

    for (i = 0; i < 6; i++)
    {
        coord = from_u32(buffer+itemoffs[1] + i*4);
        coord = coord * scale[i % 3];
        for (int j = 0; j < 4; j++)
        {
            buffer[itemoffs[1]+ i*4 + j] = coord % 256;
            coord = coord / 256;
        }
    }

    for (i = 2; i < itemcount; i++)
       resize_entity(buffer + itemoffs[i], itemoffs[i+1] - itemoffs[i], scale);
}

void resize_entity(unsigned char *item, int itemsize, double scale[3])
{
    int i;
    int off0x4B = 0;
    short int coord;

    for (i = 0; i < item[0xC]; i++)
    {
        if (item[0x10 + i*8] == 0x4B && item[0x11 +i*8] == 0)
            off0x4B = BYTE * item[0x13 + i*8] + item[0x12+i*8]+0xC;
    }

    if (off0x4B)
    {
        for (i = 0; i < item[off0x4B]*6; i += 2)
            {
                if (item[off0x4B + 0x5 + i] < 0x80)
                {
                    coord = BYTE * (signed) item[off0x4B + 0x5 + i] + (signed) item[off0x4B + 0x4 + i];
                    coord = coord * scale[i/2 % 3];
                    item[off0x4B + 0x5 + i] = coord / 256;
                    item[off0x4B + 0x4 + i] = coord % 256;
                }
                else
                {
                    coord = 65536 - BYTE * (signed) item[off0x4B + 0x5 + i] - (signed) item[off0x4B + 0x4 + i];
                    coord = coord * scale[i/2 % 3];
                    item[off0x4B + 0x5 + i] = 255 - coord / 256;
                    item[off0x4B + 0x4 + i] = 256 - coord % 256;
                    if (item[off0x4B + 0x4 + i] == 0) item[off0x4B + 0x5 + i]++;
                }
            }

    }
}

void resize_scenery(int fsize, unsigned char *buffer, double scale[3])
{
    unsigned int i,item1off,origin,j,curr_off,next_off,group,rest,vert;

    item1off = buffer[0x10];
    for (i = 0; i < 3; i++)
    {
        origin = from_u32(buffer + item1off + 4 * i);
        origin = scale[i] * origin;
        for (j = 0; j < 4; j++)
        {
            buffer[item1off + j + i*4] = origin % 256;
            origin /= 256;
        }
    }

    curr_off = BYTE * buffer[0x15] + buffer[0x14];
    next_off = BYTE * buffer[0x19] + buffer[0x18];

    for (i = curr_off; i < next_off; i += 2)
    {
        group = 256 * buffer[i + 1] + buffer[i];
        vert = group / 16;
        rest = group % 16;
        if (gamemode == 3 && vert >= 2048)
        {
            vert = 4096 - vert;
            if (i < 2*(next_off-curr_off)/3)
            {
                if (i % 4 == 0)
                    vert = vert * scale[0];
                else
                    vert = vert * scale[1];
            }
            else vert = vert * scale[2];
            vert = 4096 - vert;
        }
        else
        {
            if (i < 2*(next_off-curr_off)/3)
            {
                if (i % 4 == 0)
                    vert = vert * scale[0];
                else
                    vert = vert * scale[1];
            }
            else vert = vert * scale[2];
        }

        group = 16*vert + rest;
        buffer[i + 1] = group / 256;
        buffer[i] = group % 256;
    }
}

void chunksave(unsigned char *chunk, int *index, int *curr_off, int *curr_chunk, FILE *fnew, int offsets[])
// saves the current chunk
{
    char help[1024];
    int i;
    unsigned int checksum;

    for (i = 0; i < 1024; i++) help[i] = 0;

    *index = *index - 1;
    help[0] = 0x34;
    help[1] = 0x12;
    help[2] = help[3] = help[6] = help[7] = help[0xA] = help[0xB] = 0;
    help[4] = *curr_chunk % 256;
    help[5] = *curr_chunk / 256;
    help[8] = (*index + 1) % 256;
    help[9] = (*index + 1) / 256;
    for (i = 0; i < *index + 2; i++)
        {
            help[0x11 + i*4] = (0x10 + offsets[i] + (*index + 2)*4) / 256;
            help[0x10 + i*4] = (0x10 + offsets[i] + (*index + 2)*4) % 256;
        }
    memmove(chunk + 0x10 + (*index + 2)*4, chunk, CHUNKSIZE);
    memcpy(chunk,help,0x10 + (*index + 2)*4);

    checksum = nsfChecksum(chunk);

    for (i = 0; i < 4; i++)
    {
        chunk[0xC + i] = checksum % 256;
        checksum /= 256;
    }

    fwrite(chunk,sizeof(unsigned char), CHUNKSIZE, fnew);
    for (i = 0; i < CHUNKSIZE; i++)
        chunk[i] = 0;
    *index = 0;
    *curr_off = 0;
    *curr_chunk += 2;
}

int camfix(unsigned char *cam, int length)
{

    /*int i, j, shift = 0, curr_off = 0, temp;

    for (i = 0; i < cam[0xC]; i++)
    {
        if (cam[0x10 + i*8] == 7 && cam[0x11 +i*8] == 3)
        {
            memmove(cam + 0x10 + i*8, cam + 0x18 + i*8, length - 0x18 - i*8);
            curr_off = BYTE * cam[0x13 + i*8] + cam[0x12+i*8]+0xC-0x8;
            shift += 8;
            cam[0xC]--;
            for (j = 0; j < cam[0xC]; j++)
            {
                temp = BYTE * cam[0x13 + j*8] + cam[0x12 + j*8];
                temp -= 8;
                cam[0x13 + j*8] = temp / 256;
                cam[0x12 + j*8] = temp % 256;

            }
        }
    }

    if (curr_off)
    {
        printf("yes i got here lmao\n");
    }*/

    int i, curr_off;

    for (i = 0; i < cam[0xC]; i++)
    {
        if (cam[0x10 + i*8] == 0x29 && cam[0x11 +i*8] == 0)
        {
            curr_off = BYTE * cam[0x13 + i*8] + cam[0x12 + i*8] + 0xC + 4;
            switch (curr_off)
            {
            case 0x80:
                cam[curr_off] = 8;
                break;
            case 4:
                cam[curr_off] = 3;
                break;
            case 1:
                cam[curr_off] = 0;
                break;
            case 2:
                break;
            default:
                break;
            }
        }
    }
    return 0;
}

void entitycoordfix(unsigned char *item, int itemlength)
// fixes entity coords
{
    int i;
    int off0x4B = 0, off0x30E = 0;
    double scale;
    short int coord;

    //printf("%d\n", itemlength);
    for (i = 0; i < item[0xC]; i++)
    {
        if (item[0x10 + i*8] == 0x4B && item[0x11 +i*8] == 0)
            off0x4B = BYTE * item[0x13 + i*8] + item[0x12+i*8]+0xC;
        if (item[0x10 + i*8] == 0x0E && item[0x11 +i*8] == 3)
            off0x30E = BYTE * item[0x13 + i*8] + item[0x12+i*8]+0xC;
    }

    if (off0x4B && off0x30E)
    {
        switch (item[off0x30E + 4])
        {
        case 0:
            scale = 0.25;
            break;
        case 1:
            scale = 0.5;
            break;
        case 3:
            scale = 2;
            break;
        default:
            break;
        }

        for (i = 0; i < item[off0x4B]*6; i += 2)
            {
                if (item[off0x4B + 0x5 + i] < 0x80)
                {
                    coord = BYTE * (signed) item[off0x4B + 0x5 + i] + (signed) item[off0x4B + 0x4 + i];
                    coord = coord * scale;
                    item[off0x4B + 0x5 + i] = coord / 256;
                    item[off0x4B + 0x4 + i] = coord % 256;
                }
                else
                {
                    coord = 65536 - BYTE * (signed) item[off0x4B + 0x5 + i] - (signed) item[off0x4B + 0x4 + i];
                    coord = coord * scale;
                    item[off0x4B + 0x5 + i] = 255 - coord / 256;
                    item[off0x4B + 0x4 + i] = 256 - coord % 256;
                    if (item[off0x4B + 0x4 + i] == 0) item[off0x4B + 0x5 + i]++;
                }
            }

    }
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

void scenery(unsigned char *buffer, int entrysize,char *lvlid, char *date)
// does stuff to scenery
{
    FILE *f;
    char cur_type[10];
    int eidint = 0;
    char eid[6] = "";
    char path[MAX] = "";
    int curr_off, next_off, i, j, item1off;
    int vert, rest, group;
    unsigned int origin;

    strncpy(cur_type,"scenery",10);
    for (i = 0; i < 4; i++)
    {
        eidint = (BYTE * eidint) + buffer[7 - i];
    }
    eid_conv(eidint,eid);
    pathstring(path, cur_type, eidint, lvlid, date);

    if (portmode)
    {
        if (gamemode == 3) // c3 to c2
        {
            item1off = buffer[0x10];
            for (i = 0; i < 3; i++)
            {
                origin = from_u32(buffer + item1off + 4 * i);
                //origin += 0x8000;
                for (j = 0; j < 4; j++)
                {
                    buffer[item1off + j + i*4] = origin % 256;
                    origin /= 256;
                }

            }
            curr_off = BYTE * buffer[0x15] + buffer[0x14];
            next_off = BYTE * buffer[0x19] + buffer[0x18];
            for (i = curr_off; i < next_off; i += 2)
            {
                group = 256 * buffer[i + 1] + buffer[i];
                vert = group / 16;
                rest = group % 16;
              // /* if (vert > 2048)*/ vert -= 0x10;
             //   vert -= 0x800;
                group = 16*vert + rest;
                buffer[i + 1] = group / 256;
                buffer[i] = group % 256;
            }
        }
        else
        {
            ;
        }
    }
    sprintf(temp,"\t\t saved as '%s'\n", path);
    condprint(temp);
    f = fopen(path,"wb");
    fwrite(buffer, sizeof(char), entrysize, f);
    fclose(f);
    counter[3]++;
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

int filelister(char *path, FILE *fnew)
// opens all files in a directory one by one and appends them onto a nsf crudely
{
    // i have the arrays bigger than usual so i can do some dumb stuff with memcpy to make it easier for myself
    unsigned char nrmal[CHUNKSIZE+1024];
 //   unsigned char sound[CHUNKSIZE+1024];
 //   unsigned char spech[CHUNKSIZE+1024];
//    unsigned char instr[CHUNKSIZE+1024];
    unsigned char textu[CHUNKSIZE+1024];
    FILE *file;
    char temp1[MAX], temp2[6], eid[6];
    int entrysize, i, curr_chunk, curr_off = 0, eidint, index = 0, offsets[256];
//    unsigned int checksum;

    curr_chunk = 2 * (int) ftell(fnew) / CHUNKSIZE + 1;

    for (i = 0; i < CHUNKSIZE + 1024; i++)
    {
        nrmal[i] = 0;
    //    sound[i] = 0;
    //    spech[i] = 0;
   //     instr[i] = 0;
        textu[i] = 0;
    }

    // Pointer for directory entry
    struct dirent *de;
    // opendir() returns a pointer of DIR type.
    DIR *dr = opendir(path);

    if (dr == NULL)  // opendir returns NULL if couldn't open directory
    {
        printf("Could not open selected directory\n");
        return 0;
    }

    // checks for whether its a file, opening, doing stuff
    while ((de = readdir(dr)) != NULL)
    if ((de->d_name)[0]!='.')
    {
        sprintf(temp1,"%s\\%s",path,de->d_name);
        if ((file = fopen(temp1,"rb")) != NULL)
        {
            strncpy(temp2,de->d_name,5);
           // printf("%s\t",de->d_name);
            if (!strcmp(temp2,"textu"))
            // when its a texture file
            {
                fread(textu,sizeof(unsigned char), CHUNKSIZE, file);
                eidint = 0;
                for (i = 0; i < 4; i++)
                {
                    eidint = BYTE * eidint + textu[7 - i];
                }
                eid_conv(eidint,eid);
                //to avoid eid collisions with C2 Cr10T/Cr20T/Cr30T or vice versa
                if (!strcmp(eid,"Cr10T") || !strcmp(eid, "Cr20T") || !strcmp(eid,"Cr30T"))
                {
                    textu[4] = 0xEF;
                    textu[5] += 0x1D;
                }
                fwrite(textu,sizeof(unsigned char),CHUNKSIZE, fnew);
                curr_chunk += 2;
            }
            else if (!strcmp(temp2,"sound"))
            {
                ;
            }
            else if (!strcmp(temp2,"speec"))
            {
                ;
            }
            else if (!strcmp(temp2,"instr"))
            {
                ;
            }
            else if (!strcmp(temp2,"music") || !strcmp(temp2,"zone ") || !strcmp(temp2, "SLST ") || \
                     !strcmp(temp2,"scene") || !strcmp(temp2,"model") || !strcmp(temp2, "anima") || !strcmp(temp2,"GOOL "))
            {
                fseek(file,0,SEEK_END);
                entrysize = ftell(file);
                rewind(file);
                if (entrysize + curr_off + 0x16 + (index + 2) * 4 >= 65536)
                {
                    chunksave(nrmal, &index, &curr_off, &curr_chunk, fnew, offsets);
                }
                offsets[index + 1] = offsets[index] + entrysize;
                fread(nrmal + curr_off, sizeof(unsigned char), entrysize, file);
                curr_off = curr_off + entrysize;
                index++;
            }
            fclose(file);
        }
    }
    chunksave(nrmal, &index, &curr_off, &curr_chunk, fnew, offsets);
    closedir(dr);
    return 0;
}


int import(char *time)
// brute imports entries from a selected folder into a selected level
{
    FILE *base, *importee;
    char *help, *help2;
    char path[MAX] = "";
    char lcltemp[MAX] = "", nsfpath[MAX] = "", nsfcheck[4];
    unsigned char *basebase;
    int baselength;

    printf("Input the path to the .NSF you want the files to be imported into\n");
    printf("(it won't be overwritten, a new file will be created in the same folder)\n");
    scanf(" %[^\n]",nsfpath);

    if (nsfpath[0]=='\"')
            {
                strcpy(nsfpath,nsfpath+1);
                *(strchr(nsfpath,'\0')-1) = '\0';
            }

    strncpy(nsfcheck,strchr(nsfpath,'\0')-3,3);
    if ((base = fopen(nsfpath,"rb")) == NULL || strcmp("NSF",nsfcheck))
    {
        printf("Could not open or invalid file type\n");
        return 0;
    }

    fseek(base,0,SEEK_END);
    baselength = ftell(base);
    rewind(base);

    basebase = (unsigned char *) malloc(baselength);
    fread(basebase,sizeof(unsigned char),baselength,base);

    help = strrchr(nsfpath,'\\');
    *help = '\0';
    help2 = help + 1;

    sprintf(lcltemp,"%s\\%s_%s",nsfpath,time,help2);
    printf("\nInput the path to the folder with the files you want to import:\n");
    scanf(" %[^\n]",path);

    if (path[0]=='\"')
    {
        strcpy(path,path+1);
        *(strchr(path,'\0')-1) = '\0';
    }

    importee = fopen(lcltemp,"wb");
    fwrite(basebase,sizeof(unsigned char),baselength,importee);

    filelister(path,importee);
    fclose(base);
    fclose(importee);
    sprintf(temp,"Done!\n");
    printf(temp);
    return 1;
}

void printstatus(int zonetype)
// prints current settings
{
    printf("Selected game: Crash %d, porting: %d, zone neighbours (if C2->C3): %d\n\n", gamemode, portmode, zonetype);
}

void clrscr()
// wipes the current screen
{
    system("@cls||clear");
}

void askmode(int *zonetype)
// gets the info about the game and what to do with it
{
    char c;
    printf("Which game are the files from? [2/3]\n");
    printf("Change to other game's format? [Y/N]\n");
    scanf("%d %c",&gamemode, &c);
    c = toupper(c);

    if (gamemode > 3 || gamemode < 2)
    {
    	printf("[error] invalid game, defaulting to Crash 2\n");
    	gamemode = 2;
    }

    if (c == 'Y')
		portmode = 1;
    else
		{
		if (c == 'N')
			portmode = 0;
        else
            {
                printf("[error] invalid portmode, defaulting to 0 (not changing format)\n");
                portmode = 0;
            }
    	}

    if (gamemode == 2 && portmode == 1)
    {
        printf("How many neighbours should the exported files' zones have? [8/16]\n");
        scanf("%d",zonetype);
        if (!(*zonetype == 8 || *zonetype == 16))
        {
            printf("[error] invalid neighbour count, defaulting to 8\n");
            *zonetype = 8;
        }
    }
    printstatus(*zonetype);
}

void condprint(char *s)
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

void askprint()
// pops up the dialog that lets you pick where to write most of the print statements
{
    char dest[4][10] = {"to both", "to file", "here", "nowhere"};
    char pc;
    printf("Where to print status messages? [N - nowhere|F - file|H - here|B - both] (from fastest to slowest)\n");
    scanf(" %c",&pc);
    switch (toupper(pc))
    {
    case 'B':
        print_en = 3;
        break;
    case 'F':
        print_en = 2;
        break;
    case 'H':
        print_en = 1;
        break;
    case 'N':
        print_en = 0;
        break;
    default:
        print_en = 0;
        printf("[error] invalid input, defaulting to 'N'\n");
        break;
    }
    printf("Printing %s.\n\n",dest[3 - print_en]);
}

void generic_entry(unsigned char *buffer, int entrysize,char *lvlid, char *date)
// exports entries that need nor receive no change
{
    FILE *f;
    char cur_type[MAX];
    int eidint = 0;
    char eid[6];
    char path[MAX] = "";

    switch (buffer[8])      // this is really ugly but it works, so im not touching it
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
        eidint = (BYTE * eidint) + buffer[7 - i];
    eid_conv(eidint,eid);

    pathstring(path, cur_type, eidint, lvlid, date);

    sprintf(temp,"\t\t saved as '%s'\n", path);
    condprint(temp);

    f = fopen(path,"wb");
    fwrite(buffer, sizeof(char), entrysize, f);
    fclose(f);
    counter[buffer[8]]++;
}

void gool(unsigned char *buffer, int entrysize,char *lvlid, char *date)
// exports gool, no changes right now
{
    FILE *f;
    char cur_type[10];
    int eidint = 0;
    char eid[6];
    unsigned char *cpy;
    char path[MAX] = "";
    int curr_off = 0;
    int i, j;
    int help1, help2;
    char eidhelp1[6], eidhelp2[6];

    cpy = (unsigned char *) calloc(entrysize, sizeof(char));
    memcpy(cpy,buffer,entrysize);

    for (i = 0; i < 4; i++)
        eidint = (BYTE * eidint) + buffer[7 - i];
    strncpy(cur_type,"GOOL",10);
    eid_conv(eidint,eid);

    if (cpy[0xC] == 6 && portmode)
    {
        curr_off = BYTE * cpy[0x25] + cpy[0x24];
        if (gamemode == 2) //if its c2 to c3
        {
            ;
        }
        else // c3 to c2
        {
            while (curr_off < entrysize)
            {
                switch (cpy[curr_off])
                {
                case 1:
                    help1 = 0;
                    help2 = 0;
                    for (j = 0; j < 4; j++)
                        help1 = 256 * help1 + cpy[curr_off + 0x0F - j];
                    eid_conv(help1, eidhelp1);

                    if (eidhelp1[4] == 'G')
                    {
                        for (j = 0; j < 4; j++)
                            help2 = 256 * help2 + cpy[curr_off + 0x13 - j];
                        eid_conv(help2, eidhelp2);
                        if (eidhelp2[4] == 'V')
                        {
                            anim[animrefcount][1] = help1;
                            anim[animrefcount][2] = help2;
                            anim[animrefcount][3] = help2;
                            animrefcount++;
                        }
                    }

                    for (i = 1; i < 4; i++)
                        for (j = 0; j < 4; j++)
                            cpy[curr_off + 0x10 - 4*i + j] = cpy[curr_off + 0x10 + j];

                    curr_off += 0x14;
                    break;
                case 2:
                    curr_off += 0x8 + 0x10 * cpy[curr_off + 2];
                    break;
                case 3:
                    curr_off += 0x8 + 0x14 * cpy[curr_off + 2];
                    break;
                case 4:
                    curr_off = entrysize;
                    break;
                case 5:
                    curr_off += 0xC + 0x18 * cpy[curr_off + 2] * cpy[curr_off + 8];
                    break;
                }
            }
        }
    }

    pathstring(path, cur_type, eidint, lvlid, date);
    sprintf(temp,"\t\t saved as '%s'\n", path);
    condprint(temp);
    f = fopen(path,"wb");
    fwrite(cpy, sizeof(char), entrysize, f);
    free(cpy);
    fclose(f);
    counter[11]++;
}

void zone(unsigned char *buffer, int entrysize,char *lvlid, char *date, int zonetype)
// exports zones
{
    FILE *f;
    char cur_type[10];
    int eidint = 0;
    char eid[6] = "";
    char path[MAX] = "";
    unsigned char *cpy;
    int lcl_entrysize = entrysize;
    int i, j;
    int curr_off, lcl_temp, irrelitems, next_off;

    for (int i = 0; i < 4; i++)
        eidint = (BYTE * eidint) + buffer[7 - i];
    eid_conv(eidint,eid);

    cpy = (unsigned char *) calloc(entrysize, sizeof(char));
    memcpy(cpy,buffer,entrysize);

    if (portmode)
    {
        if (gamemode == 2) //c2 to c3
        {
            if (zonetype == 16)
            {
                lcl_entrysize += 0x40;
                cpy = (unsigned char *) realloc(cpy, lcl_entrysize);

                // offset fix
                for (j = 0; j < cpy[0xC]; j++)
                {
                    lcl_temp = BYTE * cpy[0x15 + j*4] + cpy[0x14 + j*4];
                    lcl_temp += 0x40;
                    cpy[0x15 + j*4] = lcl_temp / BYTE;
                    cpy[0x14 + j*4] = lcl_temp % BYTE;
                }

                // inserts byte in a very ugly way, look away
                curr_off = BYTE * cpy[0x11] + cpy[0x10];
                for (i = lcl_entrysize - 0x21; i >= curr_off + C2_NEIGHBOURS_END; i--)
                    cpy[i] = cpy[i - 0x20];

                for (i = curr_off + C2_NEIGHBOURS_END; i < curr_off + C2_NEIGHBOURS_FLAGS_END; i++)
                    cpy[i] = 0;

                for (i = lcl_entrysize - 1; i >= curr_off + 0x200; i--)
                    cpy[i] = cpy[i - 0x20];

                for (i = curr_off + 0x200; i < curr_off + 0x220; i++)
                    cpy[i] = 0;
            }
        }
        else //c3 to c2 removes bytes
        {
            if ((BYTE * cpy[0x15] + cpy[0x14] - BYTE * cpy [0x11] - cpy[0x10]) == 0x358)
            {
                for (j = 0; j < cpy[0xC]; j++)
                {
                    lcl_temp = BYTE * cpy[0x15 + j*4] + cpy[0x14 + j*4];
                    lcl_temp -= 0x40;
                    cpy[0x15 + j*4] = lcl_temp / BYTE;
                    cpy[0x14 + j*4] = lcl_temp % BYTE;
                }

                curr_off = BYTE * cpy[0x11] + cpy[0x10];
                for (i = curr_off + C2_NEIGHBOURS_END; i < lcl_entrysize - 20; i++)
                    cpy[i] = cpy[i + 0x20];

                for (i = curr_off + C2_NEIGHBOURS_FLAGS_END; i < lcl_entrysize - 20; i++)
                    cpy[i] = cpy[i + 0x20];

                if (cpy[curr_off + 0x190] > 8) cpy[curr_off + 0x190] = 8;
                lcl_entrysize -= 0x40;
            }

            irrelitems = cpy[cpy[0x10]+256*cpy[0x11]+0x184] + cpy[cpy[0x10] + 256*cpy[0x11] + 0x188];
            for (i = irrelitems; i < cpy[0xC]; i++)
            {
                //printf("zone %s, item %d\n",eid,i);
                curr_off = BYTE * cpy[0x11 + i*4] + cpy[0x10 + i*4];
                next_off = BYTE * cpy[0x15 + i*4] + cpy[0x14 + i*4];
                if (next_off == 0) next_off = CHUNKSIZE;
                entitycoordfix(cpy + curr_off, next_off - curr_off);
            }

            irrelitems = cpy[cpy[0x10] + 256*cpy[0x11] + 0x188];
            for (i = 0; i < irrelitems/3; i++)
            {
                curr_off = BYTE * cpy[0x19 + i*0xC] + cpy[0x18 + i*0xC];
                next_off = BYTE * cpy[0x19 + i*0xC + 0x4] + cpy[0x18 + i*0xC + 0x4];
                if (next_off == 0) next_off = CHUNKSIZE;
                camfix(cpy + curr_off, next_off - curr_off);
            }
        }
    }

    strncpy(cur_type,"zone",10);
    pathstring(path, cur_type, eidint, lvlid, date);

    sprintf(temp,"\t\t saved as '%s'\n", path);
    condprint(temp);
    f = fopen(path,"wb");
    fwrite(cpy, sizeof(char), lcl_entrysize, f);
    free(cpy);
    fclose(f);
    counter[7]++;
}

void model(unsigned char *buffer, int entrysize,char *lvlid, char *date)
// exports models, changes already
{
    FILE *f;
    int i;
    float scaling = 0;
    char cur_type[10];
    int eidint = 0;
    char eid[6] = "";
    char path[MAX] = "";
    int msize = 0;

    // scale change in case its porting
    if (gamemode == 2)
        scaling = 1./8;
    else scaling = 8;

    if (portmode)
    for (i = 0; i < 3; i++)
    {
        msize = BYTE * buffer[buffer[0x10] + 1 + i * 4] + buffer[buffer[0x10] + i * 4];
        msize = msize * scaling;
        buffer[buffer[0x10] + 1 + i * 4] = msize / BYTE;
        buffer[buffer[0x10] + i * 4] = msize % BYTE;
    }

    for (i = 0; i < 4; i++)
    {
        eidint = (BYTE * eidint) + buffer[7 - i];
    }

    strncpy(cur_type,"model",10);
    eid_conv(eidint,eid);

    pathstring(path, cur_type, eidint, lvlid, date);

    sprintf(temp,"\t\t saved as '%s'\n", path);
    condprint(temp);
    f = fopen(path,"wb");
    fwrite(buffer, sizeof(char), entrysize, f);
    fclose(f);
    counter[2]++;
}

void countwipe()
// wipes the stats
{
    for (int i = 0; i < 22; i++)
        counter[i] = 0;
}

void countprint()
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
            condprint(temp);
            for (int j = 0; j < (double) counter[i]/6; j++)
            {
                sprintf(temp,"|");
                condprint(temp);
            }
            sprintf(temp,"\n");
            condprint(temp);
        }
    sprintf(temp,"\n");
    condprint(temp);
}

void animation(unsigned char *buffer, int entrysize, char *lvlid, char *date)
// will do stuff to animations and save
{
    FILE *f;
    char eid[6];
    int eidint = 0;
    char path[MAX] = "";
    char cur_type[10] = "";
    char *cpy;
//    int curr_off;
    int lcltemp;

    for (int i = 0; i < 4; i++)
        eidint = (BYTE * eidint) + buffer[7 - i];

    strncpy(cur_type,"animation",9);
    eid_conv(eidint,eid);

    cpy = (char *) calloc(entrysize, sizeof(char));
    memcpy(cpy,buffer,entrysize);

    if (portmode)
    {
        if (gamemode == 2) //c2 to c3
        {
            // offset fix
            cpy = (char *) realloc(cpy, entrysize + cpy[0xC]*4);
            for (int j = 1; j <= cpy[0xC]; j++)
            {
                lcltemp = BYTE * cpy[0x11 + 4*j] + cpy[0x10 + 4*j];
                lcltemp += 4*j;
                //cpy[0x11 + 4*j] = lcltemp / BYTE;
                //cpy[0x10 + 4*j] = lcltemp % BYTE;
            }

            //autism
            for (int j = 0; j < cpy[0xC]; j++)
            {
                //curr_off = BYTE * cpy[0x11 + 4*j] + cpy[0x10 + 4*j];
            }
        }
        else    // c3 to c2
        {
            for (int j = 0; j < cpy[0xC]; j++)
            {
                ;
            }
        }
    }

    pathstring(path, cur_type, eidint, lvlid, date);
    sprintf(temp,"\t\t saved as '%s'\n", path);
    condprint(temp);
    f = fopen(path,"wb");
    fwrite(cpy, sizeof(char), entrysize, f);
    free(cpy);
    fclose(f);
    counter[1]++;
}

void pathstring(char *finalpath, char *type, int eid, char *lvlid, char *date)
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

int normal_chunk(unsigned char *buffer, char *lvlid, char *date, int zonetype)
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
        offset_start = BYTE * buffer[0x11 + i * 4] + buffer[0x10 + i * 4];
        offset_end = BYTE * buffer[0x15 + i * 4] + buffer[0x14 + i * 4];
        if (!offset_end) offset_end = CHUNKSIZE;
        entry = (unsigned char *) calloc(offset_end - offset_start, sizeof(char));
        memcpy(entry,buffer + offset_start, offset_end - offset_start);
        switch (entry[8])
        {
        case 1:
            animation(entry, offset_end - offset_start, lvlid, date);
            break;
        case 2:
            model(entry, offset_end - offset_start, lvlid, date);
            break;
        case 3:
            scenery(entry,offset_end - offset_start, lvlid, date);
            break;
        case 7:
            zone(entry, offset_end - offset_start, lvlid, date, zonetype);
            break;
        case 11:
            gool(entry, offset_end - offset_start, lvlid, date);
            break;

        case 4: case 12: case 13: case 14: case 15: case 19: case 20: case 21:
            generic_entry(entry, offset_end - offset_start, lvlid, date);
            break;
        default:
            eidnum = 0;
            for (int j = 0; j < 4; j++)
                eidnum = (eidnum * BYTE) + entry[7 - j];
            eid_conv(eidnum,eid);
            sprintf(temp,"\t T%2d, unknown entry\t%s\n",entry[8],eid);
            condprint(temp);
            break;
        }
        free(entry);
    }
    return 0;
}

int texture_chunk(unsigned char *buffer, char *lvlid, char *date)
// creates a file with the texture chunk, lvlid for file name, st is save type
{
    FILE *f;
    unsigned int eidint = 0;
    char path[MAX] = "";
    char cur_type[] = "texture";

    for (int i = 0; i < 4; i++)
        eidint = (BYTE * eidint) + buffer[7 - i];

    pathstring(path, cur_type, eidint, lvlid, date);

    //opening, writing and closing
    f = fopen(path,"wb");
    fwrite(buffer, sizeof(char), CHUNKSIZE, f);
    fclose(f);
    sprintf(temp,"\t\t saved as '%s'\n",path);
    condprint(temp);
    counter[5]++;
    counter[0]++;
    return 0;
}


int chunk_handler(unsigned char *buffer,int chunkid, char *lvlid, char *date, int zonetype)
// receives the current chunk, its id (not ingame id, indexed by 1)
// and lvlid that just gets sent further
{
    char ctypes[7][20]={"Normal","Texture","Proto sound","Sound","Wavebank","Speech","Unknown"};
    sprintf(temp,"%s chunk \t%03d\n", ctypes[buffer[2]], chunkid*2 +1);
    condprint(temp);
    switch (buffer[2])
    {
    case 0: case 3: case 4: case 5:
            normal_chunk(buffer, lvlid, date, zonetype);
            break;
    case 1:
            texture_chunk(buffer,lvlid, date);
            break;
    default:
            break;
    }
    return 0;
}

int exprt(int zone, char *fpath, char *date)
// does the thing yea
{
    char sid[3] = "";
    countwipe();
    FILE *file;     //the NSF thats to be exported
    unsigned int numbytes, i; //zonetype - what type the zones will be once exported, 8 or 16 neighbour ones, numbytes - size of file, i - iteration
    char temp[MAX] = ""; //, nsfcheck[4];
    unsigned char chunk[CHUNKSIZE];   //array where the level data is stored
    int port;
    char lcltemp[3][6] = {"","",""};

    /*strncpy(nsfcheck,strchr(fpath,'\0')-3,3);

    if (strcmp(nsfcheck,"NSF"))
    {
        sprintf(temp,"Not a .NSF file!\n");
        printf(temp);
        return 0;
    }*/

    if (portmode == 1)
       {
        if (gamemode == 2) port = 3;
            else port = 2;
       }
       else port = gamemode;


    sid[0] = *(strchr(fpath,'\0')-6);
    sid[1] = *(strchr(fpath,'\0')-5);


    if ((file = fopen(fpath,"rb")) == NULL)
    {
        sprintf(temp,"[ERROR] Level not found or could not be opened\n");
        printf(temp);
        return 0;
    }
    else
    {
        sprintf(temp,"Exporting level %s\n",sid);
        printf(temp);
    }
    //making directories
    sprintf(temp,"C%d_to_C%d",gamemode,port);
    mkdir(temp);
    sprintf(temp,"C%d_to_C%d\\\\%s",gamemode,port,date);
    mkdir(temp);
    sprintf(temp,"C%d_to_C%d\\\\%s\\\\S00000%s",gamemode,port,date,sid);
    mkdir(temp);

    //if file print is enabled, open the file
    sprintf(temp,"C%d_to_C%d\\\\%s\\\\log.txt",gamemode,port,date);
    if (print_en >= 2) flog = fopen(temp,"w");


    //reading the file
    fseek(file, 0, SEEK_END);
    numbytes = ftell(file);
    rewind(file);
    sprintf(temp,"The NSF [ID %s] has %d pages/chunks\n",sid,(numbytes / CHUNKSIZE)*2 - 1);
    condprint(temp);

    //hands the chunks to chunk_handler one by one
    for (i = 0; (unsigned int) i < (numbytes/CHUNKSIZE); i++)
    {
        fread(chunk, sizeof(char), CHUNKSIZE, file);
        chunk_handler(chunk,i,sid,date,zone);
    }
    strcpy(temp,"\n");
    condprint(temp);

    //closing and printing
    countprint();
    if (fclose(file) == EOF)
    {
        sprintf(temp,"[ERROR] file could not be closed\n");
        printf(temp);
        return 2;
    }
    if (print_en >= 2) fclose(flog);
    sprintf(temp,"\n");
    condprint(temp);

    for (i = 0; (unsigned int) i < animrefcount; i++)
    {
        eid_conv(anim[i][1],lcltemp[1]);
        eid_conv(anim[i][2],lcltemp[2]);
        eid_conv(anim[i][3],lcltemp[3]);
       // printf("ref: %03d, model: %s, animation: %s, new eid: %s\n", i, lcltemp[1], lcltemp[2], lcltemp[3]);
    }

    sprintf(temp,"Done!\n");
    printf(temp);
    return 1;
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
    char s[MAX];
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
    int zonetype = 8;
    time_t rawtime;
    struct tm * timeinfo;
    char dpath[MAX] = "", fpath[MAX] = "", moretemp[MAX] = "";
    char nsfcheck[4] = "";
    struct dirent *de;

    animrefcount = 0;
	intro_text();

    while (1)
    {
        char p_comm_cpy[MAX]= "";
        char lcltemp[9] = "";
        char p_command[MAX];
        scanf("%s",p_command);
        time(&rawtime);
        timeinfo = localtime(&rawtime );
        sprintf(lcltemp,"%02d_%02d_%02d",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
        for (unsigned int i = 0; i < strlen(lcltemp); i++)
                if (!isalnum(lcltemp[i])) lcltemp[i] = '_';
        for (unsigned int i = 0; i < strlen(p_command); i++)
            p_comm_cpy[i] = toupper(p_command[i]);
        switch(hash(p_comm_cpy))
        {
        case KILL:
            return 0;
        case HELP:
            print_help();
            break;
        case IMPORT:
            import(lcltemp);
            printf("\n");
            break;
        case EXPORT:
            printf("Input the path to the file whose contents you want to export:\n");
            scanf(" %[^\n]",fpath);
            if (fpath[0]=='\"')
            {
                strcpy(fpath,fpath+1);
            *(strchr(fpath,'\0')-1) = '\0';
        }
            askmode(&zonetype);
            exprt(zonetype,fpath,lcltemp);
            printf("\n");
            break;

        case EXPORTALL:
            {
            askmode(&zonetype);
            printf("\nInput the path to the folder whose NSFs you want to export:\n");
            scanf(" %[^\n]",dpath);
            if (dpath[0]=='\"')
            {
                strcpy(dpath,dpath+1);
            *(strchr(dpath,'\0')-1) = '\0';
            }
            // opendir() returns a pointer of DIR type.
            DIR *df = opendir(dpath);

            if (df == NULL)  // opendir returns NULL if couldn't open directory
            {
                printf("Could not open selected directory\n");
                break;
            }

            while ((de = readdir(df)) != NULL)
            {
                strncpy(nsfcheck,strchr(de->d_name,'\0')-3,3);
                strcpy(moretemp,de->d_name);
                if (de->d_name[0]!='.' && !strcmp(nsfcheck,"NSF"))
                {
                    sprintf(fpath,"%s\\%s",dpath,de->d_name);
                    exprt(zonetype, fpath, lcltemp);
                }
            }
            closedir(df);
            break;
            }
        case CHANGEPRINT:
            askprint();
            break;
        case WIPE:
            clrscr();
            intro_text();
            break;
        case RESIZE:
            resize_main(lcltemp);
            break;
        default:
            printf("[ERROR] '%s' is not a valid command.\n\n", p_command);
            break;
        }
    }
}
