#include "macros.h"

// for recoloring, i use some dumb algorithm that i think does /some/ job and thats about it
int scenery_recolor_main()
{
    char fpath[1000];
    printf("Path to color item:\n");
    scanf(" %[^\n]",fpath);
    path_fix(fpath);

    FILE* file1;
    if ((file1 = fopen(fpath, "rb+")) == NULL) {
        printf("Couldn't open file.\n\n");
        return 0;
    }

    int r_wanted, g_wanted, b_wanted;
    printf("R G B? [hex]\n");
    scanf("%x %x %x", &r_wanted, &g_wanted, &b_wanted);

    fseek(file1, 0, SEEK_END);
    int color_count = ftell(file1) / 4;
    unsigned char stuff[color_count * 4];
    rewind(file1);
    fread(stuff, color_count, 4, file1);

    // pseudograyscale of the wanted color
    int sum_wanted = r_wanted + g_wanted + b_wanted;


    for (int i = 0; i < color_count; i++)
    {
        // read current color
        unsigned char r = stuff[4 * i + 0];
        unsigned char g = stuff[4 * i + 1];
        unsigned char b = stuff[4 * i + 2];

        // get pseudograyscale of the current color
        int sum = r + g + b;

        // get new color
        int r_new = (sum * r_wanted) / sum_wanted;
        int g_new = (sum * g_wanted) / sum_wanted;
        int b_new = (sum * b_wanted) / sum_wanted;

        // clip it at 0xFF
        r_new = min(r_new, 0xFF);
        g_new = min(g_new, 0xFF);
        b_new = min(b_new, 0xFF);

        // print stuff
        printf("old: %2X %2X %2X\n", r, g, b);
        printf("new: %2X %2X %2X\n", r_new, g_new, b_new);

        // write back
        stuff[4 * i + 0] = r_new;
        stuff[4 * i + 1] = g_new;
        stuff[4 * i + 2] = b_new;
    }

    rewind(file1);
    fwrite(stuff, color_count, 4, file1);
    fclose(file1);

    return 0;
}


// garbage, probably doesnt work properly
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
    path_fix(path);

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


/** \brief
 *  Copies texture from one texture chunk to another, doesnt include cluts.
 *
 * \return int                          unused
 */
int texture_copy_main()
{
    char fpath[1000];
    printf("Path to source texture:\n");
    scanf(" %[^\n]",fpath);
    path_fix(fpath);
    FILE* file1 = fopen(fpath, "rb");
    unsigned char texture1[65536];
    fread(texture1, 65536, 1, file1);

    printf("Path to destination texture:\n");
    scanf(" %[^\n]",fpath);
    path_fix(fpath);
    FILE* file2 = fopen(fpath, "rwb+");
    unsigned char texture2[65536];
    fread(texture2, 65536, 1, file2);

    while (1)
    {
        int i;
        int bpp, src_x, src_y, width, height, dest_x, dest_y;
        printf("bpp src_x src_y width height dest-x dest-y? [set bpp to 0 to end]\n");
        scanf("%d %d %d %d %d %d %d", &bpp, &src_x, &src_y, &width, &height, &dest_x, &dest_y);
        int end = 0;

        switch(bpp)
        {
            case 4:
                for (i = 0; i < height; i++)
                {
                    int offset1 = (i + src_y) * 0x200 + src_x/2;
                    int offset2 = (i + dest_y) * 0x200 + dest_x/2;
                    printf("i: %2d offset1: %5d, offset2: %5d\n", i, offset1, offset2);
                    memcpy(texture2 + offset2, texture1 + offset1, (width * 4) / 8);
                }
                break;
            case 8:
                for (i = 0; i < height; i++)
                {
                    int offset1 = (i + src_y) * 0x200 + src_x;
                    int offset2 = (i + dest_y) * 0x200 + dest_x;
                    printf("i: %2d offset1: %5d, offset2: %5d\n", i, offset1, offset2);
                    memcpy(texture2 + offset2, texture1 + offset1, width);
                }
                break;
            case 0:
            default:
                end = 1;
                break;
        }

        if (end)
            break;
    }

    rewind(file2);
    fwrite(texture2, 65536, 1, file2);

    fclose(file1);
    fclose(file2);

    return 0;
}

/** \brief
 *  Prints out properties present in the file.
 *
 * \param path char*                    path to the property file
 * \return void
 */
