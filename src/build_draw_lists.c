#include "macros.h"

int32_t abs(int32_t val)
{
    return (val >= 0) ? val : -val;
}

// caps the angle to 0-360
int32_t normalize_angle(int32_t angle)
{
    while (angle < 0)
        angle += 360;
    while (angle >= 360)
        angle -= 360;

    return angle;
}

// converts the ingame yaw value range to regular range
int32_t c2yaw_to_deg(int32_t yaw)
{
    int32_t angle = (yaw * 360) / 4096;
    angle += 90;

    return normalize_angle(angle);
}

// converts 0-360 range angle to ingame angle value
int32_t deg_to_c2yaw(int32_t deg)
{
    int32_t angle = deg - 90;
    int32_t yaw = (angle * 4096) / 360;
    while (yaw < 0)
        yaw += 4096;
    yaw = yaw % 4096;
    return yaw;
}

// calculates angle distance between 2 angles
int32_t angle_distance(int32_t angle1, int32_t angle2)
{
    angle1 = normalize_angle(angle1);
    angle2 = normalize_angle(angle2);

    int32_t diff1 = normalize_angle(angle2 - angle1);
    int32_t diff2 = normalize_angle(angle1 - angle2);

    return min(diff1, diff2);
}

// averages 2 angles
int32_t average_angles(int32_t angle1, int32_t angle2)
{
    // Convert degrees to radians
    double a1 = angle1 * PI / 180.0;
    double a2 = angle2 * PI / 180.0;

    // Convert to Cartesian coordinates
    double x = cos(a1) + cos(a2);
    double y = sin(a1) + sin(a2);

    // Compute the averaged angle in degrees
    return (int32_t)round(atan2(y, x) * 180.0 / PI);
}

