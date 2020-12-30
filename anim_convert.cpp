#include "macros.h"

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char blendmode;
    unsigned char n;
    unsigned char clut_x;
    unsigned char clut_y;
    unsigned int  texture_ref;
    unsigned int  uvindex;
    unsigned char colormode;
    unsigned char segment;
    unsigned char offset_x;
    unsigned char offset_y;
} C1_TEXTURE_INFO;

typedef struct {
    unsigned short int a;
    unsigned short int b;
    unsigned short int c;
    unsigned short int u;
} C1_POLY_INFO;

typedef struct {
    unsigned char x;
    unsigned char y;
    unsigned char z;
    signed char normal_x;
    signed char normal_y;
    signed char normal_z;
} C1_VERTEX_INFO;


int c1_conv_get_tpages(int **tpages, C1_TEXTURE_INFO* txtrs, int txt_ref_count)
{
    int tpage_count = 0;
    for (int i = 0; i < txt_ref_count; i++)
    {
        char temp[100];
        printf("%s\n", eid_conv(txtrs[i].texture_ref, temp));
        int found = 0;
        for (int j = 0; j < tpage_count; j++)
            if (txtrs[i].texture_ref == (*tpages)[j])
                found = 1;

        if (!found) {
            (*tpages) = (int *) realloc(*tpages, (tpage_count + 1) * sizeof(int));
            (*tpages)[tpage_count] = txtrs[i].texture_ref;
            tpage_count++;
        }
    }

    return tpage_count;
}

