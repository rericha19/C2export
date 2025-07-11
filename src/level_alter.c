#include "macros.h"

// removes draw list properties from cameras of all zone entries
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

// removes non-camera entities from all zone entries
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

// converts old draw override props to proper new ones
// (used to reuse box count properties)
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
            PROPERTY *prop_override_id = build_get_prop_full(nth_entity, ENTITY_PROP_OVERRIDE_DRAW_ID_OLD);
            PROPERTY *prop_override_mult = build_get_prop_full(nth_entity, ENTITY_PROP_OVERRIDE_DRAW_MULT_OLD);

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

// util function for swapping collision nodes
void swap_node(unsigned short int *a, unsigned short int *b)
{
    unsigned short int temp = *a;
    *a = *b;
    *b = temp;
}

typedef struct col_info_single
{
    unsigned short int offset_value;
    unsigned short int children_count;
    LIST found_at;
} CollisionNodeInfoSingle;

typedef struct col_info
{
    CollisionNodeInfoSingle nodes[CHUNKSIZE / 2];
    int count;
} CollisionNodeInfo;

// recursively retrieves list of collision nodes found in a collision item
void collflip_list_nodes(unsigned char *item, int offset, int depth, int maxx, int maxy, int maxz, CollisionNodeInfo *node_info, int parent_offset)
{
    if (offset < 0x1C)
    {
        printf("!!!!!!!!!!!!! unexpected offset value %4x (parent %4x)\n", offset, parent_offset);
        return;
    }
    unsigned short int *children = (unsigned short int *)(item + offset);

    int num_children = 1;
    if (depth)
    {
        if (depth <= maxx)
            num_children <<= 1;
        if (depth <= maxy)
            num_children <<= 1;
        if (depth <= maxz)
            num_children <<= 1;
    }

    int is_new = 1;
    for (int i = 0; i < node_info->count; i++)
    {
        if (node_info->nodes[i].offset_value == offset && node_info->nodes[i].children_count == num_children)
        {
            list_add(&node_info->nodes[i].found_at, parent_offset);
            is_new = 0;
            break;
        }
    }
    if (is_new)
    {
        int add_idx = node_info->count;
        node_info->nodes[add_idx].offset_value = offset;
        node_info->nodes[add_idx].children_count = num_children;
        node_info->nodes[add_idx].found_at = init_list();
        list_add(&node_info->nodes[add_idx].found_at, parent_offset);
        node_info->count++;
    }

    for (int i = 0; i < num_children; i++)
    {
        int is_leaf = children[i] == 0 || (children[i] & 1);

        if (!is_leaf)
        {
            if (children[i] < 0x1C)
            {
                printf("!!!!!!!!!!!!! child unexpected offset %4x (offset %x)\n", children[i], offset);
                continue;
            }
            collflip_list_nodes(item, children[i], depth + 1, maxx, maxy, maxz, node_info, offset + 2 * i);
        }
    }
}

// debug print for node info struct
void dbg_print_node_info_single(CollisionNodeInfoSingle *node)
{
    printf("  NodeInfo: offset_value=0x%04X, children_count=%d, found_at.count=%d, found_at.eids=[",
           node->offset_value, node->children_count, node->found_at.count);
    for (int j = 0; j < node->found_at.count; j++)
    {
        printf("%s0x%04X", (j > 0) ? ", " : "", node->found_at.eids[j]);
    }
    printf("]\n");
}

// function for getting rid of problematic data overlaps in collision items
// required for collision flipping, as overlapping data flip would otherwise get scrambled
int collfip_unwrap_overlaps(unsigned char *item, CollisionNodeInfo *node_info, int *new_size)
{
    for (int i = 0; i < node_info->count; i++)
    {
        CollisionNodeInfoSingle nodeA = node_info->nodes[i];
        unsigned short int offsetA = nodeA.offset_value;
        unsigned short int childrenA = nodeA.children_count;
        unsigned short int endA = offsetA + 2 * childrenA;

        for (int j = 0; j < i; j++)
        {
            CollisionNodeInfoSingle nodeB = node_info->nodes[j];
            unsigned int offsetB = nodeB.offset_value;
            unsigned short int childrenB = nodeB.children_count;
            unsigned short int endB = offsetB + 2 * childrenB;

            // exact reuse of the same node, allowed
            if (offsetA == offsetB && childrenA == childrenB)
                continue;

            int new_offset = *new_size;
            int case1 = (offsetA < offsetB && endA > offsetB);
            int case2 = (offsetB < offsetA && endB > offsetA);
            int case3 = (offsetA != offsetB && endA == endB);

            // if A is a subset of B its ok i think?
            int case4 = (offsetA == offsetB && childrenA != childrenB);
            int a_in_b = list_is_subset(nodeA.found_at, nodeB.found_at);
            int b_in_a = list_is_subset(nodeB.found_at, nodeA.found_at);
            if (a_in_b || b_in_a)
                case4 = 0;

            // if overlapping, move node B to the end of the item, update refs
            if (case1 || case2 || case3 || case4)
            {
                // printf("swapping case %d%d%d%d\n", case1, case2, case3, case4);
                // dbg_print_node_info_single(&nodeA);
                // dbg_print_node_info_single(&nodeB);
                // printf("\n");

                memcpy(item + new_offset, item + offsetB, 2 * childrenB);
                if (case4)
                {
                    for (int k = 0; k < nodeB.found_at.count; k++)
                    {
                        *(unsigned short int *)(item + nodeB.found_at.eids[k]) = new_offset;
                    }
                }
                else
                {
                    for (int k = 0x24; k < new_offset; k += 2)
                    {
                        if (*(unsigned short int *)(item + k) == offsetB)
                            *(unsigned short int *)(item + k) = new_offset;
                    }
                }
                *new_size += 2 * childrenB;
                return 1;
            }
        }
    }

    return 0;
}

