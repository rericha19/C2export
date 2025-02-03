#include "macros.h"

void build_ll_check_load_list_integrity(ENTRY *elist, int entry_count) {
    printf("\nLoad list integrity check:\n");
    int issue_found = 0;

    int i, j, l, m;
    for (i = 0; i < entry_count; i++) {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL) {
            int cam_count = build_get_cam_item_count(elist[i].data) / 3;
            for (j = 0; j < cam_count; j++) {
                LOAD_LIST load_list = build_get_load_lists(elist[i].data, 2 + 3 * j);

                // load and deload everything in 'positive' direction, whatever remains in list is undeloaded
                LIST list = init_list();
                for (l = 0; l < load_list.count; l++) {
                    if (load_list.array[l].type == 'A')
                        for (m = 0; m < load_list.array[l].list_length; m++) {
                            int len, len2;
                            char temp[6] = "";
                            char temp2[6] = "";
                            len = list.count;
                            list_add(&list, load_list.array[l].list[m]);
                            len2 = list.count;
                            if (len == len2) {
                                issue_found = 1;
                                printf("Duplicate/already loaded entry %s in zone %s path %d load list A point %d\n",
                                       eid_conv(load_list.array[l].list[m], temp), eid_conv(elist[i].eid, temp2), j, load_list.array[l].index);
                            }
                        }

                    if (load_list.array[l].type == 'B')
                        for (m = 0; m < load_list.array[l].list_length; m++)
                            list_remove(&list, load_list.array[l].list[m]);
                }

                // load and deload everything in 'negative' direction, whatever remains in list2 was not loaded
                LIST list2 = init_list();
                 for (l = load_list.count - 1; l >= 0; l--) {
                    if (load_list.array[l].type == 'A')
                        for (m = 0; m < load_list.array[l].list_length; m++)
                            list_remove(&list2, load_list.array[l].list[m]);

                    if (load_list.array[l].type == 'B')
                        for (m = 0; m < load_list.array[l].list_length; m++) {
                            int len, len2;
                            char temp[6] = "";
                            char temp2[6] = "";
                            len = list2.count;
                            list_add(&list2, load_list.array[l].list[m]);
                            len2 = list2.count;
                            if (len == len2) {
                                issue_found = 1;
                                printf("Duplicate/already loaded entry %s in zone %s path %d load list B point %d\n",
                                       eid_conv(load_list.array[l].list[m], temp), eid_conv(elist[i].eid, temp2), j, load_list.array[l].index);
                            }
                        }
                }

                char temp[100] = "";
                if (list.count || list2.count) {
                    issue_found = 1;
                    printf("Zone %5s cam path %d load list incorrect:\n", eid_conv(elist[i].eid, temp), j);
                }

                for (l = 0; l < list.count; l++)
                    printf("\t%5s (never deloaded)\n", eid_conv(list.eids[l], temp));
                for (l = 0; l < list2.count; l++)
                    printf("\t%5s (deloaded before loaded)\n", eid_conv(list2.eids[l], temp));

                delete_load_list(load_list);
            }
        }
    }
    if (issue_found == 0)
        printf("No load list issues were found\n\n");
    else printf("\n");
}


