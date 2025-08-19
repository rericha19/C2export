#include "macros.h"
// contains functions responsible for creating sound and instrument chunks
// kinda simple, sound chunks have some dumb rules that this tool still doesnt properly follow but oh well

/** \brief
 *  Creates and builds chunks for the instrument entries according to their format.
 *  Also creates actual game format output chunks.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param chunk_count int32_t*              chunk count
 * \param chunks uint8_t**        chunks, array of ptrs to data, allocated here
 * \return void
 */
void build_instrument_chunks(ENTRY *elist, int32_t entry_count, int32_t *chunk_count, uint8_t **chunks)
{
    int32_t chunk_index = *chunk_count;
    // wavebank chunks are one entry per chunk, not much to it
    for (int32_t i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_INST)
        {
            elist[i].chunk = chunk_index;
            chunks[chunk_index] = (uint8_t *)try_calloc(CHUNKSIZE, sizeof(uint8_t)); // freed by build_main
            *(uint16_t *)(chunks[chunk_index]) = MAGIC_CHUNK;
            *(uint16_t *)(chunks[chunk_index] + 2) = CHUNK_TYPE_INSTRUMENT;
            *(uint16_t *)(chunks[chunk_index] + 4) = 2 * chunk_index + 1;

            *(uint32_t *)(chunks[chunk_index] + 8) = 1;
            *(uint32_t *)(chunks[chunk_index] + 0x10) = 0x24;
            *(uint32_t *)(chunks[chunk_index] + 0x14) = 0x24 + elist[i].esize;

            memcpy(chunks[chunk_index] + 0x24, elist[i].data, elist[i].esize);
            *((uint32_t *)(chunks[chunk_index] + CHUNK_CHECKSUM_OFFSET)) = nsfChecksum(chunks[chunk_index]);
            chunk_index++;
        }

    *chunk_count = chunk_index;
}

/** \brief
 *  Creates and builds sound chunks, formats accordingly.
 *  Also creates actual game format output chunks.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param chunk_count int32_t*              chunk count
 * \param chunks uint8_t**        chunks, array of ptrs to data, allocated here
 * \return void
 */
void build_sound_chunks(ENTRY *elist, int32_t entry_count, int32_t *chunk_count, uint8_t **chunks)
{
    int32_t start_chunk_idx = *chunk_count;
    int32_t sound_entry_count = 0;

    // count sound entries, create an array of entries for them
    for (int32_t i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_SOUND)
            sound_entry_count++;
    ENTRY *sound_list = (ENTRY *)try_malloc(sound_entry_count * sizeof(ENTRY)); // freed here

    // add actual entries to the array
    int32_t indexer = 0;
    for (int32_t i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_SOUND)
            sound_list[indexer++] = elist[i];

    // sort entries in the list by size cuz largest first packing algorithm is used
    qsort(sound_list, sound_entry_count, sizeof(ENTRY), cmp_func_esize);

    // assumes 8 sound chunks, i dont think theres anywhere the vanilla game uses more than 8 soooo
    // default header is 0x14B
    int32_t sizes[8];
    for (int32_t i = 0; i < 8; i++)
        sizes[i] = 0x14;

    // put in the first chunk it fits into
    // idk what happened here anymore
    for (int32_t i = 0; i < sound_entry_count; i++)
        for (int32_t j = 0; j < 8; j++)
            if (sizes[j] + 4 + (((sound_list[i].esize + 16) >> 4) << 4) <= CHUNKSIZE)
            {
                sound_list[i].chunk = start_chunk_idx + j;
                sizes[j] += 4 + (((sound_list[i].esize + 16) >> 4) << 4);
                break;
            }

    // find out how many actual sound chunks got made
    int32_t snd_chunk_count = 0;
    for (int32_t i = 0; i < 8; i++)
        if (sizes[i] > 0x14)
            snd_chunk_count = i + 1;

    // not sure why this is here anymore, but i remember things not working properly without this
    qsort(sound_list, sound_entry_count, sizeof(ENTRY), cmp_func_eid);

    // build the actual chunks, almost identical to the build_normal_chunks function
    for (int32_t i = 0; i < snd_chunk_count; i++)
    {
        int32_t local_entry_count = 0;
        int32_t chunk_no = 2 * (start_chunk_idx + i) + 1;
        chunks[start_chunk_idx + i] = (uint8_t *)try_calloc(CHUNKSIZE, sizeof(uint8_t)); // freed by build_main

        for (int32_t j = 0; j < sound_entry_count; j++)
            if (sound_list[j].chunk == start_chunk_idx + i)
                local_entry_count++;

        uint32_t *offsets = (uint32_t *)try_malloc((local_entry_count + 2) * sizeof(uint32_t)); // idk why its +2, freed here
        *(uint16_t *)chunks[start_chunk_idx + i] = MAGIC_CHUNK;
        *(uint16_t *)(chunks[start_chunk_idx + i] + 2) = CHUNK_TYPE_SOUND;
        *(uint16_t *)(chunks[start_chunk_idx + i] + 4) = chunk_no;
        *(uint16_t *)(chunks[start_chunk_idx + i] + 8) = local_entry_count;

        indexer = 0;
        offsets[indexer] = build_align_sound(0x10 + (local_entry_count + 1) * 4);

        for (int32_t j = 0; j < sound_entry_count; j++)
            if (sound_list[j].chunk == start_chunk_idx + i)
            {
                offsets[indexer + 1] = build_align_sound(offsets[indexer] + sound_list[j].esize);
                indexer++;
            }

        for (int32_t j = 0; j < local_entry_count + 1; j++)
            *(uint32_t *)(chunks[start_chunk_idx + i] + 0x10 + j * 4) = offsets[j];

        indexer = 0;
        for (int32_t j = 0; j < sound_entry_count; j++)
            if (sound_list[j].chunk == start_chunk_idx + i)
                memcpy(chunks[start_chunk_idx + i] + offsets[indexer++], sound_list[j].data, sound_list[j].esize);

        *(uint32_t *)(chunks[start_chunk_idx + i] + CHUNK_CHECKSUM_OFFSET) = nsfChecksum(chunks[start_chunk_idx + i]);
        free(offsets);
    }

    // update the chunk assignment in the actual master entry list, this function only worked with copies of the entries
    for (int32_t i = 0; i < entry_count; i++)
        for (int32_t j = 0; j < sound_entry_count; j++)
            if (elist[i].eid == sound_list[j].eid)
                elist[i].chunk = sound_list[j].chunk;
    // update chunk count
    *chunk_count = start_chunk_idx + snd_chunk_count;
    free(sound_list);
}

/** \brief
 *  Returns a value that makes sound entries aligned as they should be (hopefully).
 *
 * \param input int32_t                     input offset
 * \return int32_t                          aligned offset
 */
int32_t build_align_sound(int32_t input)
{
    for (int32_t i = 0; i < 16; i++)
        if ((input + i) % 16 == 8)
            return (input + i);

    return input;
}
