// Crash 2 levels' entry exporter and other stuff made by Averso
#include "macros.h"

#define CMD(str) (strcmp(p_command, str) == 0)

// main, polls input and runs commands based on it
int32_t main()
{
    intro_text();

    while (1)
    {
        char p_command[MAX] = "";

        scanf("%s", p_command);
        for (uint32_t i = 0; i < strlen(p_command); i++)
            p_command[i] = toupper(p_command[i]);

        if (CMD("KILL"))                    return 0;
        else if (CMD("WIPE"))           {   clrscr();   intro_text();   }
        else if (CMD("HELP"))               print_help();
        else if (CMD("HELP2"))              print_help2();  
        else if (CMD("EXPORT"))             level_alter_pseudorebuild(Alter_Type_LevelExport);
        else if (CMD("RESIZE"))             resize_main();        
        else if (CMD("ROTATE"))             rotate_main();            
        else if (CMD("REBUILD"))            build_main(BuildType_Rebuild);
        else if (CMD("REBUILD_DL"))         build_main(BuildType_Rebuild_DL);
        else if (CMD("PROP"))               prop_main();
        else if (CMD("TEXTURE"))            texture_copy_main();
        else if (CMD("TEXTURE_RECOLOR"))    texture_recolor_stupid();
        else if (CMD("SCEN_RECOLOR"))       scenery_recolor_main2();
        else if (CMD("SCEN_RECOLOR2"))      scenery_recolor_main();
        else if (CMD("A"))                  crate_rotation_angle();
        else if (CMD("NSD"))                nsd_gool_table_print_cmd();
        else if (CMD("EID"))                eid_cmd();
        else if (CMD("PROP_REMOVE"))        prop_remove_script();
        else if (CMD("PROP_REPLACE"))       prop_replace_script();
        else if (CMD("LL_ANALYZE"))         build_ll_analyze();
        else if (CMD("GEN_SPAWN"))          generate_spawn();
        else if (CMD("TIME"))               time_convert();
        else if (CMD("ENT_RESIZE"))         c3_ent_resize();
        else if (CMD("ENT_MOVE"))           entity_move_scr();
        else if (CMD("MODEL_REFS"))         print_model_tex_refs();
        else if (CMD("MODEL_REFS_NSF"))     print_model_tex_refs_nsf();
        else if (CMD("ALL_PERMA"))          print_all_entries_perma();
        else if (CMD("ENTITY_USAGE"))       entity_usage_folder();
        else if (CMD("NSF_PROP"))           nsf_props_scr();
        else if (CMD("GEN_SLST"))           generate_slst();
        else if (CMD("PAYLOAD_INFO"))       ll_payload_info_main();
        else if (CMD("WARP_SPAWNS"))        warp_spawns_generate();
        else if (CMD("LIST_SPECIAL_LL"))    special_load_lists_list();
        else if (CMD("CHECK_UTIL"))         checkpoint_stats();
        else if (CMD("NSD_UTIL"))           nsd_util();
        else if (CMD("FOV_UTIL"))           fov_stats();
        else if (CMD("DRAW_UTIL"))          draw_util();
        else if (CMD("TPAGE_UTIL"))         tpage_util();
        else if (CMD("GOOL_UTIL"))          gool_util();
        else if (CMD("LEVEL_WIPE_DL"))      level_alter_pseudorebuild(Alter_Type_WipeDL);
        else if (CMD("LEVEL_WIPE_ENT"))     level_alter_pseudorebuild(Alter_Type_WipeEnts);
        else if (CMD("CONV_OLD_DL_OR"))     level_alter_pseudorebuild(Alter_Type_Old_DL_Override);
        else if (CMD("FLIP_Y"))             level_alter_pseudorebuild(Alter_Type_FlipScenY);
        else if (CMD("FLIP_X"))             level_alter_pseudorebuild(Alter_Type_FlipScenX);
        else if (CMD("LEVEL_RECOLOR"))      level_alter_pseudorebuild(Alter_Type_LevelRecolor);
        else printf("[ERROR] '%s' is not a valid command.\n\n", p_command);            
    }
}
