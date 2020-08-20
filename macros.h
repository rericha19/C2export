#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dir.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include "windows.h"

#define SLST_DISTANCE                   7000
#define NEIG_DISTANCE                   7000
#define DRAW_DISTANCE                   6000
#define BACKWARDS_PENALTY               0

#define FPATH_COUNT                     3
#define OFFSET                          0xC
#define CHUNKSIZE                       65536
#define BYTE                            0x100
#define MAX                             500
#define PI                              3.1415926535

// more dumb things
#define C2_NEIGHBOURS_START             0x190
#define C2_NEIGHBOURS_END               0x1B4
#define C2_NEIGHBOURS_FLAGS_END         0x1D4
#define C2_SPECIAL_METADATA_OFFSET      0x1DC
#define C2_NSD_CHUNK_COUNT              0x400
#define C2_NSD_ENTRY_COUNT              0x404
#define C2_NSD_ENTRY_TABLE              0x520
#define MAGIC_ENTRY                     0x100FFFF
#define MAGIC_CHUNK                     0x1234
#define CHUNK_TYPE_NORMAL               0
#define CHUNK_TYPE_TEXTURE              1
#define CHUNK_TYPE_PROTO_SOUND          2
#define CHUNK_TYPE_SOUND                3
#define CHUNK_TYPE_INSTRUMENT           4
#define C2_MUSIC_REF                    0x2A4
#define NONE                            0x6396347Fu

// commands
#define KILL                            2089250961u
#define HELP                            2089138798u
#define EXPORT                          2939723207u
#define EXPORTALL                       1522383616u
#define CHANGEPRINT                     2239644728u
#define CHANGEMODE                      588358864u
#define WIPE                            2089682330u
#define IMPORT                          3083219648u
#define INTRO                           223621809u
#define RESIZE                          3426052343u
#define ROTATE                          3437938580u
#define BUILD                           215559733u
#define PROP                            2089440550u
#define TEXTURE                         3979833526u
#define HASH                            2089134665u
#define A                               177638u
#define SCEN_RECOLOR                    2919463267u
#define NSD                             193464746u

#define STATUS                          3482341513u

#define ENTRY_TYPE_ANIM                 0x1
#define ENTRY_TYPE_MODEL                0x2
#define ENTRY_TYPE_SCENERY              0x3
#define ENTRY_TYPE_SLST                 0x4
#define ENTRY_TYPE_TEXTURE              0x5
#define ENTRY_TYPE_ZONE                 0x7
#define ENTRY_TYPE_GOOL                 0xB
#define ENTRY_TYPE_SOUND                0xC
#define ENTRY_TYPE_MIDI                 0xD
#define ENTRY_TYPE_INST                 0xE
#define ENTRY_TYPE_VCOL                 0xF
#define ENTRY_TYPE_DEMO                 0x13

#define ENTITY_PROP_NAME                0x2C
#define ENTITY_PROP_DEPTH_MODIFIER      0x32
#define ENTITY_PROP_PATH                0x4B
#define ENTITY_PROP_ID                  0x9F
#define ENTITY_PROP_ARGUMENTS           0xA4
#define ENTITY_PROP_TYPE                0xA9
#define ENTITY_PROP_SUBTYPE             0xAA
#define ENTITY_PROP_VICTIMS             0x287
#define ENTITY_PROP_BOX_COUNT           0x28B

#define ENTITY_PROP_CAMERA_MODE         0x29
#define ENTITY_PROP_CAM_AVG_PT_DIST     0xC9
#define ENTITY_PROP_CAM_SLST            0x103
#define ENTITY_PROP_CAM_PATH_LINKS      0x109
#define ENTITY_PROP_CAM_FOV             0x130
#define ENTITY_PROP_CAM_DRAW_LIST_A     0x13B
#define ENTITY_PROP_CAM_DRAW_LIST_B     0x13C
#define ENTITY_PROP_CAM_INDEX           0x173
#define ENTITY_PROP_CAM_SUBINDEX        0x174
#define ENTITY_PROP_CAM_LINK_COUNT      0x176
#define ENTITY_PROP_CAM_LOAD_LIST_A     0x208
#define ENTITY_PROP_CAM_LOAD_LIST_B     0x209

#define ENTITY_PROP_CAM_DISTANCE        0x142
#define ENTITY_PROP_CAM_WARP_SWITCH     0x1A8
#define ENTITY_PROP_CAM_BG_COLORS       0x1FA
#define ENTITY_PROP_CAM_UPDATE_SCENERY  0x27F

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

