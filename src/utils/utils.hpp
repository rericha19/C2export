#pragma once

#include "../include.h"

class ENTRY;

class File
{
public:
	FILE* f = NULL;
	std::string rem_path;

	bool open(const char* path, const char* mode)
	{
		f = std::fopen(path, mode);
		rem_path = path;
		return ok();
	}

	inline bool ok() const
	{
		return f != NULL;
	}

	~File()
	{
		if (f)
			std::fclose(f);
	}
};

class LIST : public std::vector<uint32_t>
{
public:
	int32_t count() const;
	int32_t find(uint32_t eid) const;
	void add(uint32_t eid);
	void remove(uint32_t eid);
	void copy_in(const LIST& other);
	void remove_all(const LIST& other);

	static bool is_subset(const LIST& subset, const LIST& superset);
};

class DEPENDENCY
{
public:
	inline DEPENDENCY(int32_t t, int32_t s = -1)
	{
		type = t;
		subtype = s;
	};


	int32_t type = 0;
	int32_t subtype = 0;
	LIST dependencies{};
};

class MATRIX_STORED_LL
{
public:
	inline MATRIX_STORED_LL(uint32_t z, int32_t path)
	{
		zone = z;
		cam_path = path;
	};


	uint32_t zone = 0;
	int32_t cam_path = 0;
	LIST full_load{};
};

// used to store load or draw list in its non-delta form
class LOAD
{
public:
	int8_t type;	 // 'A' or 'B'
	int32_t index;   // point index
	LIST list{};
};

// in build scripts, used to store spawns
class SPAWN
{
public:
	int32_t x;
	int32_t y;
	int32_t z;
	uint32_t zone;
};

class SPAWNS : public std::vector<SPAWN>
{
public:
	void swap_spawns(int32_t spawnA, int32_t spawnB);
	void sort_spawns();
	bool ask_spawn();
};

// entity/item property
struct PROPERTY
{
	uint8_t header[8]{};
	std::vector<uint8_t> data{};
	int32_t length = 0;

	static int32_t count(uint8_t* item);
	static PROPERTY get_full(uint8_t* item, uint32_t prop_code);
	static int32_t get_value(uint8_t* entity, uint32_t prop_code);
	static int32_t get_offset(uint8_t* item, uint32_t prop_code);
	static PROPERTY make_list_prop(std::vector<LIST>& list_array, int32_t code);
};

// used in matrix merge to store what entries are loaded simultaneously and how much/often
struct RELATION
{
	int32_t save_value;
	int32_t value;
	int16_t index1;
	int16_t index2;
};

// used to keep all relations
class RELATIONS
{
public:
	RELATIONS(int32_t c)
	{
		count = c;
		relations = std::make_unique<RELATION[]>(c);
	}

	void do_sort()
	{
		auto rel_sort = [](RELATION& x, RELATION& y)
			{
				return y.value < x.value;
			};

		std::sort(relations.get(), relations.get() + count, rel_sort);
	}

	int32_t count = 0;
	std::unique_ptr<RELATION[]> relations = nullptr;
};

struct MTRX_THRD_IN_STR
{
	WORST_ZONE_INFO* worst_zones_info;
	MATRIX_STORED_LLS* stored_lls;
	RELATIONS* rel_array;
	ELIST* best_elist;
	uint32_t* best_zone_ptr;
	int64_t* best_max_ptr;
	int32_t* curr_iter_ptr;
	std::mutex* mutex_best;
	std::mutex* mutex_iter;
	int16_t thread_idx;
	int32_t rnd_seed;
};

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
void rand_seed(int32_t seed);
double randfrom(double min, double max);
int32_t normalize_angle(int32_t angle);
int32_t c2yaw_to_deg(int32_t yaw);
int32_t deg_to_c2yaw(int32_t deg);
int32_t angle_distance(int32_t angle1, int32_t angle2);
int32_t average_angles(int32_t angle1, int32_t angle2);
int32_t chunk_count_base(FILE* nsf);
int32_t ask_level_ID();
ChunkType chunk_type(uint8_t* chunk);
