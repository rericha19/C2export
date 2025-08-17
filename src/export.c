#include "macros.h"

// converts camera mode from c3 to c2 values
void export_camera_fix(uint8_t *cam, int32_t length)
{
    int32_t curr_off;

    for (int32_t i = 0; i < cam[0xC]; i++)
    {
        if (cam[0x10 + i * 8] == 0x29 && cam[0x11 + i * 8] == 0)
        {
            curr_off = BYTE * cam[0x13 + i * 8] + cam[0x12 + i * 8] + 0xC + 4;
            switch (curr_off)
            {
            case 0x80:
                cam[curr_off] = 8;
                break;
            case 4:
                cam[curr_off] = 3;
                break;
            case 1:
                cam[curr_off] = 0;
                break;
            case 2:
                break;
            default:
                break;
            }
        }
    }
}

// fixes entity coords (from c3 to c2)
void export_entity_coord_fix(uint8_t *item, int32_t itemlength)
{
    int32_t off0x4B = 0, off0x30E = 0;
    double scale;
    int16_t coord;

    // printf("%d\n", itemlength);
    for (int32_t i = 0; i < item[0xC]; i++)
    {
        if (item[0x10 + i * 8] == 0x4B && item[0x11 + i * 8] == 0)
            off0x4B = BYTE * item[0x13 + i * 8] + item[0x12 + i * 8] + 0xC;
        if (item[0x10 + i * 8] == 0x0E && item[0x11 + i * 8] == 3)
            off0x30E = BYTE * item[0x13 + i * 8] + item[0x12 + i * 8] + 0xC;
    }

    if (off0x4B && off0x30E)
    {
        switch (item[off0x30E + 4])
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
            scale = 1;
            break;
        }

        for (int32_t i = 0; i < item[off0x4B] * 6; i += 2)
        {
            if (item[off0x4B + 0x5 + i] < 0x80)
            {
                coord = BYTE * (signed)item[off0x4B + 0x5 + i] + (signed)item[off0x4B + 0x4 + i];
                coord = (int16_t)coord * scale;
                item[off0x4B + 0x5 + i] = coord / 256;
                item[off0x4B + 0x4 + i] = coord % 256;
            }
            else
            {
                coord = 65536 - BYTE * (signed)item[off0x4B + 0x5 + i] - (signed)item[off0x4B + 0x4 + i];
                coord = (int16_t)coord * scale;
                item[off0x4B + 0x5 + i] = 255 - coord / 256;
                item[off0x4B + 0x4 + i] = 256 - coord % 256;
                if (item[off0x4B + 0x4 + i] == 0)
                    item[off0x4B + 0x5 + i]++;
            }
        }
    }
}

// creates a string thats a save path for the currently processed file
char *export_make_path(char *dir_path, char *type, uint32_t eid, int32_t counter)
{
    static char finalpath[MAX + 40];

    if (strcmp(type, "texture") == 0)
        sprintf(finalpath, "%s\\\\%s %s %d", dir_path, type, eid_conv2(eid), counter);
    else
        sprintf(finalpath, "%s\\\\%s %s %d.nsentry", dir_path, type, eid_conv2(eid), counter);

    return finalpath;
}