typedef struct info{
    int counter[22];                // counts entry types, counter[0] is total entry count
    int print_en;                   // variable for storing printing status 0 - nowhere, 1 - screen, 2 - file, 3 - both
    FILE *flog;                     // file where the logs are exported if file print is enabled
    char temp[MAX];                 // strings are written into this before being printed by condprint
    int gamemode;                   // 2 for C2, 3 for C3
    int portmode;                   // 0 for normal export, 1 for porting
    unsigned int anim[1024][3];     // field one is model, field 3 is animation, field 2 is original animation when c3->c2
    unsigned int animrefcount;      // count of animation references when porting c3 to c2
} INFO;

typedef struct spawn{
    int x;
    int y;
    int z;
    unsigned int zone;
} SPAWN;

typedef struct spawns{
    int spawn_count;
    SPAWN *spawns;
} SPAWNS;

typedef struct list {
    int count;
    unsigned int *eids;
} LIST;

typedef struct entry{
    unsigned int EID;
    int esize;
    int chunk;
    unsigned char *data;
    unsigned int *related = NULL;
} ENTRY;

typedef struct item {
    int eid;
    int index;
} ITEM;

typedef struct payload {
    int *chunks;
    int count;
    unsigned int zone;
} PAYLOAD;

typedef struct payloads {
    int count;
    PAYLOAD *arr;
} PAYLOADS;

typedef struct load {
    char type;
    int list_length;
    unsigned int *list;
    int index;
} LOAD;

typedef struct load_list {
    int count;
    LOAD array[1000];
} LOAD_LIST;

typedef struct relation {
    int value;
    int index1;
    int index2;
} RELATION;

typedef struct relations {
    int count;
    RELATION *relations;
} RELATIONS;

typedef struct inf {
    int type;
    int subtype;
    LIST dependencies;
} INF;

typedef struct {
    int count;
    INF *array;
} DEPENDENCIES;

typedef struct link {
    char type;
    char zone_index;
    char cam_index;
    char flag;
}   LINK;

typedef struct property {
    unsigned char header[8];
    unsigned char *data;
    int length;
} PROPERTY;


// misc.c
void         printstatus(int zonetype, int gamemode, int portmode);
void         intro_text();
void         print_help();
void         countprint(INFO status);
void         condprint(INFO status);
void         clrscr();
unsigned int from_u32(unsigned char *data);
unsigned int from_u16(unsigned char *data);
void         countwipe(INFO *status);
void         askprint(INFO *status);
unsigned long hash(const char *str);
int          hash_main();
void         swap_ints(int *a, int *b);
char*        eid_conv(unsigned int m_value, char *eid);
unsigned int eid_to_int(char *eid);
unsigned int nsfChecksum(const unsigned char *data);
void         make_path(char *finalpath, char *type, int eid, char *lvlid, char *date, INFO status);
void         askmode(int *zonetype, INFO *status);
long long int fact(int n);
int          cmpfunc(const void *a, const void *b);
int          comp(const void *a, const void *b);
int          pay_cmp(const void *a, const void *b);
int          list_comp(const void *a, const void *b);
int          load_list_sort(const void *a, const void *b);
void         combinationUtil(int arr[], int data[], int start, int end, int index, int r, int ***res, int *counter);
void         getCombinations(int arr[], int n, int r, int ***res, int *counter);
int          to_enum(const void *a);
int          cmp_entry(const void *a, const void *b);
int          cmp_entry_eid(const void *a, const void *b);
int          cmp_entry_size(const void *a, const void *b);
int          snd_cmp(const void *a, const void *b);
int          relations_cmp(const void *a, const void *b);
LIST         init_list();
SPAWNS       init_spawns();
int          list_find(LIST list, unsigned int searched);
void         list_add(LIST *list, unsigned int eid);
void         list_rem(LIST *list, unsigned int eid);
void         list_insert(LIST *list, unsigned int eid);
void         list_copy_in(LIST *destination, LIST source);
LOAD_LIST    init_load_list();
int          point_distance_3D(short int x1, short int x2, short int y1, short int y2, short int z1, short int z2);
LINK         int_to_link(unsigned int link);
void         delete_load_list(LOAD_LIST load_list);

// export.c
int          export_main(int zone, char *fpath, char *date);
int          export_chunk_handler(unsigned char *buffer,int chunkid, char *lvlid, char *date, int zonetype);
int          export_normal_chunk(unsigned char *buffer, char *lvlid, char *date, int zonetype);
int          export_texture_chunk(unsigned char *buffer, char *lvlid, char *date);
int          export_camera_fix(unsigned char *cam, int length);
void         export_entity_coord_fix(unsigned char *item, int itemlength);
void         export_scenery(unsigned char *buffer, int entrysize,char *lvlid, char *date);
void         export_generic_entry(unsigned char *buffer, int entrysize,char *lvlid, char *date);
void         export_gool(unsigned char *buffer, int entrysize,char *lvlid, char *date);
void         export_zone(unsigned char *buffer, int entrysize,char *lvlid, char *date, int zonetype);
void         export_model(unsigned char *buffer, int entrysize,char *lvlid, char *date);
void         export_animation(unsigned char *buffer, int entrysize, char *lvlid, char *date);

