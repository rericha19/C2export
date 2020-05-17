#include "macros.h"
#define PRINTING 0

int cmpfunc(const void *a, const void *b)
// for qsort
{
   return (*(int*) a - *(int*) b);
}

int to_enum(const void *a)
{
    ENTRY x = *((ENTRY *) a);
    int result = 0;

    if (x.data == NULL) return 0;

    switch(*(int *)(x.data + 0x8))
    {
        case 0xB:
            result += 1;
            break;
        case 0x2:
            result += 2;
            break;
        case 0x1:
            result += 3;
            break;
        case 0x7:
            result += 4;
            break;
        case 0x4:
            result += 5;
            break;
        case 0x3:
            result += 6;
            break;
        default:
            result += MAX;
            break;
    }

    return result;
}

int cmp_entry(const void *a, const void *b)
{
    int x = to_enum(a);
    int y = to_enum(b);

    if (x != y) return (x - y);

    return ((*(ENTRY*) a).EID - (*(ENTRY *) b).EID);
}

unsigned int get_model(unsigned char *anim)
// gets a model from an animation (from the first frame)
{
    return from_u32(anim + 0x10 + from_u32(anim + 0x10));
}

int remove_empty_chunks(int index_start, int index_end, int entry_count, ENTRY *entry_list)
{
    int i, j, is_occupied = 0, new_max = 0;
    for (i = index_start; i < index_end; i++)
    {
        is_occupied = 0;
        for (j = 0; j < entry_count; j++)
            if (entry_list[j].chunk == i) is_occupied++;
        if (is_occupied) new_max = i;
    }
    return new_max + 1;
}


void remove_invalid_references(ENTRY *elist, int entry_count, int entry_count_base)
{
    int i, j, k;
    unsigned int new_relatives[250];

    for (i = 0; i < entry_count; i++)
    {
        if (elist[i].related == NULL) continue;
        for (j = 1; (unsigned) j < elist[i].related[0] + 1; j++)
        {
            int relative_index;
            relative_index = get_index(elist[i].related[j], elist, entry_count);
            if (relative_index < entry_count_base) elist[i].related[j] = 0;
            if (elist[i].related[j] == elist[i].EID) elist[i].related[j] = 0;

            for (k = j + 1; (unsigned) k < elist[i].related[0] + 1; k++)
                if (elist[i].related[j] == elist[i].related[k])
                    elist[i].related[k] = 0;
        }

        int counter = 0;
        for (j = 1; (unsigned) j < elist[i].related[0] + 1; j++)
            if (elist[i].related[j] != 0) new_relatives[counter++] = elist[i].related[j];

        if (counter == 0)
        {
            elist[i].related = (unsigned int *) realloc(elist[i].related, 0);
            continue;
        }

        elist[i].related = (unsigned int *) realloc(elist[i].related, (counter + 1) * sizeof(unsigned int));
        elist[i].related[0] = counter;
        for (j = 0; j < counter; j++)
            elist[i].related[j + 1] = new_relatives[j];

        //qsort(elist[i].related + 1, elist[i].related[0], sizeof(unsigned int), cmpfunc);
    }
}

int get_base_chunk_border(unsigned int textr, unsigned char **chunks, int counter)
// returns entry_count of the chunk whose chunk EID is equal to searched texture EID
{
    int i, retrn = -1;

    for (i = 0; i < counter; i++)
        if (from_u32(chunks[i]+4) == textr) retrn = i;

    return retrn;
}

void get_model_references(ENTRY *elist, int entry_count, int entry_count_base)
{
    int i, j;
    unsigned int new_relatives[250];
    for (i = 0; i < entry_count; i++)
    {
        if ((elist[i].related != NULL) && (from_u32(elist[i].data + 8) == 0xB))
        {
            int relative_index;
            int new_counter = 0;
            for (j = 0; (unsigned) j < elist[i].related[0]; j++)
                if ((relative_index = get_index(elist[i].related[j + 1], elist, entry_count)) > 0)
                    if (elist[relative_index].data != NULL && (from_u32(elist[relative_index].data + 8) == 1))
                        new_relatives[new_counter++] = get_model(elist[relative_index].data);

            if (new_counter)
            {
                int relative_count;
                relative_count = elist[i].related[0];
                elist[i].related = (unsigned int *) realloc(elist[i].related, (relative_count + new_counter + 1) * sizeof(unsigned int));
                for (j = 0; j < new_counter; j++)
                    elist[i].related[j + relative_count + 1] = new_relatives[j];
                elist[i].related[0] += new_counter;
            }
        }
    }
}

int get_index(unsigned int eid, ENTRY elist[1500], int entry_count)
// returns entry_count of the struct whose EID is equal to searched EID
{
    int retrn = -1;

    for (int i = 0; i < entry_count; i++)
        if (elist[i].EID == eid) retrn = i;

    return retrn;
}

unsigned int get_slst(unsigned char *item)
// gets a slst from a camera item
{
    int i, offset = 0;
    for (i = 0; i < item[0xC]; i++)
        if ((from_u32(item + 0x10 + 8 * i) & 0xFFFF) == 0x103)
            offset = 0xC + (((from_u32(item + 0x10 + 8 * i)) & 0xFFFF0000) >> 16);

    if (offset) return from_u32(item + offset + 4);
        else return 0;
}