void c1_conv_main()
{
    int i, j;
    char fpath[MAX], fpath2[MAX];
    printf("Path to model:\n");
    scanf(" %[^\n]",fpath);
    path_fix(fpath);

    FILE* file1;
    if ((file1 = fopen(fpath, "rb")) == NULL) {
        printf("Couldn't open file.\n\n");
        return;
    }

    int filesize = get_file_length(file1);

    ENTRY model;
    model.data = (unsigned char *) malloc(filesize);
    fread(model.data, sizeof(unsigned char), filesize, file1);
    model.esize = filesize;
    model.EID = from_u32(model.data + 0x4);

    int model_item1off = from_u32(model.data + 0x10);
    int model_item2off = from_u32(model.data + 0x14);

    int txt_ref_count = from_u32(model.data + model_item1off);
    int size_x = from_u32(model.data + model_item1off + 0x4);
    int size_y = from_u32(model.data + model_item1off + 0x8);
    int size_z = from_u32(model.data + model_item1off + 0xC);
    int poly_count = from_u32(model.data + model_item1off + 0x10)/3;

    C1_TEXTURE_INFO txtrs[poly_count];
    for (i = 0; i < poly_count; i++)
    {
        int offset = model_item1off + 0x14 + 0xC * i;
        txtrs[i].r = *(model.data + offset);
        txtrs[i].g = *(model.data + offset + 1);
        txtrs[i].b = *(model.data + offset + 2);
        txtrs[i].blendmode = (*(model.data + offset + 3) >> 5) & 3;
        txtrs[i].n = *(model.data + offset + 3) & 0x10;
        txtrs[i].clut_x = *(model.data + offset + 3) & 0xF;
        txtrs[i].texture_ref = from_u32(model.data + offset + 4);
        unsigned int texinfo = from_u32(model.data + offset + 8);
        txtrs[i].uvindex = (texinfo >> 22) & 0x3FF;
        txtrs[i].colormode = (unsigned char)((texinfo >> 20) & 3);
        txtrs[i].segment = (unsigned char)((texinfo >> 18) & 3);
        txtrs[i].offset_x = (unsigned char)((texinfo >> 13) & 0x1F);
        txtrs[i].clut_y = (unsigned char)((texinfo >> 6) & 0x7F);
        txtrs[i].offset_y = (unsigned char)(texinfo & 0x1F);

        C1_TEXTURE_INFO curr = txtrs[i];
        char temp[100];
        printf("n: %d, texture seg: %d x: %d, y:%d\n\tclut x:%d y:%d texture %s\n",
               curr.n, curr.segment, curr.offset_x, curr.offset_y, curr.clut_x, curr.clut_y, eid_conv(curr.texture_ref, temp));
    }


    C1_POLY_INFO polys[poly_count];
    for (i = 0; i < poly_count; i++)
    {
        int offset = model_item2off + 8 * i;
        polys[i].a = *(short int *) (model.data + offset);
        polys[i].b = *(short int *) (model.data + offset + 2);
        polys[i].c = *(short int *) (model.data + offset + 4);
        polys[i].u = *(short int *) (model.data + offset + 6);
    }

    printf("Path to animation:\n");
    scanf(" %[^\n]",fpath2);
    path_fix(fpath2);


    FILE* file2;
    if ((file2 = fopen(fpath2, "rb")) == NULL) {
        printf("Couldn't open file.\n\n");
        fclose(file1);
        return;
    }

    int filesize2 = get_file_length(file2);
    ENTRY anim;
    anim.data = (unsigned char *) malloc(filesize2);
    fread(anim.data, sizeof(unsigned char ), filesize2, file2);
    anim.esize = filesize2;
    anim.EID = from_u32(anim.data + 0x4);

    int vertex_count;
    int frame_count = from_u32(anim.data + 0xC);
    for (i = 0; i < frame_count; i++)
    {
        int item_off = from_u32(anim.data + 0x10 + 4 * i);
        int item_off_next = from_u32(anim.data + 0x14 + 4 * i);

        vertex_count = from_u32(anim.data + item_off);
        int modelEID = from_u32(anim.data + item_off + 0x4);
        int offset_x = from_u32(anim.data + item_off + 0x8);
        int offset_y = from_u32(anim.data + item_off + 0xC);
        int offset_z = from_u32(anim.data + item_off + 0x10);

        int x1 = from_u32(anim.data + item_off + 0x14);
        int y1 = from_u32(anim.data + item_off + 0x18);
        int z1 = from_u32(anim.data + item_off + 0x1C);

        int x2 = from_u32(anim.data + item_off + 0x20);
        int y2 = from_u32(anim.data + item_off + 0x24);
        int z2 = from_u32(anim.data + item_off + 0x28);

        int global_x = from_u32(anim.data + item_off + 0x2C);
        int global_y = from_u32(anim.data + item_off + 0x30);
        int global_z = from_u32(anim.data + item_off + 0x34);

        C1_VERTEX_INFO verts[vertex_count];
        for (j = 0; j < vertex_count; j++)
        {
            int vert_off = item_off + 0x38 + 6 * j;
            verts[j].x = *(anim.data + vert_off);
            verts[j].y = *(anim.data + vert_off + 1);
            verts[j].z = *(anim.data + vert_off + 2);
            verts[j].normal_x = *(anim.data + vert_off + 3);
            verts[j].normal_y = *(anim.data + vert_off + 4);
            verts[j].normal_z = *(anim.data + vert_off + 5);

        }
    }






    int *texture_eids = NULL;
    int texture_page_count = c1_conv_get_tpages(&texture_eids, txtrs, txt_ref_count);
    int color_count = 0;
    int tri_count = 0;
    int model_item_count = 5;

    int item_lengths[model_item_count];
    unsigned char *items[model_item_count];

    item_lengths[0] = 0x50;
    items[0] = (unsigned char *) calloc(0x50, sizeof(unsigned char));

    *(int *)(items[0])        = size_x;
    *(int *)(items[0] + 0x04) = size_y;
    *(int *)(items[0] + 0x08) = size_z;

    for (i = 0; i < texture_page_count; i++)
        *(int *)(items[0] + 0xC + 4 * i) = texture_eids[i];

    *(int *)(items[0] + 0x2C) = txt_ref_count;
    *(int *)(items[0] + 0x30) = 1;
    *(int *)(items[0] + 0x34) = txt_ref_count;
    *(int *)(items[0] + 0x38) = vertex_count;
    *(int *)(items[0] + 0x3C) = color_count;
    *(int *)(items[0] + 0x40) = texture_page_count;
    *(int *)(items[0] + 0x44) = poly_count;

    for (i = 1; i < model_item_count; i++) {
        item_lengths[i] = 0x4;
        items[i] = (unsigned char *) calloc(4, sizeof (unsigned char));
    }

    char model_path[MAX];
    sprintf(model_path, "%s_converted", fpath);
    FILE *model_new = fopen(model_path, "wb");

    int model_size_new = 0x14 + model_item_count * 4;
    for (i = 0; i < model_item_count; i++)
        model_size_new += item_lengths[i];

    unsigned char buffer[model_size_new];
    *(int *) buffer = MAGIC_ENTRY;
    *(int *)(buffer + 0x4) = model.EID;
    *(int *)(buffer + 0x8) = ENTRY_TYPE_MODEL;
    *(int *)(buffer + 0xC) = model_item_count;
    *(int *)(buffer + 0x10 + model_item_count * 4) = model_size_new;

    int offset = 0x14 + model_item_count * 4;
    for (int i = 0; i < model_item_count; i++)
    {
        *(int *)(buffer + 0x10 + i * 4) = offset;
        memcpy(buffer + offset, items[i], item_lengths[i]);
        offset += item_lengths[i];
    }

    fwrite(buffer, sizeof(unsigned char), model_size_new, model_new);
    fclose(model_new);

    fclose(file1);
    fclose(file2);
    free(model.data);
    free(anim.data);
}