// import.c
int          import_main(char *time, INFO status);
int          import_file_lister(char *path, FILE *fnew);
void         import_chunksave(unsigned char *chunk, int *index, int *curr_off, int *curr_chunk, FILE *fnew, int offsets[]);


// build.c
int          build_align_sound(int input);
unsigned int build_get_model(unsigned char *anim);
int          build_remove_empty_chunks(int index_start, int index_end, int entry_count, ENTRY *entry_list);
void         build_remove_invalid_references(ENTRY *elist, int entry_count, int entry_count_base);
int          build_get_base_chunk_border(unsigned int textr, unsigned char **chunks, int index_end);
void         build_get_model_references(ENTRY *elist, int entry_count);
int          build_get_index(unsigned int eid, ENTRY *elist, int entry_count);
unsigned int build_get_slst(unsigned char *item);
unsigned int build_get_path_length(unsigned char *item);
short int *  build_get_path(ENTRY *elist, int zone_index, int item_index, int *path_len);
int *        build_seek_spawn(unsigned char *item);
int          build_get_neighbour_count(unsigned char *entry);
LIST         build_get_neighbours(unsigned char *entry);
int          build_get_cam_count(unsigned char *entry);
int          build_get_scen_count(unsigned char *entry);
int          build_get_entity_count(unsigned char *entry);
unsigned int*build_get_zone_relatives(unsigned char *entry, SPAWNS *spawns);
int          build_entry_type(ENTRY entry);
unsigned int*build_get_gool_relatives(unsigned char *entry, int entrysize);
void         build_read_nsf(ENTRY *elist, int chunk_border_base, unsigned char **chunks, int *chunk_border_texture, int *entry_count, FILE *nsf, unsigned int *gool_table);
void         build_dumb_merge(ENTRY *elist, int chunk_index_start, int *chunk_index_end, int entry_count);
void         build_read_folder(DIR *df, char *dirpath, unsigned char **chunks, ENTRY *elist, int *chunk_border_texture, int *entry_count, SPAWNS *spawns, unsigned int* gool_table);
void         build_print_relatives(ENTRY *elist, int entry_count);
void         build_swap_spawns(SPAWNS spawns, int spawnA, int spawnB);
void         build_write_nsd(char *path, ENTRY *elist, int entry_count, int chunk_count, SPAWNS spawns, unsigned int* gool_table, int level_ID);
void         build_increment_common(LIST list, LIST entries, int **entry_matrix, int rating);
void         build_matrix_merge_util(RELATIONS relations, ENTRY *elist, int entry_count, LIST entries);
LOAD_LIST    build_get_lists(int prop_code, unsigned char *entry, int cam_offset);
RELATIONS    build_transform_matrix(LIST entries, int **entry_matrix);
void         build_matrix_merge_main(ENTRY *elist, int entry_count, int chunk_border_sounds, int* chunk_count, int merge_flag);
void         build_normal_chunks(ENTRY *elist, int entry_count, int chunk_border_sounds, int chunk_count, unsigned char **chunks);
int          build_get_entity_prop(unsigned char *entity, int prop_code);
void         build_add_scen_textures_to_list(unsigned char *scenery, LIST *list);
void         build_add_model_textures_to_list(unsigned char *model, LIST *list);
unsigned char* build_add_property(unsigned int code, unsigned char *item, int* item_size, PROPERTY *prop);
unsigned char* build_rem_property(unsigned int code, unsigned char *item, int* item_size, PROPERTY *prop);
void         build_entity_alter(ENTRY *zone, int item_index, unsigned char *(func_arg)(unsigned int, unsigned char *, int *, PROPERTY *), int property_code, PROPERTY *prop);
LIST         build_get_links(unsigned char *entry, int cam_index);
void         build_load_list_util_util_forw(int cam_length, LIST *listA, LIST *listB, int distance, int final_distance, short int* coords, int path_length, LIST additions);
void         build_load_list_util_util_back(int cam_length, LIST *full_list, int distance, int final_distance, short int* coords, int path_length, LIST additions);
void         build_load_list_util_util(int zone_index, int cam_index, int link_int, LIST *full_list, int cam_length, ENTRY * elist, int entry_count, int* config, DEPENDENCIES collisisons);
LIST *       build_get_complete_draw_list(ENTRY *elist, int zone_index, int cam_index, int cam_length);
LIST         build_get_types_subtypes(ENTRY *elist, int entry_count, LIST entity_list, LIST neighbour_list);
int          build_get_distance(short int *coords, int start_index, int end_index, int cap, int *final_index);
LIST         build_get_entity_list(int point_index, int zone_index, int camera_index, int cam_length, ENTRY *elist, int entry_count, LIST *neighbours);
void         build_load_list_util(int zone_index, int camera_index, LIST* full_list, int cam_length, ENTRY *elist, int entry_count, DEPENDENCIES sub_info, DEPENDENCIES collisions, int *config);
PROPERTY     build_make_load_list_prop(LIST *list_array, int cam_length, int code);
void         build_find_unspecified_entities(ENTRY *elist, int entry_count, DEPENDENCIES sub_info);
void         build_load_list_to_delta(LIST *full_load, LIST *listA, LIST *listB, int cam_length, ENTRY *elist, int entry_count);
LIST         build_read_special_entries(unsigned char *zone);
void         build_add_special_entries(LIST *full_load, int cam_length, ENTRY zone, ENTRY *elist, int entry_count);
void         build_make_load_lists(ENTRY *elist, int entry_count, unsigned int *gool_table, LIST permaloaded, DEPENDENCIES subtype_info, DEPENDENCIES collision, int *config);
int          build_read_entry_config(LIST *permaloaded, DEPENDENCIES *subtype_info, DEPENDENCIES *collisions, char fpaths[FPATH_COUNT][MAX], ENTRY *elist, int entry_count, unsigned int *gool_table);
int          build_get_chunk_count_base(FILE *nsf);
int          build_ask_ID();
void         build_ask_list_paths(char fpaths[FPATH_COUNT][MAX]);
void         build_instrument_chunks(ENTRY *elist, int entry_count, int *chunk_count, unsigned char** chunks);
void         build_sound_chunks(ENTRY *elist, int entry_count, int *chunk_count, unsigned char** chunks);
void         build_assign_primary_chunks_all(ENTRY *elist, int entry_count, int *chunk_count, int *config);
int          build_is_normal_chunk_entry(ENTRY entry);
void         build_merge_main(ENTRY *elist, int entry_count, int chunk_border_sounds, int *chunk_count, int config2);
void         build_final_cleanup(FILE *nsf, FILE *nsfnew, DIR *df, ENTRY *elist, int entry_count, unsigned char **chunks, int chunk_count);
void         build_main(char *nsfpath, char *dirpath, int chunkcap, INFO status, char *time);

