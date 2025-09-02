#pragma once

#include "../include.h"

void texture_recolor_stupid();
void scenery_recolor_main();
void scenery_recolor_main2();
void texture_copy_main();
void prop_main();
void resize_main();
void resize_level(char* filepath, double scale[3], char* time, int32_t game);
void resize_chunk_handler(uint8_t* chunk, int32_t game, double scale[3]);
void resize_folder(char* path, double scale[3], char* time, int32_t game);
void resize_zone(int32_t fsize, uint8_t* buffer, double scale[3], int32_t game);
void resize_entity(uint8_t* item, int32_t itemsize, double scale[3]);
void resize_scenery(int32_t fsize, uint8_t* buffer, double scale[3], int32_t game);
void rotate_main();
void rotate_scenery(uint8_t* buffer, char* filepath, double rotation, char* time, int32_t filesize);
void rotate_rotate(uint32_t* y, uint32_t* x, double rotation);
void crate_rotation_angle();
void nsd_gool_table_print_cmd();
void prop_remove_script();
void prop_replace_script();
void generate_spawn();
void c3_ent_resize();
void print_model_tex_refs();
void print_model_tex_refs_nsf();
void print_all_entries_perma();
void entity_usage_folder();
void nsf_props_scr();
void generate_slst();
void cmd_payload_info();
void warp_spawns_generate();
void special_load_lists_list();
void checkpoint_stats();
void nsd_util();
void fov_stats();
void draw_util();
void tpage_util();
void gool_util();
void eid_cmd();
