#pragma once 

#define eid2str(x) (ENTRY::eid2s(x).c_str())
#define min(a, b) ((b < a) ? b : a)
#define max(a, b) ((b > a) ? b : a)

#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <cstdint>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <algorithm>
#include <string>
#include <filesystem>
#include <chrono>
#include <vector>
#include <mutex>
#include <thread>
#include <fstream>
#include <cstring>
#include <array>
#include <sstream>

struct PROPERTY;
struct RELATION;
class LOAD;
class ENTRY;
class LIST;
class ELIST;
class DEPENDENCY;
class DEPENDENCI;
class SPAWNS;
class MATRIX_STORED_LL;
class RELATIONS;
class ENTITY_PATH;

using DEPENDENCIES = std::vector<DEPENDENCY>;
using MATRIX_STORED_LLS = std::vector<MATRIX_STORED_LL>;
using GENERIC_LOAD_LIST = std::vector<LOAD>;
using LOAD_LIST = GENERIC_LOAD_LIST;
using DRAW_LIST = GENERIC_LOAD_LIST;

#define CNFG_IDX_UNUSED_0 0
#define CNFG_IDX_UNUSED_1 1
#define CNFG_IDX_MTRX_LL_POLL_FLAG 2
#define CNFG_IDX_LL_SLST_DIST_VALUE 3
#define CNFG_IDX_LL_NEIGH_DIST_VALUE 4
#define CNFG_IDX_LL_DRAW_DIST_VALUE 5
#define CNFG_IDX_LL_TRNS_PRLD_FLAG 6
#define CNFG_IDX_LL_BACKWARDS_PENALTY 7
#define CNFG_IDX_UNUSED_8 8
#define CNFG_IDX_UNUSED_9 9
#define CNFG_IDX_LL_REMAKE_FLAG 10
#define CNFG_IDX_MERGE_METHOD_VALUE 11
#define CNFG_IDX_UNUSED_12 12
#define CNFG_IDX_UNUSED_13 13
#define CNFG_IDX_DRAW_LIST_GEN_CAP_X 14
#define CNFG_IDX_DRAW_LIST_GEN_CAP_Y 15
#define CNFG_IDX_DRAW_LIST_GEN_CAP_XZ 16
#define CNFG_IDX_DRAW_LIST_GEN_ANGLE_3D 17

#define PRELOADING_NOTHING 0
#define PRELOADING_TEXTURES_ONLY 1
#define PRELOADING_REG_ENTRIES_ONLY 2
#define PRELOADING_ALL 3

#define C3_GOOL_TABLE_SIZE 0x80

#define BUILD_FPATH_COUNT 4
#define OFFSET 0xC // bad name
#define CHUNKSIZE 65536
#define CHUNK_CHECKSUM_OFFSET 0xC
#define BYTE 0x100
#define MAX 1000
#define PI 3.1415926535
#define CHUNK_LIST_DEFAULT_SIZE 2000
#define PENALTY_MULT_CONSTANT 1000000

#define SPECIAL_METADATA_MASK_LLCOUNT 0xFF
#define SPECIAL_METADATA_MASK_SKIPFLAG 0xFF000000

#define C2_SCEN_OFFSET 0
#define C2_CAMERA_COUNT_OFF 0x188
#define C2_ENTITY_COUNT_OFF 0x18C
#define C2_NEIGHBOURS_START 0x190
#define C2_NEIGHBOURS_END 0x1B4
#define C2_NEIGHBOURS_FLAGS_END 0x1D4
#define C2_SPECIAL_METADATA_OFFSET 0x1DC
#define C2_MUSIC_REF 0x2A4

#define C2_NSD_CHUNK_COUNT_OFFSET 0x400
#define C2_NSD_ENTRY_COUNT_OFFSET 0x404
#define C2_NSD_DATxL_EID 0x408
#define C2_NSD_ENTRY_TABLE_OFFSET 0x520

#define MAGIC_ENTRY 0x100FFFF
#define MAGIC_CHUNK 0x1234
#define EID_NONE 0x6396347Fu

