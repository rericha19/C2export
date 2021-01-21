#include "macros.h"
// responsible for generating load lists using already input or here collected data and info

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
 * \param entry_count int               current amount of entries
 * \param gool_table unsigned int*      table that contains GOOL entries on their expected slot
 * \param permaloaded LIST              list of permaloaded entries
 * \param subtype_info DEPENDENCIES     list of {type, subtype, count, dependencies[count]}
 * \return void
 */
void build_remake_load_lists(ENTRY* elist, int entry_count, unsigned int* gool_table, LIST permaloaded, DEPENDENCIES subtype_info, DEPENDENCIES collision, int* config) {

    int load_list_sound_entry_inc_flag = config[9];
    int i, j, k, l;

    // gets a list of sound EIDs (one per chunk) to make load lists smaller
    int chunks[8];
    int sound_chunk_count = 0;
    unsigned int sounds_to_load[8];
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_SOUND) {
            int is_there = 0;
            for (j = 0; j < sound_chunk_count; j++)
                if (chunks[j] == elist[i].chunk) {
                    is_there = 1;
                    break;
                }

            if (!is_there) {
                chunks[sound_chunk_count] = elist[i].chunk;
                sounds_to_load[sound_chunk_count] = elist[i].EID;
                sound_chunk_count++;
            }
        }

    // for each zone entry do load list (also for each zone's each camera)
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE) {
            int cam_count = build_get_cam_item_count(elist[i].data) / 3;
            if (cam_count == 0)
                continue;

            char temp[100] = "";
            printf("Doing load lists for %s\n", eid_conv(elist[i].EID, temp));

            // get list of special entries that can be placed inside zones' first item
            // as a special zone-specific dependency/load list
            LIST special_entries = build_get_special_entries(elist[i], elist, entry_count);

            for (j = 0; j < cam_count; j++) {
                printf("\t cam path %d\n", j);
                int cam_offset = get_nth_item_offset(elist[i].data, 2 + 3 * j);
                int cam_length = build_get_path_length(elist[i].data + cam_offset);

                // initialise full non-delta load list used to represent the load list during its building
                LIST* full_load = (LIST*)malloc(cam_length * sizeof(LIST));
                for (k = 0; k < cam_length; k++)
                    full_load[k] = init_list();

                // add permaloaded entries
                for (k = 0; k < cam_length; k++)
                    list_copy_in(&full_load[k], permaloaded);

                // add current zone's special entries
                for (k = 0; k < cam_length; k++)
                    list_copy_in(&full_load[k], special_entries);

                // add relatives (directly related entries like slst, direct neighbours, scenery)
                // might not be necessary, relative collection was implemented for previous methods and it was easier to keep it
                if (elist[i].related != NULL)
                    for (k = 0; (unsigned)k < elist[i].related[0]; k++)
                        for (l = 0; l < cam_length; l++)
                            list_insert(&full_load[l], elist[i].related[k + 1]);

                // all sounds
                if (load_list_sound_entry_inc_flag == 0) {
                    for (k = 0; k < entry_count; k++)
                        if (build_entry_type(elist[k]) == ENTRY_TYPE_SOUND)
                            for (l = 0; l < cam_length; l++)
                                list_insert(&full_load[l], elist[k].EID);
                }
                // one sound per chunk
                if (load_list_sound_entry_inc_flag == 1) {
                    for (k = 0; k < cam_length; k++)
                        for (l = 0; l < sound_chunk_count; l++)
                            list_insert(&full_load[k], sounds_to_load[l]);
                }

                // add direct neighbours
                LIST neighbours = build_get_neighbours(elist[i].data);
                for (k = 0; k < cam_length; k++)
                    list_copy_in(&full_load[k], neighbours);

                // for each scenery in current zone's scen reference list, add its textures to the load list
                int scenery_count = build_get_scen_count(elist[i].data);
                for (k = 0; k < scenery_count; k++) {
                    int scenery_index = build_elist_get_index(from_u32(elist[i].data + get_nth_item_offset(elist[i].data, 0) + 0x4 + 0x30 * k),
                        elist, entry_count);
                    for (l = 0; l < cam_length; l++)
                        build_add_scen_textures_to_list(elist[scenery_index].data, &full_load[l]);
                }

                // path link, draw list and other bs dependent additional load list improvements
                build_load_list_util(i, 2 + 3 * j, full_load, cam_length, elist, entry_count, subtype_info, collision, config);

                // checks whether the load list doesnt try to load more than 8 textures, prints if yes
                build_texture_count_check(elist, entry_count, full_load, cam_length, i, j);

                // creates and initialises delta representation of the load list
                LIST* listA = (LIST*)malloc(cam_length * sizeof(LIST));
                LIST* listB = (LIST*)malloc(cam_length * sizeof(LIST));
                for (k = 0; k < cam_length; k++) {
                    listA[k] = init_list();
                    listB[k] = init_list();
                }

                // converts full load list to delta based, then creates game-format load list properties based on the delta lists
                build_load_list_to_delta(full_load, listA, listB, cam_length, elist, entry_count);
                PROPERTY prop_0x208 = build_make_load_list_prop(listA, cam_length, 0x208);
                PROPERTY prop_0x209 = build_make_load_list_prop(listB, cam_length, 0x209);

                // removes existing load list properties, inserts newly made ones
                build_entity_alter(&elist[i], 2 + 3 * j, build_rem_property, 0x208, NULL);
                build_entity_alter(&elist[i], 2 + 3 * j, build_rem_property, 0x209, NULL);
                build_entity_alter(&elist[i], 2 + 3 * j, build_add_property, 0x208, &prop_0x208);
                build_entity_alter(&elist[i], 2 + 3 * j, build_add_property, 0x209, &prop_0x209);

                free(full_load);
                free(listA);
                free(listB);
            }
        }
}


/** \brief
 *  Converts the full load list into delta form.
 *
 * \param full_load LIST*               full form of load list
 * \param listA LIST*                   new delta based load list, A side
 * \param listB LIST*                   new delta based load list, B side
 * \param cam_length int                length of the current camera
 * \return void
 */
