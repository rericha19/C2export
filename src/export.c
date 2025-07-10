#include "macros.h"

// convoluted, but does the thing (i think)
// absolutely fucking awful never look at it again

// prints current settings
void export_printstatus(int zonetype, int gamemode, int portmode)
{
    printf("Selected game: Crash %d, porting: %d, zone neighbours (if C2->C3): %d\n\n", gamemode, portmode, zonetype);
}

// wipes the stats
void export_countwipe(DEPRECATE_INFO_STRUCT *status)
{
    for (int i = 0; i < 22; i++)
        status->counter[i] = 0;
}

// creates a string thats a save path for the currently processed file
void export_make_path(char *finalpath, char *type, int eid, char *lvlid, char *date, DEPRECATE_INFO_STRUCT status)
{
    char eidstr[6] = "";
    eid_conv(eid, eidstr);
    int port;

    if (status.portmode == 1)
    {
        if (status.gamemode == 2)
            port = 3;
        else
            port = 2;
    }
    else
        port = status.gamemode;

    if (strcmp(type, "texture") == 0)
        sprintf(finalpath, "C%d_to_C%d\\\\%s\\\\S00000%s\\\\%s %s %d", status.gamemode, port, date, lvlid, type, eidstr, status.counter[0]);
    else
        sprintf(finalpath, "C%d_to_C%d\\\\%s\\\\S00000%s\\\\%s %s %d.nsentry", status.gamemode, port, date, lvlid, type, eidstr, status.counter[0]);
}

// gets the info about the game and what to do with it
void export_askmode(int *zonetype, DEPRECATE_INFO_STRUCT *status)
{
    char c;
    int help;
    printf("Which game are the files from? [2/3]\n");
    printf("Change to other game's format? [Y/N]\n");
    scanf("%d %c", &help, &c);
    c = toupper(c);

    status->gamemode = help;

    if (status->gamemode > 3 || status->gamemode < 2)
    {
        printf("[error] invalid game, defaulting to Crash 2\n");
        status->gamemode = 2;
    }

    if (c == 'Y')
        status->portmode = 1;
    else
    {
        if (c == 'N')
            status->portmode = 0;
        else
        {
            printf("[error] invalid portmode, defaulting to 0 (not changing format)\n");
            status->portmode = 0;
        }
    }

    if (status->gamemode == 2 && status->portmode == 1)
    {
        printf("How many neighbours should the exported files' zones have? [8/16]\n");
        scanf("%d", zonetype);
        if (!(*zonetype == 8 || *zonetype == 16))
        {
            printf("[error] invalid neighbour count, defaulting to 8\n");
            *zonetype = 8;
        }
    }
    export_printstatus(*zonetype, status->gamemode, status->portmode);
}

// pops up the dialog that lets you pick where to write most of the print statements
// broken
void export_askprint(DEPRECATE_INFO_STRUCT *status)
{
    char dest[4][10] = {"to both", "to file", "here", "nowhere"};
    char pc;
    printf("Where to print status messages? [N - nowhere|F - file|H - here|B - both] (from fastest to slowest)[broken rn, doesnt write to file]\n");
    scanf(" %c", &pc);
    switch (toupper(pc))
    {
    case 'B':
        status->print_en = 3;
        break;
    case 'F':
        status->print_en = 2;
        break;
    case 'H':
        status->print_en = 1;
        break;
    case 'N':
        status->print_en = 0;
        break;
    default:
        status->print_en = 0;
        printf("[error] invalid input, defaulting to 'N'\n");
        break;
    }
    printf("Printing %s.\n\n", dest[3 - status->print_en]);
}

// prints the stats
void export_countprint(DEPRECATE_INFO_STRUCT status)
{
    int i;
    char lcltemp[] = "(fixed)";
    char lcltemp2[10] = "";
    char prefix[22][30] = {"Entries: \t", "Animations: \t", "Models:\t\t", "Sceneries: \t", "SLSTs: \t\t", "Textures: \t", "",
                           "Zones: \t\t", "", "", "", "GOOL entries: \t", "Sound entries: \t", "Music tracks:\t", "Instruments: \t", "VCOL entries:\t",
                           "", "", "", "Demo entries:\t", "Speech entries:\t", "T21 entries: \t"};

    for (i = 0; i < 22; i++)
        if (status.counter[i])
        {
            if (status.portmode && (i == 2 || i == 7 || (i == 11 && status.portmode && (status.gamemode == 3))))
                strcpy(lcltemp2, lcltemp);
            else
                strcpy(lcltemp2, "\t");
            sprintf(status.temp, "%s %s%3d\t", prefix[i], lcltemp2, status.counter[i]);
            export_condprint(status);
            for (int j = 0; j < (double)status.counter[i] / 6; j++)
            {
                sprintf(status.temp, "|");
                export_condprint(status);
            }
            sprintf(status.temp, "\n");
            export_condprint(status);
        }
    sprintf(status.temp, "\n");
    export_condprint(status);
}