void build_ll_check_draw_list_integrity(ENTRY *elist, int entry_count) {
    printf("Draw list integrity check:\n");
    int issue_found = 0;

    for (int i = 0; i < entry_count; i++) {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE) {
            int cam_count = build_get_cam_item_count(elist[i].data) / 3;
            for (int j = 0; j < cam_count; j++) {

                LOAD_LIST draw_list = build_get_draw_lists(elist[i].data, 2 + 3 * j);

                LIST ids = init_list();
                for (int k = 0; k < draw_list.count; k++) {
                     if (draw_list.array[k].type == 'B')
                        for (int m = 0; m < draw_list.array[k].list_length; m++) {
                            DRAW_ITEM draw_item = build_decode_draw_item(draw_list.array[k].list[m]);
                            int len, len2;
                            char temp2[6] = "";
                            len = ids.count;
                            list_add(&ids, draw_item.ID);
                            len2 = ids.count;
                            if (len == len2) {
                                issue_found = 1;
                                 printf("Duplicate/already drawn ID %4d in zone %s path %d load list B point %d\n",
                                       draw_item.ID, eid_conv(elist[i].eid, temp2), j, draw_list.array[k].index);
                            }
                        }

                    if (draw_list.array[k].type == 'A')
                        for (int m = 0; m < draw_list.array[k].list_length; m++){
                            DRAW_ITEM draw_item = build_decode_draw_item(draw_list.array[k].list[m]);
                            list_remove(&ids, draw_item.ID);
                        }
                }

                LIST ids2 = init_list();
                for (int k = draw_list.count - 1; k >= 0; k--) {
                     if (draw_list.array[k].type == 'B')
                        for (int m = 0; m < draw_list.array[k].list_length; m++) {
                            DRAW_ITEM draw_item = build_decode_draw_item(draw_list.array[k].list[m]);
                            list_remove(&ids2, draw_item.ID);
                        }

                    if (draw_list.array[k].type == 'A')
                        for (int m = 0; m < draw_list.array[k].list_length; m++){
                            DRAW_ITEM draw_item = build_decode_draw_item(draw_list.array[k].list[m]);
                            int len, len2;
                            char temp2[6] = "";
                            len = ids2.count;
                            list_add(&ids2, draw_item.ID);
                            len2 = ids2.count;
                            if (len == len2) {
                                issue_found = 1;
                                 printf("Duplicate/already drawn ID %4d in zone %s path %d load list B point %d\n",
                                       draw_item.ID, eid_conv(elist[i].eid, temp2), j, draw_list.array[k].index);
                            }
                        }
                }

                char temp[100] = "", temp2[100] = "";
                if (ids.count || ids2.count) {
                    issue_found = 1;
                    printf("Zone %s cam path %d draw list incorrect:\n", eid_conv(elist[i].eid, temp), j);
                }

                for (int l = 0; l < ids.count; l++)
                    printf("\t%5d (never undrawn)\n", ids.eids[l]);
                for (int l = 0; l < ids2.count; l++)
                    printf("\t%5d (undrawn before drawn)\n", ids2.eids[l]);

                for (int k = 0; k < draw_list.count; k++) {
                    for (int m = 0; m < draw_list.array[k].list_length; m++) {
                        DRAW_ITEM draw_item = build_decode_draw_item(draw_list.array[k].list[m]);

                        int item1off = build_get_nth_item_offset(elist[i].data, 0);
                        int neighbour_eid = from_u32(elist[i].data + item1off + C2_NEIGHBOURS_START + 4 + 4 * draw_item.neighbour_zone_index);

                        int neighbour_index = build_get_index(neighbour_eid, elist, entry_count);
                        if (neighbour_index == -1) {
                            issue_found = 1;
                            printf("Zone %s cam path %d drawing ent %4d (list%c pt %2d), tied to invalid neighbour %s (neigh.index %d)\n",
                                   eid_conv(elist[i].eid, temp), j, draw_item.ID, draw_list.array[k].type, draw_list.array[k].index,
                                   eid_conv(neighbour_eid, temp2), draw_item.neighbour_zone_index);
                            continue;
                        }

                        int neighbour_cam_item_count = build_get_cam_item_count(elist[neighbour_index].data);
                        int neighbour_entity_count = build_get_entity_count(elist[neighbour_index].data);

                        if (draw_item.neighbour_item_index >= neighbour_entity_count) {
                            issue_found = 1;
                            printf("Zone %s cam path %d drawing ent %4d (list%c pt %2d), tied to %s's %2d. entity, which doesnt exist\n",
                                   eid_conv(elist[i].eid, temp), j, draw_item.ID, draw_list.array[k].type, draw_list.array[k].index,
                                   eid_conv(neighbour_eid, temp2), draw_item.neighbour_item_index);
                            continue;
                        }

                        int neighbour_entity_offset = build_get_nth_item_offset(elist[neighbour_index].data,
                                                                                2 + neighbour_cam_item_count + draw_item.neighbour_item_index);
                        int neighbour_entity_ID = build_get_entity_prop(elist[neighbour_index].data + neighbour_entity_offset, ENTITY_PROP_ID);

                        if (draw_item.ID != neighbour_entity_ID) {
                            issue_found = 1;
                            printf("Zone %s cam path %d drawing ent %4d (list%c pt %2d), tied to %s's %2d. entity, which has ID %4d\n",
                                   eid_conv(elist[i].eid, temp), j, draw_item.ID, draw_list.array[k].type, draw_list.array[k].index,
                                   eid_conv(neighbour_eid, temp2), draw_item.neighbour_item_index, neighbour_entity_ID);
                        }

                    }
                }

                delete_load_list(draw_list);
            }
        }
    }
    if (issue_found == 0)
        printf("No draw list issues were found\n");
    printf("\n");
}

