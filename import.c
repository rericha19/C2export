#include "macros.h"

// this entire thing is just awful, probably never use

int import_main(char *time, INFO status)
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
    path_fix(nsfpath);

    strncpy(nsfcheck,strchr(nsfpath,'\0')-3,3);
    if ((base = fopen(nsfpath,"rb")) == NULL || strcmp("NSF",nsfcheck))
    {
        printf("Could not open or invalid file type\n");
        return 0;
    }

    baselength = get_file_length(base);

    basebase = (unsigned char *) malloc(baselength);
    fread(basebase,sizeof(unsigned char),baselength,base);

    help = strrchr(nsfpath,'\\');
    *help = '\0';
    help2 = help + 1;

    sprintf(lcltemp,"%s\\%s_%s",nsfpath,time,help2);
    printf("\nInput the path to the folder with the files you want to import:\n");
    scanf(" %[^\n]",path);
    path_fix(path);

    importee = fopen(lcltemp,"wb");
    fwrite(basebase,sizeof(unsigned char),baselength,importee);

    import_file_lister(path, importee);
    fclose(base);
    fclose(importee);
    sprintf(status.temp,"Done!\n");
    printf(status.temp);
    return 1;
}


int import_file_lister(char *path, FILE *fnew)
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
                entrysize = get_file_length(file);
                if (entrysize + curr_off + 0x16 + (index + 2) * 4 >= 65536)
                    import_chunksave(nrmal, &index, &curr_off, &curr_chunk, fnew, offsets);
                offsets[index + 1] = offsets[index] + entrysize;
                fread(nrmal + curr_off, sizeof(unsigned char), entrysize, file);
                curr_off = curr_off + entrysize;
                index++;
            }
            fclose(file);
        }
    }
    import_chunksave(nrmal, &index, &curr_off, &curr_chunk, fnew, offsets);
    closedir(dr);
    return 0;
}

void import_chunksave(unsigned char *chunk, int *index, int *curr_off, int *curr_chunk, FILE *fnew, int offsets[])
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
