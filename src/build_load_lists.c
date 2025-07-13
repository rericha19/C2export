#include "macros.h"
// responsible for generating load lists
// has memory leaks, cba to fix

/** \brief
 *  A function that for each zone's each camera path creates new load lists using
 *  the provided list of permanently loaded entries and
 *  the provided list of dependencies of certain entitites, gets models from the animations.
 *  For the zone itself and its linked neighbours it adds all relatives and their dependencies.
 *  Loads only one sound per sound chunk (sound chunks should already be built by this point).
 *  Load list properties get removed and then replaced by the provided list using 'camera_alter' function.
 *  Load lists get sorted later during the nsd writing process.
 *
 * \param elist ENTRY*                  list of current entries, array of ENTRY
 * \param entry_count int32_t               current amount of entries
 * \param gool_table uint32_t*      table that contains GOOL entries on their expected slot
 * \param permaloaded LIST              list of permaloaded entries
 * \param subtype_info DEPENDENCIES     list of {type, subtype, count, dependencies[count]}
 * \return void
 */
void build_remake_load_lists(ENTRY *elist, int32_t entry_count, uint32_t *gool_table, LIST permaloaded, DEPENDENCIES subtype_info, DEPENDENCIES collision, DEPENDENCIES mus_deps, int32_t *config)
{
    int32_t load_list_sound_entry_inc_flag = config[CNFG_IDX_LL_SND_INCLUSION_FLAG];
    int32_t i, j, k, l;

    int32_t dbg_print = 0;

    // gets a list of sound eids (one per chunk) to make load lists smaller
    int32_t chunks[8];
    int32_t sound_chunk_count = 0;
    uint32_t sounds_to_load[8];
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_SOUND)
        {
            int32_t is_there = 0;
            for (j = 0; j < sound_chunk_count; j++)
                if (chunks[j] == elist[i].chunk)
                {
                    is_there = 1;
                    break;
                }

            if (!is_there)
            {
                chunks[sound_chunk_count] = elist[i].chunk;
                sounds_to_load[sound_chunk_count] = elist[i].eid;
                sound_chunk_count++;
            }
        }

    // for each zone entry do load list (also for each zone's each camera)
    for (i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            int32_t cam_count = build_get_cam_item_count(elist[i].data) / 3;
            if (cam_count == 0)
                continue;

            char temp[100] = "";
            printf("Making load lists for %s\n", eid_conv(elist[i].eid, temp));

            // get list of special entries that can be placed inside zones' first item
            // as a special zone-specific dependency/load list
            LIST special_entries = build_get_special_entries(elist[i], elist, entry_count);

            uint32_t music_ref = build_get_zone_track(elist[i].data);

            for (j = 0; j < cam_count; j++)
            {
                printf("\t cam path %d\n", j);
                int32_t cam_offset = build_get_nth_item_offset(elist[i].data, 2 + 3 * j);
                int32_t cam_length = build_get_path_length(elist[i].data + cam_offset);

                // initialise full non-delta load list used to represent the load list during its building
                LIST *full_load = (LIST *)malloc(cam_length * sizeof(LIST)); // freed here
                for (k = 0; k < cam_length; k++)
                    full_load[k] = init_list();

                // add permaloaded entries
                for (k = 0; k < cam_length; k++)
                    list_copy_in(&full_load[k], permaloaded);

                for (l = 0; l < mus_deps.count; l++)
                {
                    if (mus_deps.array[l].type == music_ref)
                    {
                        for (k = 0; k < cam_length; k++)
                            list_copy_in(&full_load[k], mus_deps.array[l].dependencies);
                    }
                }

                if (dbg_print)
                    printf("Copied in permaloaded and music refs\n");

                // add current zone's special entries
                for (k = 0; k < cam_length; k++)
                    list_copy_in(&full_load[k], special_entries);

                if (dbg_print)
                    printf("Copied in special\n");

                // add relatives (directly related entries like slst, direct neighbours, scenery)
                // might not be necessary, relative collection was implemented for previous methods and it was easier to keep it
                if (elist[i].related != NULL)
                    for (k = 0; (unsigned)k < elist[i].related[0]; k++)
                        for (l = 0; l < cam_length; l++)
                            list_add(&full_load[l], elist[i].related[k + 1]);

                if (dbg_print)
                    printf("Copied in deprecate relatives\n");

                // all sounds
                if (load_list_sound_entry_inc_flag == 0)
                {
                    for (k = 0; k < entry_count; k++)
                        if (build_entry_type(elist[k]) == ENTRY_TYPE_SOUND)
                            for (l = 0; l < cam_length; l++)
                                list_add(&full_load[l], elist[k].eid);
                }
                // one sound per chunk
                if (load_list_sound_entry_inc_flag == 1)
                {
                    for (k = 0; k < cam_length; k++)
                        for (l = 0; l < sound_chunk_count; l++)
                            list_add(&full_load[k], sounds_to_load[l]);
                }

                if (dbg_print)
                    printf("Copied in sounds\n");

                // add direct neighbours
                LIST neighbours = build_get_neighbours(elist[i].data);
                for (k = 0; k < cam_length; k++)
                    list_copy_in(&full_load[k], neighbours);

                if (dbg_print)
                    printf("Copied in neighbours\n");

                // for each scenery in current zone's scen reference list, add its textures to the load list
                int32_t scenery_count = build_get_scen_count(elist[i].data);
                for (k = 0; k < scenery_count; k++)
                {
                    int32_t scenery_index = build_get_index(from_u32(build_get_nth_item(elist[i].data, 0) + 0x4 + 0x30 * k),
                                                        elist, entry_count);
                    for (l = 0; l < cam_length; l++)
                        build_add_scen_textures_to_list(elist[scenery_index].data, &full_load[l]);
                }

                if (dbg_print)
                    printf("Copied in some scenery stuff\n");

                // path link, draw list and other bs dependent additional load list improvements
                build_load_list_util(i, 2 + 3 * j, full_load, cam_length, elist, entry_count, subtype_info, collision, config);

                if (dbg_print)
                    printf("Load list util ran\n");

                // checks whether the load list doesnt try to load more than 8 textures, prints if yes
                build_texture_count_check(elist, entry_count, full_load, cam_length, i, j);

                if (dbg_print)
                    printf("Texture chunk was checked\n");

                // creates and initialises delta representation of the load list
                LIST *listA = (LIST *)malloc(cam_length * sizeof(LIST)); // freed here
                LIST *listB = (LIST *)malloc(cam_length * sizeof(LIST)); // freed here
                for (k = 0; k < cam_length; k++)
                {
                    listA[k] = init_list();
                    listB[k] = init_list();
                }

                // converts full load list to delta based, then creates game-format load list properties based on the delta lists
                build_load_list_to_delta(full_load, listA, listB, cam_length, elist, entry_count, 0);
                PROPERTY prop_0x208 = build_make_load_list_prop(listA, cam_length, 0x208);
                PROPERTY prop_0x209 = build_make_load_list_prop(listB, cam_length, 0x209);

                if (dbg_print)
                    printf("Converted full list to delta and delta to props\n");

                // removes existing load list properties, inserts newly made ones
                build_entity_alter(&elist[i], 2 + 3 * j, build_rem_property, 0x208, NULL);
                build_entity_alter(&elist[i], 2 + 3 * j, build_rem_property, 0x209, NULL);
                build_entity_alter(&elist[i], 2 + 3 * j, build_add_property, 0x208, &prop_0x208);
                build_entity_alter(&elist[i], 2 + 3 * j, build_add_property, 0x209, &prop_0x209);

                if (dbg_print)
                    printf("Replaced load list props\n");

                free(full_load);
                free(listA);
                free(listB);

                if (dbg_print)
                    printf("Freed some stuff, end\n");
                // free(prop_0x208.data);
                // free(prop_0x209.data);
            }
        }
    }
}

