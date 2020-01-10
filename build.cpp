#include "macros.h"

unsigned int get_model(unsigned char *anim)
{
    return from_u32(anim + 0x10 + from_u32(anim + 0x10));
}

int get_index(unsigned int eid, ENTRY elist[1500], int counter)
{
    int retrn = -1;

    for (int i = 0; i < counter; i++)
        if (elist[i].EID == eid) retrn = i;

    return retrn;
}

unsigned int get_slst(unsigned char *item)
{
    int i, offset = 0;
    for (i = 0; i < item[0xC]; i++)
        if ((from_u32(item + 0x10 + 8 * i) & 0xFFFF) == 0x103)
            offset = 0xC + (((from_u32(item + 0x10 + 8 * i)) & 0xFFFF0000) >> 16);

    if (offset) return from_u32(item + offset + 4);
        else return 0;
}

unsigned int* getrelatives(unsigned char *entry)
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
{
    int curr_off;
    int i, counter = 0;
    char temp[6];
    int curr_eid;
    unsigned int local[256];
    unsigned int *relatives;


    curr_off = from_u32(entry + 0x24);
    while (curr_off < entrysize)
    {
        switch (entry[curr_off])
        {
            case 1:
                curr_eid = from_u32(entry + curr_off + 4);
                eid_conv(curr_eid, temp);
                if (temp[4] == 'G' || temp[4] == 'V')
                    local[counter++] = curr_eid;
                curr_off += 0xC;
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
    }

    if (!counter) return NULL;

    relatives = (unsigned int *) malloc((counter + 1) * sizeof(unsigned int));
    relatives[0] = counter;
    for (i = 0; i < counter; i++) relatives[i + 1] = local[i];

    return relatives;
}

void build_main(char *nsfpath, char *dirpath, int chunkcap, INFO status)
{
    unsigned char *chunks[512];
    FILE *nsf, *file = NULL;
    DIR *df;
    unsigned int new_relatives[50];
    unsigned char chunk[CHUNKSIZE], entry[CHUNKSIZE];
    ENTRY elist[1500];
    char temp[MAX];
    int chunkindex = 0, fsize, offset, index = 0, j, i, k, chunkcount, help, help2;
    struct dirent *de;

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
                elist[index].chunk = i * 2 + 1;
                elist[index++].esize = -1;
            }
    }

    while ((de = readdir(df)) != NULL)
    if ((de->d_name)[0] != '.')
    {
        if (file != NULL) fclose(file);
        sprintf(temp, "%s\\%s",dirpath, de->d_name);
        file = fopen(temp, "rb");
        fseek(file, 0, SEEK_END);
        fsize = ftell(file);
        rewind(file);
        fread(entry, fsize, sizeof(unsigned char), file);
        if (fsize == CHUNKSIZE && from_u32(entry) == MAGIC_TEXTURE)
        {
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

    for (i = 0; i < index; i++)
    {
        if ((elist[i].related != NULL) && (from_u32(elist[i].data + 8) == 0xB))
        {
            help2 = 0;
            for (j = 0; (unsigned) j < elist[i].related[0]; j++)
                if ((help = get_index(elist[i].related[j + 1], elist, index)) > 0)
                    if (elist[help].data != NULL)
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

    for (i = 0; i < index; i++)
    {
        if (elist[i].related == NULL) continue;
        for (j = 1; (unsigned) j < elist[i].related[0] + 1; j++)
        {
            if (get_index(elist[i].related[j], elist, index) < 0) elist[i].related[j] = 0;
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

    }

    for (i = 0; i < index; i++)
    {
        printf("%04d %s %02d %d\n", i, eid_conv(elist[i].EID, temp), elist[i].chunk, elist[i].esize);
        if (elist[i].related != NULL)
        {
            printf("------ %05d\n", elist[i].related[0]);
            for (j = 0; j < (signed) elist[i].related[0]; j++)
                printf("--%02d-- %s\n", j + 1, eid_conv(elist[i].related[j+1],temp));
        }
    }

    for (i = 0; i < chunkindex; i++)
        printf("\n%03d: %s",i, eid_conv(from_u32(chunks[i] + 4), temp));

    for (i = 0; i < chunkindex; i++)
        free(chunks[i]);

    for (i = 0; i < index; i++)
        if (elist[i].data != NULL) free(elist[i].data);


    fclose(nsf);
    closedir(df);
}

