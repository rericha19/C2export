// Crash 2 levels' entry exporter and other stuff made by Averso
#include "macros.h"

// main, polls input and runs commands based on it
int32_t main()
{
    intro_text();

    while (1)
    {
        char p_comm_cpy[MAX] = "";
        char p_command[MAX] = "";

        scanf("%s", p_command);
        for (uint32_t i = 0; i < strlen(p_command); i++)
            p_comm_cpy[i] = toupper(p_command[i]);

        switch (comm_str_hash(p_comm_cpy))
        {
        case KILL:
            return 0;
            break;
        case HELP:
            print_help();
            break;
        case HELP2:
            print_help2();
            break;
        case EXPORT:
            level_alter_pseudorebuild(Alter_Type_LevelExport);
            break;
        case HASH:
            hash_cmd();
            break;
        case WIPE:
            clrscr();
            intro_text();
            break;
        case RESIZE:
            resize_main();
            break;
        case ROTATE:
            rotate_main();
            break;
        case REBUILD:
            build_main(BuildType_Rebuild);
            break;
        case REBUILD_DL:
            build_main(BuildType_Rebuild_DL);
            break;
        case PROP:
            prop_main();
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
            nsd_gool_table_print_cmd();
            break;
        case EID:
            eid_cmd();
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
        case FOV_UTIL:
            fov_stats();
            break;
        case DRAW_UTIL:
            draw_util();
            break;
        case TPAGE_UTIL:
            tpage_util();
            break;
        case GOOL_UTIL:
            gool_util();
            break;
        case LEVEL_WIPE_DL:
            level_alter_pseudorebuild(Alter_Type_WipeDL);
            break;
        case LEVEL_WIPE_ENT:
            level_alter_pseudorebuild(Alter_Type_WipeEnts);
            break;
        case CONV_OLD_DL_OVERRIDE:
            level_alter_pseudorebuild(Alter_Type_Old_DL_Override);
            break;
        case FLIP_Y:
            level_alter_pseudorebuild(Alter_Type_FlipScenY);
            break;
        case FLIP_X:
            level_alter_pseudorebuild(Alter_Type_FlipScenX);
            break;
        case LEVEL_RECOLOR:
            level_alter_pseudorebuild(Alter_Type_LevelRecolor);
            break;
        default:
            printf("[ERROR] '%s' is not a valid command.\n\n", p_command);
            break;
        }
    }
}