// expands supplied collision item into a form that doesnt have data/node overlaps
unsigned char *coll_get_expanded(unsigned char *item1, int *new_size, int maxx, int maxy, int maxz)
{
    int found_overlap = 1;
    unsigned char *item1_edited = (unsigned char *)calloc(CHUNKSIZE, 1);
    memcpy(item1_edited, item1, *new_size);
    while (found_overlap)
    {
        // maybe could be done nicer (not in a while loop) but good enough
        CollisionNodeInfo node_info;
        node_info.count = 0;
        collflip_list_nodes(item1_edited, 0x1C, 0, maxx, maxy, maxz, &node_info, 0);
        found_overlap = collfip_unwrap_overlaps(item1_edited, &node_info, new_size);
    }
    return item1_edited;
}

// flips/mirrors the (expanded) collision item either on x or y
void collision_flip(unsigned char *item, int offset, int depth, int maxx, int maxy, int maxz, LIST *flipped, int is_x)
{
    if (offset < 0x1C)
    {
        printf("!!!!!!!!!!!!! collision_flip x:%d || offset %4x\n", is_x, offset);
        return;
    }

    if (list_find(*flipped, offset) != -1)
        return;

    list_add(flipped, offset);
    unsigned short int *children = (unsigned short int *)(item + offset);
    int num_children = 1;
    if (depth)
    {
        if (depth <= maxx)
            num_children <<= 1;
        if (depth <= maxy)
            num_children <<= 1;
        if (depth <= maxz)
            num_children <<= 1;
    }

    int M[] = {maxx, maxy, maxz};
    qsort(M, 3, sizeof(int), cmp_func_int);
    int min = M[0];
    // int min2 = M[1];
    int max = M[2];

    if (is_x)
    {
        if (num_children == 8)
        {
            // good
            swap_node(&children[0], &children[4]);
            swap_node(&children[1], &children[5]);
            swap_node(&children[2], &children[6]);
            swap_node(&children[3], &children[7]);
        }
        else if (num_children == 4 && min != maxx)
        {
            // e.g 665/656 good?
            if (maxx == maxy || maxx == maxz)
            {
                swap_node(&children[0], &children[2]);
                swap_node(&children[1], &children[3]);
            }
        }
        else if (num_children == 2 && max == maxx)
        {
            // eg 655
            swap_node(&children[0], &children[1]);
        }
    }
    else
    {
        if (num_children == 8)
        {
            // e.g 555
            swap_node(&children[0], &children[2]);
            swap_node(&children[1], &children[3]);
            swap_node(&children[4], &children[6]);
            swap_node(&children[5], &children[7]);
        }
        else if (num_children == 4 && min != maxy)
        {
            // e.g 566/665
            swap_node(&children[0], &children[2]);
            swap_node(&children[1], &children[3]);
        }
        else if (num_children == 2 && max == maxy)
        {
            // 565
            swap_node(&children[0], &children[1]);
        }
    }

    for (int i = 0; i < num_children; i++)
    {
        int is_leaf = children[i] == 0 || (children[i] & 1);
        if (!is_leaf)
            collision_flip(item, children[i], depth + 1, maxx, maxy, maxz, flipped, is_x);
    }
}

// for flipping entity/camera path when doing flip_y
void flip_entity_y(unsigned char *item, int offset_y)
{
    int offset = build_get_prop_offset(item, ENTITY_PROP_PATH);
    if (!offset)
        return;

    int length = from_u32(item + offset);
    for (int k = 0; k < length; k++)
    {
        *(short int *)(item + offset + 4 + 6 * k + 2) *= -1;
        *(short int *)(item + offset + 4 + 6 * k + 2) += offset_y;
    }
}

