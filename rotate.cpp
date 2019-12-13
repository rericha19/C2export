#include "macros.h"

void rotate_main(char *time)
{
    double rotation;
    char path[MAX];
    FILE *file;
    unsigned char *entry;
    int filesize;

    scanf("%lf",&rotation);
    if (rotation < 0 || rotation >= 360)
    {
        printf("Input a value in range (0; 360) degrees\n");
        return ;
    }

    rotation = 2*PI*rotation/360;

    printf("%lf\n\n",rotation);

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
    if (entry[8] == 3) rotate_scenery(entry, path, rotation, time, filesize);
    fclose(file);
}

void rotate_scenery(unsigned char *buffer, char *filepath, double rotation, char *time, int filesize)
{
    FILE *filenew;
    char *help, lcltemp[MAX];
    int curr_off,next_off,group,rest,vert, i;
    curr_off = BYTE * buffer[0x15] + buffer[0x14];
    next_off = BYTE * buffer[0x19] + buffer[0x18];
    int vertcount = (next_off-curr_off)/6;
    int verts[vertcount][2];

    for (i = curr_off; i < curr_off + 6*vertcount; i += 2)
    {
        group = 256 * buffer[i + 1] + buffer[i];
        vert = group / 16;
        if (vert >= 2048)
        {
            vert = vert - 4096;
            if (i < (4*vertcount + curr_off) && (i % 4 == 0))
                verts[(i-curr_off)/4][0] = vert;

            else if (i >= (4*vertcount + curr_off))
                verts[(i - curr_off)/2-vertcount*2][1] = vert;

            vert = vert + 4096;
        }
        else
        {
            if (i < (4*vertcount + curr_off) && (i % 4 == 0))
                verts[(i-curr_off)/4][0] = vert;

            else if (i >= (4*vertcount + curr_off))
                verts[(i - curr_off)/2-vertcount*2][1] = vert;
        }
    }

    for (i = 0; i < vertcount; i++)
    {
        rot(&verts[i][0],&verts[vertcount - 1 - i][1], rotation);
    }

    for (i = curr_off; i < curr_off + 6 * vertcount; i++)
    {
        group = 256 * buffer[i + 1] + buffer[i];
        rest = group % 16;
        vert = group / 16;
        if (i < (4*vertcount + curr_off) && (i % 4 == 0))
            vert = verts[(i - curr_off)/4][0];

        else if (i >= (4*vertcount + curr_off))
            verts[(i - curr_off)/2-vertcount*2][1] = vert;
        group = 16 * vert + rest;
        buffer[i + 1] = group / 256;
        buffer[i] = group % 256;
    }

    help = strrchr(filepath,'\\');
    *help = '\0';
    help = help + 1;
    sprintf(lcltemp,"%s\\%s_%s",filepath,time,help);

    filenew = fopen(lcltemp,"wb");
    fwrite(buffer, sizeof(unsigned char), filesize, filenew);
    fclose(filenew);
}

void rotate_zone(unsigned char *buffer, char *filepath, double rotation)
{
    printf("ayo man im a zone deadass\n");
}


void rot(int *x, int *y, double rotation)
{
    int temp1, temp2;

    printf("%d %d\n",*x,*y);

    temp1 = *x * cos(rotation) - *y * sin(rotation);
    temp2 = *x * sin(rotation) + *y * cos(rotation);

    *x = temp1;
    *y = temp2;

    printf("%d %d\n",*x,*y);
}
