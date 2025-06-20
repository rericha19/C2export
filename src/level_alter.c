#include "macros.h"

void wipe_draw_lists(ENTRY *elist, int entry_count)
{
    for (int i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE)
            continue;

        int cam_count = build_get_cam_item_count(elist[i].data) / 3;
        for (int j = 0; j < cam_count; j++)
        {
            build_entity_alter(&elist[i], 2 + 3 * j, build_rem_property, ENTITY_PROP_CAM_DRAW_LIST_A, NULL);
            build_entity_alter(&elist[i], 2 + 3 * j, build_rem_property, ENTITY_PROP_CAM_DRAW_LIST_B, NULL);
        }
    }
}

void wipe_entities(ENTRY *elist, int entry_count)
{
    for (int i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE)
            continue;

        int cam_item_count = build_get_cam_item_count(elist[i].data);
        int entity_count = build_get_entity_count(elist[i].data);

        for (int j = entity_count - 1; j >= 0; j--)
        {
            build_remove_nth_item(&elist[i], 2 + cam_item_count + j);
        }

        int item1off = build_get_nth_item_offset(elist[i].data, 0);
        *(int *)(elist[i].data + item1off + 0x18C) = 0;
    }
}

void convert_old_dl_override(ENTRY *elist, int entry_count)
{
    for (int i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE)
            continue;

        int entity_count = build_get_entity_count(elist[i].data);
        int cam_item_count = build_get_cam_item_count(elist[i].data);

        for (int j = 0; j < entity_count; j++)
        {
            int entity_n = 2 + cam_item_count + j;
            unsigned char *nth_entity = build_get_nth_item(elist[i].data, entity_n);
            int id = build_get_entity_prop(nth_entity, ENTITY_PROP_ID);
            int type = build_get_entity_prop(nth_entity, ENTITY_PROP_TYPE);
            int subt = build_get_entity_prop(nth_entity, ENTITY_PROP_SUBTYPE);
            // skip box counter object
            if (type == 4 && subt == 17)
                continue;

            int old_override_id = build_get_entity_prop(nth_entity, ENTITY_PROP_OVERRIDE_DRAW_ID_OLD);
            int old_override_mult = build_get_entity_prop(nth_entity, ENTITY_PROP_OVERRIDE_DRAW_MULT_OLD);
            PROPERTY *prop_override_id = get_prop(nth_entity, ENTITY_PROP_OVERRIDE_DRAW_ID_OLD);
            PROPERTY *prop_override_mult = get_prop(nth_entity, ENTITY_PROP_OVERRIDE_DRAW_MULT_OLD);

            printf("zone %s -id %d: -ts %d-%d\t\tid %d mult %d\n", eid_conv2(elist[i].eid), id, type, subt, (short int)(old_override_id >> 8), (short int)(old_override_mult >> 8));

            if (old_override_id != -1)
            {
                build_entity_alter(&elist[i], entity_n, build_rem_property, ENTITY_PROP_OVERRIDE_DRAW_ID_OLD, NULL);
                *(short int *)(prop_override_id->header) = ENTITY_PROP_OVERRIDE_DRAW_ID;
                build_entity_alter(&elist[i], entity_n, build_add_property, ENTITY_PROP_OVERRIDE_DRAW_ID, prop_override_id);
            }
            if (old_override_mult != -1)
            {
                build_entity_alter(&elist[i], entity_n, build_rem_property, ENTITY_PROP_OVERRIDE_DRAW_MULT_OLD, NULL);
                *(short int *)(prop_override_mult->header) = ENTITY_PROP_OVERRIDE_DRAW_MULT;
                build_entity_alter(&elist[i], entity_n, build_add_property, ENTITY_PROP_OVERRIDE_DRAW_MULT, prop_override_mult);
            }
        }
    }
}

