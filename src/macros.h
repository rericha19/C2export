#define _CRT_SECURE_NO_WARNINGS
#define HAVE_STRUCT_TIMESPEC

#include <stdint.h>
#include "dirent.h"
#include <time.h>
#include <math.h>
#include <stdio.h>

#if COMPILE_WITH_THREADS
#include <pthread.h>
#endif

#ifdef _MSC_VER
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL _Thread_local
#endif

// various constants
#define HASH_TABLE_SIZE 100000000
#define HEAP_SIZE_INCREMENT 200000

#define PAYLOAD_MERGE_STATS_ONLY 1
#define PAYLOAD_MERGE_FULL_USE 0

#define CNFG_IDX_DPR_MERGE_GOOL_FLAG 0
#define CNFG_IDX_DPR_MERGE_ZONE_FLAG 1
#define CNFG_IDX_MTRX_LL_POLL_FLAG 2
#define CNFG_IDX_LL_SLST_DIST_VALUE 3
#define CNFG_IDX_LL_NEIGH_DIST_VALUE 4
#define CNFG_IDX_LL_DRAW_DIST_VALUE 5
#define CNFG_IDX_LL_TRNS_PRLD_FLAG 6
#define CNFG_IDX_LL_BACKWARDS_PENALTY 7
#define CNFG_IDX_MTRI_REL_SORT_FLAG 8
#define CNFG_IDX_LL_SND_INCLUSION_FLAG 9
#define CNFG_IDX_LL_REMAKE_FLAG 10
#define CNFG_IDX_MERGE_METHOD_VALUE 11
#define CNFG_IDX_MTRX_PERMA_INC_FLAG 12
#define CNFG_IDX_MTRI_ZEROVAL_INC_FLAG 13
#define CNFG_IDX_DRAW_LIST_GEN_CAP_X 14
#define CNFG_IDX_DRAW_LIST_GEN_CAP_Y 15
#define CNFG_IDX_DRAW_LIST_GEN_CAP_XZ 16
#define CNFG_IDX_DRAW_LIST_GEN_ANGLE_3D 17

#define PRELOADING_NOTHING 0
#define PRELOADING_TEXTURES_ONLY 1
#define PRELOADING_REG_ENTRIES_ONLY 2
#define PRELOADING_ALL 3

#define C2_GOOL_TABLE_SIZE 0x40
#define BUILD_FPATH_COUNT 4
#define OFFSET 0xC // bad name
#define CHUNKSIZE 65536
#define CHUNK_CHECKSUM_OFFSET 0xC
#define BYTE 0x100
#define MAX 1000
#define PI 3.1415926535
#define QUEUE_ITEM_COUNT 2500
#define ELIST_DEFAULT_SIZE 2500
#define CHUNK_LIST_DEFAULT_SIZE 2500
#define PENALTY_MULT_CONSTANT 1000000

#define SPECIAL_METADATA_MASK_LLCOUNT 0xFF
#define SPECIAL_METADATA_MASK_SKIPFLAG 0xFF000000

#define C2_NEIGHBOURS_START 0x190
#define C2_NEIGHBOURS_END 0x1B4
#define C2_NEIGHBOURS_FLAGS_END 0x1D4
#define C2_SPECIAL_METADATA_OFFSET 0x1DC
#define C2_NSD_CHUNK_COUNT_OFFSET 0x400
#define C2_NSD_ENTRY_COUNT_OFFSET 0x404
#define C2_NSD_DATxL_EID 0x408
#define C2_NSD_ENTRY_TABLE_OFFSET 0x520
#define MAGIC_ENTRY 0x100FFFF
#define MAGIC_CHUNK 0x1234
#define CHUNK_TYPE_NORMAL 0
#define CHUNK_TYPE_TEXTURE 1
#define CHUNK_TYPE_PROTO_SOUND 2
#define CHUNK_TYPE_SOUND 3
#define CHUNK_TYPE_INSTRUMENT 4
#define C2_MUSIC_REF 0x2A4
#define EID_NONE 0x6396347Fu

// commands
#define HASH 2089134665u
#define HELP 2089138798u
#define HELP2 222103648u