// level alter utility - level flip y
void flip_level_y(ENTRY *elist, int entry_count, int *chunk_count)
{
    for (int i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_SCENERY)
        {
            // vertices
            unsigned char *item0 = build_get_nth_item(elist[i].data, 0);
            unsigned char *item1 = build_get_nth_item(elist[i].data, 1);

            int vert_count = from_u32(item0 + 0x10);
            for (int j = 0; j < vert_count; j++)
            {
                unsigned char *vert = item1 + j * 4;
                short int y = from_s16(vert + 2) / 16;
                short int rest = from_s16(vert + 2) % 16;

                *(short int *)(vert + 2) = -(y * 16) + rest;
            }

            // position
            *(int *)(item0 + 0x4) *= -1;
        }

        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            unsigned char *item1 = build_get_nth_item(elist[i].data, 1);
            int new_size = build_get_nth_item_offset(elist[i].data, 2) - build_get_nth_item_offset(elist[i].data, 1);

            // zone position
            *(int *)(item1 + 0x4) *= -1;
            *(int *)(item1 + 0x4) -= from_s32(item1 + 0x10);

            // flip collision
            int maxx = from_u16(item1 + 0x1E);
            int maxy = from_u16(item1 + 0x20);
            int maxz = from_u16(item1 + 0x22);
            printf("y flipping coll in zone %s, %d %d %d\n", eid_conv2(elist[i].eid), maxx, maxy, maxz);
            unsigned char *item1_edited = coll_get_expanded(item1, &new_size, maxx, maxy, maxz);
            LIST flipped = init_list();
            collision_flip(item1_edited, 0x1C, 0, maxx, maxy, maxz, &flipped, 0);
            build_replace_item(&elist[i], 1, item1_edited, new_size);

            // object positions
            int cam_item_count = build_get_cam_item_count(elist[i].data);
            int entity_count = build_get_entity_count(elist[i].data);

            for (int j = 0; j < cam_item_count / 3; j++)
            {
                unsigned char *item = build_get_nth_item(elist[i].data, 2 + 3 * j);
                flip_entity_y(item, from_s32(item1 + 0x10));
            }

            for (int j = 0; j < entity_count; j++)
            {
                unsigned char *item = build_get_nth_item(elist[i].data, 2 + cam_item_count + j);
                flip_entity_y(item, from_s32(item1 + 0x10) / 4);
            }

            elist[i].chunk = *chunk_count;
            *chunk_count += 1;
        }
    }
}

// for flipping entity/camera path when doing flip_x
void flip_entity_x(unsigned char *item, int offset_x)
{
    int offset = build_get_prop_offset(item, ENTITY_PROP_PATH);

    if (!offset)
        return;

    int length = from_u32(item + offset);
    for (int k = 0; k < length; k++)
    {
        *(short int *)(item + offset + 4 + 6 * k + 0) *= -1;
        *(short int *)(item + offset + 4 + 6 * k + 0) += offset_x;
    }
}

// for flipping camera
void flip_camera_angle_x(unsigned char *item2)
{
    int offset = build_get_prop_offset(item2, ENTITY_PROP_PATH);
    if (!offset)
        return;

    int length = from_u32(item2 + offset);
    for (int k = 0; k < length; k++)
    {
        int angle = *(short int *)(item2 + offset + 4 + 6 * k + 2);
        angle = c2yaw_to_deg(-angle);
        angle = normalize_angle(angle);
        angle = deg_to_c2yaw(angle);
        *(short int *)(item2 + offset + 4 + 6 * k + 2) = angle;
    }
}

// fixes elevator destinations for elevators when doing level flip x
void flip_fix_elev_coords_x(unsigned char *item)
{
    int type = build_get_entity_prop(item, ENTITY_PROP_TYPE);
    int subtype = build_get_entity_prop(item, ENTITY_PROP_SUBTYPE);
    int offset = build_get_prop_offset(item, ENTITY_PROP_ARGUMENTS);

    if (!offset)
        return;

    int count = from_u32(item + offset);
    // basic elev, then bonus return elev
    if ((count == 8 && type == 9 && subtype == 6) ||
        (count == 8 && type == 9 && subtype == 31))
    {
        *(int *)(item + offset + 6 * 4) *= -1;
    }
}

// fixes warp in coordinates property when doing level flip x
void flip_fix_prop_198_x(unsigned char *item)
{
    int offset = build_get_prop_offset(item, ENTITY_PROP_WARPIN_COORDS);
    if (!offset)
        return;

    if (from_s32(item + offset) == 5)
        *(int *)(item + offset + 0xC) *= -1;
}

