#include "macros.h"

// one of old texture recoloring utils
int32_t texture_recolor_stupid()
{
    char fpath[1000];
    printf("Path to color item:\n");
    scanf(" %[^\n]", fpath);
    path_fix(fpath);

    FILE *file1;
    if ((file1 = fopen(fpath, "rb+")) == NULL)
    {
        printf("[ERROR] Couldn't open file.\n\n");
        return 0;
    }

    int32_t r_wanted, g_wanted, b_wanted;
    printf("R G B? [hex]\n");
    scanf("%x %x %x", &r_wanted, &g_wanted, &b_wanted);

    fseek(file1, 0, SEEK_END);
    int32_t file_len = ftell(file1);
    uint8_t *buffer = (uint8_t *)malloc(file_len * sizeof(uint8_t *));
    rewind(file1);
    fread(buffer, 1, file_len, file1);

    // pseudograyscale of the wanted color
    int32_t sum_wanted = r_wanted + g_wanted + b_wanted;

    for (int32_t i = 0; i < file_len; i += 2)
    {
        uint16_t temp = *(uint16_t *)(buffer + i);
        int32_t r = temp & 0x1F;
        int32_t g = (temp >> 5) & 0x1F;
        int32_t b = (temp >> 10) & 0x1F;
        int32_t a = (temp >> 15) & 1;

        r *= 8;
        g *= 8;
        b *= 8;

        // get pseudograyscale of the current color
        int32_t sum = r + g + b;

        // get new color
        int32_t r_new = (sum * r_wanted) / sum_wanted;
        int32_t g_new = (sum * g_wanted) / sum_wanted;
        int32_t b_new = (sum * b_wanted) / sum_wanted;

        // clip it at 0xFF
        r_new = min(r_new, 0xFF);
        g_new = min(g_new, 0xFF);
        b_new = min(b_new, 0xFF);

        // print stuff
        printf("old: %2X %2X %2X\n", r, g, b);
        printf("new: %2X %2X %2X\n", r_new, g_new, b_new);

        r_new /= 8;
        g_new /= 8;
        b_new /= 8;

        // write back
        uint16_t temp2 = (a << 15) + (b_new << 10) + (g_new << 5) + (r_new);
        *(uint16_t *)(buffer + i) = temp2;
    }

    rewind(file1);
    fwrite(buffer, 1, file_len, file1);
    fclose(file1);
    free(buffer);
    return 0;
}

// for recoloring, i use some dumb algorithm that i think does /some/ job and thats about it
int32_t scenery_recolor_main()
{
    char fpath[1000];
    printf("Path to color item:\n");
    scanf(" %[^\n]", fpath);
    path_fix(fpath);

    FILE *file1;
    if ((file1 = fopen(fpath, "rb+")) == NULL)
    {
        printf("[ERROR] Couldn't open file.\n\n");
        return 0;
    }

    fseek(file1, 0, SEEK_END);
    int32_t color_count = ftell(file1) / 4;
    uint8_t *buffer = (uint8_t *)malloc((color_count * 4) * sizeof(uint8_t *));
    rewind(file1);
    fread(buffer, color_count, 4, file1);

    int32_t r_wanted = 0x85;
    int32_t g_wanted = 0x28;
    int32_t b_wanted = 0x6A;

    int32_t sum_wanted = r_wanted + g_wanted + b_wanted;

    for (int32_t i = 0; i < color_count; i++)
    {
        // read current color
        uint8_t r = buffer[4 * i + 0];
        uint8_t g = buffer[4 * i + 1];
        uint8_t b = buffer[4 * i + 2];

        // get pseudograyscale of the current color
        int32_t sum = r + g + b;
        int32_t avg = sum / 3;

        // get new color
        int32_t r_new = (sum * r_wanted) / sum_wanted;
        int32_t g_new = (sum * g_wanted) / sum_wanted;
        int32_t b_new = (sum * b_wanted) / sum_wanted;

        r_new = (r_new + 2 * avg) / 3;
        g_new = (g_new + 2 * avg) / 3;
        b_new = (b_new + 2 * avg) / 3;

        float mult = 0.7f;
        r_new *= mult;
        g_new *= mult;
        b_new *= mult;

        // print stuff
        // printf("old: %2X %2X %2X\n", r, g, b);
        // printf("new: %2X %2X %2X\n", r_new, g_new, b_new);

        // clamp
        r_new = min(r_new, 255);
        g_new = min(g_new, 255);
        b_new = min(b_new, 255);
        r_new = max(0, r_new);
        g_new = max(0, g_new);
        b_new = max(0, b_new);

        // write back
        buffer[4 * i + 0] = r_new;
        buffer[4 * i + 1] = g_new;
        buffer[4 * i + 2] = b_new;
    }

    rewind(file1);
    fwrite(buffer, color_count, 4, file1);
    fclose(file1);
    free(buffer);

    printf("\nDone\n\n");
    return 0;
}

// garbage, probably doesnt work properly
void rotate_main(char *time)
{
    double rotation;
    char path[MAX];
    FILE *file;
    uint8_t *entry;
    int32_t filesize;

    scanf("%lf", &rotation);
    if (rotation < 0 || rotation >= 360)
    {
        printf("Input a value in range (0; 360) degrees\n");
        return;
    }

    rotation = 2 * PI * rotation / 360;

    printf("%lf\n\n", rotation);

    printf("Input the path to the ENTRY you want to export:\n");
    scanf(" %[^\n]", path);
    path_fix(path);

    if ((file = fopen(path, "rb")) == NULL)
    {
        printf("[ERROR] File specified could not be opened\n");
        return;
    }
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    rewind(file);
    entry = (uint8_t *)malloc(filesize); // freed here
    fread(entry, sizeof(uint8_t), filesize, file);
    // if (entry[8] == 7)
    // rotate_zone(entry, path, rotation);
    if (entry[8] == 3)
        rotate_scenery(entry, path, rotation, time, filesize);
    free(entry);
    fclose(file);
}

// rotates scenery
void rotate_scenery(uint8_t *buffer, char *filepath, double rotation, char *time, int32_t filesize)
{
    FILE *filenew;
    char *help, lcltemp[MAX];
    int32_t curr_off, next_off, i;
    uint32_t group, rest, vert;
    curr_off = BYTE * buffer[0x15] + buffer[0x14];
    next_off = BYTE * buffer[0x19] + buffer[0x18];
    int32_t vertcount = (next_off - curr_off) / 6;
    uint32_t **verts = (uint32_t **)malloc(vertcount * sizeof(uint32_t *)); // freed here
    for (i = 0; i < vertcount; i++)
        verts[i] = (uint32_t *)malloc(2 * sizeof(uint32_t *)); // freed here

    for (i = curr_off; i < curr_off + 6 * vertcount; i += 2)
    {
        group = 256 * buffer[i + 1] + buffer[i];
        vert = group / 16;
        if (i < (4 * vertcount + curr_off) && (i % 4 == 0))
            verts[(i - curr_off) / 4][0] = vert;

        else if (i >= (4 * vertcount + curr_off))
            verts[(i - curr_off) / 2 - vertcount * 2][1] = vert;
        if (vert > 2048)
            printf("x_index: %d, z_index: %d\n", (i - curr_off) / 4, (i - curr_off) / 2 - vertcount * 2);
    }

    for (i = 0; i < vertcount; i++)
        rotate_rotate(&verts[i][0], &verts[vertcount - 1 - i][1], rotation);

    for (i = curr_off; i < curr_off + 6 * vertcount; i += 2)
    {
        group = 256 * buffer[i + 1] + buffer[i];
        rest = group % 16;
        vert = group / 16;
        if (i < (4 * vertcount + curr_off) && (i % 4 == 0))
        {
            vert = verts[(i - curr_off) / 4][0];
        }

        else if (i >= (4 * vertcount + curr_off))
        {
            vert = verts[(i - curr_off) / 2 - vertcount * 2][1];
        }

        group = 16 * vert + rest;
        buffer[i + 1] = group / 256;
        buffer[i] = group % 256;
    }

    help = strrchr(filepath, '\\');
    *help = '\0';
    help = help + 1;
    sprintf(lcltemp, "%s\\%s_%s", filepath, time, help);

    filenew = fopen(lcltemp, "wb");
    fwrite(buffer, sizeof(uint8_t), filesize, filenew);
    fclose(filenew);
    for (i = 0; i < vertcount; i++)
        free(verts[i]);
    free(verts);
}

// rotates xy coord by some angle
void rotate_rotate(uint32_t *y, uint32_t *x, double rotation)
{
    int32_t temp1, temp2;
    int32_t x_t = *x, y_t = *y;

    temp1 = x_t;
    temp2 = y_t;

    temp1 = (int32_t)(x_t * cos(rotation) - y_t * sin(rotation));
    temp2 = (int32_t)(x_t * sin(rotation) + y_t * cos(rotation));

    *x = temp1;
    *y = temp2;
}