int* seek_spawn(unsigned char *item)
{
    int i, code, offset, type = -1, subtype = -1, coords_offset = -1;
    for (i = 0; (unsigned) i < from_u32(item + 0xC); i++)
    {
        code = from_u16(item + 0x10 + 8*i);
        offset = from_u16(item + 0x12 + 8*i) + OFFSET;
        if (code == 0xA9)
            type = from_u32(item + offset + 4);
        if (code == 0xAA)
            subtype = from_u32(item + offset + 4);
        if (code == 0x4B)
            coords_offset = offset;
    }

    if ((!type && !subtype && coords_offset != -1) || (type == 34 && subtype == 4 && coords_offset != -1))
    {
        int *coords = (int*) malloc(3 * sizeof(int));
        for (i = 0; i < 3; i++)
            coords[i] = from_u16(item + coords_offset + 4 + 2 * i) * 4;
        return coords;
    }

    return NULL;
}

unsigned int* getrelatives(unsigned char *entry, SPAWNS *spawns)
// gets zone's relatives
{
    int item1len, relcount, item1off, camcount, neighbourcount, scencount, i, entry_count = 0;
    unsigned int* relatives;

    item1off = from_u32(entry + 0x10);
    item1len = from_u32(entry + 0x14) - item1off;
    if (!(item1len == 0x358 || item1len == 0x318)) return NULL;

    camcount = entry[item1off + 0x188];
    int entity_count = entry[item1off + 0x18C];
    scencount = entry[item1off];
    if (camcount == 0) return NULL;
    neighbourcount = entry[item1off + 0x190];
    relcount = camcount/3 + neighbourcount + scencount + 1;

    if (relcount == 1) return NULL;

    if (from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]) != NONE)
        relcount++;
    relatives = (unsigned int *) malloc(relcount * sizeof(unsigned int));

    relatives[entry_count++] = relcount - 1;

    for (i = 0; i < (camcount/3); i++)
        relatives[entry_count++] = get_slst(entry + from_u32(entry + 0x18 + 0xC * i));

    for (i = 0; i < neighbourcount; i++)
        relatives[entry_count++] = from_u32(entry + item1off + 0x194 + 4*i);

    for (i = 0; i < scencount; i++)
        relatives[entry_count++] = from_u32(entry + item1off + 0x4 + 0x30 * i);

    if (from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]) != NONE)
        relatives[entry_count++] = from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]);

    for (i = 0; i < entity_count; i++)
    {
        int *coords_ptr;
        coords_ptr = seek_spawn(entry + from_u32(entry + 0x18 + 4 * camcount + 4 * i));
        if (coords_ptr != NULL)
        {
            spawns->spawn_count += 1;
            int cnt = spawns->spawn_count;
            spawns->spawns = (SPAWN *) realloc(spawns->spawns, cnt * sizeof(SPAWN));
            spawns->spawns[cnt -1].x = coords_ptr[0] + from_u32(entry + from_u32(entry + 0x14));
            spawns->spawns[cnt -1].y = coords_ptr[1] + from_u32(entry + from_u32(entry + 0x14) + 4);
            spawns->spawns[cnt -1].z = coords_ptr[2] + from_u32(entry + from_u32(entry + 0x14) + 8);
            spawns->spawns[cnt -1].zone = from_u32(entry + 0x4);
        }
    }
    return relatives;
}

int entry_type(ENTRY entry)
{
    if (entry.data == NULL) return -1;

    return *(int *)(entry.data + 8);
}

unsigned int* GOOL_relatives(unsigned char *entry, int entrysize)
// gets gool relatives
{
    int curr_off, type = 0, help;
    int i, counter = 0;
    char temp[6];
    int curr_eid;
    unsigned int local[256];
    unsigned int *relatives = NULL;

    curr_off = from_u32(entry + 0x24);

    while (curr_off < entrysize)
        switch (entry[curr_off])
        {
            case 1:
                if (!type)
                {
                    help = from_u32(entry + curr_off + 0xC) & 0xFF00FFFF;
                    if (help <= 05 && help > 0) type = 2;
                        else type = 3;
                }

                if (type == 2)
                {
                    curr_eid = from_u32(entry + curr_off + 4);
                    eid_conv(curr_eid, temp);
                    if (temp[4] == 'G' || temp[4] == 'V')
                        local[counter++] = curr_eid;
                    curr_off += 0xC;
                }
                else
                {
                    for (i = 0; i < 4; i++)
                    {
                        curr_eid = from_u32(entry + curr_off + 4 + 4*i);
                        eid_conv(curr_eid, temp);
                        if (temp[4] == 'G' || temp[4] == 'V')
                            local[counter++] = curr_eid;
                    }
                    curr_off += 0x14;
                }
                break;
            case 2:
                curr_off += 0x8 + 0x10 * entry[curr_off + 2];
                break;
            case 3:
                curr_off += 0x8 + 0x14 * entry[curr_off + 2];
                break;
            case 4:
                curr_off = entrysize;
                break;
            case 5:
                curr_off += 0xC + 0x18 * entry[curr_off + 2] * entry[curr_off + 8];
                break;
        }


    if (!counter) return NULL;

    relatives = (unsigned int *) malloc((counter + 1) * sizeof(unsigned int));
    relatives[0] = counter;
    for (i = 0; i < counter; i++) relatives[i + 1] = local[i];

    return relatives;
}

