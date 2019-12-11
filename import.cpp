#include "macros.h"

extern char temp[MAX];

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