// for sorting draw list properties by id
int32_t cmp_func_draw(const void *a, const void *b)
{
    uint32_t int_a = *(uint32_t *)a;
    uint32_t int_b = *(uint32_t *)b;

    DRAW_ITEM item_a = build_decode_draw_item(int_a);
    DRAW_ITEM item_b = build_decode_draw_item(int_b);

    return item_a.ID - item_b.ID;
}

/** \brief
 *  Converts the full load list into delta form.
 *
 * \param full_load LIST*               full form of load list
 * \param listA LIST*                   new delta based load list, A side
 * \param listB LIST*                   new delta based load list, B side
 * \param cam_length int32_t                length of the current camera
 * \return void
 */
void build_load_list_to_delta(LIST *full_load, LIST *listA, LIST *listB, int32_t cam_length, ENTRY *elist, int32_t entry_count, int32_t is_draw)
{
    int32_t i, j;

    // full item, listA point 0
    for (i = 0; i < full_load[0].count; i++)
        if (is_draw || build_get_index(full_load[0].eids[i], elist, entry_count) != -1)
            list_add(&listA[0], full_load[0].eids[i]);

    // full item, listB point n-1
    int32_t n = cam_length - 1;
    for (i = 0; i < full_load[n].count; i++)
        if (is_draw || build_get_index(full_load[n].eids[i], elist, entry_count) != -1)
            list_add(&listB[n], full_load[n].eids[i]);

    // creates delta items
    // for each point it takes full load list and for each entry checks whether it has just become loaded/deloaded
    for (i = 1; i < cam_length; i++)
    {

        for (j = 0; j < full_load[i].count; j++)
        {
            uint32_t curr_eid = full_load[i].eids[j];
            if (!is_draw && build_get_index(curr_eid, elist, entry_count) == -1)
                continue;

            // is loaded on i-th point but not on i-1th point -> just became loaded, add to listA[i]
            if (list_find(full_load[i - 1], curr_eid) == -1)
                list_add(&listA[i], curr_eid);
        }

        for (j = 0; j < full_load[i - 1].count; j++)
        {
            uint32_t curr_eid = full_load[i - 1].eids[j];
            if (!is_draw && build_get_index(curr_eid, elist, entry_count) == -1)
                continue;

            // is loaded on i-1th point but not on i-th point -> no longer loaded, add to listB[i - 1]
            if (list_find(full_load[i], curr_eid) == -1)
                list_add(&listB[i - 1], curr_eid);
        }
    }

    // gets rid of cases when an item is in both listA and listB on the same index (getting loaded and instantly deloaded or vice versa)
    // if its in both it removes it from both
    for (i = 0; i < cam_length; i++)
    {
        LIST iter_copy = init_list();
        list_copy_in(&iter_copy, listA[i]);

        for (j = 0; j < iter_copy.count; j++)
            if (list_find(listB[i], iter_copy.eids[j]) != -1)
            {
                list_remove(&listA[i], iter_copy.eids[j]);
                list_remove(&listB[i], iter_copy.eids[j]);
            }
    }

    if (is_draw)
    {
        // sort by entity id
        for (int32_t i = 0; i < cam_length; i++)
        {
            qsort(listA[i].eids, listA[i].count, sizeof(uint32_t), cmp_func_draw);
            qsort(listB[i].eids, listB[i].count, sizeof(uint32_t), cmp_func_draw);
        }
    }
}

/** \brief
 *  Handles most load list creating, except for permaloaded and sound entries.
 *  First part is used for neighbours & scenery & slsts.
 *  Second part is used for draw lists.
 *
 * \param zone_index int32_t                index of the zone in the entry list
 * \param camera_index int32_t              index of the camera item in the zone
 * \param listA LIST*                   load list A, later transformed into actual load list data etc
 * \param listB LIST*                   load list B, later transformed into actual load list data and placed into the entry
 * \param cam_length int32_t                length of the camera
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \return void
 */
void build_load_list_util(int32_t zone_index, int32_t camera_index, LIST *full_list, int32_t cam_length, ENTRY *elist, int32_t entry_count, DEPENDENCIES sub_info, DEPENDENCIES collisions, int32_t *config)
{
    int32_t i, j, k;
    // char temp[6] = "";

    // neighbours, slsts, scenery
    LIST links = build_get_links(elist[zone_index].data, camera_index);
    for (i = 0; i < links.count; i++)
        build_load_list_util_util(zone_index, camera_index, links.eids[i], full_list, cam_length, elist, entry_count, config, collisions);

    // draw lists
    for (i = 0; i < cam_length; i++)
    {
        LIST neighbour_list = init_list();
        // get a list of entities drawn within set distance of current camera point
        LIST entity_list = build_get_entity_list(i, zone_index, camera_index, cam_length, elist, entry_count, &neighbour_list, config);

        neighbour_list.count = neighbour_list.count; // this has to be here otherwise VS optimises it away or some shit, idk im scared
        // get a list of types and subtypes from the entity list
        LIST types_subtypes = build_get_types_subtypes(elist, entry_count, entity_list, neighbour_list);

        // copy in dependency list for each found type/subtype
        for (j = 0; j < types_subtypes.count; j++)
        {
            int32_t type = types_subtypes.eids[j] >> 0x10;
            int32_t subtype = types_subtypes.eids[j] & 0xFF;

            /*char temp[6] = "";
            printf("%s point %2d to load type %2d subtype %2d stuff\n", eid_conv(elist[zone_index].eid, temp), i, type, subtype);*/
            for (k = 0; k < sub_info.count; k++)
                if (sub_info.array[k].subtype == subtype && sub_info.array[k].type == type)
                {
                    list_copy_in(&full_list[i], sub_info.array[k].dependencies);

                    /*for (int32_t l = 0; l < sub_info.array[k].dependencies.count; l++)
                        printf("\t%s\n", eid_conv(sub_info.array[k].dependencies.eids[l], temp));*/
                }
        }
    }
}

/** \brief
 *  Makes a load list property from the input arrays, later its put into the item.
 *
 * \param list_array LIST*              array of lists containing delta load list (one side)
 * \param cam_length int32_t                length of the camera and so the array too
 * \param code int32_t                      property code
 * \return PROPERTY                     property with game's format
 */
PROPERTY build_make_load_list_prop(LIST *list_array, int32_t cam_length, int32_t code)
{
    int32_t i, j, delta_counter = 0, total_length = 0;
    PROPERTY prop;

    // count total length and individual deltas
    for (i = 0; i < cam_length; i++)
        if (list_array[i].count != 0)
        {
            total_length += list_array[i].count * 4; // space individual load list items of current sublist will take up
            total_length += 4;                       // each load list sublist uses 2 bytes for its length and 2 bytes for its index
            delta_counter++;
        }

    // header info
    *(int16_t *)(prop.header) = code;
    if (code == ENTITY_PROP_CAM_LOAD_LIST_A || code == ENTITY_PROP_CAM_LOAD_LIST_B)
    {
        if (delta_counter > 1)
            *(int16_t *)(prop.header + 4) = 0x0464;
        else
            *(int16_t *)(prop.header + 4) = 0x0424;
    }
    if (code == ENTITY_PROP_CAM_DRAW_LIST_A || code == ENTITY_PROP_CAM_DRAW_LIST_B)
    {
        if (delta_counter > 1)
            *(int16_t *)(prop.header + 4) = 0x0473;
        else
            *(int16_t *)(prop.header + 4) = 0x0433;
    }
    *(int16_t *)(prop.header + 6) = delta_counter;

    prop.length = total_length;
    prop.data = (uint8_t *)malloc(total_length * sizeof(uint8_t)); // freed by caller

    int32_t indexer = 0;
    int32_t offset = 4 * delta_counter;
    for (i = 0; i < cam_length; i++)
        if (list_array[i].count != 0)
        {
            *(int16_t *)(prop.data + 2 * indexer) = list_array[i].count; // i-th sublist's length (item count)
            *(int16_t *)(prop.data + 2 * (indexer + delta_counter)) = i; // i-th sublist's index
            for (j = 0; j < list_array[i].count; j++)
                *(int32_t *)(prop.data + offset + 4 * j) = list_array[i].eids[j]; // individual items
            offset += list_array[i].count * 4;
            indexer++;
        }

    return prop;
}