//  Copies texture from one texture chunk to another, doesnt include cluts.
int32_t texture_copy_main()
{
    char fpath[1000];
    printf("Path to source texture:\n");
    scanf(" %[^\n]", fpath);
    path_fix(fpath);
    FILE *file1 = fopen(fpath, "rb");
    uint8_t *texture1 = (uint8_t *)malloc(65536);

    printf("Path to destination texture:\n");
    scanf(" %[^\n]", fpath);
    path_fix(fpath);
    FILE *file2 = fopen(fpath, "rb+");
    uint8_t *texture2 = (uint8_t *)malloc(65536);

    if (file1 == NULL)
    {
        printf("[ERROR] Source texture could not be opened.\n\n");
        return 0;
    }

    if (file2 == NULL)
    {
        printf("[ERROR] Destination texture could not be opened.\n\n");
        return 0;
    }

    fread(texture1, 65536, 1, file1);
    fread(texture2, 65536, 1, file2);

    while (1)
    {
        int32_t i;
        int32_t bpp, src_x, src_y, width, height, dest_x, dest_y;
        printf("bpp src_x src_y width height dest-x dest-y? [set bpp to 0 to end]\n");
        scanf("%d", &bpp);
        if (bpp == 0)
            break;
        scanf(" %x %x %x %x %x %x", &src_x, &src_y, &width, &height, &dest_x, &dest_y);

        switch (bpp)
        {
        case 4:
            for (i = 0; i < height; i++)
            {
                int32_t offset1 = (i + src_y) * 0x200 + src_x / 2;
                int32_t offset2 = (i + dest_y) * 0x200 + dest_x / 2;
                printf("i: %2d offset1: %5d, offset2: %5d\n", i, offset1, offset2);
                memcpy(texture2 + offset2, texture1 + offset1, (width * 4) / 8);
            }
            break;
        case 8:
            for (i = 0; i < height; i++)
            {
                int32_t offset1 = (i + src_y) * 0x200 + src_x;
                int32_t offset2 = (i + dest_y) * 0x200 + dest_x;
                printf("i: %2d offset1: %5d, offset2: %5d\n", i, offset1, offset2);
                memcpy(texture2 + offset2, texture1 + offset1, width);
            }
            break;
        default:
            break;
        }
    }

    rewind(file2);
    fwrite(texture2, 65536, 1, file2);

    fclose(file1);
    fclose(file2);

    free(texture1);
    free(texture2);
    printf("Done\n\n");
    return 0;
}

// property printing util
void print_prop_header(uint8_t *arr, int32_t off)
{
    printf("\nheader\t");
    for (int32_t j = 0; j < 8; j++)
    {
        printf("%02X", arr[off + j]);
        if (j % 4 == 3)
            printf(" ");
    }
}

// property printing util
void print_prop_body(uint8_t *arr, int32_t offset, int32_t offset_next)
{
    printf("\ndata\t");

    for (int32_t j = 0; j < offset_next - offset; j++)
    {
        printf("%02X", arr[offset + j]);
        if (j % 4 == 3)
            printf(" ");
        if ((j % 16 == 15) && (j + 1 != offset_next - offset))
            printf("\n\t");
    }

    printf("\n\n");
}

/** \brief
 *  Prints out properties present in the file.
 *
 * \param path char *                    path to the property file
 * \return void
 */
void prop_main(char *path)
{
    FILE *file = NULL;
    uint32_t fsize, i, code, offset, offset_next;
    uint8_t *arr;

    if ((file = fopen(path, "rb")) == NULL)
    {
        printf("[ERROR] File could not be opened\n\n");
        return;
    }

    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    rewind(file);

    arr = (uint8_t *)malloc(fsize); // freed here
    fread(arr, fsize, sizeof(uint8_t), file);

    printf("\n");
    for (i = 0; i < build_prop_count(arr); i++)
    {
        code = from_u16(arr + 0x10 + 8 * i);
        offset = from_u16(arr + 0x12 + 8 * i) + OFFSET;
        offset_next = from_u16(arr + 0x1A + 8 * i) + OFFSET;
        if (i == (build_prop_count(arr) - 1))
            offset_next = from_u16(arr);
        printf("0x%03X\t", code);
        switch (code)
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
        default:
            break;
        }

        print_prop_header(arr, 0x10 + 8 * i);
        print_prop_body(arr, offset, offset_next);
    }

    free(arr);
    fclose(file);
    printf("Done \n\n");
}

// command for removing an entity property
void prop_remove_script()
{
    char fpath[1000];
    printf("Path to item:\n");
    scanf(" %[^\n]", fpath);
    path_fix(fpath);

    FILE *file1 = fopen(fpath, "rb");
    if (file1 == NULL)
    {
        printf("[ERROR] File could not be opened\n\n");
        return;
    }

    fseek(file1, 0, SEEK_END);
    int32_t fsize = ftell(file1);
    rewind(file1);

    uint8_t *item = malloc(fsize * sizeof(uint8_t));
    fread(item, 1, fsize, file1);
    fclose(file1);

    int32_t prop_code;
    printf("\nWhat prop do you wanna remove? (hex)\n");
    scanf("%x", &prop_code);
    int32_t fsize2 = fsize;
    item = build_rem_property(prop_code, item, &fsize2, NULL);

    char fpath2[1004];
    sprintf(fpath2, "%s-alt", fpath);

    FILE *file2 = fopen(fpath2, "wb");
    fwrite(item, 1, fsize, file2);
    fclose(file2);
    free(item);

    if (fsize == fsize2)
        printf("Seems like no changes were made\n");
    else
        printf("Property removed\n");
    printf("Done. Altered file saved as %s\n\n", fpath2);
}

// command for retrieving full property data
PROPERTY *build_get_prop_full(uint8_t *item, int32_t prop_code)
{

    PROPERTY *prop = malloc(sizeof(PROPERTY));
    prop->length = 0;
    int32_t property_count = build_prop_count(item);
    uint8_t property_header[8];

    for (int32_t i = 0; i < property_count; i++)
    {
        memcpy(property_header, item + 0x10 + 8 * i, 8);
        if (from_u16(property_header) == prop_code)
        {
            memcpy(prop->header, property_header, 8);
            int32_t next_offset = 0;
            if (i == property_count - 1)
                next_offset = *(uint16_t *)(item);
            else
                next_offset = *(uint16_t *)(item + 0x12 + (i * 8) + 8) + OFFSET;
            int32_t curr_offset = *(uint16_t *)(item + 0x12 + 8 * i) + OFFSET;
            prop->length = next_offset - curr_offset;
            prop->data = malloc(prop->length);
            memcpy(prop->data, item + curr_offset, prop->length);
            break;
        }
    }

    if (prop->length == 0)
        return NULL;
    else
        return prop;
}

// command for replacing property data
void prop_replace_script()
{
    char fpath[1000];
    printf("Path to source item:\n");
    scanf(" %[^\n]", fpath);
    path_fix(fpath);

    FILE *file1 = fopen(fpath, "rb");
    if (file1 == NULL)
    {
        printf("[ERROR] File could not be opened\n\n");
        return;
    }

    fseek(file1, 0, SEEK_END);
    int32_t fsize = ftell(file1);
    rewind(file1);

    uint8_t *item = malloc(fsize * sizeof(uint8_t));
    fread(item, 1, fsize, file1);
    fclose(file1);

    char fpath2[1000];
    printf("Path to destination item:\n");
    scanf(" %[^\n]", fpath2);
    path_fix(fpath2);

    FILE *file2 = fopen(fpath2, "rb+");
    if (file2 == NULL)
    {
        printf("[ERROR] File could not be opened\n\n");
        free(item);
        return;
    }

    fseek(file2, 0, SEEK_END);
    int32_t fsize2 = ftell(file2);
    rewind(file2);

    uint8_t *item2 = malloc(fsize2 * sizeof(uint8_t));
    fread(item2, 1, fsize2, file2);

    int32_t fsize2_before;
    int32_t fsize2_after;
    int32_t prop_code;
    printf("\nWhat prop do you wanna replace/insert? (hex)\n");
    scanf("%x", &prop_code);

    PROPERTY *prop = build_get_prop_full(item, prop_code);
    if (prop == NULL)
    {
        printf("Property wasnt found in the source file\n\n");
        free(item);
        free(item2);
        return;
    }

    fsize2_before = fsize2;
    item2 = build_rem_property(prop_code, item2, &fsize2, NULL);

    fsize2_after = fsize2;
    item2 = build_add_property(prop_code, item2, &fsize2, prop);

    rewind(file2);
    fwrite(item2, 1, fsize2, file2);
    fclose(file2);
    free(item);
    free(item2);
    if (fsize2_before == fsize2_after)
        printf("Property inserted. ");
    else
        printf("Property replaced. ");
    printf("Done.\n\n");
}

// im not gonna comment the resize stuff cuz it looks atrocious
void resize_main(char *time, DEPRECATE_INFO_STRUCT status)
{
    FILE *level = NULL;
    char path[MAX] = "";
    double scale[3];
    int32_t check = 1;
    DIR *df = NULL;

    scanf("%d %lf %lf %lf", &status.gamemode, &scale[0], &scale[1], &scale[2]);
    if (status.gamemode != 2 && status.gamemode != 3)
    {
        printf("[error] invalid gamemode, defaulting to 2");
        status.gamemode = 2;
    }
    printf("Input the path to the directory or level whose contents you want to resize:\n");
    scanf(" %[^\n]", path);
    path_fix(path);

    if ((df = opendir(path)) != NULL) // opendir returns NULL if couldn't open directory
        resize_folder(df, path, scale, time, status);
    else if ((level = fopen(path, "rb")) != NULL)
        resize_level(level, path, scale, time, status);
    else
    {
        printf("Couldn't open\n");
        check--;
    }

    fclose(level);
    closedir(df);
    if (check)
        printf("\nEntries' dimensions resized\n\n");
    return;
}