// conditional print controlled by print_en (print state) - both(3) file(2) here(1) or nowhere(0)
void export_condprint(DEPRECATE_INFO_STRUCT status)
{
    switch (status.print_en)
    {
    case 3:
        printf("%s", status.temp);
        // fprintf(status.flog,"%s",status.temp);
        break;
    case 1:
        printf("%s", status.temp);
        break;
    case 2:
        // fprintf(status.flog,"%s",status.temp);
        break;
    default:
        break;
    }
}

// does the thing yea
int export_main(int zone, char *fpath, char *date, DEPRECATE_INFO_STRUCT *status)
{
    char sid[3] = "";
    export_countwipe(status);
    FILE *file;                     // the NSF thats to be exported
    unsigned int numbytes, i;       // zonetype - what type the zones will be once exported, 8 or 16 neighbour ones, numbytes - size of file, i - iteration
    char temp[MAX] = "";            //, nsfcheck[4];
    unsigned char chunk[CHUNKSIZE]; // array where the level data is stored
    int port;
    char lcltemp[3][6] = {"", "", ""};

    /*strncpy(nsfcheck,strchr(fpath,'\0')-3,3);

    if (strcmp(nsfcheck,"NSF"))
    {
        sprintf(temp,"Not a .NSF file!\n");
        printf(temp);
        return 0;
    }*/

    if (status->portmode == 1)
    {
        if (status->gamemode == 2)
            port = 3;
        else
            port = 2;
    }
    else
        port = status->gamemode;

    sid[0] = *(strchr(fpath, '\0') - 6);
    sid[1] = *(strchr(fpath, '\0') - 5);

    if ((file = fopen(fpath, "rb")) == NULL)
    {
        sprintf(temp, "[ERROR] Level not found or could not be opened\n");
        printf(temp);
        return 0;
    }
    else
    {
        sprintf(temp, "Exporting level %s\n", sid);
        printf(temp);
    }
    // making directories
    sprintf(status->temp, "C%d_to_C%d", status->gamemode, port);
    mkdir(status->temp);
    sprintf(status->temp, "C%d_to_C%d\\\\%s", status->gamemode, port, date);
    mkdir(status->temp);
    sprintf(status->temp, "C%d_to_C%d\\\\%s\\\\S00000%s", status->gamemode, port, date, sid);
    mkdir(status->temp);

    // if file print is enabled, open the file
    sprintf(temp, "C%d_to_C%d\\\\%s\\\\log.txt", status->gamemode, port, date);
    if (status->print_en >= 2)
        status->flog = fopen(status->temp, "w");

    // reading the file
    fseek(file, 0, SEEK_END);
    numbytes = ftell(file);
    rewind(file);
    sprintf(status->temp, "The NSF [ID %s] has %d pages/chunks\n", sid, (numbytes / CHUNKSIZE) * 2 - 1);
    export_condprint(*status);

    // hands the chunks to chunk_handler one by one
    for (i = 0; (unsigned int)i < (numbytes / CHUNKSIZE); i++)
    {
        fread(chunk, sizeof(char), CHUNKSIZE, file);
        export_chunk_handler(chunk, i, sid, date, zone, status);
    }
    strcpy(status->temp, "\n");
    export_condprint(*status);

    // closing and printing
    export_countprint(*status);
    if (fclose(file) == EOF)
    {
        sprintf(temp, "[ERROR] file could not be closed\n");
        printf(temp);
        return 2;
    }
    if (status->print_en >= 2)
        fclose(status->flog);
    sprintf(status->temp, "\n");
    export_condprint(*status);

    for (i = 0; (unsigned int)i < status->animrefcount; i++)
    {
        eid_conv(status->anim[i][0], lcltemp[0]);
        eid_conv(status->anim[i][1], lcltemp[1]);
        eid_conv(status->anim[i][2], lcltemp[2]);
        // printf("ref: %03d, model: %s, animation: %s, new eid: %s\n", i, lcltemp[1], lcltemp[2], lcltemp[3]);
    }

    sprintf(temp, "Done!\n");
    printf(temp);
    return 1;
}

