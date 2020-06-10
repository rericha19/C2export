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
    int curr_off,next_off, i;
    unsigned int group, rest, vert;
    curr_off = BYTE * buffer[0x15] + buffer[0x14];
    next_off = BYTE * buffer[0x19] + buffer[0x18];
    int vertcount = (next_off-curr_off)/6;
    unsigned int verts[vertcount][2];

    for (i = curr_off; i < curr_off + 6 * vertcount; i += 2)
    {
        group = 256 * buffer[i + 1] + buffer[i];
        vert = group / 16;
        if (i < (4*vertcount + curr_off) && (i % 4 == 0))
            verts[(i-curr_off)/4][0] = vert;

        else if (i >= (4*vertcount + curr_off))
            verts[(i - curr_off)/2-vertcount*2][1] = vert;
        if (vert > 2048) printf("x_index: %d, z_index: %d\n",(i-curr_off)/4, (i - curr_off)/2-vertcount*2);
    }

    for (i = 0; i < vertcount; i++)
        rotate_rotate(&verts[i][0],&verts[vertcount - 1 - i][1], rotation);

    for (i = curr_off; i < curr_off + 6 * vertcount; i += 2)
    {
        group = 256 * buffer[i + 1] + buffer[i];
        rest = group % 16;
        vert = group / 16;
        if (i < (4*vertcount + curr_off) && (i % 4 == 0))
            {
                vert = verts[(i - curr_off)/4][0];
            }

        else if (i >= (4*vertcount + curr_off))
            {
                vert = verts[(i - curr_off)/2-vertcount*2][1];
            }

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


void rotate_rotate(unsigned int *y,unsigned int *x, double rotation)
{
    int temp1, temp2;
    int x_t = *x, y_t = *y;

    temp1 = x_t;
    temp2 = y_t;

    temp1 = x_t * cos(rotation) - y_t * sin(rotation);
    temp2 = x_t * sin(rotation) + y_t * cos(rotation);

    *x = temp1;
    *y = temp2;
}