void build_ll_print_avg(ENTRY *elist, int entry_count) {
 /* part that counts avg portion taken of used normal chunks */
    int chunk_count = 0;

    for (int i = 0; i < entry_count; i++)
        if (elist[i].chunk >= chunk_count)
            chunk_count = elist[i].chunk + 1;

    int *chunksizes = (int *) calloc(chunk_count, sizeof(int));
    for (int i = 0; i < entry_count; i++) {
        if (build_is_normal_chunk_entry(elist[i])) {
            int chunk = elist[i].chunk;
            if (chunksizes[chunk] == 0)
                chunksizes[chunk] = 0x14;
            chunksizes[chunk] += 4 + elist[i].esize;
        }
    }

    int normal_chunk_counter = 0;
    int total_size = 0;
    for (int i = 0; i < chunk_count; i++)
        if (chunksizes[i] != 0) {
            normal_chunk_counter++;
            total_size += chunksizes[i];
        }
    free(chunksizes);
    printf("Average normal chunk portion taken: %.3f%c\n", (100 * (double) total_size / normal_chunk_counter) / CHUNKSIZE, '%');
}


int pay_cmp2(const void *a, const void *b) {
    PAYLOAD x = *(PAYLOAD *) a;
    PAYLOAD y = *(PAYLOAD *) b;

    if (x.zone != y.zone)
        return x.zone - y.zone;
    else
        return x.cam_path - y.cam_path;
}

void build_ll_print_full_payload_info(ENTRY *elist, int entry_count, int print_full) {
 /* gets and prints payload ladder */
    PAYLOADS payloads = deprecate_build_get_payload_ladder(elist, entry_count, 0);

    int ans = 1;
    /*printf("\nPick payload print option:\n");
    printf("\tsort by payload, print only payloads over 20 [0]\n");
    printf("\tsort by zones' names, printf all payloads    [1]\n");
    scanf("%d", &ans);
    printf("\n");*/

    if (ans)
        qsort(payloads.arr, payloads.count, sizeof(PAYLOAD), pay_cmp2);
    else
        qsort(payloads.arr, payloads.count, sizeof(PAYLOAD), cmp_func_payload);

    printf("\nFull payload info (max payload of each camera path):\n");
    for (int k = 0; k < payloads.count; k++) {

        if (payloads.arr[k].count >= 21 || ans == 1) {
            printf("%d\t", k + 1);
            deprecate_build_print_payload(payloads.arr[k], 0);
        }

        if (payloads.arr[k].count >= 21 || print_full) {
            qsort(payloads.arr[k].chunks, payloads.arr[k].count, sizeof(int), cmp_func_int);
            printf("    chunks:");
            for (int l = 0; l < payloads.arr[k].count; l++)
                printf(" %3d", 1 + 2 *payloads.arr[k].chunks[l]);
            printf("\n");
        }

        if (payloads.arr[k].tcount >= 9 || print_full) {
            if (payloads.arr[k].tcount >= 9)
                printf("    !!!tpages:");
            else
                printf("    tpages:");
            char temp[6] = "";
            for (int l = 0; l < payloads.arr[k].tcount; l++)
                printf(" %5s", eid_conv(payloads.arr[k].tchunks[l], temp));
            printf("\n");
        }

        free(payloads.arr[k].chunks);
        free(payloads.arr[k].tchunks);
        if (print_full)
            printf("\n");
    }
    if (!print_full)
        printf("\n");
}