void export_zone_patch(ENTRY *zone, int32_t zonetype, int32_t game, bool porting)
{
    if (!porting)
        return;

    int32_t lcl_entrysize = zone->esize;
    int32_t curr_off, lcl_temp, irrelitems, next_off;

    uint8_t *cpy = (uint8_t *)calloc(lcl_entrysize, sizeof(uint8_t));
    memcpy(cpy, zone->data, lcl_entrysize);

    if (game == 2) // c2 to c3
    {
        if (zonetype == 16)
        {
            lcl_entrysize += 0x40;
            cpy = (uint8_t *)realloc(cpy, lcl_entrysize);

            // offset fix
            for (int32_t j = 0; j < cpy[0xC]; j++)
            {
                lcl_temp = BYTE * cpy[0x15 + j * 4] + cpy[0x14 + j * 4];
                lcl_temp += 0x40;
                cpy[0x15 + j * 4] = lcl_temp / BYTE;
                cpy[0x14 + j * 4] = lcl_temp % BYTE;
            }

            // inserts byte in a very ugly way, look away
            curr_off = BYTE * cpy[0x11] + cpy[0x10];
            for (int32_t i = lcl_entrysize - 0x21; i >= curr_off + C2_NEIGHBOURS_END; i--)
                cpy[i] = cpy[i - 0x20];

            for (int32_t i = curr_off + C2_NEIGHBOURS_END; i < curr_off + C2_NEIGHBOURS_FLAGS_END; i++)
                cpy[i] = 0;

            for (int32_t i = lcl_entrysize - 1; i >= curr_off + 0x200; i--)
                cpy[i] = cpy[i - 0x20];

            for (int32_t i = curr_off + 0x200; i < curr_off + 0x220; i++)
                cpy[i] = 0;
        }
    }
    else // c3 to c2 removes bytes
    {
        if ((BYTE * cpy[0x15] + cpy[0x14] - BYTE * cpy[0x11] - cpy[0x10]) == 0x358)
        {
            for (int32_t j = 0; j < cpy[0xC]; j++)
            {
                lcl_temp = BYTE * cpy[0x15 + j * 4] + cpy[0x14 + j * 4];
                lcl_temp -= 0x40;
                cpy[0x15 + j * 4] = lcl_temp / BYTE;
                cpy[0x14 + j * 4] = lcl_temp % BYTE;
            }

            curr_off = BYTE * cpy[0x11] + cpy[0x10];
            for (int32_t i = curr_off + C2_NEIGHBOURS_END; i < lcl_entrysize - 20; i++)
                cpy[i] = cpy[i + 0x20];

            for (int32_t i = curr_off + C2_NEIGHBOURS_FLAGS_END; i < lcl_entrysize - 20; i++)
                cpy[i] = cpy[i + 0x20];

            if (cpy[curr_off + 0x190] > 8)
                cpy[curr_off + 0x190] = 8;
            lcl_entrysize -= 0x40;
        }

        irrelitems = cpy[cpy[0x10] + 256 * cpy[0x11] + 0x184] + cpy[cpy[0x10] + 256 * cpy[0x11] + 0x188];
        for (int32_t i = irrelitems; i < cpy[0xC]; i++)
        {
            curr_off = BYTE * cpy[0x11 + i * 4] + cpy[0x10 + i * 4];
            next_off = BYTE * cpy[0x15 + i * 4] + cpy[0x14 + i * 4];
            if (next_off == 0)
                next_off = CHUNKSIZE;
            export_entity_coord_fix(cpy + curr_off, next_off - curr_off);
        }

        irrelitems = cpy[cpy[0x10] + 256 * cpy[0x11] + 0x188];
        for (int32_t i = 0; i < irrelitems / 3; i++)
        {
            curr_off = BYTE * cpy[0x19 + i * 0xC] + cpy[0x18 + i * 0xC];
            next_off = BYTE * cpy[0x19 + i * 0xC + 0x4] + cpy[0x18 + i * 0xC + 0x4];
            if (next_off == 0)
                next_off = CHUNKSIZE;
            export_camera_fix(cpy + curr_off, next_off - curr_off);
        }
    }

    free(zone->data);
    zone->data = cpy;
    zone->esize = lcl_entrysize;
}

void export_gool_patch(ENTRY *gool, int32_t game, bool porting)
{
    if (!porting)
        return;

    int entrysize = gool->esize;
    int32_t curr_off = 0;

    uint8_t *cpy = (uint8_t *)calloc(entrysize, sizeof(uint8_t));
    memcpy(cpy, gool->data, entrysize);

    if (cpy[0xC] == 6)
    {
        curr_off = BYTE * cpy[0x25] + cpy[0x24];
        if (game == 2) // if its c2 to c3
        {
            ;
        }
        else // c3 to c2
        {
            while (curr_off < entrysize)
            {
                switch (cpy[curr_off])
                {
                case 1:
                    for (int32_t i = 1; i < 4; i++)
                        for (int32_t j = 0; j < 4; j++)
                            cpy[curr_off + 0x10 - 4 * i + j] = cpy[curr_off + 0x10 + j];

                    curr_off += 0x14;
                    break;
                case 2:
                    curr_off += 0x8 + 0x10 * cpy[curr_off + 2];
                    break;
                case 3:
                    curr_off += 0x8 + 0x14 * cpy[curr_off + 2];
                    break;
                case 4:
                    curr_off = entrysize;
                    break;
                case 5:
                    curr_off += 0xC + 0x18 * cpy[curr_off + 2] * cpy[curr_off + 8];
                    break;
                }
            }
        }
    }

    free(gool->data);
    gool->data = cpy;
    gool->esize = entrysize;
}

