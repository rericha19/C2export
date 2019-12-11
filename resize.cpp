#include "macros.h"

void resize_main(char *time, INFO status)
{
    FILE *level = NULL;
    char path[MAX] = "";
    double scale[3];
    int check = 1;
    DIR *df = NULL;

    scanf("%d %lf %lf %lf",&status.gamemode,&scale[0],&scale[1],&scale[2]);
    if (status.gamemode != 2 && status.gamemode != 3)
    {
        printf("[error] invalid gamemode, defaulting to 2");
        status.gamemode = 2;
    }
    printf("Input the path to the directory or level whose contents you want to export:\n");
    scanf(" %[^\n]",path);

    if (path[0]=='\"')
    {
        strcpy(path,path+1);
        *(strchr(path,'\0')-1) = '\0';
    }

    if ((df = opendir(path)) != NULL)  // opendir returns NULL if couldn't open directory
        resize_folder(df,path,scale,time,status);
    else if ((level = fopen(path,"rb")) != NULL)
        resize_level(level,path,scale,time,status);
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

void resize_level(FILE *level, char *filepath, double scale[3], char *time, INFO status)
{
    FILE *filenew;
    char *help, lcltemp[MAX];
    unsigned char buffer[CHUNKSIZE];
    int i,chunkcount;

    help = strrchr(filepath,'\\');
    *help = '\0';
    help = help + 1;
    sprintf(lcltemp,"%s\\%s_%s",filepath,time,help);

    filenew = fopen(lcltemp,"wb");
    fseek(level,0,SEEK_END);
    chunkcount = ftell(level) / CHUNKSIZE;
    rewind(level);

    for (i = 0; i < chunkcount; i++)
    {
        fread(buffer, sizeof(unsigned char), CHUNKSIZE, level);
        resize_chunk_handler(buffer, status, scale);
        fwrite(buffer, sizeof(unsigned char), CHUNKSIZE, filenew);
    }
    fclose(filenew);
}

void resize_chunk_handler(unsigned char *chunk, INFO status, double scale[3])
{
    int offset_start,offset_end, i, checksum;
    unsigned char *entry = NULL;
    if (chunk[2] != 0) return;

    for (i = 0; i < chunk[8]; i++)
    {
        offset_start = BYTE * chunk[0x11 + i * 4] + chunk[0x10 + i * 4];
        offset_end = BYTE * chunk[0x15 + i * 4] + chunk[0x14 + i * 4];
        if (!offset_end) offset_end = CHUNKSIZE;
        entry = chunk + offset_start;
        if (entry[8] == 7) resize_zone(offset_end - offset_start, entry, scale, status);
        if (entry[8] == 3) resize_scenery(offset_end - offset_start, entry, scale, status);
    }

    checksum = nsfChecksum(chunk);

    for (i = 0; i < 4; i++)
    {
        chunk[0xC + i] = checksum % 256;
        checksum /= 256;
    }
}

void resize_folder(DIR *df, char *path, double scale[3], char *time, INFO status)
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
                sprintf(status.temp,"%s\n",de->d_name);
                condprint(status);
                file = fopen(fpath,"rb");
                fseek(file,0,SEEK_END);
                filesize = ftell(file);
                rewind(file);
                buffer = (unsigned char *) calloc(sizeof(unsigned char), filesize);
                fread(buffer,sizeof(unsigned char),filesize,file);
                resize_scenery(filesize,buffer,scale,status);
                sprintf(help,"%s\\%s\\%s",path,time,strrchr(fpath,'\\')+1);
                filenew = fopen(help,"wb");
                fwrite(buffer,sizeof(unsigned char),filesize,filenew);
                fclose(file);
                fclose(filenew);
            }
            if (!strcmp("zone ",lcltemp))
            {
                sprintf(status.temp,"%s\n",de->d_name);
                condprint(status);
                file = fopen(fpath,"rb");
                fseek(file,0,SEEK_END);
                filesize = ftell(file);
                rewind(file);
                buffer = (unsigned char *) calloc(sizeof(unsigned char), filesize);
                fread(buffer,sizeof(unsigned char),filesize,file);
                resize_zone(filesize,buffer,scale,status);
                sprintf(help,"%s\\%s\\%s",path,time,strrchr(fpath,'\\')+1);
                filenew = fopen(help,"wb");
                fwrite(buffer,sizeof(unsigned char),filesize,filenew);
                fclose(file);
                fclose(filenew);
            }
        }
    }
}

void resize_zone(int fsize, unsigned char *buffer, double scale[3], INFO status)
{
    int i, itemcount;
    unsigned int coord;
    itemcount = buffer[0xC];
    int itemoffs[itemcount];

    for (i = 0; i < itemcount; i++)
        itemoffs[i] = 256 * buffer[0x11 + i*4] + buffer[0x10 + i*4];

    for (i = 0; i < 6; i++)
    {
        coord = from_u32(buffer+itemoffs[1] + i*4);
        if (coord >= (1LL<<31))
        {
            coord = (1LL<<32) - coord;
            coord = coord * scale[i % 3];
            coord = (1LL<<32) - coord;
            for (int j = 0; j < 4; j++)
            {
                buffer[itemoffs[1]+ i*4 + j] = coord % 256;
                coord = coord / 256;
            }
        }
        else {
            coord = coord * scale[i % 3];
            for (int j = 0; j < 4; j++)
            {
                buffer[itemoffs[1]+ i*4 + j] = coord % 256;
                coord = coord / 256;
            }
        }
    }

    for (i = 2; i < itemcount; i++)
       if (i > buffer[itemoffs[0]+0x188] || (i % 3 == 2))
           resize_entity(buffer + itemoffs[i], itemoffs[i+1] - itemoffs[i], scale, status);
}

void resize_entity(unsigned char *item, int itemsize, double scale[3], INFO status)
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

void resize_scenery(int fsize, unsigned char *buffer, double scale[3], INFO status)
{
    unsigned int i,item1off,j,curr_off,next_off,group,rest,vert;
    long long int origin;

    item1off = buffer[0x10];
    for (i = 0; i < 3; i++)
    {
        origin = from_u32(buffer + item1off + 4 * i);
        if (origin >= (1LL<<31))
        {
            origin = (1LL<<32) - origin;
            origin = scale[i] * origin;
            origin = (1LL<<32) - origin;
            for (j = 0; j < 4; j++)
            {
                buffer[item1off + j + i*4] = origin % 256;
                origin /= 256;
            }
        }
        else
        {
            origin = scale[i] * origin;
            for (j = 0; j < 4; j++)
            {
                buffer[item1off + j + i*4] = origin % 256;
                origin /= 256;
            }
        }
    }

    curr_off = BYTE * buffer[0x15] + buffer[0x14];
    next_off = BYTE * buffer[0x19] + buffer[0x18];

    for (i = curr_off; i < next_off; i += 2)
    {
        group = 256 * buffer[i + 1] + buffer[i];
        vert = group / 16;
        rest = group % 16;
        if (status.gamemode == 2 && vert >= 2048)
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