int cmp_func_dep(const void *a, const void *b) {
    DEPENDENCY x = *(DEPENDENCY *) a;
    DEPENDENCY y = *(DEPENDENCY *) b;

    if (x.type != y.type)
        return x.type - y.type;
    else
        return x.subtype - y.subtype;
}

void build_ll_various_stats(ENTRY *elist, int entry_count) {
    int total_entity_count = 0;
    DEPENDENCIES deps = build_init_dep();

    for (int i = 0; i < entry_count; i++)
    if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE) {
        int camera_count = build_get_cam_item_count(elist[i].data);
        int entity_count = build_get_entity_count(elist[i].data);

        for (int j = 0; j < entity_count; j++) {
            unsigned char *entity = elist[i].data + build_get_nth_item_offset(elist[i].data, (2 + camera_count + j));
            int type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
            int subt = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);
            int id = build_get_entity_prop(entity, ENTITY_PROP_ID);

            if (id == -1) continue;
            total_entity_count++;

            int found_before = 0;
            for (int k = 0; k < deps.count; k++)
            if (deps.array[k].type == type && deps.array[k].subtype == subt) {
                list_add(&deps.array[k].dependencies, total_entity_count);
                found_before = 1;
            }
            if (!found_before) {
                deps.array = realloc(deps.array, (deps.count + 1) * sizeof(DEPENDENCY));
                deps.array[deps.count].type = type;
                deps.array[deps.count].subtype = subt;
                deps.array[deps.count].dependencies = init_list();
                list_add(&deps.array[deps.count].dependencies, total_entity_count);
                deps.count++;
            }
        }
    }

    qsort(deps.array, deps.count, sizeof(DEPENDENCY), cmp_func_dep);
    printf("Entity type/subtype usage:\n");
    for (int i = 0; i < deps.count; i++)
        printf("Type %2d, subtype %2d: %3d entities\n", deps.array[i].type, deps.array[i].subtype, deps.array[i].dependencies.count);
    printf("\n");
    printf("Miscellaneous stats:\n");
    printf("Entity count:\t%4d\n", total_entity_count);
    printf("Entry count:\t%4d\n", entry_count);
}