#define WIPE 2089682330u
#define BUILD 215559733u
#define REBUILD 1370829996u
#define EXPORT 2939723207u
#define EXPORTALL 1522383616u
#define PROP 2089440550u
#define NSF_PROP 387011244u
#define PROP_REMOVE 4293790963u
#define PROP_REPLACE 4259575393u
#define TEXTURE 3979833526u
#define SCEN_RECOLOR 2919463267u
#define TEXTURE_RECOLOR 1888631019u
#define NSD 193464746u
#define EID 193454615u
#define LL_ANALYZE 4033854192u
#define GEN_SPAWN 1178716487u
#define MODEL_REFS 2556337893u
#define MODEL_REFS_NSF 3538090027u
#define ENTITY_USAGE 3822178934u
#define PAYLOAD_INFO 2536593146u
#define REBUILD_DL 242786907u
#define LEVEL_WIPE_DL 759838528u
#define LEVEL_WIPE_ENT 3599836183u

#define KILL 2089250961u
#define WARP_SPAWNS 3243144250u
#define CHECK_UTIL 2424829088u
#define LIST_SPECIAL_LL 2035609912u
#define A 177638u
#define TIME 2089574420u
#define GEN_SLST 2118123300u
#define ALL_PERMA 1727230258u
#define SCEN_RECOLOR2 1853007349u
#define RESIZE 3426052343u
#define ROTATE 3437938580u
#define ENT_RESIZE 2772317469u
#define ENT_MOVE 1753493186u
#define CHANGEPRINT 2239644728u
#define IMPORT 3083219648u
#define NSD_UTIL 308922119u
#define FOV_UTIL 2792294477u
#define DRAW_UTIL 3986824656u
#define TPAGE_UTIL 3993095059u
#define GOOL_UTIL 2723710419u
#define CONV_OLD_DL_OVERRIDE 2382887815u
#define FLIP_Y 2964377512u
#define FLIP_X 2964377511u
#define LEVEL_RECOLOR 2720813778u

// #define INTRO                           223621809u
// #define STATUS                          3482341513u
// #define CHANGEMODE                      588358864u

enum
{
    BuildType_Build = 1,
    BuildType_Rebuild,
    BuildType_Rebuild_DL,
};

enum
{
    Alter_Type_WipeDL,
    Alter_Type_WipeEnts,
    Alter_Type_Old_DL_Override,
    Alter_Type_FlipScenY,
    Alter_Type_FlipScenX,
    Alter_Type_LevelRecolor,
};

#define ENTRY_TYPE_ANIM 0x1
#define ENTRY_TYPE_MODEL 0x2
#define ENTRY_TYPE_SCENERY 0x3
#define ENTRY_TYPE_SLST 0x4
#define ENTRY_TYPE_TEXTURE 0x5
#define ENTRY_TYPE_ZONE 0x7
#define ENTRY_TYPE_GOOL 0xB
#define ENTRY_TYPE_SOUND 0xC
#define ENTRY_TYPE_MIDI 0xD
#define ENTRY_TYPE_INST 0xE
#define ENTRY_TYPE_VCOL 0xF
#define ENTRY_TYPE_DEMO 0x13
#define ENTRY_TYPE_T21 21

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

#define STATE_SEARCH_EVAL_INVALID 0x80000000u
#define STATE_SEARCH_EVAL_SUCCESS 0

// #define min(a,b) (((a)<(b))?(a):(b)) // .c doesnt need these two
// #define max(a,b) (((a)>(b))?(a):(b))
// #define abs(a)   (((a)> 0) ?(a):(-a))

// in export script used to keep track of status stuff
typedef struct info
{
    int32_t counter[22];    // counts entry types, counter[0] is total entry count
    int32_t print_en;       // variable for storing printing status 0 - nowhere, 1 - screen, 2 - file, 3 - both
    FILE *flog;             // file where the logs are exported if file print is enabled
    char temp[MAX];         // strings are written into this before being printed by condprint
    int32_t gamemode;       // 2 for C2, 3 for C3
    int32_t portmode;       // 0 for normal export, 1 for porting
    uint32_t anim[1024][3]; // field one is model, field 3 is animation, field 2 is original animation when c3->c2
    uint32_t animrefcount;  // count of animation references when porting c3 to c2
} DEPRECATE_INFO_STRUCT;

// in build scripts, used to store spawns
typedef struct spawn
{
    int32_t x;
    int32_t y;
    int32_t z;
    uint32_t zone;
} SPAWN;

// spawns struct used in build to store spawns
typedef struct spawns
{
    int32_t spawn_count;
    SPAWN *spawns;
} SPAWNS;

// list struct, used to store various values and eids
typedef struct list
{
    int32_t count;
    // int32_t real_count;
    uint32_t *eids;
} LIST;

// used to store entry information in the build script
typedef struct entry
{
    uint32_t eid;
    int32_t esize;
    int32_t chunk;
    // int32_t og_chunk;
    uint8_t *data;
    uint32_t *related;
    uint32_t *distances;
    uint32_t *visited;
    uint8_t norm_chunk_ent_is_loaded;
} ENTRY;