// yep resizes level
void resize_level(FILE *level, char *filepath, double scale[3], char *time, DEPRECATE_INFO_STRUCT status)
{
    FILE *filenew;
    char *help, lcltemp[MAX];
    uint8_t buffer[CHUNKSIZE];
    int32_t i, chunkcount;

    help = strrchr(filepath, '\\');
    *help = '\0';
    help = help + 1;
    sprintf(lcltemp, "%s\\%s_%s", filepath, time, help);

    filenew = fopen(lcltemp, "wb");
    fseek(level, 0, SEEK_END);
    chunkcount = ftell(level) / CHUNKSIZE;
    rewind(level);

    for (i = 0; i < chunkcount; i++)
    {
        fread(buffer, sizeof(uint8_t), CHUNKSIZE, level);
        resize_chunk_handler(buffer, status, scale);
        fwrite(buffer, sizeof(uint8_t), CHUNKSIZE, filenew);
    }
    fclose(filenew);
}

// yep resizes model
void resize_model(int32_t fsize, uint8_t *buffer, double scale[3])
{
    int32_t eid = from_u32(buffer + 4);
    int32_t pb10g = eid_to_int("Pb10G");
    int32_t bd10g = eid_to_int("Bd10G");
    int32_t cry1g = eid_to_int("Cry1G");
    int32_t cry2g = eid_to_int("Cry2G");
    int32_t rel1g = eid_to_int("Rel1G");
    int32_t ge10g = eid_to_int("Ge10G");
    int32_t ge30g = eid_to_int("Ge30G");
    int32_t ge40g = eid_to_int("Ge40G");

    if (eid == pb10g || eid == bd10g || eid == cry1g || eid == cry2g || eid == rel1g || eid == ge10g || eid == ge30g || eid == ge40g)
    {
        printf("model Pb10G/Bd10G/Cry1G/Cry2G/Rel1G/Ge10G/Ge30G/Ge40G, skipping\n");
        return;
    }

    int32_t o = build_get_nth_item_offset(buffer, 0);

    int32_t sx = *(int32_t *)(buffer + o);
    int32_t sy = *(int32_t *)(buffer + o + 4);
    int32_t sz = *(int32_t *)(buffer + o + 8);

    sx = (int32_t)(sx * scale[0]);
    sy = (int32_t)(sy * scale[1]);
    sz = (int32_t)(sz * scale[2]);

    *(int32_t *)(buffer + o) = sx;
    *(int32_t *)(buffer + o + 4) = sy;
    *(int32_t *)(buffer + o + 8) = sz;
}

// yep resizes animation
void resize_animation(int32_t fsize, uint8_t *buffer, double scale[3])
{
    int32_t itemc = build_item_count(buffer);

    for (int32_t i = 0; i < itemc; i++)
    {
        int32_t o = build_get_nth_item_offset(buffer, i);

        if (*(int32_t *)(buffer + o + 0xC))
        {
            int32_t xyz_iter = 0;
            int32_t end = *(int32_t *)(buffer + o + 0x14);
            for (int32_t j = 0x1C; j < end; j += 4)
            {
                int32_t val = *(int32_t *)(buffer + o + j);
                val = (int32_t)(val * scale[xyz_iter]);
                *(int32_t *)(buffer + o + j) = val;
                xyz_iter = (xyz_iter + 1) % 3;
            }
        }
    }
}

// yep fixes camera distance
void resize_zone_camera_distance(int32_t fsize, uint8_t *buffer, double scale[3])
{
    int32_t cam_c = build_get_cam_item_count(buffer);

    for (int32_t i = 0; i < cam_c; i += 3)
    {
        int32_t o = build_get_nth_item_offset(buffer, 2 + i + 1);

        uint8_t *entity = buffer + o;
        for (int32_t j = 0; j < build_prop_count(entity); j++)
        {
            int32_t code = from_u16(entity + 0x10 + 8 * j);
            int32_t offset = from_u16(entity + 0x12 + 8 * j) + OFFSET;
            int32_t val_c = from_u16(entity + 0x16 + 8 * j);

            if (code == ENTITY_PROP_CAM_DISTANCE)
            {
                int32_t prop_meta_len = 4;
                if (val_c == 2 || val_c == 3)
                    prop_meta_len = 8;

                for (int32_t k = 0; k < val_c * 4; k++)
                {
                    int32_t real_off = o + offset + prop_meta_len + (k * 2);

                    int32_t val = *(int16_t *)(buffer + real_off);
                    val = (int16_t)(val * scale[0]);
                    *(int16_t *)(buffer + real_off) = val;
                }
            }
        }
    }
}

// yep resizes chunk
void resize_chunk_handler(uint8_t *chunk, DEPRECATE_INFO_STRUCT status, double scale[3])
{
    int32_t offset_start, offset_end, i;
    uint32_t checksum;
    uint8_t *entry = NULL;
    if (chunk[2] != 0)
        return;

    for (i = 0; i < chunk[8]; i++)
    {
        offset_start = BYTE * chunk[0x11 + i * 4] + chunk[0x10 + i * 4];
        offset_end = BYTE * chunk[0x15 + i * 4] + chunk[0x14 + i * 4];
        if (!offset_end)
            offset_end = CHUNKSIZE;
        entry = chunk + offset_start;

        ENTRY curr_entry;
        curr_entry.data = entry;
        int32_t etype = build_entry_type(curr_entry);

        if (etype == ENTRY_TYPE_ZONE)
            resize_zone(offset_end - offset_start, entry, scale, status);
        else if (etype == ENTRY_TYPE_SCENERY)
            resize_scenery(offset_end - offset_start, entry, scale, status);
        else if (etype == ENTRY_TYPE_MODEL)
            resize_model(offset_end - offset_start, entry, scale);
        else if (etype == ENTRY_TYPE_ANIM)
            resize_animation(offset_end - offset_start, entry, scale);

        if (etype == ENTRY_TYPE_ZONE)
            resize_zone_camera_distance(offset_end - offset_start, entry, scale);
    }

    checksum = nsfChecksum(chunk);

    for (i = 0; i < 4; i++)
    {
        chunk[0xC + i] = checksum % 256;
        checksum /= 256;
    }
}

// yep resizes folder
void resize_folder(DIR *df, char *path, double scale[3], char *time, DEPRECATE_INFO_STRUCT status)
{
    struct dirent *de;
    char lcltemp[6] = "", help[MAX] = "";
    uint8_t *buffer = NULL;
    FILE *file, *filenew;
    int32_t filesize;

    sprintf(help, "%s\\%s", path, time);
    mkdir(help);

    while ((de = readdir(df)) != NULL)
    {
        if (de->d_name[0] != '.')
        {
            char fpath[MAX] = "";
            sprintf(fpath, "%s\\%s", path, de->d_name);
            strncpy(lcltemp, de->d_name, 5);
            lcltemp[5] = '\0';
            if (buffer != NULL)
                free(buffer);

            if (!strcmp("scene", lcltemp))
            {
                sprintf(status.temp, "%s\n", de->d_name);
                export_condprint(status);
                file = fopen(fpath, "rb");
                fseek(file, 0, SEEK_END);
                filesize = ftell(file);
                rewind(file);
                buffer = (uint8_t *)calloc(sizeof(uint8_t), filesize); // freed here
                fread(buffer, sizeof(uint8_t), filesize, file);
                resize_scenery(filesize, buffer, scale, status);
                sprintf(help, "%s\\%s\\%s", path, time, strrchr(fpath, '\\') + 1);
                filenew = fopen(help, "wb");
                fwrite(buffer, sizeof(uint8_t), filesize, filenew);
                free(buffer);
                fclose(file);
                fclose(filenew);
            }
            if (!strcmp("zone ", lcltemp))
            {
                sprintf(status.temp, "%s\n", de->d_name);
                export_condprint(status);
                file = fopen(fpath, "rb");
                fseek(file, 0, SEEK_END);
                filesize = ftell(file);
                rewind(file);
                buffer = (uint8_t *)calloc(sizeof(uint8_t), filesize); // freed here
                fread(buffer, sizeof(uint8_t), filesize, file);
                resize_zone(filesize, buffer, scale, status);
                sprintf(help, "%s\\%s\\%s", path, time, strrchr(fpath, '\\') + 1);
                filenew = fopen(help, "wb");
                fwrite(buffer, sizeof(uint8_t), filesize, filenew);
                free(buffer);
                fclose(file);
                fclose(filenew);
            }
        }
    }
}