void build_ll_check_zone_references(ENTRY *elist, int entry_count) {

    int issue_found = 0;
    char temp1[100] = "";
    char temp2[100] = "";
    printf("\nZones' references check (only zones with camera paths are checked):\n");
    LIST valid_referenced_eids = init_list();

    for (int i = 0; i < entry_count; i++) {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE) {
            int cam_item_count = build_get_cam_item_count(elist[i].data);
            if (cam_item_count == 0) continue;

            LIST sceneries = build_get_sceneries(elist[i].data);
            LIST neighbours = build_get_neighbours(elist[i].data);
            unsigned int music_entry = build_get_zone_track(elist[i].data);

            int music_entry_index = build_get_index(music_entry, elist, entry_count);
            if (music_entry_index == -1 && music_entry != EID_NONE) {
                issue_found = 1;
                printf("Zone %5s references track %5s, which isnt present in the level\n",
                           eid_conv(elist[i].eid, temp1), eid_conv(music_entry, temp2));
            }
            else list_add(&valid_referenced_eids, music_entry);

            for (int j = 0; j < sceneries.count; j++) {
                unsigned int scenery_ref = sceneries.eids[j];
                int elist_index = build_get_index(scenery_ref, elist, entry_count);
                if (elist_index == -1) {
                    issue_found = 1;
                    printf("Zone %5s references scenery %5s, which isnt present in the level\n",
                           eid_conv(elist[i].eid, temp1), eid_conv(scenery_ref, temp2));
                }
                else list_add(&valid_referenced_eids, scenery_ref);
            }

            for (int j = 0; j < neighbours.count; j++) {
                unsigned int neighbour_ref = neighbours.eids[j];
                int elist_index = build_get_index(neighbour_ref, elist, entry_count);
                if (elist_index == -1) {
                    issue_found = 1;
                    printf("Zone %5s references neighbour %5s, which isnt present in the level\n",
                           eid_conv(elist[i].eid, temp1), eid_conv(neighbour_ref, temp2));
                }
                else list_add(&valid_referenced_eids, neighbour_ref);
            }

            for (int j = 0; j < cam_item_count/3; j++) {
                unsigned char *item = build_get_nth_item_offset(elist[i].data, 2 + 3 * j) + elist[i].data;
                unsigned int slst = build_get_slst(item);
                int slst_index = build_get_index(slst, elist, entry_count);
                if (slst_index == -1) {
                    issue_found = 1;
                    printf("Zone %s cam path %d references SLST %5s, which isnt present in the level\n",
                           eid_conv(elist[i].eid, temp1), j, eid_conv(slst, temp2));
                }
                else list_add(&valid_referenced_eids, slst);
            }
        }
    }

    if (issue_found == 0)
        printf("No zone reference issues were found\n");



    printf("\nUnused entries (not referenced by any zone w/ a camera path):\n");
    int unused_entries_found = 0;

    for (int i = 0; i < entry_count; i++) {
        int entry_type = build_entry_type(elist[i]);
        if (entry_type == ENTRY_TYPE_SCENERY ||
            entry_type == ENTRY_TYPE_ZONE ||
            entry_type == ENTRY_TYPE_MIDI ||
            entry_type == ENTRY_TYPE_SLST)
                if (list_find(valid_referenced_eids, elist[i].eid) == -1) {
                    printf("Entry %5s not referenced\n", eid_conv(elist[i].eid, temp1));
                    unused_entries_found = 1;
                }
    }

    if (unused_entries_found == 0)
        printf("No unused entries were found\n");
}


void build_ll_check_gool_references(ENTRY *elist, int entry_count, unsigned int* gool_table) {
    LIST used_types = init_list();

    for (int i = 0; i < entry_count; i++)
    if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE) {
        int camera_count = build_get_cam_item_count(elist[i].data);
        int entity_count = build_get_entity_count(elist[i].data);

        for (int j = 0; j < entity_count; j++) {
            unsigned char *entity = elist[i].data + build_get_nth_item_offset(elist[i].data, (2 + camera_count + j));
            int type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
            int id = build_get_entity_prop(entity, ENTITY_PROP_ID);

            if (type >= 0 && id != -1)
                list_add(&used_types, type);
        }
    }

    char temp[100] = "";
    int unused_gool_entries = 0;
    printf("\nUnused GOOL entries:\n");
    for (int i = 0; i < C2_GOOL_TABLE_SIZE; i++) {
        if (gool_table[i] == EID_NONE) continue;

        if (list_find(used_types, i) == -1) {
            unused_gool_entries = 1;
            printf("GOOL entry %5s type %2d not directly used by any entity (could still be used by other means)\n",
                   eid_conv(gool_table[i], temp), i);
        }
    }

    if (unused_gool_entries == 0)
        printf("No unused gool entries were found\n");

    LIST gool_i6_references = init_list();
    for (int i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_GOOL && build_item_count(elist[i].data) == 6) {
            unsigned int i6_off = build_get_nth_item_offset(elist[i].data, 5);
            unsigned int i6_end = build_get_nth_item_offset(elist[i].data, 6);
            unsigned int i6_len = i6_end - i6_off;

            for (int j = 0; j < i6_len/4; j++) {
                unsigned int potential_eid = from_u32(elist[i].data + i6_off + 4 * j);
                if (build_get_index(potential_eid, elist, entry_count) == -1)
                    continue;

                list_add(&gool_i6_references, potential_eid);
            }
        }

    int unreferenced_animations = 0;
    printf("\nAnimations not referenced by item6 of any GOOL entry:\n");
    LIST model_references = init_list();
    for (int i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ANIM) {
            if (list_find(gool_i6_references, elist[i].eid) == -1) {
                unreferenced_animations = 1;
                printf("Animation %5s not referenced by any GOOL's i6\n", eid_conv(elist[i].eid, temp));
            }

            LIST animations_models = build_get_models(elist[i].data);
            list_copy_in(&model_references, animations_models);
        }

    if (unreferenced_animations == 0)
        printf("No unreferenced animations found\n");

    int unreferenced_models = 0;
    printf("\nModels not referenced by any animation:\n");
    for (int i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_MODEL) {
            if (list_find(model_references, elist[i].eid) == -1) {
                unreferenced_models = 1;
                printf("Model %5s not referenced by any animation\n", eid_conv(elist[i].eid, temp));
            }
        }

    if (unreferenced_models == 0)
        printf("No unreferenced models found\n");
}