typedef struct draw_item
{
    uint8_t neighbour_zone_index;
    uint16_t ID;
    uint8_t neighbour_item_index;
} DRAW_ITEM;

// used to sort load lists
typedef struct item
{
    int32_t eid;
    int32_t index;
} LOAD_LIST_ITEM_UTIL;

// used to store payload information
typedef struct payload
{
    int32_t *chunks;
    int32_t count;
    int32_t *tchunks;
    int32_t tcount;
    int32_t entcount;
    uint32_t zone;
    int32_t cam_path;
} PAYLOAD;

// load list data stored for matrix merge, so payload doesnt need to read them every iter
typedef struct load_matrix
{
    uint32_t zone;
    int32_t cam_path;
    LIST full_load;
} MATRIX_STORED_LL;

typedef struct loads_matrix
{
    int32_t count;
    MATRIX_STORED_LL *stored_lls;
} MATRIX_STORED_LLS;

// used to store payload ladder
typedef struct payloads
{
    int32_t count;
    PAYLOAD *arr;
} PAYLOADS;

// used to store load or draw list in its non-delta form
typedef struct load
{
    char type;
    int32_t list_length;
    uint32_t *list;
    int32_t index;
} LOAD;

// used to keep the entire non-delta load/draw list
typedef struct load_list
{
    int32_t count;
    LOAD array[1000]; // yep
} LOAD_LIST;

// used in matrix merge to store what entries are loaded simultaneously and how much/often
typedef struct relation
{
    int32_t value;
    int32_t total_occurences;
    int32_t index1;
    int32_t index2;
} RELATION;

// used to keep all relations
typedef struct relations
{
    int32_t count;
    RELATION *relations;
} RELATIONS;

// used to store type & subtype dependencies in the build script (and collision dependencies too)
typedef struct inf
{
    int32_t type;
    int32_t subtype;
    LIST dependencies;
} DEPENDENCY;

// all dependencies
typedef struct
{
    int32_t count;
    DEPENDENCY *array;
} DEPENDENCIES;

// stores a camera path link
typedef struct link
{
    uint8_t type;
    uint8_t zone_index;
    uint8_t cam_index;
    uint8_t flag;
} CAMERA_LINK;

// entity/item property
typedef struct property
{
    uint8_t header[8];
    uint8_t *data;
    int32_t length;
} PROPERTY;

// used when figuring out cam paths' distance to spawn
typedef struct entry_queue
{
    int32_t add_index;
    int32_t pop_index;
    int32_t zone_indices[QUEUE_ITEM_COUNT];
    int32_t camera_indices[QUEUE_ITEM_COUNT];
} DIST_GRAPH_Q;

// used to represent states in a* alg
typedef struct state_set_search_struct
{
    uint16_t *entry_chunk_array;
    uint32_t estimated;
    uint32_t elapsed;
} STATE_SEARCH_STR;

// stores queue of states in a* alg
typedef struct state_set_search_heap
{
    uint32_t length;
    uint32_t real_size;
    STATE_SEARCH_STR **heap_array;
} STATE_SEARCH_HEAP;

// used to store visited/considered/not-to-be-visited again configurations of entry-chunk assignments
typedef struct hash_item
{
    struct hash_item *next;
    uint16_t *entry_chunk_array;
} HASH_ITEM;

// master hash table that contains hash items
typedef struct hash_table
{
    int32_t table_length;
    int32_t key_length;
    int32_t item_count; // not really necessary
    HASH_ITEM **items;
    int32_t (*hash_function)(struct hash_table *table, uint16_t *entry_chunk_array);
} HASH_TABLE;

// used to sort chunks in states in state search to get rid of as many permutations as possible
typedef struct chunk_str
{
    int32_t chunk_index;
    uint16_t chunk_size;
    uint16_t entry_count;
} CHUNK_STR;

// misc.c