// receives the current chunk, its id (not ingame id, indexed by 1)
// and lvlid that just gets sent further
int export_chunk_handler(unsigned char *buffer, int chunkid, char *lvlid, char *date, int zonetype, DEPRECATE_INFO_STRUCT *status)
{
    char ctypes[7][20] = {"Normal", "Texture", "Proto sound", "Sound", "Wavebank", "Speech", "Unknown"};
    sprintf(status->temp, "%s chunk \t%03d\n", ctypes[buffer[2]], chunkid * 2 + 1);
    export_condprint(*status);
    switch (buffer[2])
    {
    case 0:
    case 3:
    case 4:
    case 5:
        export_normal_chunk(buffer, lvlid, date, zonetype, status);
        break;
    case 1:
        export_texture_chunk(buffer, lvlid, date, status);
        break;
    default:
        break;
    }
    return 0;
}

// breaks the normal chunk into entries and saves them one by one
int export_normal_chunk(unsigned char *buffer, char *lvlid, char *date, int zonetype, DEPRECATE_INFO_STRUCT *status)
{
    int i;
    int offset_start;
    int offset_end;
    unsigned char *entry;
    char eid[6];
    unsigned int eidnum = 0;

    for (i = 0; i < buffer[8]; i++)
    {
        status->counter[0]++;
        offset_start = BYTE * buffer[0x11 + i * 4] + buffer[0x10 + i * 4];
        offset_end = BYTE * buffer[0x15 + i * 4] + buffer[0x14 + i * 4];
        if (!offset_end)
            offset_end = CHUNKSIZE;
        entry = (unsigned char *)calloc(offset_end - offset_start, sizeof(char)); // freed here
        memcpy(entry, buffer + offset_start, offset_end - offset_start);
        switch (entry[8])
        {
        case 1:
            export_animation(entry, offset_end - offset_start, lvlid, date, status);
            break;
        case 2:
            export_model(entry, offset_end - offset_start, lvlid, date, status);
            break;
        case 3:
            export_scenery(entry, offset_end - offset_start, lvlid, date, status);
            break;
        case 7:
            export_zone(entry, offset_end - offset_start, lvlid, date, zonetype, status);
            break;
        case 11:
            export_gool(entry, offset_end - offset_start, lvlid, date, status);
            break;

        case 4:
        case 12:
        case 13:
        case 14:
        case 15:
        case 19:
        case 20:
        case 21:
            export_generic_entry(entry, offset_end - offset_start, lvlid, date, status);
            break;
        default:
            eidnum = 0;
            for (int j = 0; j < 4; j++)
                eidnum = (eidnum * BYTE) + entry[7 - j];
            eid_conv(eidnum, eid);
            sprintf(status->temp, "\t T%2d, unknown entry\t%s\n", entry[8], eid);
            export_condprint(*status);
            break;
        }
        free(entry);
    }
    return 0;
}

// creates a file with the texture chunk, lvlid for file name, st is save type
int export_texture_chunk(unsigned char *buffer, char *lvlid, char *date, DEPRECATE_INFO_STRUCT *status)
{
    FILE *f;
    unsigned int eidint = 0;
    char path[MAX - 20] = ""; // -20 to get rid of a warning
    char cur_type[] = "texture";

    for (int i = 0; i < 4; i++)
        eidint = (BYTE * eidint) + buffer[7 - i];

    export_make_path(path, cur_type, eidint, lvlid, date, *status);

    // opening, writing and closing
    f = fopen(path, "wb");
    fwrite(buffer, sizeof(char), CHUNKSIZE, f);
    fclose(f);
    sprintf(status->temp, "\t\t saved as '%s'\n", path);
    export_condprint(*status);
    status->counter[5]++;
    status->counter[0]++;
    return 0;
}