void build_load_list_to_delta(LIST* full_load, LIST* listA, LIST* listB, int cam_length, ENTRY* elist, int entry_count) {
    int i, j;

    // full item, listA point 0
    for (i = 0; i < full_load[0].count; i++)
        if (build_elist_get_index(full_load[0].eids[i], elist, entry_count) != -1)
            list_insert(&listA[0], full_load[0].eids[i]);

    // full item, listB point n-1
    int n = cam_length - 1;
    for (i = 0; i < full_load[n].count; i++)
        if (build_elist_get_index(full_load[n].eids[i], elist, entry_count) != -1)
            list_insert(&listB[n], full_load[n].eids[i]);

    // creates delta items
    // for each point it takes full load list and for each entry checks whether it has just become loaded/deloaded
    for (i = 1; i < cam_length; i++) {

        for (j = 0; j < full_load[i].count; j++) {
            unsigned int curr_eid = full_load[i].eids[j];
            if (build_elist_get_index(curr_eid, elist, entry_count) == -1) continue;

            // is loaded on i-th point but not on i-1th point -> just became loaded, add to listA[i]
            if (list_find(full_load[i - 1], curr_eid) == -1)
                list_insert(&listA[i], curr_eid);
        }

        for (j = 0; j < full_load[i - 1].count; j++) {
            unsigned int curr_eid = full_load[i - 1].eids[j];
            if (build_elist_get_index(curr_eid, elist, entry_count) == -1) continue;

            // is loaded on i-1th point but not on i-th point -> no longer loaded, add to listB[i - 1]
            if (list_find(full_load[i], curr_eid) == -1)
                list_insert(&listB[i - 1], curr_eid);
        }
    }

    // gets rid of cases when an item is in both listA and listB on the same index (getting loaded and instantly deloaded or vice versa, fucky)
    // if its in both it removes it from both
    for (i = 0; i < cam_length; i++) {
        LIST iter_copy = init_list();
        list_copy_in(&iter_copy, listA[i]);

        for (j = 0; j < iter_copy.count; j++)
            if (list_find(listB[i], iter_copy.eids[j]) != -1) {
                list_remove(&listA[i], iter_copy.eids[j]);
                list_remove(&listB[i], iter_copy.eids[j]);
            }
    }
}


/** \brief
 *  Handles most load list creating, except for permaloaded and sound entries.
 *  First part is used for neighbours & scenery & slsts.
 *  Second part is used for draw lists.
 *
 * \param zone_index int                index of the zone in the entry list
 * \param camera_index int              index of the camera item in the zone
 * \param listA LIST*                   load list A, later transformed into actual load list data etc
 * \param listB LIST*                   load list B, later transformed into actual load list data and placed into the entry
 * \param cam_length int                length of the camera
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \return void
 */
void build_load_list_util(int zone_index, int camera_index, LIST* full_list, int cam_length, ENTRY* elist, int entry_count, DEPENDENCIES sub_info, DEPENDENCIES collisions, int* config) {
    int i, j, k;
    // char temp[100] = "";

    // neighbours, slsts, scenery
    LIST links = build_get_links(elist[zone_index].data, camera_index);
    for (i = 0; i < links.count; i++)
        build_load_list_util_util(zone_index, camera_index, links.eids[i], full_list, cam_length, elist, entry_count, config, collisions);

    // draw lists
    for (i = 0; i < cam_length; i++) {
        LIST neighbour_list = init_list();
        // get a list of entities drawn within set distance of current camera point
        LIST entity_list = build_get_entity_list(i, zone_index, camera_index, cam_length, elist, entry_count, &neighbour_list, config);

        /*printf("%s point %2d:\n", eid_conv(elist[zone_index].EID, temp), i);
        for (j = 0; j < entity_list.count; j++)
            printf("\t%d\n", entity_list.eids[j]);*/

            // get a list of types and subtypes from the entity list
        LIST types_subtypes = build_get_types_subtypes(elist, entry_count, entity_list, neighbour_list);

        // copy in dependency list for each found type/subtype
        for (j = 0; j < types_subtypes.count; j++) {
            int type = types_subtypes.eids[j] >> 0x10;
            int subtype = types_subtypes.eids[j] & 0xFF;

            // printf("%s point %2d to load type %2d subtype %2d stuff\n", eid_conv(elist[zone_index].EID, temp), i, type, subtype);
            for (k = 0; k < sub_info.count; k++)
                if (sub_info.array[k].subtype == subtype && sub_info.array[k].type == type)
                    list_copy_in(&full_list[i], sub_info.array[k].dependencies);
        }
    }
}

/** \brief
 *  Makes a load list property from the input arrays, later its put into the item.
 *
 * \param list_array LIST*              array of lists containing delta load list (one side)
 * \param cam_length int                length of the camera and so the array too
 * \param code int                      property code
 * \return PROPERTY                     property with game's format
 */
PROPERTY build_make_load_list_prop(LIST* list_array, int cam_length, int code) {
    int i, j, delta_counter = 0, total_length = 0;
    PROPERTY prop;

    // count total length and individual deltas
    for (i = 0; i < cam_length; i++)
        if (list_array[i].count != 0) {
            total_length += list_array[i].count * 4;        // space individual load list items of current sublist will take up
            total_length += 4;                              // each load list sublist uses 2 bytes for its length and 2 bytes for its index
            delta_counter++;
        }

    // header info
    *(short int*)(prop.header) = code;
    *(short int*)(prop.header + 4) = 0x0464; // idr why
    *(short int*)(prop.header + 6) = delta_counter;

    prop.length = total_length;
    prop.data = (unsigned char*)malloc(total_length * sizeof(unsigned char));

    int indexer = 0;
    int offset = 4 * delta_counter;
    for (i = 0; i < cam_length; i++)
        if (list_array[i].count != 0) {
            *(short int*)(prop.data + 2 * indexer) = list_array[i].count;             // i-th sublist's length (item count)
            *(short int*)(prop.data + 2 * (indexer + delta_counter)) = i;             // i-th sublist's index
            for (j = 0; j < list_array[i].count; j++)
                *(int*)(prop.data + offset + 4 * j) = list_array[i].eids[j];          // individual items
            offset += list_array[i].count * 4;
            indexer++;
        }

    return prop;
}



