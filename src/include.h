#pragma once 

#define eid2str(x) (ENTRY::eid2s(x).c_str())

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
#include <unordered_set>
#include <random>
#include <variant>
#include <map>
#include <functional>
#include "utils/json.hpp"

struct PROPERTY;
struct RELATION;

class LOAD;
class ENTRY;
class LIST;
class LABELED_LIST;
class ELIST;
class DEPENDENCY;
class DEPENDENCI;
class SPAWNS;
class MATRIX_STORED_LL;
class RELATIONS;
class ENTITY_PATH;
class WORST_ZONE_INFO;

using DEPENDENCIES = std::vector<DEPENDENCY>;
using MATRIX_STORED_LLS = std::vector<MATRIX_STORED_LL>;
using GENERIC_LOAD_LIST = std::vector<LOAD>;
using LOAD_LIST = GENERIC_LOAD_LIST;
using DRAW_LIST = GENERIC_LOAD_LIST;
using FULL_LABELED_LL = std::vector<LABELED_LIST>;
using CHUNKS = std::vector<std::unique_ptr<uint8_t[]>>;

using RebuildConfig = std::map<uint32_t, int32_t>;

enum __conf_enum
{
	LL_Matrix_Polling_Type_CONST,

	Remake_Draw_Lists,
	Remake_Load_Lists,
	Chunk_Merge_Method,

	LL_SLST_Distance,
	LL_Neighbour_Distance,
	LL_Drawlist_Distance,
	LL_Transition_Preloading_Type,
	LL_Backwards_Loading_Penalty_DBL,

	DL_Distance_Cap_X,
	DL_Distance_Cap_Y,
	DL_Distance_Cap_XZ,
	DL_Distance_Cap_Angle3D,

	Rebuild_Payload_Limit,
	Rebuild_Iteration_Limit,
	Rebuild_Random_Mult_DBL,
	Rebuild_Base_Seed,
	Rebuild_Thread_Count,
};

#define CONFIG_FLOAT_MULT 1000000

#define PRELOADING_NOTHING 0
#define PRELOADING_TEXTURES_ONLY 1
#define PRELOADING_REG_ENTRIES_ONLY 2
#define PRELOADING_ALL 3

#define BUILD_FPATH_COUNT 4
#define OFFSET 0xC // bad name
#define C2_GOOL_TABLE_SIZE 0x40
#define C3_GOOL_TABLE_SIZE 0x80
#define CHUNKSIZE 65536
#define CHUNK_CHECKSUM_OFFSET 0xC
#define BYTE 0x100
#define MAX 1000
#define PI 3.1415926535
#define CHUNK_LIST_DEFAULT_SIZE 1000

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
	AT_Medieval_Rain_Fix,
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

template <typename T1, typename T2>
auto min(T1 a, T2 b)
{
	return (b < a) ? b : a;
}
template <typename T1, typename T2>
auto max(T1 a, T2 b)
{
	return (b > a) ? b : a;
}

inline int32_t double_to_config_int(double val)
{
	return int32_t(CONFIG_FLOAT_MULT * val);
};
inline double config_to_double(int32_t v)
{
	return ((double)v / CONFIG_FLOAT_MULT);
};

inline std::string int_to_hex4(int32_t value)
{
	std::stringstream ss;
	ss << std::setw(4) << std::setfill('0') << std::hex << std::uppercase << (value & 0xFFFF);
	return ss.str();
}