// converts camera mode from c3 to c2 values
int export_camera_fix(unsigned char *cam, int length)
{
    int i, curr_off;

    for (i = 0; i < cam[0xC]; i++)
    {
        if (cam[0x10 + i * 8] == 0x29 && cam[0x11 + i * 8] == 0)
        {
            curr_off = BYTE * cam[0x13 + i * 8] + cam[0x12 + i * 8] + 0xC + 4;
            switch (curr_off)
            {
            case 0x80:
                cam[curr_off] = 8;
                break;
            case 4:
                cam[curr_off] = 3;
                break;
            case 1:
                cam[curr_off] = 0;
                break;
            case 2:
                break;
            default:
                break;
            }
        }
    }
    return 0;
}

// fixes entity coords (from c3 to c2)
void export_entity_coord_fix(unsigned char *item, int itemlength)
{
    int i;
    int off0x4B = 0, off0x30E = 0;
    double scale;
    short int coord;

    // printf("%d\n", itemlength);
    for (i = 0; i < item[0xC]; i++)
    {
        if (item[0x10 + i * 8] == 0x4B && item[0x11 + i * 8] == 0)
            off0x4B = BYTE * item[0x13 + i * 8] + item[0x12 + i * 8] + 0xC;
        if (item[0x10 + i * 8] == 0x0E && item[0x11 + i * 8] == 3)
            off0x30E = BYTE * item[0x13 + i * 8] + item[0x12 + i * 8] + 0xC;
    }

    if (off0x4B && off0x30E)
    {
        switch (item[off0x30E + 4])
        {
        case 0:
            scale = 0.25;
            break;
        case 1:
            scale = 0.5;
            break;
        case 3:
            scale = 2;
            break;
        case 4:
            scale = 4; //?
            break;
        default:
            scale = 1;
            break;
        }

        for (i = 0; i < item[off0x4B] * 6; i += 2)
        {
            if (item[off0x4B + 0x5 + i] < 0x80)
            {
                coord = BYTE * (signed)item[off0x4B + 0x5 + i] + (signed)item[off0x4B + 0x4 + i];
                coord = (short int)coord * scale;
                item[off0x4B + 0x5 + i] = coord / 256;
                item[off0x4B + 0x4 + i] = coord % 256;
            }
            else
            {
                coord = 65536 - BYTE * (signed)item[off0x4B + 0x5 + i] - (signed)item[off0x4B + 0x4 + i];
                coord = (short int)coord * scale;
                item[off0x4B + 0x5 + i] = 255 - coord / 256;
                item[off0x4B + 0x4 + i] = 256 - coord % 256;
                if (item[off0x4B + 0x4 + i] == 0)
                    item[off0x4B + 0x5 + i]++;
            }
        }
    }
}

// does stuff to scenery
// exports scenery
void export_scenery(unsigned char *buffer, int entrysize, char *lvlid, char *date, DEPRECATE_INFO_STRUCT *status)
{
    FILE *f;
    char cur_type[10];
    int eidint = 0;
    char eid[6] = "";
    char path[MAX - 20] = "";
    int curr_off, next_off, i, j, item1off;
    int vert, rest, group;
    unsigned int origin;

    strncpy(cur_type, "scenery", 10);
    for (i = 0; i < 4; i++)
    {
        eidint = (BYTE * eidint) + buffer[7 - i];
    }
    eid_conv(eidint, eid);
    export_make_path(path, cur_type, eidint, lvlid, date, *status);

    if (status->portmode)
    {
        if (status->gamemode == 3) // c3 to c2
        {
            item1off = buffer[0x10];
            for (i = 0; i < 3; i++)
            {
                origin = from_u32(buffer + item1off + 4 * i);
                // origin += 0x8000;
                for (j = 0; j < 4; j++)
                {
                    buffer[item1off + j + i * 4] = origin % 256;
                    origin /= 256;
                }
            }
            curr_off = BYTE * buffer[0x15] + buffer[0x14];
            next_off = BYTE * buffer[0x19] + buffer[0x18];
            for (i = curr_off; i < next_off; i += 2)
            {
                group = 256 * buffer[i + 1] + buffer[i];
                vert = group / 16;
                rest = group % 16;
                // /* if (vert > 2048)*/ vert -= 0x10;
                //   vert -= 0x800;
                group = 16 * vert + rest;
                buffer[i + 1] = group / 256;
                buffer[i] = group % 256;
            }
        }
        else
        {
            ;
        }
    }
    sprintf(status->temp, "\t\t saved as '%s'\n", path);
    export_condprint(*status);
    f = fopen(path, "wb");
    fwrite(buffer, sizeof(char), entrysize, f);
    fclose(f);
    status->counter[3]++;
}

