#include "macros.h"
// contains functions responsible for creating sound and instrument chunks
// kinda simple, sound chunks have some dumb rules that this tool still doesnt properly follow but oh well

/** \brief
 *  Creates and builds chunks for the instrument entries according to their format.
 *  Also creates actual game format output chunks.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_count int*              chunk count
 * \param chunks unsigned char**        chunks, array of ptrs to data, allocated here
 * \return void
 */
void build_instrument_chunks(ENTRY *elist, int entry_count, int *chunk_count, unsigned char** chunks) {
    int chunk_index = *chunk_count;
    // wavebank chunks are one entry per chunk, not much to it
    for (int i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_INST) {
            elist[i].chunk = chunk_index;
            chunks[chunk_index] = (unsigned char *) calloc(CHUNKSIZE, sizeof(unsigned char));       // freed by build_main
            *(unsigned short int *)(chunks[chunk_index]) = MAGIC_CHUNK;
            *(unsigned short int *)(chunks[chunk_index] + 2) = CHUNK_TYPE_INSTRUMENT;
            *(unsigned short int *)(chunks[chunk_index] + 4) = 2 * chunk_index + 1;

            *(unsigned int *)(chunks[chunk_index] + 8) = 1;
            *(unsigned int *)(chunks[chunk_index] + 0x10) = 0x24;
            *(unsigned int *)(chunks[chunk_index] + 0x14) = 0x24 + elist[i].esize;

            memcpy(chunks[chunk_index] + 0x24, elist[i].data, elist[i].esize);
            *((unsigned int *)(chunks[chunk_index] + 0xC)) = nsfChecksum(chunks[chunk_index]);
            chunk_index++;
        }

    *chunk_count = chunk_index;
}


/** \brief
 *  Creates and builds sound chunks, formats accordingly.
 *  Also creates actual game format output chunks.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_count int*              chunk count
 * \param chunks unsigned char**        chunks, array of ptrs to data, allocated here
 * \return void
 */
void build_sound_chunks(ENTRY *elist, int entry_count, int *chunk_count, unsigned char** chunks) {
    int i, j, temp_chunk_count = *chunk_count;
    int sound_entry_count = 0;

    // count sound entries, create an array of entries for them (temporary)
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_SOUND)
            sound_entry_count++;
    ENTRY *sound_list = (ENTRY *) malloc(sound_entry_count * sizeof(ENTRY));        // freed here

    // add actual entries to the array
    int indexer = 0;
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_SOUND)
            sound_list[indexer++] = elist[i];

    // sort entries in the list by size cuz largest first packing algorithm is used
    qsort(sound_list, sound_entry_count, sizeof(ENTRY), cmp_entry_size);

    // assumes 8 sound chunks, i dont think theres anywhere the vanilla game uses more than 8 soooo
    // default header is 0x14B
    int sizes[8];
    for (i = 0; i < 8; i++)
        sizes[i] = 0x14;

    // put in the first chunk it fits into
    // idk what happened here anymore
    for (i = 0; i < sound_entry_count; i++)
        for (j = 0; j < 8; j++)
            if (sizes[j] + 4 + (((sound_list[i].esize + 16) >> 4) << 4) <= CHUNKSIZE) {
                sound_list[i].chunk = temp_chunk_count + j;
                sizes[j] += 4 + (((sound_list[i].esize + 16) >> 4) << 4);
                break;
            }

    // find out how many actual sound chunks got made
    int snd_chunk_count = 0;
    for (i = 0; i < 8; i++)
        if (sizes[i] > 0x14)
            snd_chunk_count = i + 1;

    // not sure why this is here anymore, but i remember things not working properly without this
    qsort(sound_list, sound_entry_count, sizeof(ENTRY), cmp_entry_eid);

    // write the actual chunks, almost identical to the build_normal_chunks function
    for (i = 0; i < snd_chunk_count; i++) {
        int local_entry_count = 0;
        int chunk_no = 2 * (temp_chunk_count + i) + 1;
        chunks[temp_chunk_count + i] = (unsigned char*) calloc(CHUNKSIZE, sizeof(unsigned char));           // freed by build_main

        for (j = 0; j < sound_entry_count; j++)
            if (sound_list[j].chunk == temp_chunk_count + i)
                local_entry_count++;

        unsigned int* offsets = (unsigned int*) malloc( (local_entry_count + 2) * sizeof(unsigned int));    //idk why its +2, freed here
        *(unsigned short int *) chunks[temp_chunk_count + i] = MAGIC_CHUNK;
        *(unsigned short int *)(chunks[temp_chunk_count + i] + 2) = CHUNK_TYPE_SOUND;
        *(unsigned short int *)(chunks[temp_chunk_count + i] + 4) = chunk_no;
        *(unsigned short int *)(chunks[temp_chunk_count + i] + 8) = local_entry_count;

        indexer = 0;
        offsets[indexer] = build_align_sound(0x10 + (local_entry_count + 1) * 4);

        for (j = 0; j < sound_entry_count; j++)
            if (sound_list[j].chunk == temp_chunk_count + i) {
                offsets[indexer + 1] = build_align_sound(offsets[indexer] + sound_list[j].esize);
                indexer++;
            }

        for (j = 0; j < local_entry_count + 1; j++)
            *(unsigned int *) (chunks[temp_chunk_count + i] + 0x10 + j * 4) = offsets[j];

        indexer = 0;
        for (j = 0; j < sound_entry_count; j++)
            if (sound_list[j].chunk == temp_chunk_count + i)
                memcpy(chunks[temp_chunk_count + i] + offsets[indexer++], sound_list[j].data, sound_list[j].esize);

        *(unsigned int*)(chunks[temp_chunk_count + i] + 0xC) = nsfChecksum(chunks[temp_chunk_count + i]);
        free(offsets);
    }

    // update the chunk assignment in the actual master entry list, this function only worked with copies of the entries
    for (i = 0; i < entry_count; i++)
        for (j = 0; j < sound_entry_count; j++)
            if (elist[i].eid == sound_list[j].eid)
                elist[i].chunk = sound_list[j].chunk;
    // update chunk count
    *chunk_count = temp_chunk_count + snd_chunk_count;
    free(sound_list);
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