enum BuildType
{
	BuildType_Rebuild,
	BuildType_Rebuild_DL,
};

enum AlterType
{
	AT_WipeDL,
	AT_WipeEnts,
	AT_Old_DL_Override,
	AT_FlipScenY,
	AT_FlipScenX,
	AT_LevelRecolor,
	AT_LevelExport,
};

enum class EntryType : int32_t
{
	__invalid = -1,
	// 0
	Anim = 1,
	Model = 2,
	Scenery = 3,
	SLST = 4,
	Texture = 5,
	LDAT = 6,
	Zone = 7,
	// 8
	// 9
	// 10
	GOOL = 11,
	Sound = 12,
	MIDI = 13,
	Inst = 14,
	VCOL = 15,
	// 16
	// 17
	// 18
	Demo = 19,
	SDIO = 20,
	T21 = 21,
};

enum class ChunkType : int32_t
{
	__invalid = -1,
	NORMAL = 0,
	TEXTURE = 1,
	PROTO_SOUND = 2,
	SOUND = 3,
	INSTRUMENT = 4,
	SPEECH = 5,
};

#define C2_CAM_MODE_3D 0
#define C2_CAM_MODE_CUTSCENE 2
#define C2_CAM_MODE_2D 3
#define C2_CAM_MODE_VERTICAL 8

#define ENTITY_PROP_NAME 0x2C
#define ENTITY_PROP_DEPTH_MODIFIER 0x32
#define ENTITY_PROP_PATH 0x4B
#define ENTITY_PROP_ID 0x9F
#define ENTITY_PROP_ARGUMENTS 0xA4
#define ENTITY_PROP_TYPE 0xA9
#define ENTITY_PROP_SUBTYPE 0xAA
#define ENTITY_PROP_WARPIN_COORDS 0x198
#define ENTITY_PROP_DDA_DEATHS 0x277
#define ENTITY_PROP_VICTIMS 0x287
#define ENTITY_PROP_DDA_SECTION 0x288
#define ENTITY_PROP_BOX_COUNT 0x28B

#define ENTITY_PROP_OVERRIDE_DRAW_ID 0x3FE
#define ENTITY_PROP_OVERRIDE_DRAW_MULT 0x3FF
#define ENTITY_PROP_OVERRIDE_DRAW_MULT_OLD 0x337
#define ENTITY_PROP_OVERRIDE_DRAW_ID_OLD 0x28B

#define ENTITY_PROP_CAMERA_MODE 0x29
#define ENTITY_PROP_CAM_AVG_PT_DIST 0xC9
#define ENTITY_PROP_CAM_SLST 0x103
#define ENTITY_PROP_CAM_PATH_LINKS 0x109
#define ENTITY_PROP_CAM_FOV 0x130
#define ENTITY_PROP_CAM_DRAW_LIST_A 0x13B
#define ENTITY_PROP_CAM_DRAW_LIST_B 0x13C
#define ENTITY_PROP_CAM_INDEX 0x173
#define ENTITY_PROP_CAM_SUBINDEX 0x174
#define ENTITY_PROP_CAM_LINK_COUNT 0x176
#define ENTITY_PROP_CAM_LOAD_LIST_A 0x208
#define ENTITY_PROP_CAM_LOAD_LIST_B 0x209

#define ENTITY_PROP_CAM_DISTANCE 0x142
#define ENTITY_PROP_CAM_WARP_SWITCH 0x1A8
#define ENTITY_PROP_CAM_BG_COLORS 0x1FA
#define ENTITY_PROP_CAM_UPDATE_SCENERY 0x27F

// misc