void read_nsf(ENTRY *elist, int chunk_border_base, unsigned char **chunks, int *chunk_border_texture, int *entry_count, FILE *nsf, unsigned int *gool_table)
{
    int i, j , offset;
    unsigned char chunk[CHUNKSIZE];

    for (i = 0; i < chunk_border_base; i++)
    {
        fread(chunk, CHUNKSIZE, sizeof(unsigned char), nsf);
        chunks[*chunk_border_texture] = (unsigned char*) calloc(CHUNKSIZE, sizeof(unsigned char));
        memcpy(chunks[*chunk_border_texture], chunk, CHUNKSIZE);
        (*chunk_border_texture)++;
        if (chunk[2] != 1)
            for (j = 0; j < chunk[8]; j++)
            {
                offset = *(int*) (chunk + 0x10 + j * 4);
                elist[*entry_count].EID = from_u32(chunk + offset + 4);
                elist[*entry_count].chunk = i;
                elist[*entry_count].esize = -1;
                if (*(int *)(chunk + offset + 8) == ENTRY_TYPE_GOOL && *(int*)(chunk + offset + 0xC) > 3)
                {
                    int item1_offset = *(int *)(chunk + offset + 0x10);
                    gool_table[*(int*)(chunk + offset + item1_offset)] = elist[*entry_count].EID;
                }
                (*entry_count)++;
            }
    }
}

void dumb_merge(ENTRY *elist, int chunk_index_start, int *chunk_index_end, int entry_count)
{
    int i, j, k;
    while(1)
    {
        int merge_happened = 0;
        for (i = chunk_index_start; i < *chunk_index_end; i++)
        {
            int size1 = 0;
            int count1 = 0;

            for (j = 0; j < entry_count; j++)
                if (elist[j].chunk == i)
                {
                    size1 += elist[j].esize;
                    count1++;
                }

            int maxsize = 0;
            int maxentry_count = 0;

            for (j = i + 1; j < *chunk_index_end; j++)
            {
                int size2 = 0;
                int count2 = 0;
                for (k = 0; k < entry_count; k++)
                    if (elist[k].chunk == j)
                    {
                        size2 += elist[k].esize;
                        count2++;
                    }

                if ((size1 + size2 + 4 * count1 + 4 * count2 + 0x14) <= CHUNKSIZE)
                    if (size2 > maxsize)
                    {
                        maxsize = size2;
                        maxentry_count = j;
                    }
            }

            if (maxentry_count)
            {
                for (j = 0; j < entry_count; j++)
                    if (elist[j].chunk == maxentry_count) elist[j].chunk = i;
                merge_happened++;
            }
        }
        if (!merge_happened) break;
    }

    *chunk_index_end = remove_empty_chunks(chunk_index_start, *chunk_index_end, entry_count, elist);
}

int is_relative(unsigned int searched, unsigned int *array, int count)
{
    for (int i = 0; i < count; i++)
        if (searched == array[i]) return 1; //(count - i)*(count - i)/2;

    return 0;
}

void gool_merge(ENTRY *elist, int chunk_index_start, int *chunk_index_end, int entry_count)
{
    int i, j, k;
    while(1)
    {
        int merge_happened = 0;
        for (i = chunk_index_start; i < *chunk_index_end; i++)
        {
            int size1 = 0, count1 = 0, max_rating = 0, max_entry_count = 0;;
            unsigned int relatives[250];
            int relative_counter = 0;

            for (j = 0; j < entry_count; j++)
                if (elist[j].chunk == i)
                {
                    size1 += elist[j].esize;
                    count1++;
                    if (elist[j].related != NULL)
                        for (k = 0; (unsigned) k < elist[j].related[0]; k++)
                        {
                            relatives[relative_counter] = elist[j].related[k];
                            relative_counter++;
                        }
                }

            for (j = i + 1; j < *chunk_index_end; j++)
            {
                int size2 = 0;
                int count2 = 0;
                int has_relative = 0;

                for (k = 0; k < entry_count; k++)
                    if (elist[k].chunk == j)
                    {
                        size2 += elist[k].esize;
                        count2++;
                        has_relative += is_relative(elist[k].EID, relatives, relative_counter);
                    }

                if ((size1 + size2 + 4 * count1 + 4 * count2 + 0x14) <= CHUNKSIZE)
                {
                    int rating = has_relative;
                    if (rating > max_rating)
                    {
                        max_rating = rating;
                        max_entry_count = j;
                    }
                }
            }

            if (max_entry_count)
            {
                for (j = 0; j < entry_count; j++)
                    if (elist[j].chunk == max_entry_count) elist[j].chunk = i;
                merge_happened++;
                //printf("merge happened\n");
            }
        }
        if (!merge_happened) break;
    }

    *chunk_index_end = remove_empty_chunks(chunk_index_start, *chunk_index_end, entry_count, elist);
}