// mix of rebuild and keeping original stuff
void level_alter_pseudorebuild(int alter_type)
{
    // start unpacking stuff
    char nsfpath[MAX] = "";
    char nsdpath[MAX] = "";
    char nsfpathout[MAX] = "";
    char nsdpathout[MAX] = "";

    printf("Input the path to the level (.nsf):\n");
    scanf(" %[^\n]", nsfpath);
    path_fix(nsfpath);

    strcpy(nsdpath, nsfpath);
    nsdpath[strlen(nsdpath) - 1] = 'D';

    int level_ID = build_ask_ID();

    // make output paths
    strcpy(nsfpathout, nsfpath);
    sprintf(nsfpathout + strlen(nsfpathout) - 6, "%02X.NSF", level_ID);
    strcpy(nsdpathout, nsfpathout);
    nsdpathout[strlen(nsdpathout) - 1] = 'D';

    unsigned char *chunks[CHUNK_LIST_DEFAULT_SIZE] = {NULL};  // array of pointers to potentially built chunks, fixed length cuz lazy
    unsigned char *chunks2[CHUNK_LIST_DEFAULT_SIZE] = {NULL}; // keeping original non-normal chunks to keep level order
    ENTRY elist[ELIST_DEFAULT_SIZE];
    int entry_count = 0;
    int chunk_count = 0;

    SPAWNS spawns = init_spawns();               // struct with spawns found during reading and parsing of the level data
    unsigned int gool_table[C2_GOOL_TABLE_SIZE]; // table w/ eids of gool entries, needed for nsd, filled using input entries
    for (int i = 0; i < C2_GOOL_TABLE_SIZE; i++)
        gool_table[i] = EID_NONE;

    if (build_read_and_parse_rebld(NULL, NULL, NULL, &chunk_count, gool_table, elist, &entry_count, chunks, &spawns, 1, nsfpath))
        return;

    // get original (texture/sound/instrument) chunk data
    // its opened and read again :))) dont care
    FILE *nsf_og = fopen(nsfpath, "rb");
    if (nsf_og == NULL)
    {
        printf("[ERROR] Could not open selected NSF\n\n");
        return;
    }
    int nsf_chunk_count = build_get_chunk_count_base(nsf_og);
    unsigned char buffer[CHUNKSIZE];
    for (int i = 0; i < nsf_chunk_count; i++)
    {
        fread(buffer, sizeof(unsigned char), CHUNKSIZE, nsf_og);
        int ct = build_chunk_type(buffer);
        if (ct == CHUNK_TYPE_TEXTURE || ct == CHUNK_TYPE_SOUND || ct == CHUNK_TYPE_INSTRUMENT)
        {
            chunks2[i] = (unsigned char *)malloc(CHUNKSIZE);
            memcpy(chunks2[i], buffer, CHUNKSIZE);
        }
    }
    chunk_count = nsf_chunk_count;
    fclose(nsf_og);

    // just to get original spawn
    FILE *nsd_og = fopen(nsdpath, "rb");
    if (nsd_og)
    {
        unsigned char nsd_buf[CHUNKSIZE];
        fseek(nsd_og, 0, SEEK_END);
        int nsd_size = ftell(nsd_og);
        rewind(nsd_og);
        fread(nsd_buf, nsd_size, 1, nsd_og);

        int spawn0_offset =
            C2_NSD_ENTRY_TABLE_OFFSET +
            (from_u32(nsd_buf + C2_NSD_ENTRY_COUNT_OFFSET) * 8) +
            0x10 +
            0x1CC;

        spawns.spawns[0].zone = from_u32(nsd_buf + spawn0_offset);
        spawns.spawns[0].x = from_s32(nsd_buf + spawn0_offset + 0xC) >> 8;
        spawns.spawns[0].y = from_s32(nsd_buf + spawn0_offset + 0x10) >> 8;
        spawns.spawns[0].z = from_s32(nsd_buf + spawn0_offset + 0x14) >> 8;

        spawns.spawns[1].zone = from_u32(nsd_buf + spawn0_offset);
        spawns.spawns[1].x = from_s32(nsd_buf + spawn0_offset + 0xC) >> 8;
        spawns.spawns[1].y = from_s32(nsd_buf + spawn0_offset + 0x10) >> 8;
        spawns.spawns[1].z = from_s32(nsd_buf + spawn0_offset + 0x14) >> 8;

        spawns.spawn_count = 2;
    }
    // end unpacking stuff

    // apply alterations
    switch (alter_type)
    {
    case Alter_Type_WipeDL:
        wipe_draw_lists(elist, entry_count);
        break;
    case Alter_Type_WipeEnts:
        wipe_draw_lists(elist, entry_count);
        wipe_entities(elist, entry_count);
        break;
    case Alter_Type_Old_DL_Override:
        convert_old_dl_override(elist, entry_count);
        break;
    default:
        break;
    }

    // start repacking stuff
    for (int i = 0; i < entry_count; i++)
    {
        int et = build_entry_type(elist[i]);
        if (et == ENTRY_TYPE_SOUND || et == ENTRY_TYPE_INST || et == -1)
            elist[i].chunk = -1;
    }

    FILE *nsfnew = fopen(nsfpathout, "wb");
    if (nsfnew == NULL)
    {
        printf("\n[ERROR] Couldn't open out NSF - path %s\n\n", nsfpathout);
        return;
    }

    FILE *nsdnew = fopen(nsdpathout, "wb");
    if (nsdnew == NULL)
    {
        fclose(nsfnew);
        printf("\n[ERROR] Couldn't open out NSD - path %s\n\n", nsdpathout);
        return;
    }

    build_write_nsd(nsdnew, NULL, elist, entry_count, chunk_count, spawns, gool_table, level_ID);
    build_sort_load_lists(elist, entry_count);
    build_normal_chunks(elist, entry_count, 0, chunk_count, chunks, 0);
    // re-use original non-normal chunks
    for (int i = 0; i < chunk_count; i++)
    {
        if (chunks2[i] != NULL)
        {
            chunks[i] = (unsigned char *)malloc(CHUNKSIZE);
            memcpy(chunks[i], chunks2[i], CHUNKSIZE);
        }
    }
    build_write_nsf(nsfnew, elist, entry_count, 0, chunk_count, chunks, NULL);

    // cleanup
    build_cleanup_elist(elist, entry_count);
    fclose(nsfnew);
    fclose(nsdnew);
    for (int i = 0; i < chunk_count; i++)
    {
        if (chunks2[i])
            free(chunks2[i]);
        if (chunks[i])
            free(chunks[i]);
    }

    printf("\nSaved as %s / .NSD\n", nsfpathout);
    printf("Saving with CrashEdit recommended\n");
    printf("Done.\n\n");
}
