#include "macros.h"

int cmpfunc(const void *a, const void *b)
// for qsort
{
   return (*(int*) a - *(int*) b);
}

unsigned int get_model(unsigned char *anim)
// gets a model from an animation (from the first frame)
{
    return from_u32(anim + 0x10 + from_u32(anim + 0x10));
}

int get_chunkindex(unsigned int textr, unsigned char **chunks, int counter)
// returns index of the chunk whose chunk EID is equal to searched texture EID
{
    int i, retrn = -1;

    for (i = 0; i < counter; i++)
        if (from_u32(chunks[i]+4) == textr) retrn = i;

    return retrn;
}

int get_index(unsigned int eid, ENTRY elist[1500], int counter)
// returns index of the struct whose EID is equal to searched EID
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
    int item1len, relcount, item1off, camcount, neighbourcount, scencount, i, index = 0;
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
    if (from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]) != NONE) relcount++;
    relatives = (unsigned int *) malloc(relcount * sizeof(unsigned int));

    relatives[index++] = relcount - 1;

    for (i = 0; i < scencount; i++)
        relatives[index++] = from_u32(entry + item1off + 0x4 + 0x30 * i);

    if (from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]) != NONE)
        relatives[index++] = from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]);

    for (i = 0; i < neighbourcount; i++)
        relatives[index++] = from_u32(entry + item1off + 0x194 + 4*i);

    for (i = 0; i < (camcount/3); i++)
        relatives[index++] = get_slst(entry + from_u32(entry + 0x18 + 0xC * i));

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
    int chunkindex = 0, fsize, offset, index = 0, chunkcount, help, help2, help3, help4, baseborder, chunkborder;
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
        chunks[chunkindex] = (unsigned char*) calloc(CHUNKSIZE, sizeof(unsigned char));
        memcpy(chunks[chunkindex++], chunk, CHUNKSIZE);
        if (chunk[2] != 1)
            for (j = 0; j < chunk[8]; j++)
            {
                offset = 256 * chunk[j * 4 + 0x11] + chunk[j * 4 + 0x10];
                elist[index].EID = from_u32(chunk + offset + 4);
                elist[index].chunk = i;
                elist[index++].esize = -1;
            }
    }

    baseborder = index;

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
            if (get_chunkindex(from_u32(entry + 4), chunks, chunkindex) > 0) continue;
            chunks[chunkindex] = (unsigned char *) calloc(CHUNKSIZE, sizeof(unsigned char));
            memcpy(chunks[chunkindex++], entry, CHUNKSIZE);
            continue;
        }

        if (from_u32(entry) != MAGIC_ENTRY) continue;
        if (get_index(from_u32(entry + 4), elist, index) > 0) continue;

        elist[index].EID = from_u32(entry + 4);
        elist[index].chunk = -1;
        elist[index].esize = fsize;
        elist[index].data = (unsigned char *) malloc(fsize * sizeof(unsigned char));
        if (entry[8] == 7)
            elist[index].related = getrelatives(entry);
        if (entry[8] == 11 && entry[0xC] == 6)
            elist[index].related = GOOL_relatives(entry, fsize);
        memcpy(elist[index++].data, entry, fsize);
    }

    printf("Getting model references.\n");
    // gets model references6
    for (i = 0; i < index; i++)
    {
        if ((elist[i].related != NULL) && (from_u32(elist[i].data + 8) == 0xB))
        {
            help2 = 0;
            for (j = 0; (unsigned) j < elist[i].related[0]; j++)
                if ((help = get_index(elist[i].related[j + 1], elist, index)) > 0)
                    if (elist[help].data != NULL && (from_u32(elist[help].data + 8) == 1))
                        new_relatives[help2++] = get_model(elist[help].data);

            if (help2)
            {
                help = elist[i].related[0];
                elist[i].related = (unsigned int *) realloc(elist[i].related, (help + help2 + 1) * sizeof(unsigned int));
                for (j = 0; j < help2; j++)
                    elist[i].related[j + help + 1] = new_relatives[j];
                elist[i].related[0] += help2;
            }
        }
    }

    printf("Removing invalid references.\n");
    // gets rid of dupes and invalid references, sorts
    for (i = 0; i < index; i++)
    {
        if (elist[i].related == NULL) continue;
        for (j = 1; (unsigned) j < elist[i].related[0] + 1; j++)
        {
            help = get_index(elist[i].related[j], elist, index);
            if (help < baseborder) elist[i].related[j] = 0;
            if (elist[i].related[j] == elist[i].EID) elist[i].related[j] = 0;
            for (k = j + 1; (unsigned) k < elist[i].related[0] + 1; k++)
                if (elist[i].related[j] == elist[i].related[k])
                    elist[i].related[k] = 0;
        }

        help = 0;
        for (j = 1; (unsigned) j < elist[i].related[0] + 1; j++)
            if (elist[i].related[j] != 0) new_relatives[help++] = elist[i].related[j];

        if (help == 0)
        {
            elist[i].related = (unsigned int *) realloc(elist[i].related, 0);
            continue;
        }

        elist[i].related = (unsigned int *) realloc(elist[i].related, (help + 1) * sizeof(unsigned int));
        elist[i].related[0] = help;
        for (j = 0; j < help; j++)
            elist[i].related[j + 1] = new_relatives[j];

        qsort(elist[i].related + 1, elist[i].related[0], sizeof(unsigned int), cmpfunc);
    }

    chunkborder = chunkindex;

    printf("Building T11s' chunks.\n");
    // tries to do T11 and their ralatives' chunk assignment
    for (i = 0; i < index; i++)
    {
        int sizes[10] = {0};
        int counters[10] = {0};
        help4 = 0;
        if ((elist[i].data != NULL) && (from_u32(elist[i].data + 8) == 0xB))
        {
            elist[i].chunk = chunkborder;
            sizes[0] += elist[i].esize;
            help2++;
            if (elist[i].related != NULL)
            for (j = 0; (unsigned) j < elist[i].related[0]; j++)
            {
                help3 = get_index(elist[i].related[j + 1], elist, index);
                for (k = 0; k <= help4; k++)
                    if ((elist[help3].esize + sizes[k] + 0x10 + 4 * (counters[k]+2)) < CHUNKSIZE)
                    {
                        elist[help3].chunk = chunkborder + k;
                        sizes[k] += elist[help3].esize;
                        counters[k]++;
                    }
                    else
                    {
                        help4++;
                        elist[help3].chunk = chunkborder + help4;
                        sizes[help4] += elist[help3].esize;
                        counters[help4]++;
                    }
            }
        chunkborder += help4 + 1;
        }
    }

    //TODO merge sets that share assets somehow (merge the first match probably)

    printf("Merging T11 chunks.\n");
    while(1)
    {
        int help5 = 0;
        for (i = chunkindex; i < chunkborder; i++)
        {
            help = 0;
            help2 = 0;

            for (j = 0; j < index; j++)
                if (elist[j].chunk == i)
                {
                    help += elist[j].esize;
                    help2++;
                }

            int maxsize = 0;
            int maxindex = 0;

            for (j = i + 1; j < chunkborder; j++)
            {
                help3 = 0;
                help4 = 0;
                for (k = 0; k < index; k++)
                    if (elist[k].chunk == j)
                    {
                        help3 += elist[k].esize;
                        help4++;
                    }

                if ((help + help3 + 4 * help2 + 4 * help4 + 0x14) <= CHUNKSIZE)
                    if (help3 > maxsize)
                    {
                        maxsize = help3;
                        maxindex = j;
                    }
            }

            if (maxindex)
            {
                for (j = 0; j < index; j++)
                    if (elist[j].chunk == maxindex) elist[j].chunk = i;
                help5++;
            }
        }
        if (!help5) break;
    }

    printf("Getting rid of empty chunks.\n");
    help2 = 0;
    for (i = chunkindex; i < chunkborder; i++)
    {
        help = 0;
        for (j = 0; j < index; j++)
            if (elist[j].chunk == i) help++;
        if (help) help2 = i;
    }
    chunkborder = help2 + 1;

    printf("Building actual chunks.\n");
    for (i = chunkindex; i < chunkborder; i++)
    {
        help2 = 2 * i + 1;
        help = 0;
        chunks[i] = (unsigned char*) calloc(CHUNKSIZE, sizeof(unsigned char));

        for (j = 0; j < index; j++)
            if (elist[j].chunk == i) help++;

        unsigned int offsets[help + 2];

        chunks[i][0] = 0x34;
        chunks[i][1] = 0x12;
        chunks[i][2] = chunks[i][3] = chunks[i][6] = chunks[i][7] = chunks[i][0xA] = chunks[i][0xB] = 0;
        chunks[i][4] = help2 % 256;
        chunks[i][5] = help2 / 256;
        chunks[i][8] = help % 256;
        chunks[i][9] = help / 256;

        help++;
        help2 = 0;
        offsets[help2] = 0x10 + help*4;

        for (j = 0; j < index; j++)
            if (elist[j].chunk == i)
            {
                offsets[help2 + 1] = offsets[help2] + elist[j].esize;
                help2++;
                //printf("%X\n", offsets[help2]);
            }

        for (j = 0; j < help; j++)
        {
            chunks[i][0x10 + j * 4] = offsets[j] % 256;
            chunks[i][0x11 + j * 4] = offsets[j] / 256;
        }

        help3 = offsets[0];
        for (j = 0; j < index; j++)
            if (elist[j].chunk == i)
            {
                //printf("%02d %d\n", i, help3);
                memcpy(chunks[i] + help3, elist[j].data, elist[j].esize);
                help3 += elist[j].esize;
            }

        unsigned int checksum = nsfChecksum(chunks[i]);

        for (j = 0; j < 4; j++)
        {
            chunks[i][0xC + j] = checksum % 256;
            checksum /= 256;
        }
    }

    // prints stuff
   for (i = 0; i < index; i++)
    {
        printf("%04d %s %02d %d\n", i, eid_conv(elist[i].EID, temp), elist[i].chunk, elist[i].esize);
        if (elist[i].related != NULL)
        {
            printf("------ %05d\n", elist[i].related[0]);
            for (j = 0; j < (signed) elist[i].related[0]; j++)
            {
                help3 = elist[i].related[j+1];
                printf("--%02d-- %s %02d %05d\n", j + 1, eid_conv(help3,temp),
                       elist[get_index(help3, elist, index)].chunk, elist[get_index(help3, elist, index)].esize);
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

    for (i = 0; i < index; i++)
        if (elist[i].data != NULL) free(elist[i].data);

    fclose(nsfnew);
    fclose(nsf);
    closedir(df);
}