void prop_main(char* path)
{
    FILE *file = NULL;
    unsigned int fsize, i, j, code, offset, offset_next;
    unsigned char *arr;

    if ((file = fopen(path, "rb")) == NULL)
    {
        printf("File could not be opened\n\n");
        return;
    }

    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    rewind(file);

    arr = (unsigned char *) malloc(fsize);
    fread(arr, fsize, sizeof(unsigned char), file);

    printf("\n");
    for (i = 0; i < from_u32(arr + 0xC); i++)
    {
        code = from_u16(arr + 0x10 + 8*i);
        offset = from_u16(arr + 0x12 + 8*i) + OFFSET;
        offset_next = from_u16(arr + 0x1A + 8 * i) + OFFSET;
        if (i == from_u32(arr + 0xC) - 1) offset_next = from_u16(arr);
        printf("0x%03X\t", code);
        switch(code)
        {
            case 0x29:
                printf("camera mode");
                break;
            case 0x4B:
                printf("positions/angles");
                break;
            case 0xC9:
                printf("avg point distance");
                break;
            case 0x103:
                printf("SLST");
                break;
            case 0x109:
                printf("path links");
                break;
            case 0x130:
                printf("FOV");
                break;
            case 0x13B:
            case 0x13C:
                printf("draw list");
                break;
            case 0x162:
                printf("progress thing?????");
                break;
            case 0x16D:
                printf("camera path related?????");
                break;
            case 0x173:
                printf("path number");
                break;
            case 0x174:
                printf("path item number");
                break;
            case 0x176:
                printf("link count");
                break;
            case 0x1A8:
                printf("warp/cam switch");
                break;
            case 0x1AA:
                printf("camera related?????");
                break;
            case 0x208:
            case 0x209:
                printf("load list");
                break;
            case 0x27F:
                printf("update scenery info");
                break;

            case 0x1FA:
                printf("item1 bg color");
                break;
            case 0x142:
                printf("item1 cam distance");
                break;
            default: break;
        }
        printf("\nheader\t");
        for (j = 0; j < 8; j++) {
            printf("%02X", arr[0x10 + 8*i + j]);
            if (j % 4 == 3) printf(" ");
        }
        printf("\ndata\t");

        for (j = 0; j < offset_next - offset; j++)
        {
            printf("%02X", arr[offset + j]);
            if (j % 4 == 3) printf(" ");
            if ((j % 16 == 15) && (j + 1 != offset_next - offset)) printf("\n\t");
        }

        printf("\n\n");
    }

    free(arr);
    fclose(file);
    printf("\n");
}

// im not gonna comment the resize stuff cuz it looks atrocious
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
    printf("Input the path to the directory or level whose contents you want to resize:\n");
    scanf(" %[^\n]",path);
    path_fix(path);

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
    int offset_start,offset_end, i;
    unsigned int checksum;
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
    int i,item1off,j,curr_off,next_off,group,rest,vert;
    long long int origin;
    int vertcount;

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
    vertcount = next_off-curr_off / 6;

    for (i = curr_off; i < next_off; i += 2)
    {
        group = 256 * buffer[i + 1] + buffer[i];
        vert = group / 16;
        rest = group % 16;
        if (status.gamemode == 2 && vert >= 2048)
        {
            vert = 4096 - vert;
            if (i < 4*vertcount + curr_off)
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
            if (i < 4*vertcount + curr_off)
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


/** \brief
 *  Rotations and rewards share one argument, this is for finding reasonable reward settings with specified angle.
 *
 * \return void
 */
void crate_rotation_angle()
{
    int angle;
    scanf("%d", &angle);
    while (angle < 0)
        angle += 360;
    while (angle > 360)
        angle -= 360;

    int game_angle = (0x1000 * angle) / 360;
    int b_value = game_angle >> 8;
    printf("A: %3d\n", game_angle & 0xFF);
    printf("B: %3d", game_angle >> 8);
    while(1)
    {
        b_value += 16;
        if (b_value > 105) break;
        printf(" | %3d", b_value);
    }
    printf(" (for rewards)\n\n");
}

void nsd_gool_table_print(char *fpath)
{
    FILE *file = NULL;
    if ((file = fopen(fpath, "rb")) == NULL)
    {
        printf("File could not be opened\n\n");
        return;
    }

    fseek(file,0,SEEK_END);
    int filesize = ftell(file);
    rewind(file);

    unsigned char buffer[filesize];
    fread(buffer, sizeof(unsigned char), filesize, file);

    int entry_count = from_u32(buffer + C2_NSD_ENTRY_COUNT);
    char temp[100];
    for (int i = 0; i < 64; i++)
    {
        int eid = from_u32(buffer + C2_NSD_ENTRY_TABLE + 0x10 + 8 * entry_count + 4 * i);
        printf("%2d: %s  ", i, eid_conv(eid, temp));
        if (i % 4 == 3)
            putchar('\n');
    }
    fclose(file);
}