// exports entries that dont receive any change
void export_generic_entry(unsigned char *buffer, int entrysize, char *lvlid, char *date, DEPRECATE_INFO_STRUCT *status)
{
    FILE *f;
    char cur_type[MAX / 10] = "";
    int eidint = 0;
    char eid[6];
    char path[MAX - 20] = "";

    switch (buffer[8]) // this is really ugly but it works, so im not touching it
    {
    case 3:
        strcpy(cur_type, "scenery");
        break;
    case 4:
        strcpy(cur_type, "SLST");
        break;
    case 12:
        strcpy(cur_type, "sound");
        break;
    case 13:
        strcpy(cur_type, "music track");
        break;
    case 14:
        strcpy(cur_type, "instruments");
        break;
    case 15:
        strcpy(cur_type, "VCOL");
        break;
    case 19:
        strcpy(cur_type, "demo");
        break;
    case 20:
        strcpy(cur_type, "speech");
        break;
    case 21:
        strcpy(cur_type, "T21");
        break;
    default:
        break;
    }

    for (int i = 0; i < 4; i++)
        eidint = (BYTE * eidint) + buffer[7 - i];
    eid_conv(eidint, eid);

    export_make_path(path, cur_type, eidint, lvlid, date, *status);

    sprintf(status->temp, "\t\t saved as '%s'\n", path);
    export_condprint(*status);

    f = fopen(path, "wb");
    fwrite(buffer, sizeof(char), entrysize, f);
    fclose(f);
    status->counter[buffer[8]]++;
}

// exports gool, no changes right now ?
void export_gool(unsigned char *buffer, int entrysize, char *lvlid, char *date, DEPRECATE_INFO_STRUCT *status)
{
    FILE *f;
    char cur_type[10];
    int eidint = 0;
    char eid[6];
    unsigned char *cpy;
    char path[MAX - 20] = "";
    int curr_off = 0;
    int i, j;
    int help1, help2;
    char eidhelp1[6], eidhelp2[6];

    cpy = (unsigned char *)calloc(entrysize, sizeof(char)); // freed here
    memcpy(cpy, buffer, entrysize);

    for (i = 0; i < 4; i++)
        eidint = (BYTE * eidint) + buffer[7 - i];
    strncpy(cur_type, "GOOL", 10);
    eid_conv(eidint, eid);

    if (cpy[0xC] == 6 && status->portmode)
    {
        curr_off = BYTE * cpy[0x25] + cpy[0x24];
        if (status->gamemode == 2) // if its c2 to c3
        {
            ;
        }
        else // c3 to c2
        {
            while (curr_off < entrysize)
            {
                switch (cpy[curr_off])
                {
                case 1:
                    help1 = 0;
                    help2 = 0;
                    for (j = 0; j < 4; j++)
                        help1 = 256 * help1 + cpy[curr_off + 0x0F - j];
                    eid_conv(help1, eidhelp1);

                    if (eidhelp1[4] == 'G')
                    {
                        for (j = 0; j < 4; j++)
                            help2 = 256 * help2 + cpy[curr_off + 0x13 - j];
                        eid_conv(help2, eidhelp2);
                        if (eidhelp2[4] == 'V')
                        {
                            status->anim[status->animrefcount][0] = help1;
                            status->anim[status->animrefcount][1] = help2;
                            status->anim[status->animrefcount][2] = help2;
                            status->animrefcount++;
                        }
                    }

                    for (i = 1; i < 4; i++)
                        for (j = 0; j < 4; j++)
                            cpy[curr_off + 0x10 - 4 * i + j] = cpy[curr_off + 0x10 + j];

                    curr_off += 0x14;
                    break;
                case 2:
                    curr_off += 0x8 + 0x10 * cpy[curr_off + 2];
                    break;
                case 3:
                    curr_off += 0x8 + 0x14 * cpy[curr_off + 2];
                    break;
                case 4:
                    curr_off = entrysize;
                    break;
                case 5:
                    curr_off += 0xC + 0x18 * cpy[curr_off + 2] * cpy[curr_off + 8];
                    break;
                }
            }
        }
    }

    export_make_path(path, cur_type, eidint, lvlid, date, *status);
    sprintf(status->temp, "\t\t saved as '%s'\n", path);
    export_condprint(*status);
    f = fopen(path, "wb");
    fwrite(cpy, sizeof(char), entrysize, f);
    free(cpy);
    fclose(f);
    status->counter[11]++;
}

