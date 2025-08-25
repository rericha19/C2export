#pragma once

#include <vector>

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
	inline DEPENDENCY(int32_t t = 0, int32_t s = 0)
	{
		type = t;
		subtype = s;
	};


	int32_t type = 0;
	int32_t subtype = 0;
	LIST dependencies{};
};
using DEPENDENCIES = std::vector<DEPENDENCY>;


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
using MATRIX_STORED_LLS = std::vector<MATRIX_STORED_LL>;

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
};

using GENERIC_LOAD_LIST = std::vector<LOAD>;
using LOAD_LIST = GENERIC_LOAD_LIST;
using DRAW_LIST = GENERIC_LOAD_LIST;


// Deconstructs the load or draw lists and saves into a convenient struct.
void get_generic_lists(GENERIC_LOAD_LIST& load_list, int32_t prop_code, uint8_t* entry, int32_t cam_index);
LOAD_LIST get_load_lists(uint8_t* entry, int32_t cam_index);
DRAW_LIST get_draw_lists(uint8_t* entry, int32_t cam_index);
