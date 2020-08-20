#include "macros.h"

// Crash 2 levels' entry exporter made by Averso
// half the stuff barely works so dont touch it much

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
        case HASH:
            hash_main();
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
        case BUILD: {
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
        case SCEN_RECOLOR:
            scenery_recolor_main();
            break;
        case A:
            crate_rotation_angle();
            break;
        case NSD:
            printf("Input the path to the NSD file:\n");
            scanf(" %[^\n]",fpath);
            if (fpath[0]=='\"')
            {
                strcpy(fpath,fpath+1);
                *(strchr(fpath,'\0')-1) = '\0';
            }
            nsd_gool_table_print(fpath);
            break;
        default:
            printf("[ERROR] '%s' is not a valid command.\n\n", p_command);
            break;
        }
    }
}