// exports zones
void export_zone(unsigned char *buffer, int entrysize, char *lvlid, char *date, int zonetype, DEPRECATE_INFO_STRUCT *status)
{
    FILE *f;
    char cur_type[10];
    int eidint = 0;
    char eid[6] = "";
    char path[MAX - 20] = "";
    unsigned char *cpy;
    int lcl_entrysize = entrysize;
    int i, j;
    int curr_off, lcl_temp, irrelitems, next_off;

    for (int i = 0; i < 4; i++)
        eidint = (BYTE * eidint) + buffer[7 - i];
    eid_conv(eidint, eid);

    cpy = (unsigned char *)calloc(entrysize, sizeof(char)); // freed here
    memcpy(cpy, buffer, entrysize);

    if (status->portmode)
    {
        if (status->gamemode == 2) // c2 to c3
        {
            if (zonetype == 16)
            {
                lcl_entrysize += 0x40;
                cpy = (unsigned char *)realloc(cpy, lcl_entrysize);

                // offset fix
                for (j = 0; j < cpy[0xC]; j++)
                {
                    lcl_temp = BYTE * cpy[0x15 + j * 4] + cpy[0x14 + j * 4];
                    lcl_temp += 0x40;
                    cpy[0x15 + j * 4] = lcl_temp / BYTE;
                    cpy[0x14 + j * 4] = lcl_temp % BYTE;
                }

                // inserts byte in a very ugly way, look away
                curr_off = BYTE * cpy[0x11] + cpy[0x10];
                for (i = lcl_entrysize - 0x21; i >= curr_off + C2_NEIGHBOURS_END; i--)
                    cpy[i] = cpy[i - 0x20];

                for (i = curr_off + C2_NEIGHBOURS_END; i < curr_off + C2_NEIGHBOURS_FLAGS_END; i++)
                    cpy[i] = 0;

                for (i = lcl_entrysize - 1; i >= curr_off + 0x200; i--)
                    cpy[i] = cpy[i - 0x20];

                for (i = curr_off + 0x200; i < curr_off + 0x220; i++)
                    cpy[i] = 0;
            }
        }
        else // c3 to c2 removes bytes
        {
            if ((BYTE * cpy[0x15] + cpy[0x14] - BYTE * cpy[0x11] - cpy[0x10]) == 0x358)
            {
                for (j = 0; j < cpy[0xC]; j++)
                {
                    lcl_temp = BYTE * cpy[0x15 + j * 4] + cpy[0x14 + j * 4];
                    lcl_temp -= 0x40;
                    cpy[0x15 + j * 4] = lcl_temp / BYTE;
                    cpy[0x14 + j * 4] = lcl_temp % BYTE;
                }

                curr_off = BYTE * cpy[0x11] + cpy[0x10];
                for (i = curr_off + C2_NEIGHBOURS_END; i < lcl_entrysize - 20; i++)
                    cpy[i] = cpy[i + 0x20];

                for (i = curr_off + C2_NEIGHBOURS_FLAGS_END; i < lcl_entrysize - 20; i++)
                    cpy[i] = cpy[i + 0x20];

                if (cpy[curr_off + 0x190] > 8)
                    cpy[curr_off + 0x190] = 8;
                lcl_entrysize -= 0x40;
            }

            irrelitems = cpy[cpy[0x10] + 256 * cpy[0x11] + 0x184] + cpy[cpy[0x10] + 256 * cpy[0x11] + 0x188];
            for (i = irrelitems; i < cpy[0xC]; i++)
            {
                // printf("zone %s, item %d\n",eid,i);
                curr_off = BYTE * cpy[0x11 + i * 4] + cpy[0x10 + i * 4];
                next_off = BYTE * cpy[0x15 + i * 4] + cpy[0x14 + i * 4];
                if (next_off == 0)
                    next_off = CHUNKSIZE;
                export_entity_coord_fix(cpy + curr_off, next_off - curr_off);
            }

            irrelitems = cpy[cpy[0x10] + 256 * cpy[0x11] + 0x188];
            for (i = 0; i < irrelitems / 3; i++)
            {
                curr_off = BYTE * cpy[0x19 + i * 0xC] + cpy[0x18 + i * 0xC];
                next_off = BYTE * cpy[0x19 + i * 0xC + 0x4] + cpy[0x18 + i * 0xC + 0x4];
                if (next_off == 0)
                    next_off = CHUNKSIZE;
                export_camera_fix(cpy + curr_off, next_off - curr_off);
            }
        }
    }

    strncpy(cur_type, "zone", 10);
    export_make_path(path, cur_type, eidint, lvlid, date, *status);

    sprintf(status->temp, "\t\t saved as '%s'\n", path);
    export_condprint(*status);
    f = fopen(path, "wb");
    fwrite(cpy, sizeof(char), lcl_entrysize, f);
    free(cpy);
    fclose(f);
    status->counter[7]++;
}