// yep resizes zone
void resize_zone(int32_t fsize, uint8_t *buffer, double scale[3], DEPRECATE_INFO_STRUCT status)
{
    int32_t i, itemcount;
    uint32_t coord;
    itemcount = build_item_count(buffer);
    int32_t *itemoffs = (int32_t *)malloc(itemcount * sizeof(int32_t)); // freed here

    for (i = 0; i < itemcount; i++)
        itemoffs[i] = 256 * buffer[0x11 + i * 4] + buffer[0x10 + i * 4];

    for (i = 0; i < 6; i++)
    {
        coord = from_u32(buffer + itemoffs[1] + i * 4);
        if (coord >= (1LL << 31))
        {
            coord = (uint32_t)((1LL << 32) - coord);
            coord = (uint32_t)coord * scale[i % 3];
            coord = (uint32_t)(1LL << 32) - coord;
            for (int32_t j = 0; j < 4; j++)
            {
                buffer[itemoffs[1] + i * 4 + j] = coord % 256;
                coord = coord / 256;
            }
        }
        else
        {
            coord = (uint32_t)coord * scale[i % 3];
            for (int32_t j = 0; j < 4; j++)
            {
                buffer[itemoffs[1] + i * 4 + j] = coord % 256;
                coord = coord / 256;
            }
        }
    }

    for (i = 2; i < itemcount; i++)
        if (i > buffer[itemoffs[0] + 0x188] || (i % 3 == 2))
            resize_entity(buffer + itemoffs[i], itemoffs[i + 1] - itemoffs[i], scale, status);

    free(itemoffs);
}

// yep resizes entity
void resize_entity(uint8_t *item, int32_t itemsize, double scale[3], DEPRECATE_INFO_STRUCT status)
{
    int32_t i;

    for (i = 0; i < build_item_count(item); i++)
    {
        int32_t code = from_u16(item + 0x10 + 8 * i);
        int32_t offset = from_u16(item + 0x12 + 8 * i) + OFFSET;

        if (code == 0x4B)
        {
            int32_t pathlen = from_u32(item + offset);
            for (int32_t j = 0; j < pathlen * 3; j++)
            {
                double sc = scale[j % 3];
                int32_t val = *(int16_t *)(item + offset + 4 + j * 2);
                val = (int16_t)(val * sc);
                *(int16_t *)(item + offset + 4 + j * 2) = val;
            }
        }
    }
}

// yep resizes scenery
void resize_scenery(int32_t fsize, uint8_t *buffer, double scale[3], DEPRECATE_INFO_STRUCT status)
{
    int32_t i, item1off, j, curr_off, next_off, group;
    int64_t origin;
    int32_t vertcount;

    item1off = buffer[0x10];
    for (i = 0; i < 3; i++)
    {
        origin = from_u32(buffer + item1off + 4 * i);
        if (origin >= (1LL << 31))
        {
            origin = (1LL << 32) - origin;
            origin = scale[i] * origin;
            origin = (1LL << 32) - origin;
            for (j = 0; j < 4; j++)
            {
                buffer[item1off + j + i * 4] = origin % 256;
                origin /= 256;
            }
        }
        else
        {
            origin = scale[i] * origin;
            for (j = 0; j < 4; j++)
            {
                buffer[item1off + j + i * 4] = origin % 256;
                origin /= 256;
            }
        }
    }

    curr_off = BYTE * buffer[0x15] + buffer[0x14];
    next_off = BYTE * buffer[0x19] + buffer[0x18];
    vertcount = next_off - curr_off / 6;

    for (i = curr_off; i < next_off; i += 2)
    {
        group = from_s16(buffer + i);
        int16_t vert = group & 0xFFF0;
        int16_t rest = group & 0xF;
        if (status.gamemode == 2)
        {
            if (i >= 4 * vertcount + curr_off)
                vert *= scale[2];
            else if (i % 4 == 2)
                vert *= scale[1];
            else
                vert *= scale[0];
        }
        if (status.gamemode == 2 && vert >= 2048)
        {
            vert = 4096 - vert;
            if (i < 4 * vertcount + curr_off)
            {
                if (i % 4 == 0)
                    vert = vert * scale[0];
                else
                    vert = vert * scale[1];
            }
            else
                vert = vert * scale[2];
            vert = 4096 - vert;
        }
        else
        {
            if (i < 4 * vertcount + curr_off)
            {
                if (i % 4 == 0)
                    vert = vert * scale[0];
                else
                    vert = vert * scale[1];
            }
            else
                vert = vert * scale[2];
        }

        group = 16 * vert + rest;
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
    int32_t angle;
    scanf("%d", &angle);
    while (angle < 0)
        angle += 360;
    while (angle > 360)
        angle -= 360;

    int32_t game_angle = (0x1000 * angle) / 360;
    int32_t b_value = game_angle >> 8;
    printf("A: %3d\n", game_angle & 0xFF);
    printf("B: %3d", game_angle >> 8);
    while (1)
    {
        b_value += 16;
        if (b_value > 105)
            break;
        printf(" | %3d", b_value);
    }
    printf(" (for rewards)\n\n");
}

// command for printing nsd table
void nsd_gool_table_print(char *fpath)
{
    FILE *file = NULL;
    if ((file = fopen(fpath, "rb")) == NULL)
    {
        printf("[ERROR] File could not be opened\n\n");
        return;
    }

    fseek(file, 0, SEEK_END);
    int32_t filesize = ftell(file);
    rewind(file);

    uint8_t *buffer = (uint8_t *)malloc(filesize * sizeof(uint8_t *)); // freed here
    fread(buffer, sizeof(uint8_t), filesize, file);

    int32_t entry_count = from_u32(buffer + C2_NSD_ENTRY_COUNT_OFFSET);
    for (int32_t i = 0; i < 64; i++)
    {
        int32_t eid = from_u32(buffer + C2_NSD_ENTRY_TABLE_OFFSET + 0x10 + 8 * entry_count + 4 * i);
        printf("%2d: %s  ", i, eid_conv2(eid));
        if (i % 4 == 3)
            putchar('\n');
    }
    fclose(file);
    free(buffer);
    printf("Done.\n\n");
}

// command for generating spawn
void generate_spawn()
{
    ENTRY elist[ELIST_DEFAULT_SIZE];
    int32_t entry_count = 0;

    if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, NULL, elist, &entry_count, NULL, NULL, 1, NULL))
        return;

    char zone[100] = "";
    printf("\nWhat zone do you wanna spawn in?\n");
    scanf("%s", zone);

    int32_t pathlen;
    int32_t entity_id;
    int32_t path_to_spawn_on = 0;
    int32_t entity_index = -1;
    uint32_t zone_eid = eid_to_int(zone);
    int32_t elist_index = build_get_index(zone_eid, elist, entry_count);

    if (elist_index == -1)
    {
        printf("Zone %5s not found\n\n", zone);
        build_cleanup_elist(elist, entry_count);
        return;
    }

    int32_t cam_count = build_get_cam_item_count(elist[elist_index].data) / 3;
    if (cam_count == 0)
    {
        printf("Zone %5s does not have a camera path\n\n", zone);
        build_cleanup_elist(elist, entry_count);
        return;
    }
    else if (cam_count > 1)
    {
        printf("Which cam path do you want to spawn on (0 - %d)\n", cam_count - 1);
        scanf("%d", &path_to_spawn_on);
    }

    printf("\nWhat entity's coordinates do you wanna spawn on? (entity has to be in the zone)\n");
    scanf("%d", &entity_id);

    for (int32_t i = 0; i < build_get_entity_count(elist[elist_index].data); i++)
    {
        uint8_t *entity = build_get_nth_item(elist[elist_index].data, 2 + 3 * cam_count + i);
        int32_t ID = build_get_entity_prop(entity, ENTITY_PROP_ID);

        if (ID == entity_id)
            entity_index = i;
    }

    if (entity_index == -1)
    {
        printf("\nEntity with ID %4d not found in %5s\n\n", entity_id, zone);
        build_cleanup_elist(elist, entry_count);
        return;
    }

    int16_t *path = build_get_path(elist, elist_index, 2 + 3 * cam_count + entity_index, &pathlen);
    if (pathlen == 0)
    {
        free(path);
        printf("\n Entity doesnt have a position\n");
        build_cleanup_elist(elist, entry_count);
        return;
    }

    uint8_t *coll_item = build_get_nth_item(elist[elist_index].data, 1);
    uint32_t zone_x = from_u32(coll_item);
    uint32_t zone_y = from_u32(coll_item + 4);
    uint32_t zone_z = from_u32(coll_item + 8);

    uint32_t x = (zone_x + 4 * path[0]) << 8;
    uint32_t y = (zone_y + 4 * path[1]) << 8;
    uint32_t z = (zone_z + 4 * path[2]) << 8;

    uint8_t arr[0x18] = {0};
    *(uint32_t *)arr = zone_eid;
    *(uint32_t *)(arr + 0x4) = path_to_spawn_on;
    *(uint32_t *)(arr + 0x8) = 0;
    *(uint32_t *)(arr + 0xC) = x;
    *(uint32_t *)(arr + 0x10) = y;
    *(uint32_t *)(arr + 0x14) = z;

    printf("\n");
    for (int32_t j = 0; j < 0x18; j++)
    {
        printf("%02X", arr[j]);
        if (j % 4 == 3)
            printf(" ");
        if ((j % 16 == 15) && (j + 1 != 0x18))
            printf("\n");
    }

    free(path);
    build_cleanup_elist(elist, entry_count);
    printf("\nDone.\n\n");
}