/** \brief
 *  Deals with slst and neighbour/scenery references of path linked entries.
 *
 * \param zone_index int                zone entry index
 * \param cam_index int                 camera item index
 * \param link_int unsigned int         link item int
 * \param full_list LIST*               full load list representation
 * \param cam_length int                length of the camera's path
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \return void
 */
void build_load_list_util_util(int zone_index, int cam_index, int link_int, LIST* full_list, int cam_length, ENTRY* elist, int entry_count, int* config, DEPENDENCIES collisisons) {
    int slst_distance = config[3];
    int neig_distance = config[4];
    int preloading_flag = config[6];
    int backwards_penalty = config[7];

    int i, j, item1off = get_nth_item_offset(elist[zone_index].data, 0);
    short int* coords;
    int path_length, distance = 0;

    CAMERA_LINK link = int_to_link(link_int);

    unsigned int neighbour_eid = from_u32(elist[zone_index].data + item1off + C2_NEIGHBOURS_START + 4 + 4 * link.zone_index);
    unsigned int neighbour_flg = from_u32(elist[zone_index].data + item1off + C2_NEIGHBOURS_START + 4 + 4 * link.zone_index + 0x20);

    int neighbour_index = build_elist_get_index(neighbour_eid, elist, entry_count);
    if (neighbour_index == -1)
        return;

    // if not preloading for transitions
    if (preloading_flag == 0 && (neighbour_flg == 0xF || neighbour_flg == 0x1F))
        return;


    int scenery_count = build_get_scen_count(elist[neighbour_index].data);
    for (i = 0; i < scenery_count; i++) {
        int item1off_neigh = from_u32(elist[neighbour_index].data + 0x10);
        int scenery_index = build_elist_get_index(from_u32(elist[neighbour_index].data + item1off_neigh + 0x4 + 0x30 * i), elist, entry_count);
        // kinda sucks but i cbf to make it better rn
        if (link.type == 1) {
            int end_index = (cam_length - 1) / 2 - 1;
            for (j = 0; j < end_index; j++)
                build_add_scen_textures_to_list(elist[scenery_index].data, &full_list[j]);
        }

        if (link.type == 2) {
            int start_index = (cam_length - 1) / 2 + 1;
            for (j = start_index; j < cam_length; j++)
                build_add_scen_textures_to_list(elist[scenery_index].data, &full_list[j]);
        }
    }

    if (elist[neighbour_index].related != NULL)
        for (i = 0; (unsigned)i < elist[neighbour_index].related[0]; i++)
            for (j = 0; j < cam_length; j++)
                list_insert(&full_list[j], elist[neighbour_index].related[i + 1]);

    int neighbour_cam_count = build_get_cam_item_count(elist[neighbour_index].data);
    if (link.cam_index >= neighbour_cam_count) {
        char temp[100] = "";
        char temp2[100] = "";
        printf("Zone %s is linked to zone %s's %d. camera path (indexing from 0) when it only has %d paths",
            eid_conv(elist[zone_index].EID, temp), eid_conv(elist[neighbour_index].EID, temp2),
            link.cam_index, neighbour_cam_count);
        return;
    }

    int offset = get_nth_item_offset(elist[neighbour_index].data, 2 + 3 * link.cam_index);
    unsigned int slst = build_get_slst(elist[neighbour_index].data + offset);
    for (i = 0; i < cam_length; i++)
        list_insert(&full_list[i], slst);

    build_add_collision_dependencies(full_list, 0, cam_length, elist[zone_index].data, collisisons, elist, entry_count);
    build_add_collision_dependencies(full_list, 0, cam_length, elist[neighbour_index].data, collisisons, elist, entry_count);

    coords = build_get_path(elist, neighbour_index, 2 + 3 * link.cam_index, &path_length);
    distance += build_get_distance(coords, 0, path_length - 1, -1, NULL);

    LIST layer2 = build_get_links(elist[neighbour_index].data, 2 + 3 * link.cam_index);
    for (i = 0; i < layer2.count; i++) {
        int item1off2 = from_u32(elist[neighbour_index].data + 0x10);
        CAMERA_LINK link2 = int_to_link(layer2.eids[i]);

        unsigned int neighbour_eid2 = from_u32(elist[neighbour_index].data + item1off2 + C2_NEIGHBOURS_START + 4 + 4 * link2.zone_index);
        unsigned int neighbour_flg2 = from_u32(elist[neighbour_index].data + item1off2 + C2_NEIGHBOURS_START + 4 + 4 * link2.zone_index + 0x20);

        int neighbour_index2 = build_elist_get_index(neighbour_eid2, elist, entry_count);
        if (neighbour_index2 == -1)
            continue;

        if (preloading_flag == 0 && (neighbour_flg2 == 0xF || neighbour_flg2 == 0x1F))
            continue;

        int offset2 = get_nth_item_offset(elist[neighbour_index2].data, 2 + 3 * link2.cam_index);
        unsigned int slst2 = build_get_slst(elist[neighbour_index2].data + offset2);

        LIST neig_list = build_get_neighbours(elist[neighbour_index2].data);
        list_copy_in(&neig_list, build_get_sceneries(elist[neighbour_index2].data));

        LIST slst_list = init_list();
        list_insert(&slst_list, slst2);

        build_add_collision_dependencies(full_list, 0, cam_length, elist[neighbour_index2].data, collisisons, elist, entry_count);
        coords = build_get_path(elist, zone_index, cam_index, &path_length);

        int neighbour_cam_count2 = build_get_cam_item_count(elist[neighbour_index2].data);
        if (link2.cam_index >= neighbour_cam_count2) {
            char temp[100] = "";
            char temp2[100] = "";
            printf("Zone %s is linked to zone %s's %d. camera path (indexing from 0) when it only has %d paths",
                eid_conv(elist[neighbour_index].EID, temp), eid_conv(elist[neighbour_index2].EID, temp2),
                link2.cam_index, neighbour_cam_count2);
            continue;
        }


        int slst_dist_w_orientation = slst_distance;
        int neig_dist_w_orientation = neig_distance;

        if (build_is_before(elist, zone_index, cam_index / 3, neighbour_index2, link2.cam_index)) {
            /*char temp[100], temp2[100];
            printf("util util %s is before %s\n", eid_conv(elist[neighbour_index2].EID, temp), eid_conv(elist[zone_index].EID, temp2));*/
            slst_dist_w_orientation = build_dist_w_penalty(slst_distance, backwards_penalty);
            neig_dist_w_orientation = build_dist_w_penalty(neig_distance, backwards_penalty);
        }


        if ((link.type == 2 && link.flag == 2 && link2.type == 1) || (link.type == 2 && link.flag == 1 && link2.type == 2)) {
            build_load_list_util_util_forw(cam_length, full_list, distance, slst_dist_w_orientation, coords, path_length, slst_list);
            build_load_list_util_util_forw(cam_length, full_list, distance, neig_dist_w_orientation, coords, path_length, neig_list);
        }
        if ((link.type == 1 && link.flag == 2 && link2.type == 1) || (link.type == 1 && link.flag == 1 && link2.type == 2)) {
            build_load_list_util_util_back(cam_length, full_list, distance, slst_dist_w_orientation, coords, path_length, slst_list);
            build_load_list_util_util_back(cam_length, full_list, distance, neig_dist_w_orientation, coords, path_length, neig_list);
        }
    }
}


