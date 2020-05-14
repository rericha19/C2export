#include "macros.h"

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

int get_chunk_index(unsigned int textr, unsigned char **chunks, int counter)
// returns entry_count of the chunk whose chunk EID is equal to searched texture EID
{
    int i, retrn = -1;

    for (i = 0; i < counter; i++)
        if (from_u32(chunks[i]+4) == textr) retrn = i;

    return retrn;
}

int get_index(unsigned int eid, ENTRY elist[1500], int counter)
// returns entry_count of the struct whose EID is equal to searched EID
{
    int retrn = -1;

    for (int i = 0; i < counter; i++)
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

unsigned int* getrelatives(unsigned char *entry)
// gets zone's relatives
{
    int item1len, relcount, item1off, camcount, neighbourcount, scencount, i, entry_count = 0;
    unsigned int* relatives;

    item1off = from_u32(entry + 0x10);
    item1len = from_u32(entry + 0x14) - item1off;
    if (!(item1len == 0x358 || item1len == 0x318)) return NULL;

    camcount = entry[item1off + 0x188];
    scencount = entry[item1off];
    if (camcount == 0) return NULL;
    neighbourcount = entry[item1off + 0x190];
    relcount = camcount/3 + neighbourcount + scencount + 1;

    if (relcount == 1) return NULL;

    /*if (from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]) != NONE)
        relcount++;*/
    relatives = (unsigned int *) malloc(relcount * sizeof(unsigned int));

    relatives[entry_count++] = relcount - 1;

    for (i = 0; i < (camcount/3); i++)
        relatives[entry_count++] = get_slst(entry + from_u32(entry + 0x18 + 0xC * i));

    /*if (from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]) != NONE)
        relatives[entry_count++] = from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]);*/

    for (i = 0; i < neighbourcount; i++)
        relatives[entry_count++] = from_u32(entry + item1off + 0x194 + 4*i);

    for (i = 0; i < scencount; i++)
        relatives[entry_count++] = from_u32(entry + item1off + 0x4 + 0x30 * i);

    return relatives;
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

void build_main(char *nsfpath, char *dirpath, int chunkcap, INFO status, char *time)
{
    unsigned char *chunks[512];
    FILE *nsf = NULL, *file = NULL, *nsfnew = NULL;
    DIR *df = NULL;
    unsigned int new_relatives[100];
    unsigned char chunk[CHUNKSIZE], entry[CHUNKSIZE];
    ENTRY elist[2500];
    char temp[MAX], *temp1, *temp2, lcltemp[MAX];
    int chunk_index = 0, fsize, offset, entry_count = 0, chunkcount, help3, baseborder, chunkborder, chunk_index2;
    register int i,j,k;
    struct dirent *de;


    // opening and stuff
    if ((nsf = fopen(nsfpath,"rb")) == NULL)
    {
        printf("[ERROR] Could not open selected NSF\n");
        return;
    }

    if ((df = opendir(dirpath)) == NULL)
    {
        printf("[ERROR] Could not open selected directory\n");
        fclose(nsf);
        return;
    }

    fseek(nsf, 0, SEEK_END);
    chunkcount = ftell(nsf) / CHUNKSIZE;
    rewind(nsf);

    printf("\nReading from the NSF.\n");
    // getting stuff from the base NSF
    for (i = 0; i < chunkcount; i++)
    {
        fread(chunk, CHUNKSIZE, sizeof(unsigned char), nsf);
        chunks[chunk_index] = (unsigned char*) calloc(CHUNKSIZE, sizeof(unsigned char));
        memcpy(chunks[chunk_index++], chunk, CHUNKSIZE);
        if (chunk[2] != 1)
            for (j = 0; j < chunk[8]; j++)
            {
                offset = 256 * chunk[j * 4 + 0x11] + chunk[j * 4 + 0x10];
                elist[entry_count].EID = from_u32(chunk + offset + 4);
                elist[entry_count].chunk = i;
                elist[entry_count++].esize = -1;
            }
    }

    baseborder = entry_count;

    printf("Reading from the folder.\n");
    // getting stuff from the folder
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
            if (get_chunk_index(from_u32(entry + 4), chunks, chunk_index) > 0) continue;
            chunks[chunk_index] = (unsigned char *) calloc(CHUNKSIZE, sizeof(unsigned char));
            memcpy(chunks[chunk_index++], entry, CHUNKSIZE);
            continue;
        }

        if (from_u32(entry) != MAGIC_ENTRY) continue;
        if (get_index(from_u32(entry + 4), elist, entry_count) > 0) continue;

        elist[entry_count].EID = from_u32(entry + 4);
        elist[entry_count].chunk = -1;
        elist[entry_count].esize = fsize;
        elist[entry_count].data = (unsigned char *) malloc(fsize * sizeof(unsigned char));
        if (entry[8] == 7)
            elist[entry_count].related = getrelatives(entry);
        if (entry[8] == 11 && entry[0xC] == 6)
            elist[entry_count].related = GOOL_relatives(entry, fsize);
        memcpy(elist[entry_count++].data, entry, fsize);
    }

    printf("Getting model references.\n");
    // gets model references
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

    printf("Removing invalid references.\n");
    // gets rid of dupes and invalid references
    for (i = 0; i < entry_count; i++)
    {
        if (elist[i].related == NULL) continue;
        for (j = 1; (unsigned) j < elist[i].related[0] + 1; j++)
        {
            int relative_index;
            relative_index = get_index(elist[i].related[j], elist, entry_count);
            if (relative_index < baseborder) elist[i].related[j] = 0;
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

    chunkborder = chunk_index;
    qsort(elist, entry_count, sizeof(ENTRY), cmp_entry);


    printf("Building T11s' chunks.\n");
    // tries to do T11 and their relatives' chunk assignment
    for (i = 0; i < entry_count; i++)
    {
        int size;
        int counter;
        int relative_index;
        if ((elist[i].data != NULL) && (from_u32(elist[i].data + 8) == 0xB))
        {
            elist[i].chunk = chunkborder;
            size = elist[i].esize;
            counter = 0;
            if (elist[i].related != NULL)
            for (j = 0; (unsigned) j < elist[i].related[0]; j++)
            {
                relative_index = get_index(elist[i].related[j + 1], elist, entry_count);
                if ((elist[relative_index].esize + size + 0x10 + 4 * (counter + 2)) > CHUNKSIZE)
                {
                    chunkborder++;
                    counter = 0;
                    size = 0;
                }
                elist[relative_index].chunk = chunkborder;
                size += elist[relative_index].esize;
                counter++;
            }
            chunkborder++;
        }
    }

    /*printf("Merging T11 chunks that share assets\n");
    for (i = chunk_index; i < chunkborder; i++)
    {
        for (j = 0; j < entry_count; j++) ;
    }*/

    printf("Merging T11 chunks.\n");
    while(1)
    {
        int merge_happened = 0;
        for (i = chunk_index; i < chunkborder; i++)
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

            for (j = i + 1; j < chunkborder; j++)
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

    printf("Getting rid of empty chunks.\n");
    chunkborder = remove_empty_chunks(chunk_index, chunkborder, entry_count, elist);
    chunk_index2 = chunkborder;


    printf("Building zones' chunks.\n");
    // tries to do zones and their relatives' chunk assignment
    for (i = 0; i < entry_count; i++)
    {
        int size = 0;
        int counter = 0;
        int relative_index;
        if ((elist[i].data != NULL) && (from_u32(elist[i].data + 8) == 0x7) && elist[i].related != NULL)
        {
            if (elist[i].chunk == -1)
                elist[i].chunk = chunkborder;
            size += elist[i].esize;
            if (elist[i].related != NULL)
                for (j = 0; (unsigned) j < elist[i].related[0]; j++)
                {
                    relative_index = get_index(elist[i].related[j + 1], elist, entry_count);
                    if (elist[relative_index].chunk != -1 || elist[relative_index].related != NULL) continue;
                    if ((elist[relative_index].esize + size + 0x10 + 4 * (counter + 2)) > CHUNKSIZE)
                    {
                        chunkborder++;
                        size = 0;
                        counter = 0;
                    }
                    elist[relative_index].chunk = chunkborder;
                    size += elist[relative_index].esize;
                    counter++;
                }
            chunkborder++;
        }
    }


    printf("Merging zone chunks.\n");
    while(1)
    {
        int merge_happened = 0;
        for (i = chunk_index2; i < chunkborder; i++)
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

            for (j = i + 1; j < chunkborder; j++)
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

    printf("Getting rid of empty chunks.\n");
    chunkborder = remove_empty_chunks(chunk_index2, chunkborder, entry_count, elist);


    printf("Building actual chunks.\n");
    for (i = chunk_index; i < chunkborder; i++)
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
            if (elist[j].chunk == i)
            {
                memcpy(chunks[i] + curr_offset, elist[j].data, elist[j].esize);
                curr_offset += elist[j].esize;
            }

        // writes checksum
        *((unsigned int *)(chunks[i] + 0xC)) = nsfChecksum(chunks[i]);
    }

    // prints stuff
   for (i = 0; i < entry_count; i++)
    {
        printf("%04d %s %02d %d\n", i, eid_conv(elist[i].EID, temp), elist[i].chunk, elist[i].esize);
        if (elist[i].related != NULL)
        {
            printf("------ %5d\n", elist[i].related[0]);
            for (j = 0; j < (signed) elist[i].related[0]; j++)
            {
                help3 = elist[i].related[j+1];
                printf("--%2d-- %s %2d %5d\n", j + 1, eid_conv(help3, temp),
                       elist[get_index(help3, elist, entry_count)].chunk, elist[get_index(help3, elist, entry_count)].esize);
            }
        }
    }

    for (i = 0; i < chunkborder; i++)
        printf("%03d: %s\n",i, eid_conv(from_u32(chunks[i] + 4), temp));


    printf("Writing a new NSF.\n");
    // writes into a new nsf
    temp1 = strrchr(nsfpath,'\\');
    *temp1 = '\0';
    temp2 = temp1 + 1;

    sprintf(lcltemp,"%s\\%s_%s", nsfpath, time, temp2);
    nsfnew = fopen(lcltemp, "wb");

    for (i = 0; i < chunkborder; i++)
        fwrite(chunks[i], sizeof(unsigned char), CHUNKSIZE, nsfnew);

    // frees stuff and closes stuff
    for (i = 0; i < chunkborder; i++)
        free(chunks[i]);

    for (i = 0; i < entry_count; i++)
        if (elist[i].data != NULL) free(elist[i].data);

    fclose(nsfnew);
    fclose(nsf);
    closedir(df);
}