// fixes sidescrolling camera distances, if possible
void flip_camera_dist_2D_x(unsigned char *item0, unsigned char *item1)
{
    int mode = build_get_entity_prop(item0, ENTITY_PROP_CAMERA_MODE);
    if (mode != C2_CAM_MODE_2D)
        return;

    PROPERTY *prop = build_get_prop_full(item1, ENTITY_PROP_CAM_DISTANCE);
    int offset = build_get_prop_offset(item1, ENTITY_PROP_CAM_DISTANCE);

    int row_c = from_u16(prop->header + 6);
    // printf("%d row count\n", row_c);

    int meta_length = 0;
    if (row_c == 1)
        meta_length = 4;
    else if (row_c == 2 || row_c == 3)
        meta_length = 8;
    else
    {
        printf("unhandled sidescrolling camera distance, row count %d\n", row_c);
        return;
    }

    for (int i = 0; i < row_c; i++)
    {
        short int value1 = *(short int *)(item1 + offset + meta_length + i * 8 + 0);
        short int value2 = *(short int *)(item1 + offset + meta_length + i * 8 + 2);
        short int value3 = *(short int *)(item1 + offset + meta_length + i * 8 + 4);
        short int value4 = *(short int *)(item1 + offset + meta_length + i * 8 + 6);

        *(short int *)(item1 + offset + meta_length + i * 8 + 0) = -value4;
        *(short int *)(item1 + offset + meta_length + i * 8 + 2) = -value3;
        *(short int *)(item1 + offset + meta_length + i * 8 + 4) = -value2;
        *(short int *)(item1 + offset + meta_length + i * 8 + 6) = -value1;
    }
}

// level alter utility - level flip x
void flip_level_x(ENTRY *elist, int entry_count, int *chunk_count)
{
    for (int i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_SCENERY)
        {
            // vertices
            unsigned char *item0 = build_get_nth_item(elist[i].data, 0);
            unsigned char *item1 = build_get_nth_item(elist[i].data, 1);

            int vert_count = from_u32(item0 + 0x10);
            for (int j = 0; j < vert_count; j++)
            {
                unsigned char *vert = item1 + j * 4;
                short int y = from_s16(vert + 0) / 16;
                short int rest = from_s16(vert + 0) % 16;

                *(short int *)(vert + 0) = -(y * 16) + rest;
            }

            // position
            *(int *)(item0 + 0x0) *= -1;
        }

        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            unsigned char *item1 = build_get_nth_item(elist[i].data, 1);
            int new_size = build_get_nth_item_offset(elist[i].data, 2) - build_get_nth_item_offset(elist[i].data, 1);

            // zone position
            *(int *)(item1 + 0x0) *= -1;
            *(int *)(item1 + 0x0) -= from_s32(item1 + 0xC);

            // flip collision
            int maxx = from_u16(item1 + 0x1E);
            int maxy = from_u16(item1 + 0x20);
            int maxz = from_u16(item1 + 0x22);
            printf("x flipping coll in zone %s, %d %d %d\n", eid_conv2(elist[i].eid), maxx, maxy, maxz);
            unsigned char *item1_edited = coll_get_expanded(item1, &new_size, maxx, maxy, maxz);
            LIST flipped = init_list();
            collision_flip(item1_edited, 0x1C, 0, maxx, maxy, maxz, &flipped, 1);
            build_replace_item(&elist[i], 1, item1_edited, new_size);

            // object positions
            int cam_item_count = build_get_cam_item_count(elist[i].data);
            int entity_count = build_get_entity_count(elist[i].data);

            for (int j = 0; j < cam_item_count / 3; j++)
            {
                unsigned char *cam_item0 = build_get_nth_item(elist[i].data, 2 + 3 * j);
                flip_entity_x(cam_item0, from_s32(item1 + 0xC));
                flip_fix_prop_198_x(cam_item0);

                unsigned char *cam_item1 = build_get_nth_item(elist[i].data, 2 + 3 * j + 1);
                flip_camera_angle_x(cam_item1);
                flip_camera_dist_2D_x(cam_item0, cam_item1);
                // todo camera switching property fix?
            }

            for (int j = 0; j < entity_count; j++)
            {
                unsigned char *item = build_get_nth_item(elist[i].data, 2 + cam_item_count + j);
                flip_entity_x(item, from_s32(item1 + 0xC) / 4);
                flip_fix_elev_coords_x(item);
            }

            elist[i].chunk = *chunk_count;
            *chunk_count += 1;
        }
    }
}

// mix of rebuild and keeping original stuff
// useful for making operations on a level then saving it again
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
    case Alter_Type_FlipScenY:
        printf("\n!!!Zones are put into indidual chunks at the end\n");
        flip_level_y(elist, entry_count, &chunk_count);
        break;
    case Alter_Type_FlipScenX:
        printf("\n!!!Zones are put into indidual chunks at the end\n");
        flip_level_x(elist, entry_count, &chunk_count);
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