/** \brief
 *  Deals with slst and neighbour/scenery references of path linked entries.
 *
 * \param zone_index int32_t                zone entry index
 * \param cam_index int32_t                 camera item index
 * \param link_int uint32_t         link item int32_t
 * \param full_list LIST*               full load list representation
 * \param cam_length int32_t                length of the camera's path
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \return void
 */
void build_load_list_util_util(int32_t zone_index, int32_t cam_index, int32_t link_int, LIST *full_list, int32_t cam_length, ENTRY *elist, int32_t entry_count, int32_t *config, DEPENDENCIES collisisons)
{
    int32_t slst_distance = config[CNFG_IDX_LL_SLST_DIST_VALUE];
    int32_t neig_distance = config[CNFG_IDX_LL_NEIGH_DIST_VALUE];
    int32_t preloading_flag = config[CNFG_IDX_LL_TRNS_PRLD_FLAG];
    int32_t backwards_penalty = config[CNFG_IDX_LL_BACKWARDS_PENALTY];

    int32_t i, j, item1off = build_get_nth_item_offset(elist[zone_index].data, 0);
    int16_t *coords;
    int32_t path_length, distance = 0;

    CAMERA_LINK link = int_to_link(link_int);

    uint32_t neighbour_eid = from_u32(elist[zone_index].data + item1off + C2_NEIGHBOURS_START + 4 + 4 * link.zone_index);
    uint32_t neighbour_flg = from_u32(elist[zone_index].data + item1off + C2_NEIGHBOURS_START + 4 + 4 * link.zone_index + 0x20);

    int32_t neighbour_index = build_get_index(neighbour_eid, elist, entry_count);
    if (neighbour_index == -1)
        return;

    // if preloading nothing
    if (preloading_flag == PRELOADING_NOTHING && (neighbour_flg == 0xF || neighbour_flg == 0x1F))
        return;

    if (preloading_flag == PRELOADING_TEXTURES_ONLY || preloading_flag == PRELOADING_ALL)
    {
        int32_t scenery_count = build_get_scen_count(elist[neighbour_index].data);

        for (i = 0; i < scenery_count; i++)
        {
            int32_t item1off_neigh = from_u32(elist[neighbour_index].data + 0x10);
            int32_t scenery_index = build_get_index(from_u32(elist[neighbour_index].data + item1off_neigh + 0x4 + 0x30 * i), elist, entry_count);
            // kinda sucks but i cbf to make it better rn
            if (link.type == 1)
            {
                int32_t end_index = (cam_length - 1) / 2 - 1;
                for (j = 0; j < end_index; j++)
                    build_add_scen_textures_to_list(elist[scenery_index].data, &full_list[j]);
            }

            if (link.type == 2)
            {
                int32_t start_index = (cam_length - 1) / 2 + 1;
                for (j = start_index; j < cam_length; j++)
                    build_add_scen_textures_to_list(elist[scenery_index].data, &full_list[j]);
            }
        }
    }

    // if preloading only textures
    if (preloading_flag == PRELOADING_TEXTURES_ONLY && (neighbour_flg == 0xF || neighbour_flg == 0x1F))
        return;

    if (elist[neighbour_index].related != NULL)
        for (i = 0; (unsigned)i < elist[neighbour_index].related[0]; i++)
            for (j = 0; j < cam_length; j++)
                list_add(&full_list[j], elist[neighbour_index].related[i + 1]);

    int32_t neighbour_cam_count = build_get_cam_item_count(elist[neighbour_index].data);
    if (link.cam_index >= neighbour_cam_count)
    {
        char temp[100] = "";
        char temp2[100] = "";
        printf("Zone %s is linked to zone %s's %d. camera path (indexing from 0) when it only has %d paths",
               eid_conv(elist[zone_index].eid, temp), eid_conv(elist[neighbour_index].eid, temp2),
               link.cam_index, neighbour_cam_count);
        return;
    }

    int32_t offset = build_get_nth_item_offset(elist[neighbour_index].data, 2 + 3 * link.cam_index);
    uint32_t slst = build_get_slst(elist[neighbour_index].data + offset);
    for (i = 0; i < cam_length; i++)
        list_add(&full_list[i], slst);

    build_add_collision_dependencies(full_list, 0, cam_length, elist[zone_index].data, collisisons, elist, entry_count);
    build_add_collision_dependencies(full_list, 0, cam_length, elist[neighbour_index].data, collisisons, elist, entry_count);

    coords = build_get_path(elist, neighbour_index, 2 + 3 * link.cam_index, &path_length);
    distance += build_get_distance(coords, 0, path_length - 1, -1, NULL);

    LIST layer2 = build_get_links(elist[neighbour_index].data, 2 + 3 * link.cam_index);
    for (i = 0; i < layer2.count; i++)
    {
        int32_t item1off2 = from_u32(elist[neighbour_index].data + 0x10);
        CAMERA_LINK link2 = int_to_link(layer2.eids[i]);

        uint32_t neighbour_eid2 = from_u32(elist[neighbour_index].data + item1off2 + C2_NEIGHBOURS_START + 4 + 4 * link2.zone_index);
        uint32_t neighbour_flg2 = from_u32(elist[neighbour_index].data + item1off2 + C2_NEIGHBOURS_START + 4 + 4 * link2.zone_index + 0x20);

        int32_t neighbour_index2 = build_get_index(neighbour_eid2, elist, entry_count);
        if (neighbour_index2 == -1)
            continue;

        // not preloading everything
        if ((preloading_flag == PRELOADING_NOTHING || preloading_flag == PRELOADING_TEXTURES_ONLY) && (neighbour_flg2 == 0xF || neighbour_flg2 == 0x1F))
            continue;

        int32_t offset2 = build_get_nth_item_offset(elist[neighbour_index2].data, 2 + 3 * link2.cam_index);
        uint32_t slst2 = build_get_slst(elist[neighbour_index2].data + offset2);

        LIST neig_list = build_get_neighbours(elist[neighbour_index2].data);
        list_copy_in(&neig_list, build_get_sceneries(elist[neighbour_index2].data));

        LIST slst_list = init_list();
        list_add(&slst_list, slst2);

        build_add_collision_dependencies(full_list, 0, cam_length, elist[neighbour_index2].data, collisisons, elist, entry_count);
        coords = build_get_path(elist, zone_index, cam_index, &path_length);

        int32_t neighbour_cam_count2 = build_get_cam_item_count(elist[neighbour_index2].data);
        if (link2.cam_index >= neighbour_cam_count2)
        {
            char temp[100] = "";
            char temp2[100] = "";
            printf("Zone %s is linked to zone %s's %d. camera path (indexing from 0) when it only has %d paths",
                   eid_conv(elist[neighbour_index].eid, temp), eid_conv(elist[neighbour_index2].eid, temp2),
                   link2.cam_index, neighbour_cam_count2);
            continue;
        }

        int32_t slst_dist_w_orientation = slst_distance;
        int32_t neig_dist_w_orientation = neig_distance;

        if (build_is_before(elist, zone_index, cam_index / 3, neighbour_index2, link2.cam_index))
        {
            /*char temp[100], temp2[100];
            printf("util util %s is before %s\n", eid_conv(elist[neighbour_index2].eid, temp), eid_conv(elist[zone_index].eid, temp2));*/
            slst_dist_w_orientation = build_dist_w_penalty(slst_distance, backwards_penalty);
            neig_dist_w_orientation = build_dist_w_penalty(neig_distance, backwards_penalty);
        }

        if ((link.type == 2 && link.flag == 2 && link2.type == 1) || (link.type == 2 && link.flag == 1 && link2.type == 2))
        {
            build_load_list_util_util_forw(cam_length, full_list, distance, slst_dist_w_orientation, coords, path_length, slst_list);
            build_load_list_util_util_forw(cam_length, full_list, distance, neig_dist_w_orientation, coords, path_length, neig_list);
        }
        if ((link.type == 1 && link.flag == 2 && link2.type == 1) || (link.type == 1 && link.flag == 1 && link2.type == 2))
        {
            build_load_list_util_util_back(cam_length, full_list, distance, slst_dist_w_orientation, coords, path_length, slst_list);
            build_load_list_util_util_back(cam_length, full_list, distance, neig_dist_w_orientation, coords, path_length, neig_list);
        }
    }
}

