#pragma once

// used to store payload information
class PAYLOAD
{
public:
	std::vector<int32_t> chunks{};
	int16_t entcount;
	int16_t cam_path;
	std::vector<int32_t> tchunks;
	uint32_t zone;

	int32_t page_count() const;
	int32_t tpage_count() const;
	void print_info() const;

	static PAYLOAD get_payload_single(ELIST& elist, LIST& loaded, uint32_t zone, int32_t chunk_min, bool get_tpages);
};


class PAYLOADS : public std::vector<PAYLOAD>
{
public:
	void insert(const PAYLOAD& insertee);
	int64_t calculate_score() const;

	static PAYLOADS get_payload_ladder(MATRIX_STORED_LLS& stored_lls, ELIST& elist, int32_t chunk_min, int32_t get_tpages);
	static PAYLOADS get_payload_ladder_ll(ELIST& elist, int32_t chunk_min);
};

// stores worst zone info during merge
struct WORST_ZONE_INFO_SINGLE
{
	uint32_t zone;
	int32_t count;
	int32_t cam_path;
	int32_t sum;
};

class WORST_ZONE_INFO : public std::vector<WORST_ZONE_INFO_SINGLE>
{
public:
	void update(const PAYLOAD& worst);
	void print_summary();
};