// for recoloring, i use some dumb algorithm that i think does /some/ job and thats about it
int32_t scenery_recolor_main2()
{
    char fpath[1000];
    printf("Path to color item:\n");
    scanf(" %[^\n]", fpath);
    path_fix(fpath);
    float mult;

    FILE *file1;
    if ((file1 = fopen(fpath, "rb+")) == NULL)
    {
        printf("[ERROR] Couldn't open file.\n\n");
        return 0;
    }

    int32_t r_wanted, g_wanted, b_wanted;
    printf("R G B? [hex]\n");
    scanf("%x %x %x", &r_wanted, &g_wanted, &b_wanted);

    printf("brigtness mutliplicator? (float)\n");
    scanf("%f", &mult);

    fseek(file1, 0, SEEK_END);
    int32_t color_count = ftell(file1) / 4;
    uint8_t *buffer = (uint8_t *)malloc((color_count * 4) * sizeof(uint8_t *));
    rewind(file1);
    fread(buffer, color_count, 4, file1);

    // pseudograyscale of the wanted color
    int32_t sum_wanted = r_wanted + g_wanted + b_wanted;

    for (int32_t i = 0; i < color_count; i++)
    {
        // read current color
        uint8_t r = buffer[4 * i + 0];
        uint8_t g = buffer[4 * i + 1];
        uint8_t b = buffer[4 * i + 2];

        // get pseudograyscale of the current color
        int32_t sum = r + g + b;

        // get new color
        int32_t r_new = (sum * r_wanted) / sum_wanted;
        int32_t g_new = (sum * g_wanted) / sum_wanted;
        int32_t b_new = (sum * b_wanted) / sum_wanted;

        r_new *= mult;
        g_new *= mult;
        b_new *= mult;

        // clip it at 0xFF
        r_new = min(r_new, 0xFF);
        g_new = min(g_new, 0xFF);
        b_new = min(b_new, 0xFF);

        // print stuff
        printf("old: %2X %2X %2X\n", r, g, b);
        printf("new: %2X %2X %2X\n", r_new, g_new, b_new);

        // write back
        buffer[4 * i + 0] = r_new;
        buffer[4 * i + 1] = g_new;
        buffer[4 * i + 2] = b_new;
    }

    rewind(file1);
    fwrite(buffer, color_count, 4, file1);
    fclose(file1);
    free(buffer);

    return 0;
}

// converts time from nice format to hex/data value
void time_convert()
{
    int32_t m, s, ms;
    printf("time: (eg 00:51:40)\n");
    scanf("%d:%02d:%02d", &m, &s, &ms);
    if (ms % 4)
        printf("hundredths should be a multiple of 4\n");
    int32_t relictime = 1500 * m + 25 * s + ms / 4;
    printf("relictime values:\n%d\n%02X %02X 00 00\n\n", relictime, relictime & 0xFF, (relictime >> 8) & 0xFF);
}

// script for resizing c3 entity coords to c2 scale?
void c3_ent_resize()
{
    char fpath[1000];
    printf("Path to entity item:\n");
    scanf(" %[^\n]", fpath);
    path_fix(fpath);

    FILE *file1;
    if ((file1 = fopen(fpath, "rb+")) == NULL)
    {
        printf("[ERROR] Couldn't open file.\n\n");
        return;
    }

    fseek(file1, 0, SEEK_END);
    int32_t fsize = ftell(file1);
    double scale = 1;
    rewind(file1);

    uint8_t *buffer = (uint8_t *)malloc(fsize);
    fread(buffer, fsize, 1, file1);

    uint32_t offset = 0;

    for (int32_t i = 0; i < build_prop_count(buffer); i++)
    {

        if ((from_u16(buffer + 0x10 + 8 * i)) == ENTITY_PROP_PATH)
            offset = OFFSET + from_u16(buffer + 0x10 + 8 * i + 2);

        if ((from_u16(buffer + 0x10 + 8 * i)) == 0x30E)
        {
            int32_t off_scale = OFFSET + from_u16(buffer + 0x10 + 8 * i + 2);

            switch (from_u32(buffer + off_scale + 4))
            {
            case 0:
                scale = 0.25;
                break;
            case 1:
                scale = 0.5;
                break;
            case 3:
                scale = 2;
                break;
            case 4:
                scale = 4; //?
                break;
            default:
                scale = 0.25;
                break;
            }

            printf("scale:    %lf\n", scale);
        }
    }

    if (offset)
    {
        int32_t path_len = from_u32(buffer + offset);
        printf("path len: %d\n", path_len);
        for (int32_t i = 0; i < path_len * 3; i++)
        {
            int32_t off_curr = offset + 4 + i * 2;
            *(int16_t *)(buffer + off_curr) = (*(int16_t *)(buffer + off_curr)) * scale;
        }
    }
    else
    {
        printf("[error] no path property found\n");
    }

    rewind(file1);
    fwrite(buffer, 1, fsize, file1);
    free(buffer);
    fclose(file1);
    printf("Done.\n\n");
}

// script for moving whole entity by xyz
void entity_move_scr()
{
    char fpath[1000];
    printf("Path to entity item:\n");
    scanf(" %[^\n]", fpath);
    path_fix(fpath);

    FILE *file1;
    if ((file1 = fopen(fpath, "rb+")) == NULL)
    {
        printf("[ERROR] Couldn't open file.\n\n");
        return;
    }

    printf("xyz move? (%%d %%d %%d)\n");
    int32_t xm, ym, zm;
    scanf("%d %d %d", &xm, &ym, &zm);

    fseek(file1, 0, SEEK_END);
    int32_t fsize = ftell(file1);
    rewind(file1);

    uint8_t *buffer = (uint8_t *)malloc(fsize);
    fread(buffer, fsize, 1, file1);

    uint32_t offset = 0;

    for (int32_t i = 0; i < build_prop_count(buffer); i++)
    {

        if ((from_u16(buffer + 0x10 + 8 * i)) == ENTITY_PROP_PATH)
            offset = OFFSET + from_u16(buffer + 0x10 + 8 * i + 2);
    }

    if (offset)
    {
        int32_t path_len = from_u32(buffer + offset);
        printf("path len: %d\n", path_len);
        for (int32_t i = 0; i < path_len * 3; i++)
        {
            int32_t off_curr = offset + 4 + i * 2;
            int32_t add;
            if (i % 3 == 0)
                add = xm;
            else if (i % 3 == 1)
                add = ym;
            else
                add = zm;
            *(int16_t *)(buffer + off_curr) = (*(int16_t *)(buffer + off_curr)) + add;
        }
    }
    else
    {
        printf("[error] no path property found\n");
    }

    rewind(file1);
    fwrite(buffer, 1, fsize, file1);
    free(buffer);
    fclose(file1);
    printf("Done.\n\n");
}

// prints a model's texture references
void print_model_refs_util(uint8_t *model)
{
    char *strs[200];
    int32_t str_cnt = 0;
    for (int32_t i = 0; i < 200; i++)
        strs[i] = (char *)malloc(50);
    char curr_str[50] = "";

    int32_t item1off = build_get_nth_item_offset(model, 0);
    int32_t item4off = build_get_nth_item_offset(model, 3);
    int32_t item5off = build_get_nth_item_offset(model, 4);
    int32_t tex_ref_item_size = item5off - item4off;

    printf("\nTexture references: \n");
    printf("TEXTURE CLUT B S-X  Y  W  H\n");
    for (int32_t i = 0; i < tex_ref_item_size; i += 12)
    {
        uint8_t *curr_off = model + item4off + i;
        int32_t seg = from_u8(curr_off + 6) & 0xF;
        int32_t bit = from_u8(curr_off + 6) & 0x80;

        if (bit)
            bit = 8;
        else
            bit = 4;

        int32_t clut = from_u16(curr_off + 2);
        int32_t tex = from_u8(curr_off + 7);

        int32_t startx = min(min(from_u8(curr_off + 0), from_u8(curr_off + 4)), from_u8(curr_off + 8));
        int32_t starty = min(min(from_u8(curr_off + 1), from_u8(curr_off + 5)), from_u8(curr_off + 9));

        int32_t endx = max(max(from_u8(curr_off + 0), from_u8(curr_off + 4)), from_u8(curr_off + 8));
        int32_t endy = max(max(from_u8(curr_off + 1), from_u8(curr_off + 5)), from_u8(curr_off + 9));

        int32_t width = endx - startx + 1;
        int32_t height = endy - starty + 1;

        sprintf(curr_str, " %5s: %04X %d %d-%02X %02X %02X %02X",
                eid_conv2(from_u32(model + item1off + 0xC + tex)), clut, bit, seg, startx, starty, width, height);
        int32_t str_listed = 0;
        for (int32_t j = 0; j < str_cnt; j++)
        {
            if (strcmp(curr_str, strs[j]) == 0)
            {
                str_listed = 1;
                break;
            }
        }

        if (!str_listed)
        {
            strcpy(strs[str_cnt], curr_str);
            str_cnt++;
        }
    }

    for (int32_t i = 0; i < str_cnt; i++)
    {
        printf("%s\n", strs[i]);
        free(strs[i]);
    }
}