// gets full draw list for all points of a camera path according to config
void build_draw_list_util(ENTRY *elist, int32_t entry_count, LIST *full_draw, int32_t *config, int32_t curr_idx, int32_t neighbour_idx, int32_t cam_idx, int32_t neighbour_ref_idx)
{
    int32_t cam_len, ent_len, angles_len = 0;
    char temp[6] = "";
    ENTRY curr = elist[curr_idx];
    ENTRY neighbour = elist[neighbour_idx];
    LIST remember = init_list();

    int32_t cam_mode = build_get_entity_prop(build_get_nth_item(curr.data, 2 + 3 * cam_idx), ENTITY_PROP_CAMERA_MODE);
    int16_t *path = build_get_path(elist, curr_idx, 2 + cam_idx * 3, &cam_len);
    int16_t *angles = build_get_path(elist, curr_idx, 2 + cam_idx * 3 + 1, &angles_len);

    int32_t neigh_ents = build_get_entity_count(neighbour.data);
    int32_t neigh_cams = build_get_cam_item_count(neighbour.data);

    if (cam_len != angles_len / 2)
    {
        printf("[warning] Zone %s camera %d camera path and angles path length mismatch (%d %d)\n", eid_conv(curr.eid, temp), cam_idx, cam_len, angles_len);
    }

    int32_t *avg_angles = malloc(cam_len * sizeof(int32_t));
    if (cam_mode == C2_CAM_MODE_3D || cam_mode == C2_CAM_MODE_CUTSCENE)
    {
        for (int32_t n = 0; n < angles_len; n += 2)
        {
            int32_t angle1 = c2yaw_to_deg(angles[3 * n + 1]);
            int32_t angle2 = c2yaw_to_deg(angles[3 * n + 4]);
            avg_angles[n / 2] = normalize_angle(average_angles(angle1, angle2));
        }
    }

    // for each point of the camera path
    for (int32_t m = 0; m < cam_len; m++)
    {
        int32_t cam_x = path[3 * m + 0] + from_u32(build_get_nth_item(curr.data, 1) + 0);
        int32_t cam_y = path[3 * m + 1] + from_u32(build_get_nth_item(curr.data, 1) + 4);
        int32_t cam_z = path[3 * m + 2] + from_u32(build_get_nth_item(curr.data, 1) + 8);

        // check all neighbour entities
        for (int32_t n = 0; n < neigh_ents; n++)
        {
            unsigned char *entity = build_get_nth_item(neighbour.data, 2 + neigh_cams + n);
            int32_t ent_id = build_get_entity_prop(entity, ENTITY_PROP_ID);
            int32_t ent_override_mult = build_get_entity_prop(entity, ENTITY_PROP_OVERRIDE_DRAW_MULT);
            int32_t pos_override_id = build_get_entity_prop(entity, ENTITY_PROP_OVERRIDE_DRAW_ID);

            int32_t type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
            int32_t subt = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);

            if (ent_id == -1)
                continue;

            int32_t ref_ent_idx = n;
            if (pos_override_id != -1 && !(type == 4 && subt == 17))
            {
                pos_override_id = pos_override_id / 0x100;
                for (int32_t o = 0; o < neigh_ents; o++)
                {
                    int32_t ent_id2 = build_get_entity_prop(build_get_nth_item(neighbour.data, 2 + neigh_cams + o), ENTITY_PROP_ID);
                    if (ent_id2 == pos_override_id)
                    {
                        ref_ent_idx = o;
                        if (list_find(remember, ent_id) == -1)
                        {
                            printf("%d using position from another entity %d\n", ent_id, ent_id2);
                            list_add(&remember, ent_id);
                        }
                        break;
                    }
                }
            }

            if (ent_override_mult == -1)
                ent_override_mult = 100;
            else
                ent_override_mult = ent_override_mult / 0x100;

            int32_t allowed_dist_x = ((ent_override_mult * config[CNFG_IDX_DRAW_LIST_GEN_CAP_X]) / 100);
            int32_t allowed_dist_y = ((ent_override_mult * config[CNFG_IDX_DRAW_LIST_GEN_CAP_Y]) / 100);
            int32_t allowed_dist_xz = ((ent_override_mult * config[CNFG_IDX_DRAW_LIST_GEN_CAP_XZ]) / 100);
            int32_t allowed_angledist = config[CNFG_IDX_DRAW_LIST_GEN_ANGLE_3D];

            int16_t *ent_path = build_get_path(elist, neighbour_idx, 2 + neigh_cams + ref_ent_idx, &ent_len);

            // check all entity points to see whether its visible
            for (int32_t o = 0; o < ent_len; o++)
            {
                int32_t ent_x = (4 * ent_path[3 * o + 0]) + from_s32(build_get_nth_item(neighbour.data, 1) + 0);
                int32_t ent_y = (4 * ent_path[3 * o + 1]) + from_s32(build_get_nth_item(neighbour.data, 1) + 4);
                int32_t ent_z = (4 * ent_path[3 * o + 2]) + from_s32(build_get_nth_item(neighbour.data, 1) + 8);

                int32_t dist_x = abs(cam_x - ent_x);
                int32_t dist_y = abs(cam_y - ent_y);
                int32_t dist_z = abs(cam_z - ent_z);
                int32_t dist_xz = sqrt(pow(dist_x, 2) + pow(dist_z, 2));

                if (allowed_dist_x && dist_x > allowed_dist_x)
                    continue;
                if (allowed_dist_y && dist_y > allowed_dist_y)
                    continue;
                if (allowed_dist_xz && dist_xz > allowed_dist_xz)
                    continue;

                if (cam_mode == C2_CAM_MODE_2D || cam_mode == C2_CAM_MODE_VERTICAL)
                {
                    list_add(&full_draw[m], neighbour_ref_idx | (ent_id << 8) | (n << 24));
                    break;
                }
                else if (cam_mode == C2_CAM_MODE_3D || cam_mode == C2_CAM_MODE_CUTSCENE)
                {
                    // in 3D, entities closer than 20% of the distance cap are drawn regardless of the check
                    if (dist_xz < (allowed_dist_xz / 5) || !allowed_dist_xz)
                    {
                        list_add(&full_draw[m], neighbour_ref_idx | (ent_id << 8) | (n << 24));
                        break;
                    }

                    // simple frustum check
                    // Zs swapped because of the z axis orientation in c2
                    double t = atan2(cam_z - ent_z, ent_x - cam_x);
                    int32_t angle_to_ent = (int32_t)(t * (180.0 / PI));
                    int32_t camera_angle = avg_angles[m];
                    int32_t angle_dist = angle_distance(angle_to_ent, camera_angle);

                    if (angle_dist < allowed_angledist && dist_xz < allowed_dist_xz)
                    {
                        list_add(&full_draw[m], neighbour_ref_idx | (ent_id << 8) | (n << 24));
                        break;
                    }
                }
                else
                {
                    printf("[warning] Unknown camera mode %d in %s cam %d\n", cam_mode, eid_conv(curr.eid, temp), cam_idx);
                }
            }
        }
    }

    free(avg_angles);
}

