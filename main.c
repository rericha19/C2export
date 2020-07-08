#include "macros.h"

// Crash 2 levels' entry exporter made by Averso
// half the stuff barely works so dont touch it much

/*
In main loop it waits for a command thats hashed and etc and called, before that it fetches the current time and keeps it until the next command.
There are couple commands that can get called, help and info and stuff arent really interesting, just print some helping messages for at least some UX.

The actual commands are EXPORT, EXPORTALL and IMPORT.
EXPORTALL and EXPORT are fundamentally the same command, both hand file(s) to the "exprt" function.
EXPORTALL hands them one by one from a folder thats specified by the user, theres some basic check on whether its a .NSF file.

"exprt" function opens a file with a level, creates some folders and then chunk by chunk hands the level to "chunk_handler" function, then prints stuff and ends.
"chunk_handler" decides what kind of a chunk it got, either a texture one or not, then hands it to "texture_chunk" or "normal_chunk".

"texture_chunk" increases given entry counter and saves it to given location, no changes made to it at all
"normal_chunk" reads the chunk entry by entry and depending on the entry type hands the entry to another functions, generic or specific

"generic" receives a generic entry and just saves it with a certain name, no changes made to it at all
"zone", "gool", "model" and "animation" make changes based on gamemode, zonemode and portmode to change the files to a given format if needed

IMPORT
stuffs all files from a chosen folder into a chosen .nsf
*/

// TODO
// animations when porting
// t11s when porting C2 TO C3
// write a function for chunk saving
// fix scenery change
// rewrite all times you read big endian stuff, make a from_u16 function, use from_u32 more


INFO status;

int main()
 //actual main
{
    status.print_en = 2;
    status.flog = NULL;
    int zonetype = 8;
    time_t rawtime;
    struct tm * timeinfo;
    char dpath[MAX] = "", fpath[MAX] = "", moretemp[MAX] = "";
    char nsfcheck[4] = "";
    struct dirent *de;

    status.animrefcount = 0;
	intro_text();

    while (1)
    {
        char p_comm_cpy[MAX]= "";
        char lcltemp[9] = "";
        char p_command[MAX];
        scanf("%s",p_command);
        time(&rawtime);
        timeinfo = localtime(&rawtime );
        sprintf(lcltemp,"%02d_%02d_%02d",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
        for (unsigned int i = 0; i < strlen(lcltemp); i++)
                if (!isalnum(lcltemp[i])) lcltemp[i] = '_';
        for (unsigned int i = 0; i < strlen(p_command); i++)
            p_comm_cpy[i] = toupper(p_command[i]);
        switch(hash(p_comm_cpy))
        {
        case KILL:
            return 0;
        case HELP:
            print_help();
            break;
        case IMPORT:
            import_main(lcltemp,status);
            printf("\n");
            break;
        case EXPORT:
            askmode(&zonetype,&status);
            printf("Input the path to the file whose contents you want to export:\n");
            scanf(" %[^\n]",fpath);
            if (fpath[0]=='\"')
            {
                strcpy(fpath,fpath+1);
                *(strchr(fpath,'\0')-1) = '\0';
            }
            export_main(zonetype,fpath,lcltemp);
            printf("\n");
            break;

        case EXPORTALL:
            {
            askmode(&zonetype, &status);
            printf("\nInput the path to the folder whose NSFs you want to export:\n");
            scanf(" %[^\n]",dpath);
            if (dpath[0]=='\"')
            {
                strcpy(dpath,dpath+1);
                *(strchr(dpath,'\0')-1) = '\0';
            }
            // opendir() returns a pointer of DIR type.
            DIR *df = opendir(dpath);

            if (df == NULL)  // opendir returns NULL if couldn't open directory
            {
                printf("[ERROR] Could not open selected directory\n");
                break;
            }

            while ((de = readdir(df)) != NULL)
            {
                strncpy(nsfcheck,strchr(de->d_name,'\0')-3,3);
                strcpy(moretemp,de->d_name);
                if (de->d_name[0]!='.' && !strcmp(nsfcheck,"NSF"))
                {
                    sprintf(fpath,"%s\\%s",dpath,de->d_name);
                    export_main(zonetype, fpath, lcltemp);
                }
            }
            closedir(df);
            break;
            }
        case CHANGEPRINT:
            askprint(&status);
            break;
        case WIPE:
            clrscr();
            intro_text();
            break;
        case RESIZE:
            resize_main(lcltemp,status);
            break;
        case ROTATE:
            rotate_main(lcltemp);
            break;
        case BUILD:
            {
            printf("Input the path to the base level (.nsf)[CAN BE A BLANK FILE]:\n");
            scanf(" %[^\n]",fpath);
            if (fpath[0]=='\"')
            {
                strcpy(fpath,fpath+1);
                *(strchr(fpath,'\0')-1) = '\0';
            }
            printf("\nInput the path to the folder whose contents you want to import:\n");
            scanf(" %[^\n]",dpath);
            if (dpath[0]=='\"')
            {
                strcpy(dpath,dpath+1);
                *(strchr(dpath,'\0')-1) = '\0';
            }
            build_main(fpath, dpath, 21, status, lcltemp);
            printf("Done.\n\n");
            break;
            }
        case PROP:
            printf("Input the path to the file:\n");
            scanf(" %[^\n]",fpath);
            if (fpath[0]=='\"')
            {
                strcpy(fpath,fpath+1);
                *(strchr(fpath,'\0')-1) = '\0';
            }
            prop_main(fpath);
            break;
        case TEXTURE:
            texture_copy_main();
            break;
        default:
            printf("[ERROR] '%s' is not a valid command.\n\n", p_command);
            break;
        }
    }
}