// deprecate_build.c
PAYLOADS     deprecate_build_get_payload_ladder(ENTRY *elist, int entry_count, int chunk_min);
void         deprecate_build_payload_merge(ENTRY *elist, int entry_count, int chunk_min, int *chunk_count);
void         deprecate_build_insert_payload(PAYLOADS *payloads, PAYLOAD insertee);
void         deprecate_build_print_payload(PAYLOAD payload, int stopper);
int          deprecate_build_merge_thing(ENTRY *elist, int entry_count, int *chunks, int chunk_count);
int          deprecate_build_get_common(int* listA, int countA, int *listB, int countB);
void         deprecate_build_chunk_merge(ENTRY *elist, int entry_count, int *chunks, int chunk_count);
PAYLOAD      deprecate_build_get_payload(ENTRY *elist, int entry_count, LIST list, unsigned int zone, int chunk_min);
void         deprecate_build_gool_merge(ENTRY *elist, int chunk_index_start, int *chunk_index_end, int entry_count);
int          deprecate_build_is_relative(unsigned int searched, unsigned int *array, int count);
void         deprecate_build_ll_add_children(unsigned int eid, ENTRY *elist, int entry_count, LIST *list, unsigned int *gool_table, DEPENDENCIES dependencies);
void         deprecate_build_assign_primary_chunks_gool(ENTRY *elist, int entry_count, int *real_chunk_count, int grouping_flag);
void         deprecate_build_assign_primary_chunks_rest(ENTRY *elist, int entry_count, int *chunk_count);
void         deprecate_build_assign_primary_chunks_zones(ENTRY *elist, int entry_count, int *real_chunk_count, int grouping_flag);

// side_scripts.cpp
int          scenery_recolor_main();
int          texture_copy_main();
void         prop_main(char* path);
void         resize_main(char *time, INFO status);
void         resize_level(FILE *level, char *filepath, double scale[3], char *time, INFO status);
void         resize_chunk_handler(unsigned char *chunk, INFO status, double scale[3]);
void         resize_folder(DIR *df, char *path, double scale[3], char *time, INFO status);
void         resize_zone(int fsize, unsigned char *buffer, double scale[3], INFO status);
void         resize_entity(unsigned char *item, int itemsize, double scale[3], INFO status);
void         resize_scenery(int fsize, unsigned char *buffer, double scale[3], INFO status);
void         rotate_main(char *time);
void         rotate_scenery(unsigned char *buffer, char *filepath, double rotation, char *time, int filesize);
void         rotate_zone(unsigned char *buffer, char *filepath, double rotation);
void         rotate_rotate(unsigned int *y,unsigned int *x, double rotation);
void         crate_rotation_angle();
void         nsd_gool_table_print(char *fpath);