// prints texture references of a model or all models from a nsf
void print_model_tex_refs_nsf()
{
    ENTRY elist[ELIST_DEFAULT_SIZE];
    int32_t entry_count = 0;
    uint32_t gool_table[C2_GOOL_TABLE_SIZE];
    for (int32_t i = 0; i < C2_GOOL_TABLE_SIZE; i++)
        gool_table[i] = EID_NONE;

    if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, gool_table, elist, &entry_count, NULL, NULL, 1, NULL))
        return;

    int32_t printed_something = 0;
    char temp[6] = "";
    printf("Model name? (type \"all\" to print all models' tex refs)\n");
    char ename[6] = "";
    scanf("%5s", ename);

    for (int32_t i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_MODEL)
        {
            if (strcmp(eid_conv(elist[i].eid, temp), ename) == 0 || strcmp(ename, "all") == 0)
            {
                printf("\nModel %s:", eid_conv2(elist[i].eid));
                print_model_refs_util(elist[i].data);
                printed_something = 1;
            }
        }
    }

    build_cleanup_elist(elist, entry_count);
    if (!printed_something)
        printf("No such model was found\n");
    printf("Done.\n\n");
}

// prints texture references of a model from a file
void print_model_tex_refs()
{
    char fpath[1000];
    printf("Path to model entry:\n");
    scanf(" %[^\n]", fpath);
    path_fix(fpath);

    FILE *file1;
    if ((file1 = fopen(fpath, "rb+")) == NULL)
    {
        printf("[ERROR] Couldn't open file.\n\n");
        return;
    }

    fseek(file1, 0, SEEK_END);
    int32_t fsize = ftell(file1);
    rewind(file1);
    uint8_t *model = (uint8_t *)malloc(fsize);
    fread(model, fsize, 1, file1);
    fclose(file1);

    print_model_refs_util(model);
    free(model);
    printf("Done\n\n");
}

// prints all entries?
void print_all_entries_perma()
{
    ENTRY elist[ELIST_DEFAULT_SIZE];
    int32_t entry_count = 0;
    uint32_t gool_table[C2_GOOL_TABLE_SIZE];
    for (int32_t i = 0; i < C2_GOOL_TABLE_SIZE; i++)
        gool_table[i] = EID_NONE;

    if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, gool_table, elist, &entry_count, NULL, NULL, 1, NULL))
        return;

    for (int32_t i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) != ENTRY_TYPE_INST)
            printf("%5s\n", eid_conv2(elist[i].eid));
    }

    build_cleanup_elist(elist, entry_count);
    printf("Done.\n\n");
}

// collects entity usage info from a single nsf
void entity_usage_single_nsf(char *fpath, DEPENDENCIES *deps, uint32_t *gool_table)
{
    printf("Checking %s\n", fpath);

    ENTRY elist[ELIST_DEFAULT_SIZE];
    int32_t entry_count = 0;

    if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, gool_table, elist, &entry_count, NULL, NULL, 1, fpath))
        return;

    int32_t total_entity_count = 0;

    for (int32_t i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            int32_t camera_count = build_get_cam_item_count(elist[i].data);
            int32_t entity_count = build_get_entity_count(elist[i].data);

            for (int32_t j = 0; j < entity_count; j++)
            {
                uint8_t *entity = build_get_nth_item(elist[i].data, (2 + camera_count + j));
                int32_t type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
                int32_t subt = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);
                int32_t id = build_get_entity_prop(entity, ENTITY_PROP_ID);

                if (id == -1)
                    continue;
                total_entity_count++;

                int32_t found_before = 0;
                for (int32_t k = 0; k < deps->count; k++)
                    if (deps->array[k].type == type && deps->array[k].subtype == subt)
                    {
                        list_add(&deps->array[k].dependencies, total_entity_count);
                        found_before = 1;
                    }
                if (!found_before)
                {
                    deps->array = realloc(deps->array, (deps->count + 1) * sizeof(DEPENDENCY));
                    deps->array[deps->count].type = type;
                    deps->array[deps->count].subtype = subt;
                    deps->array[deps->count].dependencies = init_list();
                    list_add(&deps->array[deps->count].dependencies, total_entity_count);
                    deps->count++;
                }
            }
        }

    build_cleanup_elist(elist, entry_count);
}

int32_t cmp_func_dep2(const void *a, const void *b)
{
    DEPENDENCY x = *(DEPENDENCY *)a;
    DEPENDENCY y = *(DEPENDENCY *)b;

    return y.dependencies.count - x.dependencies.count;
}

// folder iterator for getting entity usage
void entity_usage_folder_util(const char *dpath, DEPENDENCIES *deps, uint32_t *gool_table)
{
    char fpath[MAX + 300] = "", moretemp[MAX] = "";
    DIR *df = opendir(dpath);
    if (df == NULL)
    {
        printf("[ERROR] Could not open selected directory\n");
        return;
    }

    char nsfcheck[4] = "";
    struct dirent *de;
    while ((de = readdir(df)) != NULL)
    {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
        {
            continue;
        }
        strncpy(nsfcheck, strchr(de->d_name, '\0') - 3, 3);
        strcpy(moretemp, de->d_name);
        if (de->d_name[0] != '.' && !strcmp(nsfcheck, "NSF"))
        {
            sprintf(fpath, "%s\\%s", dpath, de->d_name);
            entity_usage_single_nsf(fpath, deps, gool_table);
        }
        char filePath[256];
        snprintf(filePath, sizeof(filePath), "%s\\%s", dpath, de->d_name);
        if (de->d_type == DT_DIR)
        {
            entity_usage_folder_util(filePath, deps, gool_table);
        }
    }
    closedir(df);
}

// script for getting entity usage from folder
void entity_usage_folder()
{
    printf("Input the path to the folder\n");
    char dpath[MAX] = "";
    DEPENDENCIES deps = build_init_dep();

    uint32_t gool_table[C2_GOOL_TABLE_SIZE];
    for (int32_t i = 0; i < C2_GOOL_TABLE_SIZE; i++)
        gool_table[i] = EID_NONE;

    scanf(" %[^\n]", dpath);
    path_fix(dpath);

    entity_usage_folder_util(dpath, &deps, gool_table);
    qsort(deps.array, deps.count, sizeof(DEPENDENCY), cmp_func_dep2);

    int32_t entity_sum = 0;
    for (int32_t i = 0; i < deps.count; i++)
        entity_sum += deps.array[i].dependencies.count;

    printf("\nTotal entity count:\n");
    printf(" %5d entities\n", entity_sum);

    printf("\nEntity type/subtype usage:\n");
    for (int32_t i = 0; i < deps.count; i++)
        printf(" %2d(%5s)-%2d: %4d entities\n", deps.array[i].type, eid_conv2(gool_table[deps.array[i].type]), deps.array[i].subtype, deps.array[i].dependencies.count);

    printf("\nDone.\n\n");
}

// script for printing a property from all cameras in a level
void nsf_props_scr()
{
    ENTRY elist[ELIST_DEFAULT_SIZE];
    int32_t entry_count = 0;
    uint32_t gool_table[C2_GOOL_TABLE_SIZE];
    for (int32_t i = 0; i < C2_GOOL_TABLE_SIZE; i++)
        gool_table[i] = EID_NONE;

    if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, gool_table, elist, &entry_count, NULL, NULL, 1, NULL))
        return;

    int32_t printed_something = 0;
    int32_t prop_code;

    printf("\nProp code? (hex)\n");
    scanf("%x", &prop_code);

    printf("\n");
    for (int32_t i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            int32_t cam_count = build_get_cam_item_count(elist[i].data) / 3;
            for (int32_t j = 0; j < cam_count; j++)
            {
                for (int32_t k = 0; k < 3; k++)
                {
                    uint8_t *item = build_get_nth_item(elist[i].data, 2 + 3 * j + k);
                    int32_t prop_count = build_prop_count(item);

                    for (int32_t l = 0; l < build_prop_count(item); l++)
                    {
                        int32_t code = from_u16(item + 0x10 + 8 * l);
                        int32_t offset = from_u16(item + 0x12 + 8 * l) + OFFSET;
                        int32_t offset_next = from_u16(item + 0x1A + 8 * l) + OFFSET;
                        if (l == (prop_count - 1))
                            offset_next = from_u16(item);

                        if (code == prop_code)
                        {
                            printed_something = 1;
                            printf("Zone %5s path %d item %d", eid_conv2(elist[i].eid), j, k);
                            print_prop_header(item, 0x10 + 8 * l);
                            print_prop_body(item, offset, offset_next);
                        }
                    }
                }
            }
        }
    }

    build_cleanup_elist(elist, entry_count);
    if (!printed_something)
        printf("No such prop was found in any camera item\n");
    printf("Done.\n\n");
}