void build_ll_id_usage(ENTRY *elist, int entry_count) {

    printf("\nID usage:\n");
    int max_id = 10;

    LIST lists[1024];
    for (int i = 0; i < 1024; i++)
        lists[i] = init_list();

    for (int i = 0; i < entry_count; i++) {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE) {
            int entity_count = build_get_entity_count(elist[i].data);
            int camera_count = build_get_cam_item_count(elist[i].data);
            for (int j = 0; j < entity_count; j++) {
                unsigned char *entity = elist[i].data + build_get_nth_item_offset(elist[i].data, (2 + camera_count + j));
                int id = build_get_entity_prop(entity, ENTITY_PROP_ID);

                if (id == -1 || id >= 1024)
                    continue;

                max_id = max(max_id, id);
                list_add(&lists[id], elist[i].eid);
            }
        }
    }

    char temp[6] = "";
    for (int i = 0; i <= max_id; i++) {
        printf("id %4d: %2d\t", i, lists[i].count);

        if (i < 10)
            printf("reserved ");

        for (int j = 0; j < lists[i].count; j++)
            printf("%5s ", eid_conv(lists[i].eids[j], temp));

        printf("\n");
    }
    printf("\n");
}

void build_ll_check_tpag_references(ENTRY *elist, int entry_count) {

    printf("\nTexture reference check:\n");
    int unreferenced_textures = 0;
    char temp[6] = "";

    LIST text_refs = init_list();
    for (int i = 0; i < entry_count; i++) {
        int entry_type = build_entry_type(elist[i]);
        switch (entry_type) {
            case ENTRY_TYPE_MODEL:
                build_add_model_textures_to_list(elist[i].data, &text_refs);
                break;
            case ENTRY_TYPE_SCENERY:
                build_add_scen_textures_to_list(elist[i].data, &text_refs);
                break;
            case ENTRY_TYPE_GOOL: {
                int ic = build_item_count(elist[i].data);
                if (ic == 6) {
                    unsigned int i6_off = build_get_nth_item_offset(elist[i].data, 5);
                    unsigned int i6_end = build_get_nth_item_offset(elist[i].data, 6);
                    unsigned int i6_len = i6_end - i6_off;

                    for (int j = 0; j < i6_len / 4; j++) {
                        unsigned int potential_eid = from_u32(elist[i].data + i6_off + 4 * j);
                        if (build_get_index(potential_eid, elist, entry_count) == -1)
                            continue;

                        list_add(&text_refs, potential_eid);
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    for (int i = 0; i < entry_count; i++) {
        if (elist[i].data == NULL && eid_conv(elist[i].eid, temp)[4] == 'T') {
            int reffound = 0;
            for (int j = 0; j < text_refs.count; j++) {
                if (elist[i].eid == text_refs.eids[j]) {
                    reffound = 1;
                    break;
                }
            }
            if (reffound == 0) {
                unreferenced_textures = 1;
                printf("Texture %5s not referenced by any GOOL, scenery or model\n",
                       eid_conv(elist[i].eid, temp));
            }
        }
    }

    if (!unreferenced_textures)
        printf("No unreferenced textures found.\n");
}

void build_ll_check_sound_references(ENTRY *elist, int entry_count) {

    printf("\nSound reference check:\n");
    LIST potential_sounds = init_list();
    char temp[6] = "";
    int unreferenced_sounds = 0;

    for (int i = 0; i < entry_count; i++) {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_GOOL) {
            int ic = build_item_count(elist[i].data);
            if (ic == 6) {
                unsigned int i3off = build_get_nth_item_offset(elist[i].data, 2);
                unsigned int i4off = build_get_nth_item_offset(elist[i].data, 3);
                unsigned int i3len = i4off - i3off;

                for (int j = 0; j < i3len / 4; j++) {
                    unsigned int potential_eid = from_u32(elist[i].data + i3off + 4 * j);
                    if (build_get_index(potential_eid, elist, entry_count) == -1)
                        continue;

                    list_add(&potential_sounds, potential_eid);
                }
            }
        }
    }

    for (int i = 0; i < entry_count; i++) {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_SOUND) {
            int reffound = 0;
            for (int j = 0; j < potential_sounds.count; j++) {
                if (potential_sounds.eids[j] == elist[i].eid) {
                    reffound = 1;
                    break;
                }
            }

            if (reffound == 0) {
                unreferenced_sounds = 1;
                printf("Sound %5s not referenced by any GOOL\n",
                       eid_conv(elist[i].eid, temp));
            }
        }
    }

    if (!unreferenced_sounds)
        printf("No unreferenced sounds found\n");
}

void build_ll_check_gool_types(ENTRY *elist, int entry_count) {
    printf("\nGOOL types list:\n");
    char temp[6] = "";

    LIST lists[64];
    for (int i = 0; i < 64; i++)
        lists[i] = init_list();

    for (int i = 0; i < entry_count; i++) {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_GOOL) {
            int item1_offset = build_get_nth_item_offset(elist[i].data, 0);
            int gool_type = from_u32(elist[i].data + item1_offset);
            if (gool_type < 0 || gool_type > 63) {
                printf("Invalid GOOL type: %s type %d\n", eid_conv(elist[i].eid, temp), gool_type);
            } else {
                list_add(&lists[gool_type], elist[i].eid);
            }
        }
    }

    for (int i = 0; i < 64; i++) {
        printf("%2d:\t", i);
        for (int j = 0; j < lists[i].count; j++) {
            printf("%s\t", eid_conv(lists[i].eids[j], temp));
        }
        printf("\n");
    }
    printf("\n");
}

void ll_payload_info_main() {
    ENTRY elist[ELIST_DEFAULT_SIZE];
    int entry_count = 0;
    unsigned int gool_table[C2_GOOL_TABLE_SIZE];
    for (int i = 0; i < C2_GOOL_TABLE_SIZE; i++)
        gool_table[i] = EID_NONE;

    if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, gool_table, elist, &entry_count, NULL, NULL, 1, NULL))
        return;

    build_ll_print_full_payload_info(elist, entry_count, 1);
    build_cleanup_elist(elist, entry_count);
    printf("Done.\n\n");
}

void build_ll_analyze() {
    ENTRY elist[ELIST_DEFAULT_SIZE];
    int entry_count = 0;
    unsigned int gool_table[C2_GOOL_TABLE_SIZE];
    for (int i = 0; i < C2_GOOL_TABLE_SIZE; i++)
        gool_table[i] = EID_NONE;

    if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, gool_table, elist, &entry_count, NULL, NULL, 1, NULL))
        return;

    build_ll_id_usage(elist, entry_count);
    build_ll_various_stats(elist, entry_count);
    build_get_box_count(elist, entry_count);
    build_ll_print_avg(elist, entry_count);
    build_ll_print_full_payload_info(elist, entry_count, 0);
    build_ll_check_gool_types(elist, entry_count);
    build_ll_check_gool_references(elist, entry_count, gool_table);
    build_ll_check_tpag_references(elist, entry_count);
    build_ll_check_sound_references(elist, entry_count);
    build_ll_check_zone_references(elist, entry_count);
    build_ll_check_load_list_integrity(elist, entry_count);
    build_ll_check_draw_list_integrity(elist, entry_count);

    build_cleanup_elist(elist, entry_count);
    printf("Done.\n\n");
}