void export_printstatus(int32_t zonetype, int32_t gamemode, int32_t portmode);
void intro_text();
void print_help();
void print_help2();
void export_countprint(DEPRECATE_INFO_STRUCT status);
void export_condprint(DEPRECATE_INFO_STRUCT status);
void clrscr();
int32_t from_s32(uint8_t *data);
uint32_t from_u32(uint8_t *data);
int32_t from_s16(uint8_t *data);
uint32_t from_u16(uint8_t *data);
uint32_t from_u8(uint8_t *data);
void export_countwipe(DEPRECATE_INFO_STRUCT *status);
void export_askprint(DEPRECATE_INFO_STRUCT *status);
uint32_t comm_str_hash(const char *str);
const char *eid_conv2(uint32_t value);
const char *eid_conv(uint32_t value, char *eid);
uint32_t eid_to_int(char *eid);
uint32_t crcChecksum(const uint8_t *data, int32_t size);
uint32_t nsfChecksum(const uint8_t *data);
void export_make_path(char *finalpath, char *type, int32_t eid, char *lvlid, char *date, DEPRECATE_INFO_STRUCT status);
void export_askmode(int32_t *zonetype, DEPRECATE_INFO_STRUCT *status);
int32_t cmp_func_int(const void *a, const void *b);
int32_t cmp_func_load(const void *a, const void *b);
int32_t cmp_func_load2(const void *a, const void *b);
int32_t cmp_func_payload(const void *a, const void *b);
int32_t cmp_func_uint(const void *a, const void *b);
int32_t load_list_sort(const void *a, const void *b);
int32_t cmp_func_eid(const void *a, const void *b);
int32_t cmp_func_esize(const void *a, const void *b);
int32_t relations_cmp(const void *a, const void *b);
int32_t relations_cmp2(const void *a, const void *b);
LIST init_list();
SPAWNS init_spawns();
int32_t list_find(LIST list, uint32_t searched);
void list_add(LIST *list, uint32_t eid);
int32_t list_is_subset(LIST list_a, LIST list_b);
void list_remove(LIST *list, uint32_t eid);
void list_copy_in(LIST *destination, LIST source);
LOAD_LIST init_load_list();
int32_t point_distance_3D(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t z1, int16_t z2);
CAMERA_LINK int_to_link(uint32_t link);
void delete_load_list(LOAD_LIST load_list);
void path_fix(char *fpath);
DIST_GRAPH_Q graph_init();
void graph_add(DIST_GRAPH_Q *graph, ENTRY *elist, int32_t zone_index, int32_t camera_index);
void graph_pop(DIST_GRAPH_Q *graph, int32_t *zone_index, int32_t *cam_index);
int32_t build_get_nth_item_offset(uint8_t *entry, int32_t n);
uint8_t *build_get_nth_item(uint8_t *entry, int32_t n);
int32_t getline(char **linep, int32_t *n, FILE *fp);
int32_t getdelim(char **linep, int32_t *n, int32_t delim, FILE *fp);
DEPENDENCIES build_init_dep();

// export.c

int32_t export_main(int32_t zone, char *fpath, char *date, DEPRECATE_INFO_STRUCT *status);
int32_t export_chunk_handler(uint8_t *buffer, int32_t chunkid, char *lvlid, char *date, int32_t zonetype, DEPRECATE_INFO_STRUCT *status);
int32_t export_normal_chunk(uint8_t *buffer, char *lvlid, char *date, int32_t zonetype, DEPRECATE_INFO_STRUCT *status);
int32_t export_texture_chunk(uint8_t *buffer, char *lvlid, char *date, DEPRECATE_INFO_STRUCT *status);
int32_t export_camera_fix(uint8_t *cam, int32_t length);
void export_entity_coord_fix(uint8_t *item, int32_t itemlength);
void export_scenery(uint8_t *buffer, int32_t entrysize, char *lvlid, char *date, DEPRECATE_INFO_STRUCT *status);
void export_generic_entry(uint8_t *buffer, int32_t entrysize, char *lvlid, char *date, DEPRECATE_INFO_STRUCT *status);
void export_gool(uint8_t *buffer, int32_t entrysize, char *lvlid, char *date, DEPRECATE_INFO_STRUCT *status);
void export_zone(uint8_t *buffer, int32_t entrysize, char *lvlid, char *date, int32_t zonetype, DEPRECATE_INFO_STRUCT *status);
void export_model(uint8_t *buffer, int32_t entrysize, char *lvlid, char *date, DEPRECATE_INFO_STRUCT *status);
void export_animation(uint8_t *buffer, int32_t entrysize, char *lvlid, char *date, DEPRECATE_INFO_STRUCT *status);

// import.c

int32_t import_main(char *time, DEPRECATE_INFO_STRUCT status);
int32_t import_file_lister(char *path, FILE *fnew);
void import_chunksave(uint8_t *chunk, int32_t *index, int32_t *curr_off, int32_t *curr_chunk, FILE *fnew, int32_t offsets[]);

// build files in no particular order