// exports models, changes already
void export_model(unsigned char *buffer, int entrysize, char *lvlid, char *date, DEPRECATE_INFO_STRUCT *status)
{
    FILE *f;
    int i;
    float scaling = 0;
    char cur_type[10];
    int eidint = 0;
    char eid[6] = "";
    char path[MAX - 20] = ""; 
    int msize = 0;

    // scale change in case its porting
    if (status->gamemode == 2)
        scaling = 1. / 8;
    else
        scaling = 8;

    if (status->portmode)
        for (i = 0; i < 3; i++)
        {
            msize = BYTE * buffer[buffer[0x10] + 1 + i * 4] + buffer[buffer[0x10] + i * 4];
            msize = (int)msize * scaling;
            buffer[buffer[0x10] + 1 + i * 4] = msize / BYTE;
            buffer[buffer[0x10] + i * 4] = msize % BYTE;
        }

    for (i = 0; i < 4; i++)
    {
        eidint = (BYTE * eidint) + buffer[7 - i];
    }

    strncpy(cur_type, "model", 10);
    eid_conv(eidint, eid);

    export_make_path(path, cur_type, eidint, lvlid, date, *status);

    sprintf(status->temp, "\t\t saved as '%s'\n", path);
    export_condprint(*status);
    f = fopen(path, "wb");
    fwrite(buffer, sizeof(char), entrysize, f);
    fclose(f);
    status->counter[2]++;
}

// will do stuff to animations and save
void export_animation(unsigned char *buffer, int entrysize, char *lvlid, char *date, DEPRECATE_INFO_STRUCT *status)
{
    FILE *f;
    char eid[6];
    int eidint = 0;
    char path[MAX - 20] = ""; 
    char cur_type[10] = "";
    char *cpy;
    //    int curr_off;
    int lcltemp;

    for (int i = 0; i < 4; i++)
        eidint = (BYTE * eidint) + buffer[7 - i];

    strncpy(cur_type, "animation\0", 10);
    eid_conv(eidint, eid);

    cpy = (char *)calloc(entrysize, sizeof(char)); // freed here
    memcpy(cpy, buffer, entrysize);

    if (status->portmode)
    {
        if (status->gamemode == 2) // c2 to c3
        {
            // offset fix
            cpy = (char *)realloc(cpy, entrysize + cpy[0xC] * 4);
            for (int j = 1; j <= cpy[0xC]; j++)
            {
                lcltemp = BYTE * cpy[0x11 + 4 * j] + cpy[0x10 + 4 * j];
                lcltemp += 4 * j;
                // cpy[0x11 + 4*j] = lcltemp / BYTE;
                // cpy[0x10 + 4*j] = lcltemp % BYTE;
            }

            // autism
            for (int j = 0; j < cpy[0xC]; j++)
            {
                // curr_off = BYTE * cpy[0x11 + 4*j] + cpy[0x10 + 4*j];
            }
        }
        else // c3 to c2
        {
            for (int j = 0; j < cpy[0xC]; j++)
            {
                ;
            }
        }
    }

    export_make_path(path, cur_type, eidint, lvlid, date, *status);
    sprintf(status->temp, "\t\t saved as '%s'\n", path);
    export_condprint(*status);
    f = fopen(path, "wb");
    fwrite(cpy, sizeof(char), entrysize, f);
    free(cpy);
    fclose(f);
    status->counter[1]++;
}
