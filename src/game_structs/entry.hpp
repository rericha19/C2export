#pragma once

#include "../utils/utils.hpp"
#include <string>
#include <cstdint>

class ENTRY
{
public:
	uint32_t eid;
	char ename[6] = "";
	int32_t esize;
	int32_t chunk;
	bool norm_chunk_ent_is_loaded = false;
	std::vector<uint8_t> data{};	
	LIST related{};
	uint32_t* distances = NULL;
	bool* visited = NULL;

	inline uint8_t* _data()
	{
		return data.data();
	}
};

class ELIST : public std::vector<ENTRY>
{
public:
	void sort_by_eid();
	void update_gool_references();	
	int32_t get_index(uint32_t eid);
	int32_t count();
};

class ENTRY_2
{
public:
	static LIST get_neighbours(uint8_t* entry);
	static LIST get_models(uint8_t* animation);
	static LIST get_links(uint8_t* entry, int32_t item_idx);
	static LIST get_special_entries(uint8_t* entry);

	static std::string eid_to_str(uint32_t eid);
};