/** \brief
 *  Starting from the 0th index it finds a path index at which the distance reaches cap distance and loads
 *  the entries specified by the additions list from point 0 to end index.
 *
 * \param cam_length int32_t                length of the considered camera path
 * \param full_list LIST*               load list
 * \param distance int32_t                  distance so far
 * \param final_distance int32_t            distance cap
 * \param coords int16_t*             path of the camera
 * \param path_length int32_t               length of the camera
 * \param additions LIST                list with entries to add
 * \return void
 */
void build_load_list_util_util_back(int32_t cam_length, LIST *full_list, int32_t distance, int32_t final_distance, int16_t *coords, int32_t path_length, LIST additions)
{
    int32_t end_index = 0;
    int32_t j;

    build_get_distance(coords, 0, path_length - 1, final_distance - distance, &end_index);

    if (end_index == 0)
        return;

    for (j = 0; j < end_index; j++)
        list_copy_in(&full_list[j], additions);
}

/** \brief
 *  Starting from the n-1th index it finds a path index at which the distance reaches cap distance and loads
 *  the entries specified by the additions list from point start index n - 1.
 *
 * \param cam_length int32_t                length of the considered camera path
 * \param full_list LIST*               load list
 * \param distance int32_t                  distance so far
 * \param final_distance int32_t            distance cap
 * \param coords int16_t*             path of the camera
 * \param path_length int32_t               length of the camera
 * \param additions LIST                list with entries to add
 * \return void
 */
void build_load_list_util_util_forw(int32_t cam_length, LIST *full_list, int32_t distance, int32_t final_distance, int16_t *coords, int32_t path_length, LIST additions)
{
    int32_t start_index = cam_length - 1;
    int32_t j;

    build_get_distance(coords, path_length - 1, 0, final_distance - distance, &start_index);
    if (start_index == cam_length - 1)
        return;

    for (j = start_index; j < cam_length; j++)
        list_copy_in(&full_list[j], additions);
}

/** \brief
 *  Calculates total distance the path has between given points, if cap is set it stop at given cap and returns
 *  index at which it stopped (final distance).
 *
 * \param coords int16_t*             path
 * \param start_index int32_t               start point of the path to be considered
 * \param end_index int32_t                 end point of the path to be considered
 * \param cap int32_t                       distance at which to stop
 * \param final_index int32_t*              index of point where it reached the distance cap
 * \return int32_t                          reached distance
 */
int32_t build_get_distance(int16_t *coords, int32_t start_index, int32_t end_index, int32_t cap, int32_t *final_index)
{
    int32_t j, distance = 0;
    int32_t index = start_index;

    if (start_index > end_index)
    {
        index = start_index;
        for (j = start_index; j > end_index; j--)
        {
            if (cap != -1)
                if (distance >= cap)
                    break;
            int16_t x1 = coords[j * 3 - 3];
            int16_t y1 = coords[j * 3 - 2];
            int16_t z1 = coords[j * 3 - 1];
            int16_t x2 = coords[j * 3 + 0];
            int16_t y2 = coords[j * 3 + 1];
            int16_t z2 = coords[j * 3 + 2];
            distance += point_distance_3D(x1, x2, y1, y2, z1, z2);
            index = j;
        }
        if (distance < cap)
            index--;
    }
    if (start_index < end_index)
    {
        index = start_index;
        for (j = start_index; j < end_index; j++)
        {
            if (cap != -1)
                if (distance >= cap)
                    break;
            int16_t x1 = coords[j * 3 + 0];
            int16_t y1 = coords[j * 3 + 1];
            int16_t z1 = coords[j * 3 + 2];
            int16_t x2 = coords[j * 3 + 3];
            int16_t y2 = coords[j * 3 + 4];
            int16_t z2 = coords[j * 3 + 5];
            distance += point_distance_3D(x1, x2, y1, y2, z1, z2);
            index = j;
        }
        if (distance < cap)
            index++;
    }

    if (final_index != NULL)
        *final_index = index;
    return distance;
}

// recalculated distance cap for backwards loading, its fixed point real number cuz config is an int32_t array and im not passing more arguments thru this hell
int32_t build_dist_w_penalty(int32_t distance, int32_t backwards_penalty)
{
    return ((int32_t)((1.0 - ((double)backwards_penalty) / PENALTY_MULT_CONSTANT) * distance));
}

// checks whether a cam path is backwards relative to another cam path
int32_t build_is_before(ENTRY *elist, int32_t zone_index, int32_t camera_index, int32_t neighbour_index, int32_t neighbour_cam_index)
{
    int32_t distance_neighbour = elist[neighbour_index].distances[neighbour_cam_index];
    int32_t distance_current = elist[zone_index].distances[camera_index];

    return (distance_neighbour < distance_current);
}

/** \brief
 *  Collects list of IDs using draw lists of neighbouring camera paths, depth max 2, distance less than DRAW_DISTANCE-
 *  Another function handles retrieving entry/subtype list from the ID list.
 *
 * \param point_index int32_t               index of the point currently considered
 * \param zone_index int32_t                index of the entry in the entry list
 * \param camera_index int32_t              index of the camera item currently considered
 * \param cam_length int32_t                length of the camera
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param neighbours LIST*              list of entries it stumbled upon (used later to find types/subtypes of collected IDs)
 * \return LIST                         list of IDs it came across during the search
 */