void read_folder(DIR *df, char *dirpath, unsigned char **chunks, ENTRY *elist, int *chunk_border_texture, int *entry_count, SPAWNS *spawns, unsigned int* gool_table)
{
    struct dirent *de;
    char temp[500];
    FILE *file = NULL;
    int fsize;
    unsigned char entry[CHUNKSIZE];

    while ((de = readdir(df)) != NULL)
    if ((de->d_name)[0] != '.')
    {
        sprintf(temp, "%s\\%s", dirpath, de->d_name);
        if (file != NULL)
        {
            fclose(file);
            file = NULL;
        }
        if ((file = fopen(temp, "rb")) == NULL) continue;
        fseek(file, 0, SEEK_END);
        fsize = ftell(file);
        rewind(file);
        fread(entry, fsize, sizeof(unsigned char), file);
        if (fsize == CHUNKSIZE && from_u32(entry) == MAGIC_TEXTURE)
        {
            if (get_base_chunk_border(from_u32(entry + 4), chunks, *chunk_border_texture) > 0) continue;
            chunks[*chunk_border_texture] = (unsigned char *) calloc(CHUNKSIZE, sizeof(unsigned char));
            memcpy(chunks[*chunk_border_texture], entry, CHUNKSIZE);
            elist[*entry_count].EID = from_u32(entry + 4);
            elist[*entry_count].chunk = *chunk_border_texture;
            elist[*entry_count].data = NULL;
            elist[*entry_count].related = NULL;
            (*entry_count)++;
            (*chunk_border_texture)++;
            continue;
        }

        if (from_u32(entry) != MAGIC_ENTRY) continue;
        if (get_index(from_u32(entry + 4), elist, *entry_count) > 0) continue;

        elist[*entry_count].EID = from_u32(entry + 4);
        elist[*entry_count].chunk = -1;
        elist[*entry_count].esize = fsize;

        if (entry[8] == 7)
            elist[*entry_count].related = getrelatives(entry, spawns);
        if (entry[8] == 11 && entry[0xC] == 6)
            elist[*entry_count].related = GOOL_relatives(entry, fsize);

        elist[*entry_count].data = (unsigned char *) malloc(fsize * sizeof(unsigned char));
        memcpy(elist[*entry_count].data, entry, fsize);
        if (entry_type(elist[*entry_count]) == ENTRY_TYPE_GOOL && *(elist[*entry_count].data + 8) > 3)
        {
            int item1_offset = *(int *)(elist[*entry_count].data + 0x10);
            gool_table[*(int*)(elist[*entry_count].data + item1_offset)] = elist[*entry_count].EID;
        }
        (*entry_count)++;
    }

    fclose(file);
}


void print(ENTRY *elist, int entry_count)
{
    int i, j;
    char temp[100];
    for (i = 0; i < entry_count; i++)
    {
        printf("%04d %s %02d %d\n", i, eid_conv(elist[i].EID, temp), elist[i].chunk, elist[i].esize);
        if (elist[i].related != NULL)
        {
            printf("------ %5d\n", elist[i].related[0]);
            for (j = 0; j < (signed) elist[i].related[0]; j++)
            {
                int relative = elist[i].related[j+1];
                printf("--%2d-- %s %2d %5d\n", j + 1, eid_conv(relative, temp),
                       elist[get_index(relative, elist, entry_count)].chunk, elist[get_index(relative, elist, entry_count)].esize);
            }
        }
    }
}

void swap_spawns(SPAWNS spawns, int spawnA, int spawnB)
{
    SPAWN temp = spawns.spawns[spawnA];
    spawns.spawns[spawnA] = spawns.spawns[spawnB];
    spawns.spawns[spawnB] = temp;
}

int get_nsd_index(unsigned int searched, ENTRY *elist, int entry_count)
{
    for (int i = 0; i < entry_count; i++)
        if (elist[i].EID == searched) return i;

    return -1;
}

int load_list_sort(const void *a, const void *b)
{
    ITEM x = *(ITEM *) a;
    ITEM y = *(ITEM *) b;

    return (x.index - y.index);
}