double randfrom(double min, double max);
void build_get_box_count(ENTRY *elist, int32_t entry_count);
int32_t build_item_count(uint8_t *entry);
int32_t build_prop_count(uint8_t *item);
LOAD_LIST build_get_draw_lists(uint8_t *entry, int32_t cam_index);
LOAD_LIST build_get_load_lists(uint8_t *entry, int32_t cam_index);
LOAD_LIST build_get_lists(int32_t prop_code, uint8_t *entry, int32_t cam_index);
DRAW_ITEM build_decode_draw_item(uint32_t value);
void build_ll_analyze();
int32_t build_align_sound(int32_t input);
uint32_t build_get_zone_track(uint8_t *entry);
LIST build_get_models(uint8_t *animation);
uint32_t build_get_model(uint8_t *anim, int32_t item);
int32_t build_remove_empty_chunks(int32_t index_start, int32_t index_end, int32_t entry_count, ENTRY *entry_list);
void build_remove_invalid_references(ENTRY *elist, int32_t entry_count, int32_t entry_count_base);
int32_t build_get_base_chunk_border(uint32_t textr, uint8_t **chunks, int32_t index_end);
void build_get_model_references(ENTRY *elist, int32_t entry_count);
int32_t build_get_index(uint32_t eid, ENTRY *elist, int32_t entry_count);
uint32_t build_get_slst(uint8_t *item);
uint32_t build_get_path_length(uint8_t *item);
int16_t *build_get_path(ENTRY *elist, int32_t zone_index, int32_t item_index, int32_t *path_len);
int32_t *build_seek_spawn(uint8_t *item);
int32_t build_get_neighbour_count(uint8_t *entry);
LIST build_get_neighbours(uint8_t *entry);
int32_t build_get_cam_item_count(uint8_t *entry);
int32_t build_get_scen_count(uint8_t *entry);
int32_t build_get_entity_count(uint8_t *entry);
uint32_t *build_get_zone_relatives(uint8_t *entry, SPAWNS *spawns);
int32_t build_entry_type(ENTRY entry);
int32_t build_chunk_type(uint8_t *chunk);
uint32_t *build_get_gool_relatives(uint8_t *entry, int32_t entrysize);
void build_read_nsf(ENTRY *elist, int32_t chunk_border_base, uint8_t **chunks, int32_t *chunk_border_texture,
                    int32_t *entry_count, FILE *nsf, uint32_t *gool_table);
void build_dumb_merge(ENTRY *elist, int32_t chunk_index_start, int32_t *chunk_index_end, int32_t entry_count);
void build_read_folder(DIR *df, char *dirpath, uint8_t **chunks, ENTRY *elist, int32_t *chunk_border_texture,
                       int32_t *entry_count, SPAWNS *spawns, uint32_t *gool_table);
void deprecate_build_print_relatives(ENTRY *elist, int32_t entry_count);
void build_swap_spawns(SPAWNS spawns, int32_t spawnA, int32_t spawnB);
void build_write_nsd(FILE *nsd, FILE *nsd2, ENTRY *elist, int32_t entry_count, int32_t chunk_count, SPAWNS spawns, uint32_t *gool_table, int32_t level_ID);
void build_increment_common(LIST list, LIST entries, int32_t **entry_matrix, int32_t rating);
int32_t dsu_find_set(int32_t i);
void dsu_union_sets(int32_t a, int32_t b);
void build_matrix_merge_util(RELATIONS relations, ENTRY *elist, int32_t entry_count, double merge_ratio);
RELATIONS build_transform_matrix(LIST entries, int32_t **entry_matrix, int32_t *config, ENTRY *elist, int32_t entry_count);
void build_matrix_merge(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, int32_t *config, LIST permaloaded, double merge_ratio, double mult);
void build_normal_chunks(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t chunk_count, uint8_t **chunks, int32_t do_end_print);
int32_t build_get_prop_offset(uint8_t *item, int32_t prop_code);
int32_t build_get_entity_prop(uint8_t *entity, int32_t prop_code);
void build_add_scen_textures_to_list(uint8_t *scenery, LIST *list);
void build_add_model_textures_to_list(uint8_t *model, LIST *list);
uint8_t *build_add_property(uint32_t code, uint8_t *item, int32_t *item_size, PROPERTY *prop);
uint8_t *build_rem_property(uint32_t code, uint8_t *item, int32_t *item_size, PROPERTY *prop);
void build_replace_item(ENTRY *zone, int32_t item_index, uint8_t *new_item, int32_t new_size);
void build_entity_alter(ENTRY *zone, int32_t item_index, uint8_t *(func_arg)(uint32_t, uint8_t *, int32_t *, PROPERTY *),
                        int32_t property_code, PROPERTY *prop);
