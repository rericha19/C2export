#pragma once

class ELIST;

namespace level_analyze
{
	void ll_check_draw_list_integrity(ELIST& elist);
	void ll_check_load_list_integrity(ELIST& elist);
	void ll_check_zone_refs(ELIST& elist);
	void ll_check_sound_refs(ELIST& elist);
	void ll_check_tpage_refs(ELIST& elist);
	void ll_check_gool_types(ELIST& elist);
	void ll_check_gool_refs(ELIST& elist, uint32_t* gool_table);
	void ll_id_usage(ELIST& elist);
	void ll_various_stats(ELIST& elist);
	void ll_get_box_count(ELIST& elist);
	void ll_print_avg_chunkfill(ELIST& elist);
	void ll_print_full_payload_info(ELIST& elist, int32_t print_full);
	void ll_analyze_main();
}