// makes an empty slst entry for a camera with specified length
void generate_slst()
{
    int32_t cam_len = 0;
    char name[100] = "ABCDE";
    static uint8_t empty_source[] = {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    static uint8_t empty_delta[] = {0x2, 0x0, 0x1, 0x0, 0x2, 0x0, 0x2, 0x0};
    uint32_t real_len = 0x10;
    uint8_t buffer[0x20000];
    char fpath[500] = "";

    printf("Camera path length?\n");
    scanf("%d", &cam_len);
    if (cam_len <= 1)
    {
        printf("[ERROR] Invalid slst length %d\n\n", cam_len);
        return;
    }

    printf("\nSLST name?\n");
    scanf("%s", name);
    sprintf(fpath, "empty_slst_%5s_len_%d.nsentry", name, cam_len);

    FILE *file = NULL;
    if ((file = fopen(fpath, "wb")) == NULL)
    {
        printf("[ERROR] File %s could not be opened\n\n", fpath);
        return;
    }

    *(uint32_t *)(buffer) = MAGIC_ENTRY;
    *(uint32_t *)(buffer + 0x4) = eid_to_int(name);
    *(uint32_t *)(buffer + 0x8) = ENTRY_TYPE_SLST;
    *(uint32_t *)(buffer + 0xC) = cam_len + 1;

    for (int32_t i = 0; i < cam_len + 2; i++)
    {
        *(uint32_t *)(buffer + 0x10 + 4 * i) = (4 * (cam_len + 2)) + 0x10 + (8 * i);
        real_len += 4;
    }

    for (int32_t i = 0; i < 8; i++)
        *(uint8_t *)(buffer + real_len + i) = empty_source[i];
    real_len += 8;

    for (int32_t j = 0; j < (cam_len - 1); j++)
    {
        for (int32_t i = 0; i < 8; i++)
            *(uint8_t *)(buffer + real_len + i) = empty_delta[i];
        real_len += 8;
    }

    for (int32_t i = 0; i < 8; i++)
        *(uint8_t *)(buffer + real_len + i) = empty_source[i];
    real_len += 8;

    fwrite(buffer, real_len, 1, file);
    fclose(file);
    printf("Done. Saved as %s\n\n", fpath);
}

#define WARP_SPAWN_COUNT 32
// modpack warp spawns generation util (for nsd)
void warp_spawns_generate()
{
    ENTRY elist[ELIST_DEFAULT_SIZE];
    int32_t entry_count = 0;

    if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, NULL, elist, &entry_count, NULL, NULL, 1, NULL))
        return;

    /* older, hardcoded spawns
    int32_t spawn_ids[] = {
        27, 34, 40, 27, df, 40, 36,
        32, 41, 32, 32, 35,
        40, 31, 37, 33, 39,
        37, 30, 29, 28, 29,
        27, 37, 38, 32, 131,
        30, df, df, 37, df,
        df};
    */

    char path[MAX];
    printf("\nInput the path to the warp spawns file:\n");
    scanf(" %[^\n]", path);
    path_fix(path);

    FILE *file = fopen(path, "r");
    if (!file)
    {
        printf("File could not be opened!\n");
        return;
    }

    char buffer[1 << 16];
    int32_t count = 0;
    int32_t spawn_ids[WARP_SPAWN_COUNT] = {0};

    size_t len = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    buffer[len] = '\0';

    char *token = strtok(buffer, ",\n\r");
    while (token && count < WARP_SPAWN_COUNT)
    {
        spawn_ids[count++] = atoi(token);
        token = strtok(NULL, ",\n\r");
    }

    for (int32_t i = 0; i < WARP_SPAWN_COUNT; i++)
    {
        if (spawn_ids[i] == 0)
            spawn_ids[i] = 26; // default to ID 26
    }

    for (int32_t i = 0; i < WARP_SPAWN_COUNT; i++)
    {
        for (int32_t j = 0; j < entry_count; j++)
        {
            if (build_entry_type(elist[j]) != ENTRY_TYPE_ZONE)
                continue;

            uint8_t *curr_zone = elist[j].data;
            int32_t camc = build_get_cam_item_count(curr_zone);

            for (int32_t k = 0; k < build_get_entity_count(curr_zone); k++)
            {
                int32_t offset = build_get_nth_item_offset(curr_zone, 2 + camc + k);
                int32_t ent_id = build_get_entity_prop(curr_zone + offset, ENTITY_PROP_ID);
                if (ent_id == spawn_ids[i])
                {
                    int32_t pathlen;
                    int16_t *path = build_get_path(elist, j, 2 + camc + k, &pathlen);

                    uint8_t *coll_item = build_get_nth_item(elist[j].data, 1);
                    uint32_t zone_x = from_u32(coll_item);
                    uint32_t zone_y = from_u32(coll_item + 4);
                    uint32_t zone_z = from_u32(coll_item + 8);

                    uint32_t x = (zone_x + 4 * path[0]) << 8;
                    uint32_t y = (zone_y + 4 * path[1]) << 8;
                    uint32_t z = (zone_z + 4 * path[2]) << 8;

                    uint8_t arr[0x18] = {0};
                    *(uint32_t *)arr = elist[j].eid;
                    *(uint32_t *)(arr + 0x4) = 0;
                    *(uint32_t *)(arr + 0x8) = 0;
                    *(uint32_t *)(arr + 0xC) = x;
                    *(uint32_t *)(arr + 0x10) = y;
                    *(uint32_t *)(arr + 0x14) = z;

                    printf("\n");
                    for (int32_t j = 0; j < 0x18; j++)
                    {
                        printf("%02X", arr[j]);
                        if (j % 4 == 3)
                            printf(" ");
                    }
                }
            }
        }
    }

    build_cleanup_elist(elist, entry_count);
    printf("\nDone.\n\n");
}

// util for printing special load list info for single nsf
void special_load_lists_util(char *fpath)
{
    printf("Checking %s\n", fpath);
    int32_t printed_something = 0;

    ENTRY elist[ELIST_DEFAULT_SIZE];
    int32_t entry_count = 0;

    if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, NULL, elist, &entry_count, NULL, NULL, 1, fpath))
        return;

    for (int32_t i = 0; i < entry_count; i++)
    {
        ENTRY curr = elist[i];
        if (build_entry_type(curr) != ENTRY_TYPE_ZONE)
            continue;

        LIST special_entries = build_read_special_entries(curr.data);
        if (special_entries.count)
        {
            printed_something = 1;
            printf("Zone %5s:\t", eid_conv2(curr.eid));
            for (int32_t j = 0; j < special_entries.count; j++)
            {
                printf("%5s ", eid_conv2(special_entries.eids[j]));
                if (j && j % 8 == 7 && (j + 1) != special_entries.count)
                    printf("\n           \t");
            }

            printf("\n");
        }
    }

    build_cleanup_elist(elist, entry_count);
    if (printed_something)
        printf("\n");
}

int32_t check_fpath_contains(char *fpath, char *name)
{
    if (strstr(fpath, name))
        return 1;

    return 0;
}

// for printing gool type and category list from nsf
void nsd_util_util(char *fpath)
{
    ENTRY elist[ELIST_DEFAULT_SIZE];
    int32_t entry_count = 0;

    char *filename = strrchr(fpath, '\\');
    if (filename != NULL)
    {
        filename++;
    }
    else
    {
        filename = fpath;
    }

    if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, NULL, elist, &entry_count, NULL, NULL, 1, fpath))
        return;

    for (int32_t i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) != ENTRY_TYPE_GOOL)
            continue;

        int32_t off1 = build_get_nth_item_offset(elist[i].data, 0);

        int32_t tpe = from_u32(elist[i].data + off1);
        int32_t cat = from_u32(elist[i].data + off1 + 4) >> 8;

        printf("%s %5s, type %d category %d\n", filename, eid_conv2(elist[i].eid), tpe, cat);
    }

    build_cleanup_elist(elist, entry_count);
}

// util for printing fov info for a single level
void fov_stats_util(char *fpath)
{
    printf("Level: %s\n", fpath);
    ENTRY elist[ELIST_DEFAULT_SIZE];
    int32_t entry_count = 0;

    if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, NULL, elist, &entry_count, NULL, NULL, 1, fpath))
        return;

    for (int32_t i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE)
            continue;

        int32_t c_count = build_get_cam_item_count(elist[i].data) / 3;
        for (int32_t j = 0; j < c_count; j++)
        {
            uint8_t *entity = build_get_nth_item(elist[i].data, 2 + 3 * j);
            int32_t fov = build_get_entity_prop(entity, ENTITY_PROP_CAM_FOV);
            printf("Zone %5s : fov %d\n", eid_conv2(elist[i].eid), fov);
        }
    }

    build_cleanup_elist(elist, entry_count);
    printf("\n");
}