void write_nsd(char *path, ENTRY *elist, int entry_count, int chunk_count, SPAWNS spawns, unsigned int* gool_table, int level_ID)
{
    int i, x = 0, input;
    char temp[100];
    FILE *nsd = fopen(path, "wb");
    if (nsd == NULL) return;

    unsigned char* nsddata = (unsigned char*) calloc(CHUNKSIZE, 1);
    *(int *)(nsddata + 0x400) = chunk_count;

    // lets u pick a spawn point
    printf("Pick a spawn:\n");
    for (i = 0; i < spawns.spawn_count; i++)
        printf("Spawn %d:\tZone: %s\n", i + 1, eid_conv(spawns.spawns[i].zone, temp));

    scanf("%d", &input);
    if (input - 1 > spawns.spawn_count || input <= 0)
    {
        printf("No such spawn, defaulting to first one\n");
        input = 1;
    }

    if (input - 1)
        swap_spawns(spawns, 0, input - 1);

    for (i = 0; i < entry_count; i++)
        if (elist[i].chunk != -1)
        {
            *(int *)(nsddata + 0x520 + 8*x) = elist[i].chunk * 2 + 1;
            *(int *)(nsddata + 0x524 + 8*x) = elist[i].EID;
            x++;
        }

    *(int *)(nsddata + 0x404) = x;

    int end = 0x520 + x*8;
    *(int *)(nsddata + end) = spawns.spawn_count;
    *(int *)(nsddata + end + 8) = level_ID;
    end += 0x10;

    for (i = 0; i < 0x40; i++)
        *(int *)(nsddata + end + i * 4) = gool_table[i];

    end += 0x1CC;

    for (i = 0; i < spawns.spawn_count; i++)
    {
        *(int *)(nsddata + end) = spawns.spawns[i].zone;
        *(int *)(nsddata + end + 0x0C) = spawns.spawns[i].x * 256;
        *(int *)(nsddata + end + 0x10) = spawns.spawns[i].y * 256;
        *(int *)(nsddata + end + 0x14) = spawns.spawns[i].z * 256;
        end += 0x18;
    }

    fwrite(nsddata, 1, end, nsd);
    fclose(nsd);

    // sorts load lists
    for (i = 0; i < entry_count; i++)
        if (entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL)
        {
            int item1off = from_u32(elist[i].data + 0x10);
            int cam_count = from_u32(elist[i].data + item1off + 0x188) / 3;

            for (int j = 0; j < cam_count; j++)
            {
                int cam_offset = from_u32(elist[i].data + 0x18 + 0xC * j);
                for (int k = 0; (unsigned) k < from_u32(elist[i].data + cam_offset + 0xC); k++)
                {
                    int code = from_u16(elist[i].data + cam_offset + 0x10 + 8 * k);
                    int offset = from_u16(elist[i].data + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
                    int list_count = from_u16(elist[i].data + cam_offset + 0x16 + 8 * k);
                    if (code == 0x208 || code == 0x209)
                    {
                        int sub_list_offset = offset + 4 * list_count;
                        for (int l = 0; l < list_count; l++)
                        {
                            int item_count = from_u16(elist[i].data + offset + l * 2);
                            ITEM list[item_count];
                            for (int m = 0; m < item_count; m++)
                            {
                                list[m].eid = from_u32(elist[i].data + sub_list_offset + 4 * m);
                                list[m].index = get_nsd_index(list[m].eid, elist, entry_count);
                            }
                            qsort(list, item_count, sizeof(ITEM), load_list_sort);
                            for (int m = 0; m < item_count; m++)
                                *(unsigned int*)(elist[i].data + sub_list_offset + 4 * m) = list[m].eid;
                            sub_list_offset += item_count * 4;
                        }
                    }
                }
            }
        }

}

SPAWNS init_spawns()
{
    SPAWNS temp;
    temp.spawn_count = 0;
    temp.spawns = NULL;

    return temp;
}

PAYLOAD get_payload(ENTRY *elist, int entry_count, LIST list, unsigned int zone)
{
    int chunks[250];
    int count = 0;
    int curr_chunk;
    int is_there;
    char help[100];

    PAYLOAD temp;

    for (int i = 0; i < list.count; i++)
    {
        int elist_index = get_index(list.eids[i], elist, entry_count);
        curr_chunk = elist[elist_index].chunk;
        is_there = 0;
        for (int j = 0; j < count; j++)
            if (chunks[j] == curr_chunk) is_there = 1;

        if (!is_there && eid_conv(elist[elist_index].EID, help)[4] != 'T' && curr_chunk != -1)
        {
            chunks[count] = curr_chunk;
            count++;
        }
    }

    temp.zone = zone;
    temp.count = count;
    temp.chunks = (int *) malloc(count * sizeof(int));
    memcpy(temp.chunks, chunks, sizeof(int) * count);

    return temp;
}

LIST init_list()
{
    LIST list;
    list.count = 0;
    list.eids = NULL;

    return list;
}

int list_comp(const void *a, const void *b)
{
    unsigned int x = *(unsigned int*) a;
    unsigned int y = *(unsigned int*) b;

    return (x - y);
}

int list_find(LIST list, unsigned int searched)
{
    int first = 0;
    int last = list.count - 1;
    int middle = (first + last)/2;

    while (first <= last)
    {
        if (list.eids[middle] < searched)
            first = middle + 1;
        else if (list.eids[middle] == searched)
            return middle;
        else
            last = middle - 1;

        middle = (first + last)/2;
    }

    return -1;
}

void list_add(LIST *list, unsigned int eid)
{
    list->eids = (unsigned int *) realloc(list->eids, (list->count + 1) * sizeof(unsigned int *));
    list->eids[list->count] = eid;
    list->count++;
    qsort(list->eids, list->count, sizeof(unsigned int), list_comp);
}

void list_rem(LIST *list, unsigned int eid)
{
    int index = list_find(*list, eid);
    if (index == -1) return;

    list->eids[index] = list->eids[list->count - 1];
    list->eids = (unsigned int *) realloc(list->eids, (list->count - 1) * sizeof(unsigned int *));
    list->count--;
    qsort(list->eids, list->count, sizeof(unsigned int), list_comp);
}

void list_insert(LIST *list, unsigned int eid)
{
    if (list_find(*list, eid) == -1)
        list_add(list, eid);
}

void print_payload(PAYLOAD payload)
{
    char temp[100];
    printf("Zone: %s; payload: %3d\n", eid_conv(payload.zone, temp), payload.count);
}

LOAD_LIST init_load_list()
{
    LOAD_LIST temp;
    temp.count = 0;

    return temp;
}

int comp(const void *a, const void *b)
{
    LOAD x = *(LOAD *) a;
    LOAD y = *(LOAD *) b;

    return (x.index - y.index);
}

int pay_cmp(const void *a, const void *b)
{
    PAYLOAD x = *(PAYLOAD *) a;
    PAYLOAD y = *(PAYLOAD *) b;

    return (y.count - x.count);
}

void insert_payload(PAYLOADS *payloads, PAYLOAD insertee)
{
    for (int i = 0; i < payloads->count; i++)
        if (payloads->arr[i].zone == insertee.zone)
        {
            if (payloads->arr[i].count < insertee.count)
            {
                    payloads->arr[i].count = insertee.count;
                    free(payloads->arr[i].chunks);
                    payloads->arr[i].chunks = insertee.chunks;
                    return;
            }
            else return;
        }

    payloads->arr = (PAYLOAD *) realloc(payloads->arr, (payloads->count + 1) * sizeof(PAYLOAD));
    payloads->arr[payloads->count] = insertee;
    (payloads->count)++;
}

PAYLOADS max_payload(ENTRY *elist, int entry_count)
{
    PAYLOADS payloads;
    payloads.count = 0;
    payloads.arr = NULL;
    int i, j, k, l, m;
    for (i = 0; i < entry_count; i++)
        if (entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL)
        {
            int item1off = from_u32(elist[i].data + 0x10);
            int cam_count = from_u32(elist[i].data + item1off + 0x188) / 3;

            for (j = 0; j < cam_count; j++)
            {
                int cam_offset = from_u32(elist[i].data + 0x18 + 0xC * j);
                LOAD_LIST load_list = init_load_list();
                LIST list = init_list();
                PAYLOAD payload;
                for (k = 0; (unsigned) k < from_u32(elist[i].data + cam_offset + 0xC); k++)
                {
                    int code = from_u16(elist[i].data + cam_offset + 0x10 + 8 * k);
                    int offset = from_u16(elist[i].data + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
                    int list_count = from_u16(elist[i].data + cam_offset + 0x16 + 8 * k);
                    if (code == 0x208 || code == 0x209)
                    {
                        int sub_list_offset = offset + 4 * list_count;
                        int point;
                        int load_list_item_count;
                        for (l = 0; l < list_count; l++, sub_list_offset += load_list_item_count * 4)
                        {
                            load_list_item_count = from_u16(elist[i].data + offset + l * 2);
                            point = from_u16(elist[i].data + offset + l * 2 + list_count * 2);

                            load_list.array[load_list.count].list_length = load_list_item_count;
                            load_list.array[load_list.count].list = (unsigned int *) malloc(load_list_item_count * sizeof(unsigned int));
                            memcpy(load_list.array[load_list.count].list, elist[i].data + sub_list_offset, load_list_item_count * sizeof(unsigned int*));
                            if (code == 0x208)
                                load_list.array[load_list.count].type = 'A';
                            else
                                load_list.array[load_list.count].type = 'B';
                            load_list.array[load_list.count].index = point;
                            load_list.count++;
                        }
                    }
                    qsort(load_list.array, load_list.count, sizeof(LOAD), comp);
                }

                // if (load_list.count) printf("CAMERA\n");
                for (l = 0; l < load_list.count; l++)
                    {
                        if (load_list.array[l].type == 'A')
                            for (m = 0; m < load_list.array[l].list_length; m++)
                                list_add(&list, load_list.array[l].list[m]);

                        if (load_list.array[l].type == 'B')
                            for (m = 0; m < load_list.array[l].list_length; m++)
                                list_rem(&list, load_list.array[l].list[m]);

                        payload = get_payload(elist, entry_count, list, elist[i].EID);
                        insert_payload(&payloads, payload);
                    }
            }
        }

    return payloads;
}


void combinationUtil(int arr[], int data[], int start, int end, int index, int r, int **res, int *counter)
{
    if (index == r)
    {
        memcpy(res[*counter], data, r * sizeof(int));
        (*counter)++;
        // printf("r: %d, reeee %d\n", r, *counter);
        return;
    }

    for (int i = start; i <= end && end - i + 1 >= r - index; i++)
    {
        data[index] = arr[i];
        combinationUtil(arr, data, i+1, end, index+1, r, res, counter);
    }
}

void getCombinations(int arr[], int n, int r, int **res, int *counter)
{
    int data[r];
    combinationUtil(arr, data, 0, n - 1, 0, r, res, counter);
}

long long int fact(int n)
{
    long long int result = 1;
    for (int i = 1; i <= n; i++)
        result *= i;

    return result;
}

void chunk_merge(ENTRY *elist, int entry_count, int *chunks, int chunk_count)
{
    for (int i = 0; i < entry_count; i++)
        for (int j = 0; j < chunk_count; j++)
            if (elist[i].chunk == chunks[j])
                elist[i].chunk = chunks[0];

    //for (int i = 0; i < chunk_count; i++)
    //printf("%3d ", chunks)
}

int merge_thing(ENTRY *elist, int entry_count, int *chunks, int chunk_count)
{
    int i, j, k, l;
    int max_size, size;
    int max_comb = 0;
    int *best = NULL;

    for (i = 2; i < chunk_count; i++)
    {
        int combination_count = fact(chunk_count)/(fact(i) * fact(chunk_count - i));
        int counter = 0;

        int **combinations = (int **) malloc(combination_count * sizeof(int *));
        for (j = 0; j < combination_count; j++)
            combinations[j] = (int *) malloc(i * sizeof(int));
        getCombinations(chunks, chunk_count, i, combinations, &counter);

        for (j = 0; j < combination_count; j++)
        {
            size = 0x14;
            for (k = 0; k < i; k++)
                for (l = 0; l < entry_count; l++)
                    if (elist[l].chunk == combinations[j][k])
                        size += 4 + elist[l].esize;


            if (size <= CHUNKSIZE && i > max_comb)
            {
                max_size = size;
                free(best);
                best = (int *) malloc(i * sizeof(int));
                for (k = 0; k < i; k++)
                    best[k] = combinations[j][k];
                max_comb = i;
            }
        }

        for (j = 0; j < combination_count; j++)
            free(combinations[j]);
        free(combinations);
    }

    if (max_comb)
    {
        chunk_merge(elist, entry_count, best, max_comb);
        return 1;
    }

    return 0;
}

void load_list_merge(ENTRY *elist, int entry_count, int chunk_min, int *chunk_count)
{
    unsigned int prev = 0;
    int payload = 0;

    for (int x = 0; x < 1000; x++)
    {
        //printf("AAAAAAAAAAAA\n");
        PAYLOADS payloads = max_payload(elist, entry_count);
        qsort(payloads.arr, payloads.count, sizeof(PAYLOAD), pay_cmp);

        if (payloads.arr[0].count < 20) break;
            print_payload(payloads.arr[0]);

        if (prev == payloads.arr[0].zone && payload == payloads.arr[0].count)
            break;

        prev = payloads.arr[0].zone;
        payload = payloads.arr[0].count;

        qsort(payloads.arr[0].chunks, payloads.arr[0].count, sizeof(int), cmpfunc);

        for (int i = 0; i < payloads.arr[0].count; i++)
            printf("%3d ", payloads.arr[0].chunks[i] * 2 + 1);
        printf("\n");

        int chunks[100];
        int count;
        int check = 0;

        for (int i = 0; i < payloads.count && check != 1; i++)
        {
            check = 0;
            count = 0;

            for (int j = 0; j < payloads.arr[i].count; j++)
            {
                int curr_chunk = payloads.arr[i].chunks[j];
                if (curr_chunk >= chunk_min)
                    chunks[count++] = curr_chunk;
            }
            qsort(chunks, count, sizeof(int), cmpfunc);
            if (merge_thing(elist, entry_count, chunks, count))
                check = 1;

            for (int j = chunk_min; j < *chunk_count; j++)
            {
                int empty = 1;
                for (int k = 0; k < entry_count; k++)
                    if (elist[k].chunk == j)
                        empty = 0;

                if (empty)
                {
                    for (int k = 0; k < entry_count; k++)
                        if (elist[k].chunk == (*chunk_count - 1))
                            elist[k].chunk = j;
                    (*chunk_count)--;
                }
            }
        }
    }
}

void build_main(char *nsfpath, char *dirpath, int chunkcap, INFO status, char *time)
{
    DIR *df = NULL;
    FILE *nsf = NULL, *nsfnew = NULL;
    SPAWNS spawns = init_spawns();

    char temp[MAX], lcltemp[MAX];
    unsigned int gool_table[0x40];
    for (int i = 0; i < 0x40; i++)
        gool_table[i] = NONE;

    unsigned char *chunks[512];
    ENTRY elist[2500];

    int i, j, level_ID;
    int chunk_border_base       = 0,
        chunk_border_texture    = 0,
        chunk_border_gool       = 0,
        chunk_border_instruments= 0,
        chunk_count             = 0,
        entry_count_base        = 0,
        entry_count             = 0;


    // opening and stuff
    if ((nsf = fopen(nsfpath,"rb")) == NULL)
    {
        {
            printf("[ERROR] Could not open selected NSF\n");
            return;
        }
    }

    if ((df = opendir(dirpath)) == NULL)
    {
        printf("[ERROR] Could not open selected directory\n");
        fclose(nsf);
        return;
    }

    printf("\nWhat ID do you want your level to have? [CAN OVERWRITE EXISTING FILES!]\n");
    scanf("%x", &level_ID);
    if (level_ID < 0 || level_ID > 0x3F)
    {
        printf("Invalid ID, defaulting to 0\n");
        level_ID = 0;
    }

    fseek(nsf, 0, SEEK_END);
    chunk_border_base = ftell(nsf) / CHUNKSIZE;
    rewind(nsf);

    printf("\nReading from the NSF.\n");
    read_nsf(elist, chunk_border_base, chunks, &chunk_border_texture, &entry_count, nsf, gool_table);
    entry_count_base = entry_count;

    printf("Reading from the folder.\n");
    read_folder(df, dirpath, chunks, elist, &chunk_border_texture, &entry_count, &spawns, gool_table);

    printf("Getting model references.\n");
    get_model_references(elist, entry_count, entry_count_base);

    printf("Removing invalid references.\n");
    remove_invalid_references(elist, entry_count, entry_count_base);
    chunk_count = chunk_border_texture;
    qsort(elist, entry_count, sizeof(ENTRY), cmp_entry);

    printf("Building T11s' chunks.\n");
    // tries to do T11 and their relatives' chunk assignment
    for (i = 0; i < entry_count; i++)
    {
        int size;
        int counter;
        int relative_index;
        if (entry_type(elist[i]) == ENTRY_TYPE_GOOL)
        {
            elist[i].chunk = chunk_count;
            size = elist[i].esize;
            counter = 0;
            if (elist[i].related != NULL)
            for (j = 0; (unsigned) j < elist[i].related[0]; j++)
            {
                relative_index = get_index(elist[i].related[j + 1], elist, entry_count);
                if ((elist[relative_index].esize + size + 0x10 + 4 * (counter + 2)) > CHUNKSIZE)
                {
                    chunk_count++;
                    counter = 0;
                    size = 0;
                }
                elist[relative_index].chunk = chunk_count;
                size += elist[relative_index].esize;
                counter++;
            }
            chunk_count++;
        }
    }

    // include demo and vcol entries, merge into gool_chunks
    for (i = 0; i < entry_count; i++)
    {
        int type = entry_type(elist[i]);
        if (type == ENTRY_TYPE_DEMO || type == ENTRY_TYPE_VCOL)
            elist[i].chunk = chunk_count++;
    }

    gool_merge(elist, chunk_border_texture, &chunk_count, entry_count);
    dumb_merge(elist, chunk_border_texture, &chunk_count, entry_count);
    chunk_border_gool = chunk_count;


    printf("Building zones' chunks.\n");
    // tries to do zones and their relatives' chunk assignment
    for (i = 0; i < entry_count; i++)
        if (entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].related != NULL)
        {
            if (elist[i].chunk != -1) continue;
            elist[i].chunk = chunk_count;
            int size = elist[i].esize;
            int counter = 1;
            if (elist[i].related != NULL)
                for (j = 0; (unsigned) j < elist[i].related[0]; j++)
                {
                    int relative_index = get_index(elist[i].related[j + 1], elist, entry_count);
                    if (elist[relative_index].chunk != -1 || elist[relative_index].related != NULL) continue;
                    elist[relative_index].chunk = chunk_count++;
                    size += elist[relative_index].esize;
                    counter++;
                }
        }


    load_list_merge(elist, entry_count, chunk_border_gool, &chunk_count);
    dumb_merge(elist, chunk_border_gool, &chunk_count, entry_count);


    // include demo and vcol entries, merge into normal chunks
    for (i = 0; i < entry_count; i++)
    {
        int type = entry_type(elist[i]);
        if (type == ENTRY_TYPE_DEMO || type == ENTRY_TYPE_VCOL)
            elist[i].chunk = chunk_count++;
    }
    dumb_merge(elist, chunk_border_texture, &chunk_count, entry_count);

    // include instrument entries
    for (i = 0; i < entry_count; i++)
        if (entry_type(elist[i]) == ENTRY_TYPE_INST)
            elist[i].chunk = chunk_count++;
    chunk_border_instruments = chunk_count;

    // include sounds, merge sound chunks
    for (i = 0; i < entry_count; i++)
        if (entry_type(elist[i]) == ENTRY_TYPE_SOUND)
            elist[i].chunk = chunk_count++;
    dumb_merge(elist, chunk_border_instruments, &chunk_count, entry_count);

    // only opens the nsf, does not write yet
    *(strrchr(nsfpath,'\\') + 1) = '\0';
    sprintf(lcltemp,"%s\\S00000%02X.NSF", nsfpath, level_ID);
    nsfnew = fopen(lcltemp, "wb");

    // opens nsd, writes it, sorts load lists
    *(strchr(lcltemp, '\0') - 1) = 'D';
    write_nsd(lcltemp, elist, entry_count, chunk_count, spawns, gool_table, level_ID);

    printf("Building actual chunks.\n");
    for (i = chunk_border_texture; i < chunk_count; i++)
    {
        int chunk_no = 2 * i + 1;
        int local_entry_count = 0;
        chunks[i] = (unsigned char*) calloc(CHUNKSIZE, sizeof(unsigned char));

        for (j = 0; j < entry_count; j++)
            if (elist[j].chunk == i) local_entry_count++;

        unsigned int offsets[local_entry_count + 2];

        *(unsigned short int *) chunks[i] = 0x1234;
        *((unsigned short int*) (chunks[i] + 4)) = chunk_no;
        *((unsigned short int*) (chunks[i] + 8)) = local_entry_count;
        if (i >= chunk_border_instruments) *(short int*)(chunks[i] + 2) = 3;

        local_entry_count++;
        int indexer = 0;
        offsets[indexer] = 0x10 + local_entry_count * 4;

        // calculates offsets
        for (j = 0; j < entry_count; j++)
            if (elist[j].chunk == i)
            {
                offsets[indexer + 1] = offsets[indexer] + elist[j].esize;
                indexer++;
            }

        // writes offsets
        for (j = 0; j < local_entry_count; j++)
            *((unsigned int *) (chunks[i] + 0x10 + j * 4)) = offsets[j];

        // writes entries
        int curr_offset = offsets[0];
        for (j = 0; j < entry_count; j++)
        {
            if (elist[j].chunk == i)
            {
                memcpy(chunks[i] + curr_offset, elist[j].data, elist[j].esize);
                curr_offset += elist[j].esize;
            }
            if (entry_type(elist[j]) == ENTRY_TYPE_INST && elist[j].chunk == i)
                *(short int*)(chunks[i] + 2) = 4;
        }

        // writes checksum
        *((unsigned int *)(chunks[i] + 0xC)) = nsfChecksum(chunks[i]);
    }

    // prints entries and their stats and relatives, then chunks and their EIDs
    if (PRINTING)
    {
        print(elist, entry_count);
        for (i = 0; i < chunk_count; i++)
            printf("%03d: %s\n",i, eid_conv(from_u32(chunks[i] + 4), temp));
    }


    printf("Writing new NSF.\n");
    // writes into a new nsf
    for (i = 0; i < chunk_count; i++)
        fwrite(chunks[i], sizeof(unsigned char), CHUNKSIZE, nsfnew);

    // frees stuff and closes stuff
    for (i = 0; i < chunk_count; i++)
        free(chunks[i]);

    for (i = 0; i < entry_count; i++)
        if (elist[i].data != NULL) free(elist[i].data);

    fclose(nsfnew);
    fclose(nsf);
    closedir(df);
}

