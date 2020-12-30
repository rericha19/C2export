#include "macros.h"


/** \brief
 *  Creates and builds chunks for the instrument entries according to their format.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_count int*              chunk count
 * \param chunks unsigned char**        chunks, array of ptrs to data, allocated here
 * \return void
 */
void build_instrument_chunks(ENTRY *elist, int entry_count, int *chunk_count, unsigned char** chunks) {
    int count = *chunk_count;
    for (int i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_INST) {
            elist[i].chunk = count;
            chunks[count] = (unsigned char *) calloc(CHUNKSIZE, sizeof(unsigned char));
            *(unsigned short int *)(chunks[count]) = MAGIC_CHUNK;
            *(unsigned short int *)(chunks[count] + 2) = CHUNK_TYPE_INSTRUMENT;
            *(unsigned short int *)(chunks[count] + 4) = 2 * count + 1;

            *(unsigned int *)(chunks[count] + 8) = 1;
            *(unsigned int *)(chunks[count] + 0x10) = 0x24;
            *(unsigned int *)(chunks[count] + 0x14) = 0x24 + elist[i].esize;

            memcpy(chunks[count] + 0x24, elist[i].data, elist[i].esize);
            *((unsigned int *)(chunks[count] + 0xC)) = nsfChecksum(chunks[count]);
            count++;
        }

    *chunk_count = count;
}


/** \brief
 *  Creates and builds sound chunks, formats accordingly.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_count int*              chunk count
 * \param chunks unsigned char**        chunks, array of ptrs to data, allocated here
 * \return void
 */
void build_sound_chunks(ENTRY *elist, int entry_count, int *chunk_count, unsigned char** chunks) {
    int indexer, i, j, count = *chunk_count;
    int sound_entry_count = 0;

    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_SOUND)
            sound_entry_count++;

    ENTRY sound_list[sound_entry_count];

    indexer = 0;
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_SOUND)
            sound_list[indexer++] = elist[i];


    qsort(sound_list, sound_entry_count, sizeof(ENTRY), cmp_entry_size);

    int sizes[8];
    for (i = 0; i < 8; i++)
        sizes[i] = 0x14;

    for (i = 0; i < sound_entry_count; i++)
        for (j = 0; j < 8; j++)
            if (sizes[j] + 4 + (((sound_list[i].esize + 16) >> 4) << 4) <= CHUNKSIZE) {
                sound_list[i].chunk = count + j;
                sizes[j] += 4 + (((sound_list[i].esize + 16) >> 4) << 4);
                break;
            }

    int snd_chunk_count = 0;
    for (i = 0; i < 8; i++)
        if (sizes[i] > 0x14)
            snd_chunk_count = i + 1;

    qsort(sound_list, sound_entry_count, sizeof(ENTRY), cmp_entry_eid);

    for (i = 0; i < snd_chunk_count; i++) {
        int local_entry_count = 0;
        int chunk_no = 2 * (count + i) + 1;
        chunks[count + i] = (unsigned char*) calloc(CHUNKSIZE, sizeof(unsigned char));

        for (j = 0; j < sound_entry_count; j++)
            if (sound_list[j].chunk == count + i)
                local_entry_count++;

        unsigned int offsets[local_entry_count + 2];
        *(unsigned short int *) chunks[count + i] = MAGIC_CHUNK;
        *(unsigned short int *)(chunks[count + i] + 2) = CHUNK_TYPE_SOUND;
        *(unsigned short int *)(chunks[count + i] + 4) = chunk_no;
        *(unsigned short int *)(chunks[count + i] + 8) = local_entry_count;

        indexer = 0;
        offsets[indexer] = build_align_sound(0x10 + (local_entry_count + 1) * 4);

        for (j = 0; j < sound_entry_count; j++)
            if (sound_list[j].chunk == count + i) {
                offsets[indexer + 1] = build_align_sound(offsets[indexer] + sound_list[j].esize);
                indexer++;
            }

        for (j = 0; j < local_entry_count + 1; j++)
            *(unsigned int *) (chunks[count + i] + 0x10 + j * 4) = offsets[j];

        indexer = 0;
        for (j = 0; j < sound_entry_count; j++)
            if (sound_list[j].chunk == count + i)
                memcpy(chunks[count + i] + offsets[indexer++], sound_list[j].data, sound_list[j].esize);

        *(unsigned int*)(chunks[count + i] + 0xC) = nsfChecksum(chunks[count + i]);
    }

    for (i = 0; i < entry_count; i++)
        for (j = 0; j < sound_entry_count; j++)
            if (elist[i].EID == sound_list[j].EID)
                elist[i].chunk = sound_list[j].chunk;

    *chunk_count = count + snd_chunk_count;
}



/** \brief
 *  Returns a value that makes sound entries aligned as they should be (hopefully).
 *
 * \param input int                     input offset
 * \return int                          aligned offset
 */
int build_align_sound(int input) {
    for (int i = 0; i < 16; i++)
        if ((input + i) % 16 == 8)
            return (input + i);

    return input;
}