LIST build_get_entity_list(int32_t point_index, int32_t zone_index, int32_t camera_index, int32_t cam_length, ENTRY *elist, int32_t entry_count, LIST *neighbours, int32_t *config)
{
    int32_t draw_dist = config[CNFG_IDX_LL_DRAW_DIST_VALUE];
    int32_t preloading_flag = config[CNFG_IDX_LL_TRNS_PRLD_FLAG];
    int32_t backwards_penalty = config[CNFG_IDX_LL_BACKWARDS_PENALTY];

    LIST entity_list = init_list();
    int32_t i, j, k, coord_count;

    list_copy_in(neighbours, build_get_neighbours(elist[zone_index].data));

    LIST *draw_list_zone = build_get_complete_draw_list(elist, zone_index, camera_index, cam_length);
    for (i = 0; i < cam_length; i++)
        list_copy_in(&entity_list, draw_list_zone[i]);

    /*
    char temp[6] = "";
    printf("\nZone %5s complete draw list:\n", eid_conv(elist[zone_index].eid, temp));
    for (j = 0; j < cam_length; j++) {
        printf("\n%d:\t", j);
        for (k = 0; k < draw_list_zone[j].count; k++)
            printf("%d\t", draw_list_zone[j].eids[k]);
    }
    printf("\n\n");*/

    int16_t *coords = build_get_path(elist, zone_index, camera_index, &coord_count);
    LIST links = build_get_links(elist[zone_index].data, camera_index);
    for (i = 0; i < links.count; i++)
    {
        int32_t distance = 0;
        CAMERA_LINK link = int_to_link(links.eids[i]);
        if (link.type == 1)
            distance += build_get_distance(coords, point_index, 0, -1, NULL);
        if (link.type == 2)
            distance += build_get_distance(coords, point_index, cam_length - 1, -1, NULL);

        uint32_t eid_offset = from_u32(elist[zone_index].data + 0x10) + 4 + link.zone_index * 4 + C2_NEIGHBOURS_START;
        uint32_t neighbour_eid = from_u32(elist[zone_index].data + eid_offset);
        uint32_t neighbour_flg = from_u32(elist[zone_index].data + eid_offset + 0x20);

        int32_t neighbour_index = build_get_index(neighbour_eid, elist, entry_count);
        if (neighbour_index == -1)
            continue;

        // preloading everything check
        if (preloading_flag != PRELOADING_ALL && (neighbour_flg == 0xF || neighbour_flg == 0x1F))
            continue;

        list_copy_in(neighbours, build_get_neighbours(elist[neighbour_index].data));

        int32_t neighbour_cam_length;
        int16_t *coords2 = build_get_path(elist, neighbour_index, 2 + 3 * link.cam_index, &neighbour_cam_length);
        LIST *draw_list_neighbour1 = build_get_complete_draw_list(elist, neighbour_index, 2 + 3 * link.cam_index, neighbour_cam_length);

        int32_t draw_dist_w_orientation = draw_dist;
        if (build_is_before(elist, zone_index, camera_index / 3, neighbour_index, link.cam_index))
        {
            /*char temp[100], temp2[100];
            printf("entity list %s is before %s\n", eid_conv(elist[neighbour_index].eid, temp), eid_conv(elist[zone_index].eid, temp2));*/
            draw_dist_w_orientation = build_dist_w_penalty(draw_dist, backwards_penalty);
        }

        int32_t point_index2;
        if (link.flag == 1)
        {
            distance += build_get_distance(coords2, 0, neighbour_cam_length - 1, draw_dist_w_orientation - distance, &point_index2);
            for (j = 0; j < point_index2; j++)
                list_copy_in(&entity_list, draw_list_neighbour1[j]);
        }
        if (link.flag == 2)
        {
            distance += build_get_distance(coords2, neighbour_cam_length - 1, 0, draw_dist_w_orientation - distance, &point_index2);
            for (j = point_index2; j < neighbour_cam_length - 1; j++)
                list_copy_in(&entity_list, draw_list_neighbour1[j]);
        }

        if (distance >= draw_dist_w_orientation)
            continue;

        LIST layer2 = build_get_links(elist[neighbour_index].data, 2 + 3 * link.cam_index);
        for (j = 0; j < layer2.count; j++)
        {
            CAMERA_LINK link2 = int_to_link(layer2.eids[j]);
            uint32_t eid_offset2 = from_u32(elist[neighbour_index].data + 0x10) + 4 + link2.zone_index * 4 + C2_NEIGHBOURS_START;
            uint32_t neighbour_eid2 = from_u32(elist[neighbour_index].data + eid_offset2);
            uint32_t neighbour_flg2 = from_u32(elist[neighbour_index].data + eid_offset2 + 0x20);

            int32_t neighbour_index2 = build_get_index(neighbour_eid2, elist, entry_count);
            if (neighbour_index2 == -1)
                continue;

            // preloading everything check
            if (preloading_flag != PRELOADING_ALL && (neighbour_flg2 == 0xF || neighbour_flg2 == 0x1F))
                continue;

            list_copy_in(neighbours, build_get_neighbours(elist[neighbour_index2].data));

            int32_t neighbour_cam_length2;
            int16_t *coords3 = build_get_path(elist, neighbour_index2, 2 + 3 * link2.cam_index, &neighbour_cam_length2);
            LIST *draw_list_neighbour2 = build_get_complete_draw_list(elist, neighbour_index2, 2 + 3 * link2.cam_index, neighbour_cam_length2);

            int32_t point_index3;

            draw_dist_w_orientation = draw_dist;
            if (build_is_before(elist, zone_index, camera_index / 3, neighbour_index2, link2.cam_index))
            {
                /*char temp[100], temp2[100];
                printf("entity list %s is before %s\n", eid_conv(elist[neighbour_index2].eid, temp), eid_conv(elist[zone_index].eid, temp2));*/
                draw_dist_w_orientation = build_dist_w_penalty(draw_dist, backwards_penalty);
            }

            // start to end
            if ((link.type == 2 && link2.type == 2 && link2.flag == 1) ||
                (link.type == 1 && link2.type == 1 && link2.flag == 1))
            {
                build_get_distance(coords3, 0, neighbour_cam_length2 - 1, draw_dist_w_orientation - distance, &point_index3);
                for (k = 0; k < point_index3; k++)
                    list_copy_in(&entity_list, draw_list_neighbour2[k]);
            }

            // end to start
            if ((link.type == 2 && link2.type == 2 && link2.flag == 1) ||
                (link.type == 1 && link2.type == 1 && link2.flag == 2))
            {
                build_get_distance(coords3, neighbour_cam_length2 - 1, 0, draw_dist_w_orientation - distance, &point_index3);
                for (k = point_index3; k < neighbour_cam_length2; k++)
                    list_copy_in(&entity_list, draw_list_neighbour2[k]);
            }
        }
    }

    return entity_list;
}

/** \brief
 *  Retrieves a list of types & subtypes from the IDs and zones.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param entity_list LIST              list of entity IDs
 * \param neighbour_list LIST           list of entries to look into
 * \return LIST                         list of [2B] type & [2B] subtype items
 */
LIST build_get_types_subtypes(ENTRY *elist, int32_t entry_count, LIST entity_list, LIST neighbour_list)
{
    LIST type_subtype_list = init_list();
    int32_t i, j;
    for (i = 0; i < neighbour_list.count; i++)
    {
        int32_t curr_index = build_get_index(neighbour_list.eids[i], elist, entry_count);
        if (curr_index == -1)
            continue;

        int32_t cam_count = build_get_cam_item_count(elist[curr_index].data);
        int32_t entity_count = build_get_entity_count(elist[curr_index].data);

        for (j = 0; j < entity_count; j++)
        {
            int32_t entity_offset = build_get_nth_item_offset(elist[curr_index].data, 2 + cam_count + j);
            int32_t ID = build_get_entity_prop(elist[curr_index].data + entity_offset, ENTITY_PROP_ID);
            if (list_find(entity_list, ID) != -1)
            {
                int32_t type = build_get_entity_prop(elist[curr_index].data + entity_offset, ENTITY_PROP_TYPE);
                int32_t subtype = build_get_entity_prop(elist[curr_index].data + entity_offset, ENTITY_PROP_SUBTYPE);
                list_add(&type_subtype_list, (type << 16) + subtype);
            }
        }
    }

    return type_subtype_list;
}

/** \brief
 *  Lets the user know whether there are any entities whose type/subtype has no specified dependencies.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param sub_info DEPENDENCIES         list of types and subtypes and their potential dependencies
 * \return void
 */
void build_find_unspecified_entities(ENTRY *elist, int32_t entry_count, DEPENDENCIES sub_info)
{
    printf("\n");

    char temp[100] = "";
    int32_t i, j, k;
    LIST considered = init_list();
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            int32_t cam_count = build_get_cam_item_count(elist[i].data);
            int32_t entity_count = build_get_entity_count(elist[i].data);
            for (j = 0; j < entity_count; j++)
            {
                uint8_t *entity = elist[i].data + from_u32(elist[i].data + 0x10 + (2 + cam_count + j) * 4);
                int32_t type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
                int32_t subt = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);
                int32_t enID = build_get_entity_prop(entity, ENTITY_PROP_ID);
                if (type >= 64 || type < 0 || subt < 0)
                {
                    printf("[warning] Zone %s entity %2d is invalid! (type %2d subtype %2d)\n", eid_conv(elist[i].eid, temp), j, type, subt);
                    continue;
                }

                if (list_find(considered, (type << 16) + subt) != -1)
                    continue;

                list_add(&considered, (type << 16) + subt);

                int32_t found = 0;
                for (k = 0; k < sub_info.count; k++)
                    if (sub_info.array[k].subtype == subt && sub_info.array[k].type == type)
                        found = 1;
                if (!found)
                    printf("[warning] Entity with type %2d subtype %2d has no dependency list! (e.g. Zone %s entity %2d ID %3d)\n",
                           type, subt, eid_conv(elist[i].eid, temp), j, enID);
            }
        }
}

