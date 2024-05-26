#include "macros.h"

// Crash 2 levels' entry exporter and other stuff made by Averso
// half the stuff barely works so dont touch it much
// imo the only part thats in a reasonable shape is the level build related code


//actual main
int main() {
    DEPRECATE_INFO_STRUCT status;
    status.print_en = 2;
    status.flog = NULL;
    int zonetype = 8;
    time_t rawtime;
    struct tm * timeinfo;
    char dpath[MAX] = "", fpath[MAX + 300] = "", moretemp[MAX] = ""; // + 300 to make it shut up
    char nsfcheck[4] = "";
    struct dirent *de;

    status.animrefcount = 0;
	intro_text();

    while (1) {
        char p_comm_cpy[MAX]= "";
        char lcltemp[9] = "";
        char p_command[MAX] = "";
        time(&rawtime);
        timeinfo = localtime(&rawtime );
        sprintf(lcltemp,"%02d_%02d_%02d",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);

        scanf("%s",p_command);
        for (unsigned int i = 0; i < strlen(lcltemp); i++)
            if (!isalnum(lcltemp[i])) lcltemp[i] = '_';
        for (unsigned int i = 0; i < strlen(p_command); i++)
            p_comm_cpy[i] = toupper(p_command[i]);
        switch(comm_str_hash(p_comm_cpy)) {
            case KILL:
                return 0;
            case HELP:
                print_help();
                break;
            case HELP2:
                print_help2();
                break;
            case IMPORT:
                import_main(lcltemp,status);
                printf("\n");
                break;
            case EXPORT:
                export_askmode(&zonetype,&status);
                printf("Input the path to the file whose contents you want to export:\n");
                scanf(" %[^\n]",fpath);
                path_fix(fpath);
                export_main(zonetype, fpath, lcltemp, &status);
                printf("\n");
                break;
            case EXPORTALL:
            {
                export_askmode(&zonetype, &status);
                printf("\nInput the path to the folder whose NSFs you want to export:\n");
                scanf(" %[^\n]",dpath);
                path_fix(dpath);
                // opendir() returns a pointer of DIR type.
                DIR *df = opendir(dpath);

                // opendir returns NULL if couldn't open directory
                if (df == NULL) {
                    printf("[ERROR] Could not open selected directory\n");
                    break;
                }

                while ((de = readdir(df)) != NULL) {
                    strncpy(nsfcheck, strchr(de->d_name,'\0')-3, 3);
                    strcpy(moretemp, de->d_name);
                    if (de->d_name[0]!='.' && !strcmp(nsfcheck,"NSF")) {
                        sprintf(fpath,"%s\\%s",dpath,de->d_name);
                        export_main(zonetype, fpath, lcltemp, &status);
                    }
                }
                closedir(df);
                break;
            }
            case CHANGEPRINT:
                export_askprint(&status);
                break;
            case HASH:
                scanf("%s", fpath);
                printf("%lu\n", comm_str_hash(fpath));
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
                build_main(FUNCTION_BUILD);
                break;
            case REBUILD:
                build_main(FUNCTION_REBUILD);
                break;
            case PROP:
                printf("Input the path to the file:\n");
                scanf(" %[^\n]",fpath);
                path_fix(fpath);
                prop_main(fpath);
                break;
            case TEXTURE:
                texture_copy_main();
                break;
            case TEXTURE_RECOLOR:
                texture_recolor_stupid();
                break;
            case SCEN_RECOLOR:
                scenery_recolor_main2();
                break;
            case SCEN_RECOLOR2:
                scenery_recolor_main();
                break;
            case A:
                crate_rotation_angle();
                break;
            case NSD:
                printf("Input the path to the NSD file:\n");
                scanf(" %[^\n]",fpath);
                path_fix(fpath);
                nsd_gool_table_print(fpath);
                break;
            case EID:
                //printf("EID string:\n");
                scanf(" %[^\n]",fpath);
                unsigned int eid = eid_to_int(fpath);
                printf("%02X %02X %02X %02X\n\n", eid & 0xFF, (eid >> 8) & 0xFF, (eid >> 16) & 0xFF, (eid >> 24) & 0xFF);
                break;
            case PROP_REMOVE:
                prop_remove_script();
                break;
            case PROP_REPLACE:
                prop_replace_script();
                break;
            case LL_ANALYZE:
                build_ll_analyze();
                break;
            case GEN_SPAWN:
                generate_spawn();
                break;
            case TIME:
                time_convert();
                break;
            case ENT_RESIZE:
                c3_ent_resize();
                break;
            case ENT_MOVE:
                entity_move_scr();
                break;
            case MODEL_REFS:
                print_model_tex_refs();
                break;
            case MODEL_REFS_NSF:
                print_model_tex_refs_nsf();
                break;
            case ALL_PERMA:
                print_all_entries_perma();
                break;
            case ENTITY_USAGE:
                entity_usage_folder();
                break;
            case NSF_PROP:
                nsf_props_scr();
                break;
            case GEN_SLST:
                generate_slst();
                break;
            case PAYLOAD_INFO:
                ll_payload_info_main();
                break;
            case WARP_SPAWNS:
                warp_spawns_generate();
                break;
            case LIST_SPECIAL_LL:
                special_load_lists_list();
                break;
            case CHECK_UTIL:
                checkpoint_stats();
                break;
            case NSD_UTIL:
                nsd_util();
                break;
            default:
                printf("[ERROR] '%s' is not a valid command.\n\n", p_command);
                break;
        }
    }
}