// util function for printing checkpoint, mask and dda info
void checkpoint_stats_util(char *fpath)
{
    // printf("Checking %s\n", fpath);
    // char temp[6] = "";

    ENTRY elist[ELIST_DEFAULT_SIZE];
    int32_t entry_count = 0;

    if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, NULL, elist, &entry_count, NULL, NULL, 1, fpath))
        return;

    int32_t cam_count = 0;
    int32_t checks_non_dda = 0;
    int32_t checks_with_dda = 0;
    int32_t masks_non_dda = 0;
    int32_t masks_with_dda = 0;

    for (int32_t i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE)
            continue;

        int32_t c_count = build_get_cam_item_count(elist[i].data);
        int32_t e_count = build_get_entity_count(elist[i].data);
        cam_count += (c_count / 3);

        for (int32_t j = 0; j < e_count; j++)
        {
            uint8_t *entity = build_get_nth_item(elist[i].data, 2 + c_count + j);
            int32_t type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
            int32_t subt = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);
            // int32_t id = build_get_entity_prop(entity, ENTITY_PROP_ID);
            int32_t dda_deaths = build_get_entity_prop(entity, ENTITY_PROP_DDA_DEATHS) / 256;

            if (type == 34 && subt == 4)
            {
                if (dda_deaths)
                    checks_with_dda++;
                else
                    checks_non_dda++;
            }

            if (type == 34 && subt == 27)
                checks_non_dda++;

            if (type == 34 && subt == 9)
            {
                if (dda_deaths)
                    masks_with_dda++;
                else
                    masks_non_dda++;
            }

            if (type == 3 && subt == 6)
                masks_non_dda++;

            // snow no alt iron checks
            if (check_fpath_contains(fpath, "S0000015.NSF"))
            {
                if (type == 40 && subt == 27)
                    checks_non_dda++;
            }

            // lava checks
            if (check_fpath_contains(fpath, "S000000E.NSF"))
            {
                if (type == 33 && (subt == 0 || subt == 2))
                    checks_non_dda++;
            }

            // jet rex alt masks
            if (check_fpath_contains(fpath, "S0000019.NSF"))
            {
                if (type == 45 && subt == 7)
                    masks_non_dda++;
                if (type == 36 && subt == 0)
                {
                    if (dda_deaths)
                        masks_with_dda++;
                    else
                        masks_non_dda++;
                }
            }

            // dream zone alt iron checks
            if (check_fpath_contains(fpath, "S000001B.NSF"))
            {
                if (type == 60 && subt == 0)
                    checks_with_dda++;
            }
        }
    }

    // arab/lava alt masks (done thru args so its just added artificially)
    if (check_fpath_contains(fpath, "S0000013.NSF") || check_fpath_contains(fpath, "S000000E.NSF"))
        masks_non_dda++;

    if (checks_non_dda || checks_with_dda)
    {
        printf("\"%s\",", fpath);

        printf("%3d,", cam_count);
        printf("%3d,", checks_non_dda);
        printf("%3d,", checks_with_dda);

        printf("%3d,", masks_non_dda);
        printf("%3d ", masks_with_dda);

        printf("\n");
    }
    build_cleanup_elist(elist, entry_count);
}

// function for recursively iterating over folders and calling callback
void recursive_folder_iter(const char *dpath, void(callback)(char *))
{
    char fpath[MAX + 300] = "", moretemp[MAX] = "";
    DIR *df = opendir(dpath);
    if (df == NULL)
    {
        printf("[ERROR] Could not open selected directory\n");
        return;
    }

    char nsfcheck[4] = "";
    struct dirent *de;
    while ((de = readdir(df)) != NULL)
    {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
        {
            continue;
        }
        strncpy(nsfcheck, strchr(de->d_name, '\0') - 3, 3);
        strcpy(moretemp, de->d_name);
        if (de->d_name[0] != '.' && !strcmp(nsfcheck, "NSF"))
        {
            sprintf(fpath, "%s\\%s", dpath, de->d_name);
            callback(fpath);
        }
        char filePath[256];
        snprintf(filePath, sizeof(filePath), "%s\\%s", dpath, de->d_name);
        if (de->d_type == DT_DIR)
        {
            recursive_folder_iter(filePath, callback);
        }
    }
    closedir(df);
}

// command for listing modpack/rebuild special load lists from folder
void special_load_lists_list()
{
    printf("Input the path to the folder\n");
    char dpath[MAX] = "";

    scanf(" %[^\n]", dpath);
    path_fix(dpath);
    recursive_folder_iter(dpath, special_load_lists_util);
    printf("\nDone.\n\n");
}

// command for listing checkpoint and mask info from folder
void checkpoint_stats()
{
    printf("Input the path to the folder\n");
    char dpath[MAX] = "";

    scanf(" %[^\n]", dpath);
    path_fix(dpath);

    printf("fpath,cam_count,checks_non_dda,checks_with_dda,masks_non_dda,masks_with_dda\n");
    recursive_folder_iter(dpath, checkpoint_stats_util);
    printf("\nDone.\n\n");
}

// command for listing fov values from folder
void fov_stats()
{
    printf("Input the path to the folder\n");
    char dpath[MAX] = "";

    scanf(" %[^\n]", dpath);
    path_fix(dpath);

    recursive_folder_iter(dpath, fov_stats_util);
    printf("\nDone.\n\n");
}

// command for listing gool types from folder
void nsd_util()
{
    printf("Input the path to the folder\n");
    char dpath[MAX] = "";

    scanf(" %[^\n]", dpath);
    path_fix(dpath);

    recursive_folder_iter(dpath, nsd_util_util);
    printf("\nDone.\n\n");
}

// command for listing draw info from nsf
void draw_util()
{
    ENTRY elist[ELIST_DEFAULT_SIZE];
    int32_t entry_count = 0;

    if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, NULL, elist, &entry_count, NULL, NULL, 1, NULL))
        return;

    int32_t total = 0;
    int32_t point_count = 0;

    for (int32_t i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE)
            continue;

        int32_t cam_count = build_get_cam_item_count(elist[i].data) / 3;
        for (int32_t j = 0; j < cam_count; j++)
        {
            int32_t path_len = build_get_path_length(build_get_nth_item(elist[i].data, 2 + 3 * j));
            LIST *draw_list = build_get_complete_draw_list(elist, i, 2 + 3 * j, path_len);

            for (int32_t k = 0; k < path_len - 1; k++)
            {
                int32_t drawn = draw_list[k].count;
                printf("%s-%d, point %2d/%2d - drawing %d entities\n", eid_conv2(elist[i].eid), j, k, path_len, drawn);

                total += drawn;
                point_count += 1;
            }
        }
    }

    printf("\n");

    // copypaste, cba to optimise
    for (int32_t i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE)
            continue;

        int32_t cam_count = build_get_cam_item_count(elist[i].data) / 3;
        for (int32_t j = 0; j < cam_count; j++)
        {
            int32_t path_len = build_get_path_length(build_get_nth_item(elist[i].data, 2 + 3 * j));
            LIST *draw_list = build_get_complete_draw_list(elist, i, 2 + 3 * j, path_len);

            int32_t zone_total = 0;
            int32_t zone_points = 0;

            for (int32_t k = 0; k < path_len - 1; k++)
            {
                int32_t drawn = draw_list[k].count;
                zone_total += drawn;
                zone_points += 1;
            }

            printf("%s-%d average entities drawn: %d\n", eid_conv2(elist[i].eid), j, zone_total / zone_points);
        }
    }

    printf("\nAverage entities drawn in level: %d\n", total / point_count);

    build_cleanup_elist(elist, entry_count);
    printf("\nDone.\n\n");
}

// util for printing file texture pages
void tpage_util_util(char *fpath)
{
    ENTRY elist[ELIST_DEFAULT_SIZE];
    int32_t entry_count = 0;

    char *filename = strrchr(fpath, '\\');
    if (filename != NULL)
    {
        filename++;
    }
    else
    {
        filename = fpath;
    }

    uint8_t *chunks[CHUNK_LIST_DEFAULT_SIZE];
    int32_t chunk_border_texture = 0;
    if (build_read_and_parse_rebld(NULL, NULL, NULL, &chunk_border_texture, NULL, elist, &entry_count, chunks, NULL, 1, fpath))
        return;

    printf("File %s:\n", fpath);
    for (int32_t i = 0; i < chunk_border_texture; i++)
    {
        if (chunks[i] == NULL)
            continue;
        if (from_u16(chunks[i] + 0x2) != CHUNK_TYPE_TEXTURE)
            continue;

        int32_t checksum = from_u32(chunks[i] + CHUNK_CHECKSUM_OFFSET);
        printf("\t tpage %s \t%08X\n", eid_conv2(from_u32(chunks[i] + 0x4)), checksum);
    }
    printf("\n");
    build_cleanup_elist(elist, entry_count);
}

// prints tpage list for files from a folder
void tpage_util()
{
    printf("Input the path to the folder\n");
    char dpath[MAX] = "";

    scanf(" %[^\n]", dpath);
    path_fix(dpath);

    recursive_folder_iter(dpath, tpage_util_util);
    printf("\nDone.\n\n");
}

// for gool_util command, prints gool entries of a file plus their checksum
void gool_util_util(char *fpath)
{
    ENTRY elist[ELIST_DEFAULT_SIZE];
    int32_t entry_count = 0;

    if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, NULL, elist, &entry_count, NULL, NULL, 1, fpath))
        return;

    printf("File %s:\n", fpath);
    for (int32_t i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) != ENTRY_TYPE_GOOL)
            continue;

        int32_t off1 = build_get_nth_item_offset(elist[i].data, 0);
        int32_t tpe = from_u32(elist[i].data + off1);

        printf("\t gool %5s-%02d \t%08X\n", eid_conv2(elist[i].eid), tpe, crcChecksum(elist[i].data, elist[i].esize));
    }

    printf("\n");
    build_cleanup_elist(elist, entry_count);
}

// command for printing gool entries list from folder
void gool_util()
{
    printf("Input the path to the folder\n");
    char dpath[MAX] = "";

    scanf(" %[^\n]", dpath);
    path_fix(dpath);

    recursive_folder_iter(dpath, gool_util_util);
    printf("\nDone.\n\n");
}