/** \brief
 *  Reads draw lists of the camera of the zone, returns in a non-delta form.
 *
 * \param elist ENTRY*                  entry list
 * \param zone_index int32_t                index of the zone entry
 * \param cam_index int32_t                 index of the camera item
 * \param cam_length int32_t                length of the path of the camera item
 * \return LIST*                        array of lists as a representation of the draw list (changed to full draw list instead of delta-based)
 */
LIST *build_get_complete_draw_list(ENTRY *elist, int32_t zone_index, int32_t cam_index, int32_t cam_length)
{
    int32_t i, m;
    LIST *draw_list = (LIST *)malloc(cam_length * sizeof(LIST));
    LIST list = init_list();
    for (i = 0; i < cam_length; i++)
        draw_list[i] = init_list();

    LOAD_LIST draw_list2 = build_get_draw_lists(elist[zone_index].data, cam_index);

    int32_t sublist_index = 0;
    for (i = 0; i < cam_length && sublist_index < draw_list2.count; i++)
    {

        int32_t new_read = 0;
        if (draw_list2.array[sublist_index].index == i)
        {
            if (draw_list2.array[sublist_index].type == 'B')
                for (m = 0; m < draw_list2.array[sublist_index].list_length; m++)
                {
                    DRAW_ITEM draw_item = build_decode_draw_item(draw_list2.array[sublist_index].list[m]);
                    list_add(&list, draw_item.ID);
                }

            if (draw_list2.array[sublist_index].type == 'A')
                for (m = 0; m < draw_list2.array[sublist_index].list_length; m++)
                {
                    DRAW_ITEM draw_item = build_decode_draw_item(draw_list2.array[sublist_index].list[m]);
                    list_remove(&list, draw_item.ID);
                }

            sublist_index++;
            new_read = 1;
        }
        list_copy_in(&draw_list[i], list);

        // fixes cases when both draw list A and B contained sublist w/ the same indexes, subsequent sublists didnt get
        if (new_read)
            i--;
    }

    delete_load_list(draw_list2);
    return draw_list;
}

/** \brief
 *  Searches the entry and for each collision type it adds its dependencies (if there are any)
 *  to the list, copies it into items start_index to end_index (exc.).
 *
 * \param full_list LIST*               current load list
 * \param start_index int32_t               camera point index from which the additions are copied in
 * \param end_index int32_t                 camera point index until which the additions are copied in
 * \param entry uint8_t*          entry in which it searches collision types
 * \param collisions DEPENDENCIES       list that contains list of entries tied to a specific collision type
 * \param elist ENTRY*                  entry_list
 * \param entry_count int32_t               entry_count
 * \return void
 */
void build_add_collision_dependencies(LIST *full_list, int32_t start_index, int32_t end_index, uint8_t *entry, DEPENDENCIES collisions, ENTRY *elist, int32_t entry_count)
{
    int32_t x, i, j, k;
    LIST neighbours = build_get_neighbours(entry);

    for (x = 0; x < neighbours.count; x++)
    {
        int32_t index = build_get_index(neighbours.eids[x], elist, entry_count);
        if (index == -1)
            continue;

        int32_t item2off = from_u32(elist[index].data + 0x14);
        uint8_t *item = elist[index].data + item2off;
        int32_t count = from_u32(item + 0x18);

        for (i = 0; i < count + 2; i++)
        {
            int16_t type = from_u16(item + 0x24 + 2 * i);
            if (type % 2 == 0)
                continue;

            for (j = 0; j < collisions.count; j++)
                if (type == collisions.array[j].type)
                    for (k = start_index; k < end_index; k++)
                        list_copy_in(&full_list[k], collisions.array[j].dependencies);
        }
    }
}

/** \brief
 *  Checks for whether the current camera isnt trying to load more than 8 textures, if it does it lets the user know.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param full_load LIST*               full load list
 * \param cam_length int32_t                length of the current camera
 * \param i int32_t                         index of the current entry
 * \param j int32_t                         current camera path
 * \return void
 */
void build_texture_count_check(ENTRY *elist, int32_t entry_count, LIST *full_load, int32_t cam_length, int32_t i, int32_t j)
{
    char temp[100] = "";
    int32_t k, l;
    int32_t over_count = 0;
    uint32_t over_textures[20];
    int32_t point = 0;

    for (k = 0; k < cam_length; k++)
    {
        int32_t texture_count = 0;
        uint32_t textures[20];
        for (l = 0; l < full_load[k].count; l++)
        {
            int32_t idx = build_get_index(full_load[k].eids[l], elist, entry_count);
            if (idx == -1)
            {
                printf("Trying to load invalid entry %s\n", eid_conv(full_load[k].eids[l], temp));
            }
            else if (build_entry_type(elist[idx]) == -1 && eid_conv(full_load[k].eids[l], temp)[4] == 'T')
                textures[texture_count++] = full_load[k].eids[l];
        }

        if (texture_count > over_count)
        {
            over_count = texture_count;
            point = k;
            memcpy(over_textures, textures, texture_count * sizeof(uint32_t));
        }
    }

    if (over_count > 8)
    {
        printf("[warning] Zone %s cam path %d trying to load %d textures! (eg on point %d)\n", eid_conv(elist[i].eid, temp), j, over_count, point);
        for (k = 0; k < over_count; k++)
            printf("\t%s", eid_conv(over_textures[k], temp));
        printf("\n");
    }
}

/** \brief
 *  Gets indexes of camera linked neighbours specified in the camera link property.
 *
 * \param entry uint8_t*          entry data
 * \param cam_index int32_t                 index of the camera item
 * \return void
 */