// main function for remaking draw lists according to config and zone data
// selects entities to draw, converts to delta form and replaces camera properties
void build_remake_draw_lists(ENTRY *elist, int32_t entry_count, int32_t *config)
{
    int32_t i, j, k, l;
    int32_t dbg_print = 0;

    for (i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            int32_t cam_count = build_get_cam_item_count(elist[i].data) / 3;
            if (cam_count == 0)
                continue;

            char temp[100] = "";
            printf("\nMaking draw lists for %s\n", eid_conv(elist[i].eid, temp));

            for (j = 0; j < cam_count; j++)
            {
                int32_t cam_offset = build_get_nth_item_offset(elist[i].data, 2 + 3 * j);

                int32_t cam_length = build_get_path_length(elist[i].data + cam_offset);
                printf("\tcam path %d (%d points)\n", j, cam_length);

                // initialize full non-delta draw list used to represent the draw list during its building
                LIST *full_draw = (LIST *)malloc(cam_length * sizeof(LIST)); // freed here
                for (k = 0; k < cam_length; k++)
                    full_draw[k] = init_list();

                int32_t neighbour_count = build_get_neighbour_count(elist[i].data);
                for (l = 0; l < neighbour_count; l++)
                {
                    ENTRY curr = elist[i];
                    uint32_t neighbour_eid = from_u32(build_get_nth_item(curr.data, 0) + C2_NEIGHBOURS_START + 4 + (4 * l));
                    int32_t idx = build_get_index(neighbour_eid, elist, entry_count);
                    if (idx == -1)
                    {
                        printf("[warning] Invalid neighbour %s\n", eid_conv(neighbour_eid, temp));
                        continue;
                    }
                    build_draw_list_util(elist, entry_count, full_draw, config, i, idx, j, l);
                }

                int32_t max_c = 0;
                int32_t max_p = 0;
                printf("\t");
                for (k = 0; k < cam_length; k++)
                {
                    printf("%d,", full_draw[k].count);
                    if (full_draw[k].count > max_c)
                    {
                        max_c = full_draw[k].count;
                        max_p = k;
                    }
                }
                printf("\n\tMax count: %d (point %d)\n", max_c, max_p);

                // creates and initialises delta representation of the draw list
                LIST *listA = (LIST *)malloc(cam_length * sizeof(LIST)); // freed here
                LIST *listB = (LIST *)malloc(cam_length * sizeof(LIST)); // freed here
                for (k = 0; k < cam_length; k++)
                {
                    listA[k] = init_list();
                    listB[k] = init_list();
                }

                // converts full draw list to delta based, then creates game-format draw list properties based on the delta lists
                // listA and listB args are switched around because draw lists are like that
                build_load_list_to_delta(full_draw, listB, listA, cam_length, elist, entry_count, 1);
                PROPERTY prop_0x13B = build_make_load_list_prop(listA, cam_length, 0x13B);
                PROPERTY prop_0x13C = build_make_load_list_prop(listB, cam_length, 0x13C);

                if (dbg_print)
                    printf("Converted full list to delta form and delta to props\n");

                // removes existing draw list properties, inserts newly made ones
                build_entity_alter(&elist[i], 2 + 3 * j, build_rem_property, 0x13B, NULL);
                build_entity_alter(&elist[i], 2 + 3 * j, build_rem_property, 0x13C, NULL);

                if (max_c)
                {
                    build_entity_alter(&elist[i], 2 + 3 * j, build_add_property, 0x13B, &prop_0x13B);
                    build_entity_alter(&elist[i], 2 + 3 * j, build_add_property, 0x13C, &prop_0x13C);
                }

                if (dbg_print)
                    printf("Replaced draw list props\n");

                free(full_draw);
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
