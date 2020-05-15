#include "macros.h"

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