LIST build_get_links(uint8_t *entry, int32_t cam_index)
{
    int32_t i, k, l, link_count = 0;
    int32_t *links = NULL;
    int32_t cam_offset = build_get_nth_item_offset(entry, cam_index);

    for (k = 0; (unsigned)k < build_prop_count(entry + cam_offset); k++)
    {
        int32_t code = from_u16(entry + cam_offset + 0x10 + 8 * k);
        int32_t offset = from_u16(entry + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
        int32_t prop_len = from_u16(entry + cam_offset + 0x16 + 8 * k);

        if (code == ENTITY_PROP_CAM_PATH_LINKS)
        {
            if (prop_len == 1)
            {
                link_count = from_u16(entry + offset);
                links = (int32_t *)malloc(link_count * sizeof(int32_t));
                for (l = 0; l < link_count; l++)
                    links[l] = from_u32(entry + offset + 4 + 4 * l);
            }
            else
            {
                link_count = max(1, from_u16(entry + offset)) + max(1, from_u16(entry + offset + 2));
                links = (int32_t *)malloc(link_count * sizeof(int32_t));
                for (l = 0; l < link_count; l++)
                    links[l] = from_u32(entry + offset + 0x8 + 4 * l);
            }
        }
    }

    LIST links_list = init_list();
    for (i = 0; i < link_count; i++)
        list_add(&links_list, links[i]);

    return links_list;
}

// replaces nth item in a zone with an item with specified data and length
void build_replace_item(ENTRY *zone, int32_t item_index, uint8_t *new_item, int32_t item_size)
{
    int32_t i, offset;
    int32_t item_count = build_item_count(zone->data);
    int32_t first_item_offset = 0x14 + 4 * item_count;

    int32_t *item_lengths = (int32_t *)malloc(item_count * sizeof(int32_t));
    uint8_t **items = (uint8_t **)malloc(item_count * sizeof(uint8_t **));
    for (i = 0; i < item_count; i++)
        item_lengths[i] = build_get_nth_item_offset(zone->data, i + 1) - build_get_nth_item_offset(zone->data, i);

    for (offset = first_item_offset, i = 0; i < item_count; offset += item_lengths[i], i++)
    {
        items[i] = (uint8_t *)malloc(item_lengths[i]);
        memcpy(items[i], zone->data + offset, item_lengths[i]);
    }

    item_lengths[item_index] = item_size;
    items[item_index] = new_item;

    int32_t new_size = first_item_offset;
    for (i = 0; i < item_count; i++)
        new_size += item_lengths[i];

    uint8_t *new_data = (uint8_t *)malloc(new_size);
    *(int32_t *)(new_data) = MAGIC_ENTRY;
    *(int32_t *)(new_data + 0x4) = zone->eid;
    *(int32_t *)(new_data + 0x8) = ENTRY_TYPE_ZONE;
    *(int32_t *)(new_data + 0xC) = item_count;

    for (offset = first_item_offset, i = 0; i < item_count + 1; offset += item_lengths[i], i++)
        *(int32_t *)(new_data + 0x10 + i * 4) = offset;

    for (offset = first_item_offset, i = 0; i < item_count; offset += item_lengths[i], i++)
        memcpy(new_data + offset, items[i], item_lengths[i]);

    free(zone->data);
    zone->data = new_data;
    zone->esize = new_size;

    for (i = 0; i < item_count; i++)
        free(items[i]);
    free(items);
    free(item_lengths);
}

/** \brief
 *  Deconstructs the zone, alters specified item using the func_arg function, reconstructs the zone.
 *
 * \param zone ENTRY*                   zone data
 * \param item_index int32_t                index of the item/entity to be altered
 * \param func_arg uint8_t*       function to be used, either remove or add property
 * \param list LIST*                    list to be added (might be unused)
 * \param property_code int32_t             property code
 * \return void
 */
void build_entity_alter(ENTRY *zone, int32_t item_index, uint8_t *(func_arg)(uint32_t, uint8_t *, int32_t *, PROPERTY *), int32_t property_code, PROPERTY *prop)
{
    int32_t i, offset;
    int32_t item_count = build_item_count(zone->data);
    int32_t first_item_offset = 0x14 + 4 * item_count;

    int32_t *item_lengths = (int32_t *)malloc(item_count * sizeof(int32_t));
    uint8_t **items = (uint8_t **)malloc(item_count * sizeof(uint8_t **));
    for (i = 0; i < item_count; i++)
        item_lengths[i] = build_get_nth_item_offset(zone->data, i + 1) - build_get_nth_item_offset(zone->data, i);

    for (offset = first_item_offset, i = 0; i < item_count; offset += item_lengths[i], i++)
    {
        items[i] = (uint8_t *)malloc(item_lengths[i]);
        memcpy(items[i], zone->data + offset, item_lengths[i]);
    }

    items[item_index] = func_arg(property_code, items[item_index], &item_lengths[item_index], prop);

    int32_t new_size = first_item_offset;
    for (i = 0; i < item_count; i++)
        new_size += item_lengths[i];

    uint8_t *new_data = (uint8_t *)malloc(new_size);
    *(int32_t *)(new_data) = MAGIC_ENTRY;
    *(int32_t *)(new_data + 0x4) = zone->eid;
    *(int32_t *)(new_data + 0x8) = ENTRY_TYPE_ZONE;
    *(int32_t *)(new_data + 0xC) = item_count;

    for (offset = first_item_offset, i = 0; i < item_count + 1; offset += item_lengths[i], i++)
        *(int32_t *)(new_data + 0x10 + i * 4) = offset;

    for (offset = first_item_offset, i = 0; i < item_count; offset += item_lengths[i], i++)
        memcpy(new_data + offset, items[i], item_lengths[i]);

    free(zone->data);
    zone->data = new_data;
    zone->esize = new_size;

    for (i = 0; i < item_count; i++)
        free(items[i]);
    free(items);
    free(item_lengths);
}

/** \brief
 *  Creates a load list and inserts it basically.
 *
 * \param code uint32_t             code of the property to be added
 * \param item uint8_t*           data of item where the property will be added
 * \param item_size int32_t*                item size
 * \param list LIST *                   load list to be added
 * \return uint8_t*               new item data
 */
uint8_t *build_add_property(uint32_t code, uint8_t *item, int32_t *item_size, PROPERTY *prop)
{
    int32_t offset, i, property_count = build_prop_count(item);

    int32_t *property_sizes = (int32_t *)malloc((property_count + 1) * sizeof(int32_t));
    uint8_t **properties = (uint8_t **)malloc((property_count + 1) * sizeof(uint8_t *));
    uint8_t **property_headers = (uint8_t **)malloc((property_count + 1) * sizeof(uint8_t *));

    for (i = 0; i < property_count + 1; i++)
    {
        property_headers[i] = (uint8_t *)malloc(8 * sizeof(uint8_t));
        for (int32_t j = 0; j < 8; j++)
            property_headers[i][j] = 0;
    }

    for (i = 0; i < property_count; i++)
    {
        if (from_u16(item + 0x10 + 8 * i) > code)
            memcpy(property_headers[i + 1], item + 0x10 + 8 * i, 8);
        else
            memcpy(property_headers[i], item + 0x10 + 8 * i, 8);
    }

    int32_t insertion_index = 0;
    for (i = 0; i < property_count + 1; i++)
        if (from_u32(property_headers[i]) == 0 && from_u32(property_headers[i] + 4) == 0)
            insertion_index = i;

    if (insertion_index != property_count)
    {
        for (i = 0; i < property_count + 1; i++)
        {
            property_sizes[i] = 0;
            if (i < insertion_index - 1)
                property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
            if (i == insertion_index - 1)
                property_sizes[i] = from_u16(property_headers[i + 2] + 2) - from_u16(property_headers[i] + 2);
            // if (i == insertion_index) {}
            if (i > insertion_index)
            {
                if (i == property_count)
                    property_sizes[i] = from_u16(item) - from_u16(property_headers[i] + 2);
                else
                    property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
            }
        }
    }
    else
        for (i = 0; i < property_count; i++)
        {
            if (i != property_count - 1)
                property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
            else
                property_sizes[i] = from_u16(item) - OFFSET - from_u16(property_headers[i] + 2);
        }

    offset = 0x10 + 8 * property_count;
    for (i = 0; i < property_count + 1; i++)
    {
        if (i == insertion_index)
            continue;

        properties[i] = (uint8_t *)malloc(property_sizes[i]);
        memcpy(properties[i], item + offset, property_sizes[i]);
        offset += property_sizes[i];
    }

    property_sizes[insertion_index] = prop->length;
    memcpy(property_headers[insertion_index], prop->header, 8);
    properties[insertion_index] = prop->data;

    int32_t new_size = 0x10 + 8 * (property_count + 1);
    for (i = 0; i < property_count + 1; i++)
        new_size += property_sizes[i];
    if (insertion_index == property_count)
        new_size += OFFSET;

    uint8_t *new_item = (uint8_t *)malloc(new_size);
    *(int32_t *)(new_item) = new_size - OFFSET;

    *(int32_t *)(new_item + 0x4) = 0;
    *(int32_t *)(new_item + 0x8) = 0;
    *(int32_t *)(new_item + 0xC) = property_count + 1;

    offset = 0x10 + 8 * (property_count + 1);
    for (i = 0; i < property_count + 1; i++)
    {
        *(int16_t *)(property_headers[i] + 2) = offset - OFFSET;
        memcpy(new_item + 0x10 + 8 * i, property_headers[i], 8);
        memcpy(new_item + offset, properties[i], property_sizes[i]);
        offset += property_sizes[i];
    }

    *item_size = new_size - OFFSET;
    free(item);
    for (i = 0; i < property_count + 1; i++)
    {
        free(properties[i]);
        free(property_headers[i]);
    }
    free(properties);
    free(property_headers);
    free(property_sizes);
    return new_item;
}

/** \brief
 *  Removes the specified property.
 *
 * \param code uint32_t             code of the property to be removed
 * \param item uint8_t*           data of the item thats to be changed
 * \param item_size int32_t*                size of the item
 * \param prop PROPERTY                 unused
 * \return uint8_t*               new item data
 */
uint8_t *build_rem_property(uint32_t code, uint8_t *item, int32_t *item_size, PROPERTY *prop)
{
    int32_t offset, i, property_count = build_prop_count(item);

    int32_t *property_sizes = (int32_t *)malloc(property_count * sizeof(int32_t));
    uint8_t **properties = (uint8_t **)malloc(property_count * sizeof(uint8_t *));
    uint8_t **property_headers = (uint8_t **)malloc(property_count * sizeof(uint8_t *));

    int32_t found = 0;
    for (i = 0; i < property_count; i++)
    {
        property_headers[i] = (uint8_t *)malloc(8 * sizeof(uint8_t *));
        memcpy(property_headers[i], item + 0x10 + 8 * i, 8);
        if (from_u16(property_headers[i]) == code)
            found++;
    }

    if (!found)
        return item;

    for (i = 0; i < property_count - 1; i++)
        property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
    property_sizes[i] = from_u16(item) - OFFSET - from_u16(property_headers[i] + 2);

    offset = 0x10 + 8 * property_count;
    for (i = 0; i < property_count; offset += property_sizes[i], i++)
    {
        properties[i] = (uint8_t *)malloc(property_sizes[i]);
        memcpy(properties[i], item + offset, property_sizes[i]);
    }

    int32_t new_size = 0x10 + 8 * (property_count - 1);
    for (i = 0; i < property_count; i++)
    {
        if (from_u16(property_headers[i]) == code)
            continue;
        new_size += property_sizes[i];
    }

    uint8_t *new_item = (uint8_t *)malloc(new_size);
    *(int32_t *)(new_item) = new_size;
    *(int32_t *)(new_item + 4) = 0;
    *(int32_t *)(new_item + 8) = 0;
    *(int32_t *)(new_item + 0xC) = property_count - 1;

    offset = 0x10 + 8 * (property_count - 1);
    int32_t indexer = 0;
    for (i = 0; i < property_count; i++)
    {
        if (from_u16(property_headers[i]) != code)
        {
            *(int16_t *)(property_headers[i] + 2) = offset - OFFSET;
            memcpy(new_item + 0x10 + 8 * indexer, property_headers[i], 8);
            memcpy(new_item + offset, properties[i], property_sizes[i]);
            indexer++;
            offset += property_sizes[i];
        }
    }

    *item_size = new_size;
    for (i = 0; i < property_count; i++)
    {
        free(properties[i]);
        free(property_headers[i]);
    }
    free(properties);
    free(property_headers);
    free(property_sizes);
    free(item);
    return new_item;
}

// removes nth item from an entry
void build_remove_nth_item(ENTRY *entry, int32_t n)
{
    int32_t item_count = build_item_count(entry->data);
    if (item_count < n)
    {
        printf("[Warning] Trying to remove item %d from entry with only %d items!\n", n, build_item_count(entry->data));
        return;
    }

    int32_t i, offset;
    int32_t first_item_offset = 0x14 + 4 * item_count;

    int32_t *item_lengths = (int32_t *)malloc(item_count * sizeof(int32_t));
    uint8_t **items = (uint8_t **)malloc(item_count * sizeof(uint8_t **));
    for (i = 0; i < item_count; i++)
    {
        int32_t next_start = build_get_nth_item_offset(entry->data, i + 1);
        if (i == n)
            next_start = entry->esize;

        item_lengths[i] = next_start - build_get_nth_item_offset(entry->data, i);
    }

    for (offset = first_item_offset, i = 0; i < item_count; offset += item_lengths[i], i++)
    {
        items[i] = (uint8_t *)malloc(item_lengths[i]);
        memcpy(items[i], entry->data + offset, item_lengths[i]);
    }

    item_lengths[n] = 0;
    first_item_offset -= 4;

    int32_t new_size = first_item_offset;
    for (i = 0; i < item_count; i++)
        new_size += item_lengths[i];

    uint8_t *new_data = (uint8_t *)malloc(new_size);
    *(int32_t *)(new_data) = MAGIC_ENTRY;
    *(int32_t *)(new_data + 0x4) = entry->eid;
    *(int32_t *)(new_data + 0x8) = *(int32_t *)(entry->data + 8);
    *(int32_t *)(new_data + 0xC) = item_count - 1;

    for (offset = first_item_offset, i = 0; i < item_count + 1; offset += item_lengths[i], i++)
    {
        if (i == n)
            continue;

        int32_t curr = i;
        if (i > n)
            curr--;
        *(int32_t *)(new_data + 0x10 + curr * 4) = offset;
    }

    for (offset = first_item_offset, i = 0; i < item_count; offset += item_lengths[i], i++)
    {
        if (i == n)
            continue;
        memcpy(new_data + offset, items[i], item_lengths[i]);
    }

    free(entry->data);
    entry->data = new_data;
    entry->esize = new_size;

    for (i = 0; i < item_count; i++)
        free(items[i]);
    free(items);
    free(item_lengths);
}

/** \brief
 *  Gets texture references from a model and adds them to the list.
 *
 * \param list LIST*                        list to be added to
 * \param model uint8_t*              model data
 * \return void
 */
void build_add_model_textures_to_list(uint8_t *model, LIST *list)
{
    int32_t item1off = from_u32(model + 0x10);
    int32_t tex_count = from_u32(model + item1off + 0x40);

    for (int32_t i = 0; i < tex_count; i++)
    {
        uint32_t scen_reference = from_u32(model + item1off + 0xC + 0x4 * i);
        if (scen_reference)
            list_add(list, scen_reference);
    }
}

/** \brief
 *  Gets texture references from a scenery entry and inserts them to the list.
 *
 * \param scenery uint8_t*            scenery data
 * \param list LIST*                        list to be added to
 * \return void
 */
void build_add_scen_textures_to_list(uint8_t *scenery, LIST *list)
{
    int32_t item1off = from_u32(scenery + 0x10);
    int32_t texture_count = from_u32(scenery + item1off + 0x28);
    for (int32_t i = 0; i < texture_count; i++)
    {
        uint32_t eid = from_u32(scenery + item1off + 0x2C + 4 * i);
        list_add(list, eid);
    }
}

/** \brief
 *  Returns path the entity has & its length.
 *
 * \param elist ENTRY*                  entry list
 * \param zone_index int32_t                index of entry in the list
 * \param item_index int32_t                index of item of the entry whose path it will return
 * \param path_len int32_t*                 path length return value
 * \return int16_t*                   path
 */
int16_t *build_get_path(ENTRY *elist, int32_t zone_index, int32_t item_index, int32_t *path_len)
{
    uint8_t *item = build_get_nth_item(elist[zone_index].data, item_index);
    int32_t offset = build_get_prop_offset(item, ENTITY_PROP_PATH);

    if (offset)
        *path_len = from_u32(item + offset);
    else
    {
        *path_len = 0;
        return NULL;
    }

    int16_t *coords = (int16_t *)malloc(3 * sizeof(int16_t) * *path_len);
    memcpy(coords, item + offset + 4, 6 * (*path_len));
    return coords;
}

/** \brief
 *  Gets zone's scenery reference count.
 *
 * \param entry uint8_t*          entry data
 * \return int32_t                          scenery reference count
 */
int32_t build_get_scen_count(uint8_t *entry)
{
    int32_t item1off = build_get_nth_item_offset(entry, 0);
    return entry[item1off];
}

// get scenery list from entry data
LIST build_get_sceneries(uint8_t *entry)
{
    int32_t scen_count = build_get_scen_count(entry);
    int32_t item1off = build_get_nth_item_offset(entry, 0);
    LIST list = init_list();
    for (int32_t i = 0; i < scen_count; i++)
    {
        uint32_t scen = from_u32(entry + item1off + 0x4 + 0x30 * i);
        list_add(&list, scen);
    }

    return list;
}
