#include "macros.h"

int scenery_recolor_main()
{
    char fpath[1000];
    printf("Path to color item:\n");
    scanf(" %[^\n]",fpath);
    if (fpath[0]=='\"')    {
        strcpy(fpath,fpath+1);
        *(strchr(fpath,'\0')-1) = '\0';
    }
    FILE* file1 = fopen(fpath, "rb+");

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


int texture_copy_main()
{
    char fpath[1000];
    printf("Path to source texture:\n");
    scanf(" %[^\n]",fpath);
    if (fpath[0]=='\"')    {
        strcpy(fpath,fpath+1);
        *(strchr(fpath,'\0')-1) = '\0';
    }
    FILE* file1 = fopen(fpath, "rb");
    unsigned char texture1[65536];
    fread(texture1, 65536, 1, file1);

    printf("Path to destination texture:\n");
    scanf(" %[^\n]",fpath);
    if (fpath[0]=='\"')    {
        strcpy(fpath,fpath+1);
        *(strchr(fpath,'\0')-1) = '\0';
    }
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
