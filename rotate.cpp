#include "macros.h"

void rotate_main()
{
    double rotation;
    char path[MAX];
    FILE *file;
    unsigned char *entry;
    int filesize;

    scanf("%lf",&rotation);
    if (rotation <= 0 || rotation >= 360)
    {
        printf("Input a value in range (0; 360) degrees\n");
        return ;
    }

    rotation = 2*PI*rotation/360;

    printf("Input the path to the ENTRY you want to export:\n");
    scanf(" %[^\n]",path);
    if (path[0]=='\"')
    {
        strcpy(path,path+1);
        *(strchr(path,'\0')-1) = '\0';
    }

    if ((file = fopen(path, "rb")) == NULL)
    {
        printf("File specified could not be opened\n");
        return;
    }
    fseek(file,0,SEEK_END);
    filesize = ftell(file);
    rewind(file);
    entry = (unsigned char *) malloc(filesize);
    fread(entry, sizeof(unsigned char),filesize,file);
    if (entry[8] == 7) rotate_zone(entry, path, rotation);
    if (entry[8] == 3) rotate_scenery(entry, path, rotation);
    fclose(file);
}

void rotate_scenery(unsigned char *buffer, char *filepath, double rotation)
{
    unsigned int i,curr_off,next_off,group,rest,vert;
    curr_off = BYTE * buffer[0x15] + buffer[0x14];
    next_off = BYTE * buffer[0x19] + buffer[0x18];
    unsigned int verts[(next_off-curr_off)/6][2];
    int check1 = 0,check2 = 0;

    for (i = curr_off; i < next_off; i += 2)
    {
        group = 256 * buffer[i + 1] + buffer[i];
        vert = group / 16;
        rest = group % 16;
        if (i < (2*(next_off-curr_off)/3 + curr_off) - 1 && (i % 4 == 0))
        {
            check1++;
            //printf("%X\n",group);
        }
        else if (i >= (2*(next_off-curr_off)/3 + curr_off) - 1)
        {
            check2++;
            //printf("\n%X",group);
        }
        group = 16*vert + rest;
        buffer[i + 1] = group / 256;
        buffer[i] = group % 256;
    }

    printf("%d %d\n",check1,check2);
}

void rotate_zone(unsigned char *buffer, char *filepath, double rotation)
{
    printf("ayo man im a zone deadass\n");
}