void build_remove_nth_item(ENTRY *zone, int32_t n);
LIST build_get_links(uint8_t *entry, int32_t cam_index);
void build_load_list_util_util_back(int32_t cam_length, LIST *full_list, int32_t distance, int32_t final_distance, int16_t *coords, int32_t path_length, LIST additions);
void build_load_list_util_util_forw(int32_t cam_length, LIST *full_list, int32_t distance, int32_t final_distance, int16_t *coords, int32_t path_length, LIST additions);
void build_add_collision_dependencies(LIST *full_list, int32_t start_index, int32_t end_index, uint8_t *entry,
                                      DEPENDENCIES collisions, ENTRY *elist, int32_t entry_count);
int32_t build_dist_w_penalty(int32_t distance, int32_t backwards_penalty);
void build_load_list_util_util(int32_t zone_index, int32_t cam_index, int32_t link_int, LIST *full_list,
                               int32_t cam_length, ENTRY *elist, int32_t entry_count, int32_t *config, DEPENDENCIES collisisons);
LIST *build_get_complete_draw_list(ENTRY *elist, int32_t zone_index, int32_t cam_index, int32_t cam_length);
LIST build_get_types_subtypes(ENTRY *elist, int32_t entry_count, LIST entity_list, LIST neighbour_list);
int32_t build_get_distance(int16_t *coords, int32_t start_index, int32_t end_index, int32_t cap, int32_t *final_index);
LIST build_get_entity_list(int32_t point_index, int32_t zone_index, int32_t camera_index, int32_t cam_length, ENTRY *elist,
                           int32_t entry_count, LIST *neighbours, int32_t *config);
void build_load_list_util(int32_t zone_index, int32_t camera_index, LIST *full_list, int32_t cam_length, ENTRY *elist,
                          int32_t entry_count, DEPENDENCIES sub_info, DEPENDENCIES collisions, int32_t *config);
PROPERTY build_make_load_list_prop(LIST *list_array, int32_t cam_length, int32_t code);
void build_find_unspecified_entities(ENTRY *elist, int32_t entry_count, DEPENDENCIES sub_info);
void build_load_list_to_delta(LIST *full_load, LIST *listA, LIST *listB, int32_t cam_length, ENTRY *elist, int32_t entry_count, int32_t is_draw);
LIST build_read_special_entries(uint8_t *zone);
LIST build_get_special_entries(ENTRY zone, ENTRY *elist, int32_t entry_count);
void build_remake_draw_lists(ENTRY *elist, int32_t entry_count, int32_t *config);
void build_remake_load_lists(ENTRY *elist, int32_t entry_count, uint32_t *gool_table, LIST permaloaded,
                             DEPENDENCIES subtype_info, DEPENDENCIES collision, DEPENDENCIES mus_deps, int32_t *config);
int32_t build_read_entry_config(LIST *permaloaded, DEPENDENCIES *subtype_info, DEPENDENCIES *collisions, DEPENDENCIES *music_deps,
                                ENTRY *elist, int32_t entry_count, uint32_t *gool_table, int32_t *config);
int32_t build_get_chunk_count_base(FILE *nsf);
int32_t build_ask_ID();
void build_ask_list_paths(char fpaths[BUILD_FPATH_COUNT][MAX], int32_t *config);
void build_instrument_chunks(ENTRY *elist, int32_t entry_count, int32_t *chunk_count, uint8_t **chunks);
void build_sound_chunks(ENTRY *elist, int32_t entry_count, int32_t *chunk_count, uint8_t **chunks);
int32_t build_assign_primary_chunks_all(ENTRY *elist, int32_t entry_count, int32_t *chunk_count);
LIST build_get_normal_entry_list(ENTRY *elist, int32_t entry_count);
int32_t **build_get_occurence_matrix(ENTRY *elist, int32_t entry_count, LIST entries, int32_t *config);
int32_t build_is_normal_chunk_entry(ENTRY entry);
void build_matrix_merge_relative_main(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, int32_t *config, LIST permaloaded);
void build_matrix_merge_main(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, int32_t *config, LIST permaloaded);
void build_matrix_merge_relative(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, int32_t *config, LIST permaloaded,
                                 double merge_ratio);