void* try_malloc(uint32_t size);
void* try_calloc(uint32_t count, uint32_t size);
void intro_text();
void print_help();
void print_help2();
void clrscr();
int32_t from_s32(const uint8_t* data);
uint32_t from_u32(const uint8_t* data);
int32_t from_s16(const uint8_t* data);
uint32_t from_u16(const uint8_t* data);
uint32_t from_u8(const uint8_t* data);
uint32_t eid_to_int(std::string eid);
uint32_t nsfChecksum(const uint8_t* data, int32_t size = CHUNKSIZE);
int32_t cmp_func_int(const void* a, const void* b);
int32_t point_distance_3D(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t z1, int16_t z2);
void path_fix(char* fpath);
double randfrom(double min, double max);
int32_t normalize_angle(int32_t angle);
int32_t c2yaw_to_deg(int32_t yaw);
int32_t deg_to_c2yaw(int32_t deg);
int32_t angle_distance(int32_t angle1, int32_t angle2);
int32_t average_angles(int32_t angle1, int32_t angle2);
int32_t chunk_count_base(FILE* nsf);
int32_t ask_level_ID();
ChunkType chunk_type(uint8_t* chunk);

// build files in no particular order
void build_increment_common(LIST list, LIST entries, int32_t** entry_matrix, int32_t rating);
int32_t dsu_find_set(int32_t i);
void dsu_union_sets(int32_t a, int32_t b);
void build_matrix_merge_util(RELATIONS& relations, ELIST& elist, double merge_ratio);
RELATIONS build_transform_matrix(LIST& entries, int32_t** entry_matrix, ELIST& elist);
uint8_t* build_add_property(uint32_t code, uint8_t* item, int32_t* item_size, PROPERTY* prop);
uint8_t* build_rem_property(uint32_t code, uint8_t* item, int32_t* item_size, PROPERTY* prop);
void build_entity_alter(ENTRY& zone, int32_t item_index, uint8_t* (func_arg)(uint32_t, uint8_t*, int32_t*, PROPERTY*), int32_t property_code, PROPERTY* prop);
void build_load_list_util_util_back(int32_t cam_length, std::vector<LIST>& full_list, int32_t distance, int32_t final_distance, int16_t* coords, int32_t path_length, LIST additions);
void build_load_list_util_util_forw(int32_t cam_length, std::vector<LIST>& full_list, int32_t distance, int32_t final_distance, int16_t* coords, int32_t path_length, LIST additions);
void build_add_collision_dependencies(std::vector<LIST>& full_list, int32_t start_index, int32_t end_index, ENTRY& entry,
	DEPENDENCIES collisions, ELIST& elist);
int32_t build_dist_w_penalty(int32_t distance, int32_t backwards_penalty);
void build_load_list_util_util(ENTRY& ntry, int32_t cam_index, int32_t link_int, std::vector<LIST>& full_list,
	int32_t cam_length, ELIST& elist);
std::vector<LIST> build_get_complete_draw_list(ELIST& elist, ENTRY& zone, int32_t cam_index, int32_t cam_length);
LIST build_get_types_subtypes(ELIST& elist, LIST entity_list, LIST neighbour_list);
int32_t build_get_distance(int16_t* coords, int32_t start_index, int32_t end_index, int32_t cap, int32_t* final_index);
void build_load_list_util(ENTRY& ntry, int32_t camera_index, std::vector<LIST>& full_list, int32_t cam_length, ELIST& elist);
void build_load_list_to_delta(std::vector<LIST>& full_load, std::vector<LIST>& listA, std::vector<LIST>& listB, int32_t cam_length, ELIST& elist, bool is_draw);
void build_remake_draw_lists(ELIST& elist);
void build_remake_load_lists(ELIST& elist);
int32_t** build_get_occurence_matrix(ELIST& elist, LIST entries);
void build_main(int32_t build_rebuild_flag);
void build_texture_count_check(ELIST& elist, std::vector<LIST>& full_load, int32_t cam_length, int32_t i, int32_t j);
bool build_read_and_parse_rebld(ELIST& elist, uint8_t** chunks, bool stats_only, const char* fpath);
MATRIX_STORED_LLS build_matrix_store_lls(ELIST& elist);
void build_matrix_merge_random_main(ELIST& elist);
void build_matrix_merge_random_thr_main(ELIST& elist);
void build_draw_list_util(ELIST& elist, std::vector<LIST>& full_draw, int32_t curr_idx, int32_t neigh_idx, int32_t cam_idx, int32_t neigh_ref_idx, LIST* pos_overrides);