void export_model_patch(ENTRY *model, int32_t game, bool porting)
{
    if (!porting)
        return;

    float scaling = 0;
    int32_t msize = 0;

    // scale change in case its porting
    if (game == 2)
        scaling = 1. / 8;
    else
        scaling = 8;

    uint8_t *buffer = model->data;
    for (int32_t i = 0; i < 3; i++)
    {
        msize = BYTE * buffer[buffer[0x10] + 1 + i * 4] + buffer[buffer[0x10] + i * 4];
        msize = (int32_t)msize * scaling;
        buffer[buffer[0x10] + 1 + i * 4] = msize / BYTE;
        buffer[buffer[0x10] + i * 4] = msize % BYTE;
    }
}

void export_level(int32_t levelid, ENTRY *elist, int32_t entry_count, uint8_t **chunks, uint8_t **chunks2, int32_t chunk_count)
{
    char c = 0;
    int32_t game = 0;
    printf("Which game are the files from? [2/3]\n");
    printf("Change to other game's format? [Y/N]\n");
    scanf("%d %c", &game, &c);
    c = toupper(c);

    if ((game != 2 && game != 3) || (c != 'N' && c != 'Y'))
    {
        printf("[ERROR] invalid answer, ending\n");
        return;
    }

    int32_t out_game = 0;
    bool porting = false;
    if (c == 'Y')
    {
        porting = true;
        out_game = (game == 2) ? 3 : 2;
    }
    else
    {
        out_game = game;
    }

    int32_t zonetype = 8;
    if (game == 2 && out_game == 3)
    {
        printf("How many neighbours should the exported files' zones have? [8/16]\n");
        scanf("%d", &zonetype);
        if (!(zonetype == 8 || zonetype == 16))
        {
            printf("[error] invalid neighbour count, defaulting to 8\n");
            zonetype = 8;
        }
    }

    printf("Selected game: Crash %d, porting: %d, zone neighbours (if C2->C3): %d\n\n", game, porting, zonetype);

    char out_dir[MAX] = "";
    sprintf(out_dir, "C%d_to_C%d_S00000%02X", game, out_game, levelid);
    mkdir(out_dir);

    printf("Saving to: \n%s\n", out_dir);

    int32_t entry_counter = 0;
    for (int32_t i = 0; i < chunk_count; i++)
    {
        if (build_chunk_type(chunks2[i]) != CHUNK_TYPE_TEXTURE)
            continue;

        char *path = export_make_path(out_dir, "texture", from_u32(chunks2[i] + 4), ++entry_counter);
        FILE *f = fopen(path, "wb");
        fwrite(chunks2[i], sizeof(uint8_t), CHUNKSIZE, f);
        fclose(f);
    }

    static char entry_type_list[22][100] = {
        "unk_0",
        "animation",
        "model",
        "scenery",
        "slst",
        "tpag_5",
        "ldat_6",
        "zone",
        "unk_8",
        "unk_9",
        "unk_10",
        "gool",
        "sound",
        "music",
        "instrument",
        "vcol_15",
        "unk_16",
        "unk_17",
        "palettes_18",
        "demo",
        "sdio20",
        "t21",
    };
    for (int32_t i = 0; i < entry_count; i++)
    {
        int32_t entry_type = build_entry_type(elist[i]);
        if (entry_type == -1)
            continue;

        uint32_t eid = elist[i].eid;
        if (entry_type == ENTRY_TYPE_ZONE)
            export_zone_patch(&elist[i], zonetype, game, porting);
        else if (entry_type == ENTRY_TYPE_GOOL)
            export_gool_patch(&elist[i], game, porting);
        else if (entry_type == ENTRY_TYPE_MODEL)
            export_model_patch(&elist[i], game, porting);        

        char *path = export_make_path(out_dir, entry_type_list[entry_type], eid, ++entry_counter);
        FILE *f = fopen(path, "wb");
        fwrite(elist[i].data, sizeof(uint8_t), elist[i].esize, f);
        fclose(f);
    }

    printf("\nConversion of some things is not implemented here, use CrashEdit (scenery coords, animations)");
    printf("Done.\n\n");    
}