void build_cleanup_elist(ENTRY *elist, int32_t entry_count);
void build_final_cleanup(ENTRY *elist, int32_t entry_count, uint8_t **chunks, int32_t chunk_count, FILE *nsfnew, FILE *nsd, DEPENDENCIES dep1, DEPENDENCIES dep2);
void build_ask_spawn(SPAWNS spawns);
void build_main(int32_t build_rebuild_flag);
void build_write_nsf(FILE *nsfnew, ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t chunk_count, uint8_t **chunks, FILE *nsfnew2);
LIST build_get_sceneries(uint8_t *entry);
void build_check_item_count(uint8_t *zone, int32_t eid);
void build_get_distance_graph(ENTRY *elist, int32_t entry_count, SPAWNS spawns);
void build_ask_draw_distances(int32_t *config);
void build_ask_distances(int32_t *config);
int32_t build_is_before(ENTRY *elist, int32_t zone_index, int32_t camera_index, int32_t neighbour_index, int32_t neighbour_cam_index);
int32_t build_permaloaded_merge(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, LIST permaloaded);
void build_texture_count_check(ENTRY *elist, int32_t entry_count, LIST *full_load, int32_t cam_length, int32_t i, int32_t j);
int32_t build_read_and_parse_build(int32_t *level_ID, FILE **nsfnew, FILE **nsd, int32_t *chunk_border_texture, uint32_t *gool_table,
                                   ENTRY *elist, int32_t *entry_count, uint8_t **chunks, SPAWNS *spawns);
int32_t build_read_and_parse_rebld(int32_t *level_ID, FILE **nsfnew, FILE **nsd, int32_t *chunk_border_texture, uint32_t *gool_table,
                                   ENTRY *elist, int32_t *entry_count, uint8_t **chunks, SPAWNS *spawns, int32_t stats_only, char *fpath);
void build_sort_load_lists(ENTRY *elist, int32_t entry_count);
void build_ask_build_flags(int32_t *config);
void build_ask_premerge(int32_t *premerge_type, double *merge_ratio);
void build_matrix_merge_random_main(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, int32_t *config, LIST permaloaded);
void build_matrix_merge_random_thr_main(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, int32_t *config, LIST permaloaded);
void build_try_second_output(FILE **nsfnew2, FILE **nsd2, int32_t levelID);
int32_t abs(int32_t val);
int32_t normalize_angle(int32_t angle);
int32_t c2yaw_to_deg(int32_t yaw);
int32_t deg_to_c2yaw(int32_t deg);
int32_t angle_distance(int32_t angle1, int32_t angle2);
int32_t average_angles(int32_t angle1, int32_t angle2);
void build_draw_list_util(ENTRY *elist, int32_t entry_count, LIST *full_draw, int32_t *config, int32_t curr_idx, int32_t neigh_idx, int32_t cam_idx, int32_t neigh_ref_idx, LIST *pos_overrides);
void build_remake_draw_lists(ENTRY *elist, int32_t entry_count, int32_t *config);

// state thing

void build_merge_state_search_main(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, int32_t *config, LIST permaloaded);
STATE_SEARCH_STR *build_state_search_str_init(int32_t length);
void build_state_search_str_destroy(STATE_SEARCH_STR *state);
uint32_t build_state_search_eval_state(LIST *stored_load_lists, int32_t load_list_snapshot_count, STATE_SEARCH_STR *state, int32_t key_length,
                                       int32_t first_nonperma_chunk, int32_t perma_count, int32_t max_payload_limit, int32_t *max_pay);
int32_t build_state_search_str_chunk_max(STATE_SEARCH_STR *state, int32_t key_length);
STATE_SEARCH_STR *build_state_search_merge_chunks(STATE_SEARCH_STR *state, uint32_t chunk1, uint32_t chunk2, int32_t key_length, int32_t first_nonperma_chunk, ENTRY *temp_elist);
STATE_SEARCH_STR *build_state_search_init_state_convert(ENTRY *elist, int32_t entry_count, int32_t first_nonperma_chunk_index, int32_t key_length);
int32_t build_state_search_is_empty_chunk(STATE_SEARCH_STR *state, uint32_t chunk_index, int32_t key_length);
uint32_t *build_state_search_init_elist_convert(ENTRY *elist, int32_t entry_count, int32_t first_nonperma_chunk_index, int32_t *key_length);
void build_state_search_solve(ENTRY *elist, int32_t entry_count, int32_t first_nonperma_chunk_index, int32_t perma_chunk_count);
MATRIX_STORED_LLS
build_matrix_store_lls(ENTRY *elist, int32_t entry_count);
PAYLOADS build_matrix_get_payload_ladder(MATRIX_STORED_LLS stored_lls, ENTRY *elist, int32_t entry_count, int32_t chunk_min, int32_t get_tpages);