/** \brief
 *  Starting from the 0th index it finds a path index at which the distance reaches cap distance and loads
 *  the entries specified by the additions list from point 0 to end index.
 *
 * \param cam_length int                length of the considered camera path
 * \param full_list LIST*               load list
 * \param distance int                  distance so far
 * \param final_distance int            distance cap
 * \param coords short int*             path of the camera
 * \param path_length int               length of the camera
 * \param additions LIST                list with entries to add
 * \return void
 */
void build_load_list_util_util_back(int cam_length, LIST* full_list, int distance, int final_distance, short int* coords, int path_length, LIST additions) {
    int end_index = 0;
    int j;

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
 * \param cam_length int                length of the considered camera path
 * \param full_list LIST*               load list
 * \param distance int                  distance so far
 * \param final_distance int            distance cap
 * \param coords short int*             path of the camera
 * \param path_length int               length of the camera
 * \param additions LIST                list with entries to add
 * \return void
 */
void build_load_list_util_util_forw(int cam_length, LIST* full_list, int distance, int final_distance, short int* coords, int path_length, LIST additions) {
    int start_index = cam_length - 1;
    int j;

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
 * \param coords short int*             path
 * \param start_index int               start point of the path to be considered
 * \param end_index int                 end point of the path to be considered
 * \param cap int                       distance at which to stop
 * \param final_index int*              index of point where it reached the distance cap
 * \return int                          reached distance
 */
int build_get_distance(short int* coords, int start_index, int end_index, int cap, int* final_index) {
    int j, distance = 0;
    int index = start_index;

    if (start_index > end_index) {
        index = start_index;
        for (j = start_index; j > end_index; j--) {
            if (cap != -1)
                if (distance >= cap) break;
            short int x1 = coords[j * 3 - 3];
            short int y1 = coords[j * 3 - 2];
            short int z1 = coords[j * 3 - 1];
            short int x2 = coords[j * 3 + 0];
            short int y2 = coords[j * 3 + 1];
            short int z2 = coords[j * 3 + 2];
            distance += point_distance_3D(x1, x2, y1, y2, z1, z2);
            index = j;
        }
        if (distance < cap)
            index--;
    }
    if (start_index < end_index) {
        index = start_index;
        for (j = start_index; j < end_index; j++) {
            if (cap != -1)
                if (distance >= cap) break;
            short int x1 = coords[j * 3 + 0];
            short int y1 = coords[j * 3 + 1];
            short int z1 = coords[j * 3 + 2];
            short int x2 = coords[j * 3 + 3];
            short int y2 = coords[j * 3 + 4];
            short int z2 = coords[j * 3 + 5];
            distance += point_distance_3D(x1, x2, y1, y2, z1, z2);
            index = j;
        }
        if (distance < cap)
            index++;
    }

    if (final_index != NULL) *final_index = index;
    return distance;
}


// recalculated distance cap for backwards loading, its fixed point real number cuz config is an int array and im not passing more arguments thru this hell
int build_dist_w_penalty(int distance, int backwards_penalty) {
    return ((int)((1.0 - ((double)backwards_penalty) / PENALTY_MULT_CONSTANT) * distance));
}

// checks whether a cam path is backwards relative to another cam path
int build_is_before(ENTRY* elist, int zone_index, int camera_index, int neighbour_index, int neighbour_cam_index) {
    int distance_neighbour = elist[neighbour_index].distances[neighbour_cam_index];
    int distance_current = elist[zone_index].distances[camera_index];

    return (distance_neighbour < distance_current);
}


/** \brief
 *  Collects list of IDs using draw lists of neighbouring camera paths, depth max 2, distance less than DRAW_DISTANCE-
 *  Another function handles retrieving entry/subtype list from the ID list.
 *
 * \param point_index int               index of the point currently considered
 * \param zone_index int                index of the entry in the entry list
 * \param camera_index int              index of the camera item currently considered
 * \param cam_length int                length of the camera
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param neighbours LIST*              list of entries it stumbled upon (used later to find types/subtypes of collected IDs)
 * \return LIST                         list of IDs it came across during the search
 */
LIST build_get_entity_list(int point_index, int zone_index, int camera_index, int cam_length, ENTRY* elist, int entry_count, LIST* neighbours, int* config) {
    int preloading_flag = config[6];
    int draw_dist = config[5];
    int backwards_penalty = config[7];

    LIST entity_list = init_list();
    int i, j, k, coord_count;

    list_copy_in(neighbours, build_get_neighbours(elist[zone_index].data));

    LIST* draw_list_zone = build_get_complete_draw_list(elist, zone_index, camera_index, cam_length);
    for (i = 0; i < cam_length; i++)
        list_copy_in(&entity_list, draw_list_zone[i]);

    short int* coords = build_get_path(elist, zone_index, camera_index, &coord_count);
    LIST links = build_get_links(elist[zone_index].data, camera_index);
    for (i = 0; i < links.count; i++) {
        int distance = 0;
        CAMERA_LINK link = int_to_link(links.eids[i]);
        if (link.type == 1)
            distance += build_get_distance(coords, point_index, 0, -1, NULL);
        if (link.type == 2)
            distance += build_get_distance(coords, point_index, cam_length - 1, -1, NULL);

        unsigned int eid_offset = from_u32(elist[zone_index].data + 0x10) + 4 + link.zone_index * 4 + C2_NEIGHBOURS_START;
        unsigned int neighbour_eid = from_u32(elist[zone_index].data + eid_offset);
        unsigned int neighbour_flg = from_u32(elist[zone_index].data + eid_offset + 0x20);

        int neighbour_index = build_elist_get_index(neighbour_eid, elist, entry_count);
        if (neighbour_index == -1)
            continue;

        // preloading check
        if (preloading_flag == 0 && (neighbour_flg == 0xF || neighbour_flg == 0x1F))
            continue;

        list_copy_in(neighbours, build_get_neighbours(elist[neighbour_index].data));

        int neighbour_cam_length;
        short int* coords2 = build_get_path(elist, neighbour_index, 2 + 3 * link.cam_index, &neighbour_cam_length);
        LIST* draw_list_neighbour1 = build_get_complete_draw_list(elist, neighbour_index, 2 + 3 * link.cam_index, neighbour_cam_length);

        int draw_dist_w_orientation = draw_dist;
        if (build_is_before(elist, zone_index, camera_index / 3, neighbour_index, link.cam_index)) {
            /*char temp[100], temp2[100];
            printf("entity list %s is before %s\n", eid_conv(elist[neighbour_index].EID, temp), eid_conv(elist[zone_index].EID, temp2));*/
            draw_dist_w_orientation = build_dist_w_penalty(draw_dist, backwards_penalty);
        }

        int point_index2;
        if (link.flag == 1) {
            distance += build_get_distance(coords2, 0, neighbour_cam_length - 1, draw_dist_w_orientation - distance, &point_index2);
            for (j = 0; j < point_index2; j++)
                list_copy_in(&entity_list, draw_list_neighbour1[j]);
        }
        if (link.flag == 2) {
            distance += build_get_distance(coords2, neighbour_cam_length - 1, 0, draw_dist_w_orientation - distance, &point_index2);
            for (j = point_index2; j < neighbour_cam_length - 1; j++)
                list_copy_in(&entity_list, draw_list_neighbour1[j]);
        }

        if (distance >= draw_dist_w_orientation)
            continue;

        LIST layer2 = build_get_links(elist[neighbour_index].data, 2 + 3 * link.cam_index);
        for (j = 0; j < layer2.count; j++) {
            CAMERA_LINK link2 = int_to_link(layer2.eids[j]);
            unsigned int eid_offset2 = from_u32(elist[neighbour_index].data + 0x10) + 4 + link2.zone_index * 4 + C2_NEIGHBOURS_START;
            unsigned int neighbour_eid2 = from_u32(elist[neighbour_index].data + eid_offset2);
            unsigned int neighbour_flg2 = from_u32(elist[neighbour_index].data + eid_offset2 + 0x20);

            int neighbour_index2 = build_elist_get_index(neighbour_eid2, elist, entry_count);
            if (neighbour_index2 == -1)
                continue;

            if (preloading_flag == 0 && (neighbour_flg2 == 0xF || neighbour_flg2 == 0x1F))
                continue;

            list_copy_in(neighbours, build_get_neighbours(elist[neighbour_index2].data));

            int neighbour_cam_length2;
            short int* coords3 = build_get_path(elist, neighbour_index2, 2 + 3 * link2.cam_index, &neighbour_cam_length2);
            LIST* draw_list_neighbour2 = build_get_complete_draw_list(elist, neighbour_index2, 2 + 3 * link2.cam_index, neighbour_cam_length2);

            int point_index3;

            draw_dist_w_orientation = draw_dist;
            if (build_is_before(elist, zone_index, camera_index / 3, neighbour_index2, link2.cam_index)) {
                /*char temp[100], temp2[100];
                printf("entity list %s is before %s\n", eid_conv(elist[neighbour_index2].EID, temp), eid_conv(elist[zone_index].EID, temp2));*/
                draw_dist_w_orientation = build_dist_w_penalty(draw_dist, backwards_penalty);
            }

            // start to end
            if ((link.type == 2 && link2.type == 2 && link2.flag == 1) ||
                (link.type == 1 && link2.type == 1 && link2.flag == 1)) {
                build_get_distance(coords3, 0, neighbour_cam_length2 - 1, draw_dist_w_orientation - distance, &point_index3);
                for (k = 0; k < point_index3; k++)
                    list_copy_in(&entity_list, draw_list_neighbour2[k]);
            }

            // end to start
            if ((link.type == 2 && link2.type == 2 && link2.flag == 1) ||
                (link.type == 1 && link2.type == 1 && link2.flag == 2)) {
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
 * \param entry_count int               entry count
 * \param entity_list LIST              list of entity IDs
 * \param neighbour_list LIST           list of entries to look into
 * \return LIST                         list of [2B] type & [2B] subtype items
 */
LIST build_get_types_subtypes(ENTRY* elist, int entry_count, LIST entity_list, LIST neighbour_list) {
    LIST type_subtype_list = init_list();
    int i, j;
    for (i = 0; i < neighbour_list.count; i++) {
        int curr_index = build_elist_get_index(neighbour_list.eids[i], elist, entry_count);
        if (curr_index == -1)
            continue;

        int cam_count = build_get_cam_item_count(elist[curr_index].data);
        int entity_count = build_get_entity_count(elist[curr_index].data);

        for (j = 0; j < entity_count; j++) {
            int entity_offset = get_nth_item_offset(elist[curr_index].data, 2 + cam_count + j);
            int ID = build_get_entity_prop(elist[curr_index].data + entity_offset, ENTITY_PROP_ID);
            if (list_find(entity_list, ID) != -1) {
                int type = build_get_entity_prop(elist[curr_index].data + entity_offset, ENTITY_PROP_TYPE);
                int subtype = build_get_entity_prop(elist[curr_index].data + entity_offset, ENTITY_PROP_SUBTYPE);
                list_insert(&type_subtype_list, (type << 16) + subtype);
            }
        }
    }

    return type_subtype_list;
}


/** \brief
 *  Lets the user know whether there are any entities whose type/subtype has no specified dependencies.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param sub_info DEPENDENCIES         list of types and subtypes and their potential dependencies
 * \return void
 */
void build_find_unspecified_entities(ENTRY* elist, int entry_count, DEPENDENCIES sub_info) {
    printf("\n");

    char temp[100] = "";
    int i, j, k;
    LIST considered = init_list();
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE) {
            int cam_count = build_get_cam_item_count(elist[i].data);
            int entity_count = build_get_entity_count(elist[i].data);
            for (j = 0; j < entity_count; j++) {
                unsigned char* entity = elist[i].data + from_u32(elist[i].data + 0x10 + (2 + cam_count + j) * 4);
                int type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
                int subt = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);
                int enID = build_get_entity_prop(entity, ENTITY_PROP_ID);
                if (type >= 64 || type < 0 || subt < 0) {
                    printf("[warning] Zone %s entity %2d is invalid! (type %2d subtype %2d)\n", eid_conv(elist[i].EID, temp), j, type, subt);
                    continue;
                }

                if (list_find(considered, (type << 16) + subt) != -1)
                    continue;

                list_insert(&considered, (type << 16) + subt);

                int found = 0;
                for (k = 0; k < sub_info.count; k++)
                    if (sub_info.array[k].subtype == subt && sub_info.array[k].type == type)
                        found = 1;
                if (!found)
                    printf("[warning] Entity with type %2d subtype %2d has no dependency list! (e.g. Zone %s entity %2d ID %3d)\n",
                        type, subt, eid_conv(elist[i].EID, temp), j, enID);
            }
        }
}


/** \brief
 *  Reads draw lists of the camera of the zone, returns in a non-delta form.
 *
 * \param elist ENTRY*                  entry list
 * \param zone_index int                index of the zone entry
 * \param cam_index int                 index of the camera item
 * \param cam_length int                length of the path of the camera item
 * \return LIST*                        array of lists as a representation of the draw list (changed to full draw list instead of delta-based)
 */
LIST* build_get_complete_draw_list(ENTRY* elist, int zone_index, int cam_index, int cam_length) {
    int i, m;
    LIST* draw_list = (LIST*)malloc(cam_length * sizeof(LIST));
    LIST list = init_list();
    for (i = 0; i < cam_length; i++)
        draw_list[i] = init_list();

    int cam_offset = get_nth_item_offset(elist[zone_index].data, cam_index);
    LOAD_LIST draw_list2 = build_get_lists(ENTITY_PROP_CAM_DRAW_LIST_A, elist[zone_index].data, cam_offset);

    qsort(draw_list2.array, draw_list2.count, sizeof(LOAD), comp2);
    int sublist_index = 0;
    for (i = 0; i < cam_length; i++) {
        if (draw_list2.array[sublist_index].index == i) {
            if (draw_list2.array[sublist_index].type == 'B')
                for (m = 0; m < draw_list2.array[sublist_index].list_length; m++)
                    list_insert(&list, (draw_list2.array[sublist_index].list[m] & 0x3FF00) >> 8);

            if (draw_list2.array[sublist_index].type == 'A')
                for (m = 0; m < draw_list2.array[sublist_index].list_length; m++)
                    list_remove(&list, (draw_list2.array[sublist_index].list[m] & 0x3FF00) >> 8);

            sublist_index++;
        }
        list_copy_in(&draw_list[i], list);
    }

    delete_load_list(draw_list2);
    return draw_list;
}



/** \brief
 *  Searches the entry and for each collision type it adds its dependencies (if there are any)
 *  to the list, copies it into items start_index to end_index (exc.).
 *
 * \param full_list LIST*               current load list
 * \param start_index int               camera point index from which the additions are copied in
 * \param end_index int                 camera point index until which the additions are copied in
 * \param entry unsigned char*          entry in which it searches collision types
 * \param collisions DEPENDENCIES       list that contains list of entries tied to a specific collision type
 * \param elist ENTRY*                  entry_list
 * \param entry_count int               entry_count
 * \return void
 */
void build_add_collision_dependencies(LIST* full_list, int start_index, int end_index, unsigned char* entry, DEPENDENCIES collisions, ENTRY* elist, int entry_count) {
    int x, i, j, k;
    LIST neighbours = build_get_neighbours(entry);

    for (x = 0; x < neighbours.count; x++) {
        int index = build_elist_get_index(neighbours.eids[x], elist, entry_count);
        if (index == -1)
            continue;

        int item2off = from_u32(elist[index].data + 0x14);
        unsigned char* item = elist[index].data + item2off;
        int count = *(int*)item + 0x18;

        for (i = 0; i < count + 2; i++) {
            short int type = *(short int*)item + 0x24 + 2 * i;
            if (type % 2 == 0) continue;

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
 * \param entry_count int               entry count
 * \param full_load LIST*               full load list
 * \param cam_length int                length of the current camera
 * \param i int                         index of the current entry
 * \param j int                         current camera path
 * \return void
 */
void build_texture_count_check(ENTRY* elist, int entry_count, LIST* full_load, int cam_length, int i, int j) {
    char temp[100] = "";
    int k, l;
    int over_count = 0;
    unsigned int over_textures[20];
    int point = 0;

    for (k = 0; k < cam_length; k++) {
        int texture_count = 0;
        unsigned int textures[20];
        for (l = 0; l < full_load[k].count; l++)
            if (build_entry_type(elist[build_elist_get_index(full_load[k].eids[l], elist, entry_count)]) == -1 && eid_conv(full_load[k].eids[l], temp)[4] == 'T')
                textures[texture_count++] = full_load[k].eids[l];

        if (texture_count > over_count) {
            over_count = texture_count;
            point = k;
            memcpy(over_textures, textures, texture_count * sizeof(unsigned int));
        }
    }

    if (over_count > 8) {
        printf("[warning] Zone %s cam path %d trying to load %d textures! (eg on point %d)\n", eid_conv(elist[i].EID, temp), j, over_count, point);
        for (k = 0; k < over_count; k++)
            printf("\t%s", eid_conv(over_textures[k], temp));
        printf("\n");
    }
}


/** \brief
 *  Gets indexes of camera linked neighbours specified in the camera link property.
 *
 * \param entry unsigned char*          entry data
 * \param cam_index int                 index of the camera item
 * \return void
 */
LIST build_get_links(unsigned char* entry, int cam_index) {
    int i, k, l, link_count = 0;
    int* links = NULL;
    int cam_offset = get_nth_item_offset(entry, cam_index);

    for (k = 0; (unsigned)k < from_u32(entry + cam_offset + 0xC); k++) {
        int code = from_u16(entry + cam_offset + 0x10 + 8 * k);
        int offset = from_u16(entry + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
        int prop_len = from_u16(entry + cam_offset + 0x16 + 8 * k);

        if (code == ENTITY_PROP_CAM_PATH_LINKS) {
            if (prop_len == 1) {
                link_count = from_u16(entry + offset);
                links = (int*)malloc(link_count * sizeof(int));
                for (l = 0; l < link_count; l++)
                    links[l] = from_u32(entry + offset + 4 + 4 * l);
            }
            else {
                link_count = max(1, from_u16(entry + offset)) + max(1, from_u16(entry + offset + 2));
                links = (int*)malloc(link_count * sizeof(int));
                for (l = 0; l < link_count; l++)
                    links[l] = from_u32(entry + offset + 0x8 + 4 * l);
            }
        }
    }

    LIST links_list = init_list();
    for (i = 0; i < link_count; i++)
        list_insert(&links_list, links[i]);

    return links_list;
}


/** \brief
 *  Deconstructs the zone, alters specified item using the func_arg function, reconstructs the zone.
 *
 * \param zone ENTRY*                   zone data
 * \param item_index int                index of the item/entity to be altered
 * \param func_arg unsigned char*       function to be used, either remove or add property
 * \param list LIST*                    list to be added (might be unused)
 * \param property_code int             property code
 * \return void
 */
void build_entity_alter(ENTRY* zone, int item_index, unsigned char* (func_arg)(unsigned int, unsigned char*, int*, PROPERTY*), int property_code, PROPERTY* prop)
{
    int i, offset;
    int item_count = from_u32(zone->data + 0xC);
    int first_item_offset = 0x14 + 4 * item_count;

    int* item_lengths = (int*)malloc(item_count * sizeof(int));
    unsigned char** items = (unsigned char**)malloc(item_count * sizeof(unsigned char**));
    for (i = 0; i < item_count; i++)
        item_lengths[i] = get_nth_item_offset(zone->data, i + 1) - get_nth_item_offset(zone->data, i);

    for (offset = first_item_offset, i = 0; i < item_count; offset += item_lengths[i], i++) {
        items[i] = (unsigned char*)malloc(item_lengths[i]);
        memcpy(items[i], zone->data + offset, item_lengths[i]);
    }

    items[item_index] = func_arg(property_code, items[item_index], &item_lengths[item_index], prop);

    int new_size = first_item_offset;
    for (i = 0; i < item_count; i++)
        new_size += item_lengths[i];

    unsigned char* new_data = (unsigned char*)malloc(new_size);
    *(int*)(new_data) = MAGIC_ENTRY;
    *(int*)(new_data + 0x4) = zone->EID;
    *(int*)(new_data + 0x8) = ENTRY_TYPE_ZONE;
    *(int*)(new_data + 0xC) = item_count;

    for (offset = first_item_offset, i = 0; i < item_count + 1; offset += item_lengths[i], i++)
        *(int*)(new_data + 0x10 + i * 4) = offset;

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
 * \param code unsigned int             code of the property to be added
 * \param item unsigned char*           data of item where the property will be added
 * \param item_size int*                item size
 * \param list LIST *                   load list to be added
 * \return unsigned char*               new item data
 */
unsigned char* build_add_property(unsigned int code, unsigned char* item, int* item_size, PROPERTY* prop) {
    int offset, i, property_count = from_u32(item + 0xC);

    int* property_sizes = (int*)malloc((property_count + 1) * sizeof(int));
    unsigned char** properties = (unsigned char**)malloc((property_count + 1) * sizeof(unsigned char*));
    unsigned char** property_headers = (unsigned char**)malloc((property_count + 1) * sizeof(unsigned char*));

    for (i = 0; i < property_count + 1; i++) {
        property_headers[i] = (unsigned char*)malloc(8 * sizeof(unsigned char));
        for (int j = 0; j < 8; j++)
            property_headers[i][j] = 0;
    }

    for (i = 0; i < property_count; i++) {
        if (from_u16(item + 0x10 + 8 * i) > code)
            memcpy(property_headers[i + 1], item + 0x10 + 8 * i, 8);
        else
            memcpy(property_headers[i], item + 0x10 + 8 * i, 8);
    }

    int insertion_index = 0;
    for (i = 0; i < property_count + 1; i++)
        if (from_u32(property_headers[i]) == 0 && from_u32(property_headers[i] + 4) == 0)
            insertion_index = i;


    for (i = 0; i < property_count + 1; i++) {
        property_sizes[i] = 0;
        if (i < insertion_index - 1)
            property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
        if (i == insertion_index - 1)
            property_sizes[i] = from_u16(property_headers[i + 2] + 2) - from_u16(property_headers[i] + 2);
        if (i == insertion_index)
            ;
        if (i > insertion_index) {
            if (i == property_count)
                property_sizes[i] = from_u16(item) - from_u16(property_headers[i] + 2);
            else
                property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
        }
    }

    offset = 0x10 + 8 * property_count;
    for (i = 0; i < property_count + 1; i++) {
        if (i == insertion_index) continue;

        properties[i] = (unsigned char*)malloc(property_sizes[i]);
        memcpy(properties[i], item + offset, property_sizes[i]);
        offset += property_sizes[i];
    }

    property_sizes[insertion_index] = prop->length;
    memcpy(property_headers[insertion_index], prop->header, 8);
    properties[insertion_index] = prop->data;

    int new_size = 0x10 + 8 * (property_count + 1);
    for (i = 0; i < property_count + 1; i++)
        new_size += property_sizes[i];


    unsigned char* new_item = (unsigned char*)malloc(new_size);
    *(int*)(new_item) = new_size - 0xC;
    *(int*)(new_item + 0x4) = 0;
    *(int*)(new_item + 0x8) = 0;
    *(int*)(new_item + 0xC) = property_count + 1;

    offset = 0x10 + 8 * (property_count + 1);
    for (i = 0; i < property_count + 1; i++) {
        *(short int*)(property_headers[i] + 2) = offset - 0xC;
        memcpy(new_item + 0x10 + 8 * i, property_headers[i], 8);
        memcpy(new_item + offset, properties[i], property_sizes[i]);
        offset += property_sizes[i];
    }

    *item_size = new_size - 0xC;
    free(item);
    for (i = 0; i < property_count + 1; i++) {
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
 * \param code unsigned int             code of the property to be removed
 * \param item unsigned char*           data of the item thats to be changed
 * \param item_size int*                size of the item
 * \param prop PROPERTY                 unused
 * \return unsigned char*               new item data
 */
unsigned char* build_rem_property(unsigned int code, unsigned char* item, int* item_size, PROPERTY* prop) {
    int offset, i, property_count = from_u32(item + 0xC);

    int* property_sizes = (int*)malloc(property_count * sizeof(int));
    unsigned char** properties = (unsigned char**)malloc(property_count * sizeof(unsigned char*));
    unsigned char** property_headers = (unsigned char**)malloc(property_count * sizeof(unsigned char*));

    int found = 0;
    for (i = 0; i < property_count; i++) {
        property_headers[i] = (unsigned char*)malloc(8 * sizeof(unsigned char*));
        memcpy(property_headers[i], item + 0x10 + 8 * i, 8);
        if (from_u16(property_headers[i]) == code)
            found++;
    }

    if (!found) return item;

    for (i = 0; i < property_count - 1; i++)
        property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
    property_sizes[i] = from_u16(item) - 0xC - from_u16(property_headers[i] + 2);

    offset = 0x10 + 8 * property_count;
    for (i = 0; i < property_count; offset += property_sizes[i], i++) {
        properties[i] = (unsigned char*)malloc(property_sizes[i]);
        memcpy(properties[i], item + offset, property_sizes[i]);
    }

    int new_size = 0x10 + 8 * (property_count - 1);
    for (i = 0; i < property_count; i++) {
        if (from_u16(property_headers[i]) == code) continue;
        new_size += property_sizes[i];
    }

    unsigned char* new_item = (unsigned char*)malloc(new_size);
    *(int*)(new_item) = new_size;
    *(int*)(new_item + 4) = 0;
    *(int*)(new_item + 8) = 0;
    *(int*)(new_item + 0xC) = property_count - 1;

    offset = 0x10 + 8 * (property_count - 1);
    int indexer = 0;
    for (i = 0; i < property_count; i++) {
        if (from_u16(property_headers[i]) != code) {
            *(short int*)(property_headers[i] + 2) = offset - 0xC;
            memcpy(new_item + 0x10 + 8 * indexer, property_headers[i], 8);
            memcpy(new_item + offset, properties[i], property_sizes[i]);
            indexer++;
            offset += property_sizes[i];
        }
    }

    *item_size = new_size;
    for (i = 0; i < property_count; i++) {
        free(properties[i]);
        free(property_headers[i]);
    }
    free(properties);
    free(property_headers);
    free(property_sizes);
    free(item);
    return new_item;
}


/** \brief
 *  Gets texture references from a model and adds them to the list.
 *
 * \param list LIST*                        list to be added to
 * \param model unsigned char*              model data
 * \return void
 */
void build_add_model_textures_to_list(unsigned char* model, LIST* list) {
    int item1off = from_u32(model + 0x10);

    for (int i = 0; i < 8; i++) {
        unsigned int scen_reference = from_u32(model + item1off + 0xC + 0x4 * i);
        if (scen_reference)
            list_insert(list, scen_reference);
    }
}


/** \brief
 *  Gets texture references from a scenery entry and inserts them to the list.
 *
 * \param scenery unsigned char*            scenery data
 * \param list LIST*                        list to be added to
 * \return void
 */
void build_add_scen_textures_to_list(unsigned char* scenery, LIST* list) {
    int item1off = from_u32(scenery + 0x10);
    int texture_count = from_u32(scenery + item1off + 0x28);
    for (int i = 0; i < texture_count; i++) {
        unsigned int eid = from_u32(scenery + item1off + 0x2C + 4 * i);
        list_insert(list, eid);
    }
}


/** \brief
 *  Returns path the entity has & its length.
 *
 * \param elist ENTRY*                  entry list
 * \param zone_index int                index of entry in the list
 * \param item_index int                index of item of the entry whose path it will return
 * \param path_len int*                 path length return value
 * \return short int*                   path
 */
short int* build_get_path(ENTRY* elist, int zone_index, int item_index, int* path_len) {
    unsigned char* item = elist[zone_index].data + get_nth_item_offset(elist[zone_index].data, item_index);
    unsigned int i, offset = 0;
    for (i = 0; i < from_u32(item + 0xC); i++)
        if ((from_u16(item + 0x10 + 8 * i)) == ENTITY_PROP_PATH)
            offset = 0xC + from_u16(item + 0x10 + 8 * i + 2);

    if (offset)
        *path_len = from_u32(item + offset);
    else {
        *path_len = 0;
        return NULL;
    }

    short int* coords = (short int*)malloc(3 * sizeof(short int) * *path_len);
    memcpy(coords, item + offset + 4, 6 * (*path_len));
    return coords;
}

/** \brief
 *  Gets zone's scenery reference count.
 *
 * \param entry unsigned char*          entry data
 * \return int                          scenery reference count
 */
int build_get_scen_count(unsigned char* entry) {
    int item1off = get_nth_item_offset(entry, 0);
    return entry[item1off];
}

LIST build_get_sceneries(unsigned char* entry) {
    int scen_count = build_get_scen_count(entry);
    int item1off = get_nth_item_offset(entry, 0);
    LIST list = init_list();
    for (int i = 0; i < scen_count; i++) {
        unsigned int scen = from_u32(entry + item1off + 0x4 + 0x30 * i);
        list_insert(&list, scen);
    }

    return list;
}