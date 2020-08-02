#include "macros.h"


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