// deprecate_build.c

PAYLOADS deprecate_build_get_payload_ladder(ENTRY *elist, int32_t entry_count, int32_t chunk_min);
void deprecate_build_payload_merge(ENTRY *elist, int32_t entry_count, int32_t chunk_min, int32_t *chunk_count, int32_t stats_only_flag);
void deprecate_build_insert_payload(PAYLOADS *payloads, PAYLOAD insertee);
void deprecate_build_print_payload(PAYLOAD payload, int32_t stopper);
int32_t deprecate_build_merge_thing(ENTRY *elist, int32_t entry_count, int32_t *chunks, int32_t chunk_count);
int32_t deprecate_build_get_common(int32_t *listA, int32_t countA, int32_t *listB, int32_t countB);
void deprecate_build_chunk_merge(ENTRY *elist, int32_t entry_count, int32_t *chunks, int32_t chunk_count);
PAYLOAD deprecate_build_get_payload(ENTRY *elist, int32_t entry_count, LIST list, uint32_t zone, int32_t chunk_min, int32_t get_tpages);
void deprecate_build_gool_merge(ENTRY *elist, int32_t chunk_index_start, int32_t *chunk_index_end, int32_t entry_count);
int32_t deprecate_build_is_relative(uint32_t searched, uint32_t *array, int32_t count);
void deprecate_build_ll_add_children(uint32_t eid, ENTRY *elist, int32_t entry_count, LIST *list, uint32_t *gool_table, DEPENDENCIES dependencies);
void deprecate_build_assign_primary_chunks_gool(ENTRY *elist, int32_t entry_count, int32_t *real_chunk_count, int32_t grouping_flag);
void deprecate_build_assign_primary_chunks_rest(ENTRY *elist, int32_t entry_count, int32_t *chunk_count);
void deprecate_build_assign_primary_chunks_zones(ENTRY *elist, int32_t entry_count, int32_t *real_chunk_count, int32_t grouping_flag);
void deprecate_build_payload_merge_main(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t *chunk_count, int32_t *config, LIST permaloaded);

// side_scripts.c

int32_t texture_recolor_stupid();
int32_t scenery_recolor_main();
int32_t scenery_recolor_main2();
int32_t texture_copy_main();
void prop_main(char *path);
void resize_main(char *time, DEPRECATE_INFO_STRUCT status);
void resize_level(FILE *level, char *filepath, double scale[3], char *time, DEPRECATE_INFO_STRUCT status);
void resize_chunk_handler(uint8_t *chunk, DEPRECATE_INFO_STRUCT status, double scale[3]);
void resize_folder(DIR *df, char *path, double scale[3], char *time, DEPRECATE_INFO_STRUCT status);
void resize_zone(int32_t fsize, uint8_t *buffer, double scale[3], DEPRECATE_INFO_STRUCT status);
void resize_entity(uint8_t *item, int32_t itemsize, double scale[3], DEPRECATE_INFO_STRUCT status);
void resize_scenery(int32_t fsize, uint8_t *buffer, double scale[3], DEPRECATE_INFO_STRUCT status);
void rotate_main(char *time);
void rotate_scenery(uint8_t *buffer, char *filepath, double rotation, char *time, int32_t filesize);
void rotate_zone(uint8_t *buffer, char *filepath, double rotation);
void rotate_rotate(uint32_t *y, uint32_t *x, double rotation);
void crate_rotation_angle();
void nsd_gool_table_print(char *fpath);
PROPERTY *build_get_prop_full(uint8_t *item, int32_t prop_code);
void prop_remove_script();
void prop_replace_script();
void generate_spawn();
void time_convert();
void c3_ent_resize();
void entity_move_scr();
void print_model_tex_refs();
void print_model_tex_refs_nsf();
void print_all_entries_perma();
void entity_usage_folder();
void nsf_props_scr();
void generate_slst();
void ll_payload_info_main();
void warp_spawns_generate();
void special_load_lists_list();
void checkpoint_stats();
void nsd_util();
void fov_stats();
void draw_util();
void tpage_util();
void gool_util();

// level_alter.c

void level_alter_pseudorebuild(int32_t wipe_entities);
void wipe_draw_lists(ENTRY *elist, int32_t entry_count);
void wipe_entities(ENTRY *elist, int32_t entry_count);
void convert_old_dl_override(ENTRY *elist, int32_t entry_count);
void flip_level_y(ENTRY *elist, int32_t entry_count, int32_t *chunk_count);
void flip_level_x(ENTRY *elist, int32_t entry_count, int32_t *chunk_count);
