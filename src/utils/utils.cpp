
#include "../include.h"
#include "utils.hpp"
#include "entry.hpp"

int32_t LIST::count() const
{
	return static_cast<int32_t>(size());
}

void LIST::add(uint32_t eid)
{
	if (!empty() && std::find(begin(), end(), eid) != end())
		return;

	push_back(eid);
}

void LIST::remove(uint32_t eid)
{
	if (empty())
		return;

	auto it = std::find(begin(), end(), eid);
	if (it != end())
		erase(it);
}

int32_t LIST::find(uint32_t eid) const
{
	if (empty())
		return -1;

	auto it = std::find(begin(), end(), eid);
	if (it == end())
		return -1;

	return static_cast<int32_t>(it - begin());
}

bool LIST::is_subset(const LIST& subset, const LIST& superset)
{
	for (const auto& item : subset)
	{
		if (superset.find(item) == -1)
			return false;
	}
	return true;
}

void LIST::copy_in(const LIST& other)
{
	for (auto& eid : other)
	{
		if (std::find(begin(), end(), eid) == end())
			push_back(eid);
	}
}

void LIST::remove_all(const LIST& other)
{
	for (auto& eid : other)
	{
		auto it = std::find(begin(), end(), eid);
		if (it != end())
			erase(it);
	}
}

int32_t LABELED_LIST::count() const
{
	return static_cast<int32_t>(size());
}

int32_t LABELED_LIST::find(uint32_t eid) const
{
	for (int32_t i = 0; i < count(); i++)
	{
		if (at(i).first == eid)
			return i;
	}

	return -1;
}

void LABELED_LIST::add(uint32_t eid, const std::string& reason)
{
	for (auto& pair : *this)
	{
		if (pair.first == eid)
		{
			pair.second += ", " + reason;
			return;
		}
	}

	push_back(std::make_pair(eid, reason));
}

void LABELED_LIST::copy_in(const LIST& other, const std::string& reason)
{
	for (auto& eid : other)
		add(eid, reason);
}


void SPAWNS::swap_spawns(int32_t spawnA, int32_t spawnB)
{
	std::swap(at(spawnA), at(spawnB));
}

void SPAWNS::sort_spawns()
{
	auto spsort = [](SPAWN a, SPAWN b)
		{
			return a.zone < b.zone;
		};

	std::sort(begin(), end(), spsort);
}

// Lets the user pick the spawn that's to be used as the primary spawn.
bool SPAWNS::ask_spawn()
{
	if (!size())
		return false;

	sort_spawns();
	int32_t input = 0;

	// lets u pick a spawn point
	printf("Pick a spawn:\n");
	for (uint32_t i = 0; i < size(); i++)
		printf("Spawn %2d:\tZone: %s\n", i + 1, eid2str(at(i).zone));

	scanf("%d", &input);
	if (input > size() || input < 1)
	{
		printf("No such spawn, defaulting to first one\n");
		input = 1;
	}

	// insert the picked spawn 2x at the start
	SPAWN picked_spawn = at(input - 1);
	insert(begin(), picked_spawn);
	insert(begin(), picked_spawn);
	printf("Using spawn %2d: Zone: %s\n", input, eid2str(at(0).zone));
	return true;
}

int32_t PROPERTY::count(uint8_t* item)
{
	return from_u32(item + 0xC);
}

PROPERTY PROPERTY::get_full(uint8_t* item, uint32_t prop_code)
{
	PROPERTY prop{};
	int32_t property_count = PROPERTY::count(item);
	uint8_t property_header[8];

	for (int32_t i = 0; i < property_count; i++)
	{
		memcpy(property_header, item + 0x10 + 8 * i, 8);
		if (from_u16(property_header) == prop_code)
		{
			memcpy(prop.header, property_header, 8);
			int32_t next_offset = 0;
			if (i == property_count - 1)
				next_offset = *(uint16_t*)(item);
			else
				next_offset = *(uint16_t*)(item + 0x12 + (i * 8) + 8) + OFFSET;
			int32_t curr_offset = *(uint16_t*)(item + 0x12 + 8 * i) + OFFSET;
			prop.length = next_offset - curr_offset;
			prop.data.resize(prop.length);
			memcpy(prop.data.data(), item + curr_offset, prop.length);
			break;
		}
	}

	return prop;
}

int32_t PROPERTY::get_value(uint8_t* entity, uint32_t prop_code)
{
	int32_t offset = get_offset(entity, prop_code);
	if (offset == 0)
		return -1;

	return from_u32(entity + offset + 4);
}

int32_t PROPERTY::get_offset(uint8_t* item, uint32_t prop_code)
{
	int32_t offset = 0;
	int32_t prop_count = PROPERTY::count(item);
	for (int32_t i = 0; i < prop_count; i++)
		if ((from_u16(item + 0x10 + 8 * i)) == prop_code)
			offset = OFFSET + from_u16(item + 0x10 + 8 * i + 2);

	return offset;
}

// Makes a load/draw list property from the input arrays
PROPERTY PROPERTY::make_list_prop(std::vector<LIST>& list_array, int32_t code)
{
	int32_t cam_length = int32_t(list_array.size());
	int32_t delta_counter = 0;
	int32_t total_length = 0;
	PROPERTY prop;

	// count total length and individual deltas
	for (int32_t i = 0; i < cam_length; i++)
	{
		if (list_array[i].count() == 0)
			continue;

		total_length += list_array[i].count() * 4; // space individual load list items of
		// current sublist will take up
		total_length += 4; // each load list sublist uses 2 bytes for its length
		// and 2 bytes for its index
		delta_counter++;
	}

	// header info
	*(int16_t*)(prop.header) = code;
	if (code == ENTITY_PROP_CAM_LOAD_LIST_A || code == ENTITY_PROP_CAM_LOAD_LIST_B)
	{
		*(int16_t*)(prop.header + 4) = (delta_counter > 1) ? 0x0464 : 0x424;
	}

	if (code == ENTITY_PROP_CAM_DRAW_LIST_A || code == ENTITY_PROP_CAM_DRAW_LIST_B)
	{
		*(int16_t*)(prop.header + 4) = (delta_counter > 1) ? 0x0473 : 0x0433;
	}
	*(int16_t*)(prop.header + 6) = delta_counter;

	prop.length = total_length;
	prop.data.resize(total_length);

	int32_t indexer = 0;
	int32_t offset = 4 * delta_counter;
	for (int32_t i = 0; i < cam_length; i++)
	{
		if (list_array[i].count() == 0)
			continue;

		*(int16_t*)(prop.data.data() + 2 * indexer) = list_array[i].count(); // i-th sublist's length (item count)
		*(int16_t*)(prop.data.data() + 2 * (indexer + delta_counter)) = i;   // i-th sublist's index
		for (int32_t j = 0; j < list_array[i].count(); j++)
			*(int32_t*)(prop.data.data() + offset + 4 * j) = list_array[i][j]; // individual items
		offset += list_array[i].count() * 4;
		indexer++;
	}

	return prop;
}

// injects a property into an item
std::vector<uint8_t> PROPERTY::item_add_property(uint32_t code, std::vector<uint8_t>& item, PROPERTY* prop)
{
	int32_t offset;
	int32_t property_count = PROPERTY::count(item.data());

	std::vector<int32_t> property_sizes(property_count + 1);
	std::vector<uint8_t[8]> property_headers(property_count + 1);

	for (auto& prop_header : property_headers)
	{
		memset(prop_header, 0, 8);
	}

	for (int32_t i = 0; i < property_count; i++)
	{
		int32_t idx = (from_u16(item.data() + 0x10 + 8 * i) > code) ? i + 1 : i;
		memcpy(property_headers[idx], item.data() + 0x10 + 8 * i, 8);
	}

	int32_t insertion_index = 0;
	for (int32_t i = 0; i < property_count + 1; i++)
	{
		if (from_u32(property_headers[i]) == 0 && from_u32(property_headers[i] + 4) == 0)
			insertion_index = i;
	}

	if (insertion_index != property_count)
	{
		for (int32_t i = 0; i < property_count + 1; i++)
		{
			property_sizes[i] = 0;
			if (i <= insertion_index - 1)
				property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
			if (i == insertion_index - 1)
				property_sizes[i] = from_u16(property_headers[i + 2] + 2) - from_u16(property_headers[i] + 2);
			// if (i == insertion_index) {}
			if (i > insertion_index)
			{
				if (i == property_count)
					property_sizes[i] = from_u16(item.data()) - from_u16(property_headers[i] + 2);
				else
					property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
			}
		}
	}
	else
	{
		// last prop
		for (int32_t i = 0; i < property_count; i++)
		{
			if (i != property_count - 1)
				property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
			else
				property_sizes[i] = from_u16(item.data()) - OFFSET - from_u16(property_headers[i] + 2);
		}
	}

	std::vector<std::vector<uint8_t>> properties(property_count + 1);
	offset = 0x10 + 8 * property_count;
	for (int32_t i = 0; i < property_count + 1; i++)
	{
		if (i == insertion_index)
			continue;

		properties[i].resize(property_sizes[i]);
		memcpy(properties[i].data(), item.data() + offset, property_sizes[i]);
		offset += property_sizes[i];
	}

	property_sizes[insertion_index] = prop->length;
	memcpy(property_headers[insertion_index], prop->header, 8);
	properties[insertion_index].resize(prop->length);
	memcpy(properties[insertion_index].data(), prop->data.data(), prop->length);

	int32_t new_size = 0x10 + 8 * (property_count + 1);
	for (int32_t i = 0; i < property_count + 1; i++)
		new_size += property_sizes[i];

	std::vector<uint8_t> new_item(new_size);
	*(int32_t*)(new_item.data()) = new_size - OFFSET;
	*(int32_t*)(new_item.data() + 0x4) = 0;
	*(int32_t*)(new_item.data() + 0x8) = 0;
	*(int32_t*)(new_item.data() + 0xC) = property_count + 1;

	offset = 0x10 + 8 * (property_count + 1);
	for (int32_t i = 0; i < property_count + 1; i++)
	{
		*(int16_t*)(property_headers[i] + 2) = offset - OFFSET;
		memcpy(new_item.data() + 0x10 + 8 * i, property_headers[i], 8);
		memcpy(new_item.data() + offset, properties[i].data(), property_sizes[i]);
		offset += property_sizes[i];
	}

	new_item.resize(new_item.size() - OFFSET);
	return new_item;
}


// Removes the specified property.
std::vector<uint8_t> PROPERTY::item_rem_property(uint32_t code, std::vector<uint8_t>& item, PROPERTY* prop)
{
	int32_t property_count = PROPERTY::count(item.data());

	std::vector<int32_t> property_sizes(property_count);
	std::vector<uint8_t[8]> property_headers(property_count);

	bool found = false;
	for (int32_t i = 0; i < property_count; i++)
	{
		memcpy(property_headers[i], item.data() + 0x10 + 8 * i, 8);
		if (from_u16(property_headers[i]) == code)
			found = true;
	}

	if (!found)
		return item;

	for (int32_t i = 0; i < property_count - 1; i++)
		property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
	property_sizes[property_count - 1] = from_u16(item.data()) - OFFSET - from_u16(property_headers[property_count - 1] + 2);

	std::vector<std::vector<uint8_t>> properties(property_count);

	int32_t offset = 0x10 + 8 * property_count;
	for (int32_t i = 0; i < property_count; offset += property_sizes[i], i++)
	{
		properties[i].resize(property_sizes[i]);
		memcpy(properties[i].data(), item.data() + offset, property_sizes[i]);
	}

	int32_t new_size = 0x10 + 8 * (property_count - 1);
	for (int32_t i = 0; i < property_count; i++)
	{
		if (from_u16(property_headers[i]) == code)
			continue;
		new_size += property_sizes[i];
	}

	std::vector<uint8_t> new_item(new_size);
	*(int32_t*)(new_item.data()) = new_size;
	*(int32_t*)(new_item.data() + 4) = 0;
	*(int32_t*)(new_item.data() + 8) = 0;
	*(int32_t*)(new_item.data() + 0xC) = property_count - 1;

	offset = 0x10 + 8 * (property_count - 1);
	int32_t indexer = 0;
	for (int32_t i = 0; i < property_count; i++)
	{
		if (from_u16(property_headers[i]) != code)
		{
			*(int16_t*)(property_headers[i] + 2) = offset - OFFSET;
			memcpy(new_item.data() + 0x10 + 8 * indexer, property_headers[i], 8);
			memcpy(new_item.data() + offset, properties[i].data(), property_sizes[i]);
			indexer++;
			offset += property_sizes[i];
		}
	}

	return new_item;
}


void* try_malloc(uint32_t size)
{
	void* ret = malloc(size);
	if (ret == NULL)
	{
		printf("[ERROR] Failed to try_malloc memory (%d), exiting\n", size);
		getchar();
		exit(EXIT_FAILURE);
	}
	return ret;
}

void* try_calloc(uint32_t count, uint32_t size)
{
	void* ret = calloc(count, size);
	if (ret == NULL)
	{
		printf("[ERROR] Failed to calloc memory (%d %d), exiting\n", count, size);
		getchar();
		exit(EXIT_FAILURE);
	}
	return ret;
}

// text at the start when you open the program or wipe
void intro_text()
{
	for (int32_t i = 0; i < 100; i++)
		printf("*");
	printf("\nCrash 2 rebuilder, utility tool & c2/c3 level exporter made by Averso.\n");
	printf("If any issue pops up (instructions are unclear or it crashes), DM me @ Averso#5633 (Discord).\n");
	printf("Type \"HELP\" for list of commands and their format. Commands >ARE NOT< case sensitive.\n");
	printf("It is recommended to provide valid data as there may be edge cases that are unaccounted for.\n");
	printf("You can drag & drop the files and folders to this window instead of copying in the paths\n");
	printf("Most stuff works only for Crash 2 !!!!\n");
	for (int32_t i = 0; i < 100; i++)
		printf("*");
	printf("\n\n");
}

// help for miscellaneous, modpack-specific and obsolete commands
void print_help2()
{
	for (int32_t i = 0; i < 100; i++)
		printf("-");
	printf("\nMisc/obsolete/modpack command list:\n");

	printf("KILL\n");
	printf("\t ends the program\n");

	printf("PROP & NSF_PROP\n");
	printf("\t print a list of properties in an entity or all occurences of entity in a NSF\n");

	printf("PROP_REMOVE\n");
	printf("\t removes a property from an entity\n");

	printf("PROP_REPLACE\n");
	printf("\t replaces (or inserts) a property into dest entity with prop from source entity\n");

	printf("TEXTURE\n");
	printf("\t copies tiles from one texture chunk to another (doesnt include CLUTs)\n");

	printf("SCEN_RECOLOR\n");
	printf("\t recolor vertex colors to selected hue / brightness\n");

	printf("TEXTURE_RECOLOR\n");
	printf("\t recolor texture clut colors to selected hue / brightness\n");

	printf("WARP_SPAWNS\n");
	printf("\t modpack, used to generate warp room spawns from the nsf\n");

	printf("CHECK_UTIL\n");
	printf("\t modpack, lists regular+dda checks/masks, some levels have hardcoded exceptions (folder)\n");

	printf("LIST_SPECIAL_LL\n");
	printf("\t lists special zone load list entries (for rebuild) located within first item (folder)\n");

	printf("NSD_UTIL\n");
	printf("\t misc gool listing util (folder)\n");

	printf("FOV_UTIL\n");
	printf("\t misc fov listing util (folder)\n");

	printf("DRAW_UTIL\n");
	printf("\t detailed info about draw lists (NSF)\n");

	printf("TPAGE_UTIL\n");
	printf("\t list tpages within files (folder)\n");

	printf("GOOL_UTIL\n");
	printf("\t list gool entries within files (folder)\n");

	printf("A <angle> & TIME\n");
	printf("\t modpack crate rotation, TT value\n");

	printf("GEN_SLST\n");
	printf("\t creates an 'empty' SLST entry for path with chosen length\n");

	printf("ALL_PERMA\n");
	printf("\t list all non-instrument entries and tpages (NSF)\n");

	printf("SCEN_RECOLOR2\n");
	printf("\t modpack, hardcoded values, dont use\n");

	printf("RESIZE <game> <xmult> <ymult> <zmult>\n");
	printf("\t changes dimensions of the zones and scenery according to given parameters (can mess up warps)\n");
	printf("\t e.g. 'resize3' 1.25 1 1' - files are from C3 and it gets stretched only on X\n");

	printf("ROTATE\n");
	printf("\t rotates scenery or objects in a zone you specified\n");

	printf("ENT_RESIZE\n");
	printf("\t converts c3 entity to c2 entity (path adjustment)\n");

	printf("CONV_OLD_DL_OR\n");
	printf("\t converts old draw list override to new format (NSF)\n");

	printf("ENT_MOVE\n");
	printf("\t moves an entity by chosen amount\n");

	printf("FLIP_Y & FLIP_X\n");
	printf("\t flips the level horizontally or vertically\n");

	printf("LEVEL_RECOLOR\n");
	printf("\t recolors all level scenery to given rgb and brightness\n");

	for (int32_t i = 0; i < 100; i++)
		printf("-");
	printf("\n");
}

// main help
void print_help()
{
	for (int32_t i = 0; i < 100; i++)
		printf("-");
	printf("\nCommand list:\n");
	printf("HELP & HELP2\n");
	printf("\t print a list of commands\n");

	printf("WIPE\n");
	printf("\t wipes current screen\n");

	printf("REBUILD & REBUILD_DL\n");
	printf("\t builds a level from chosen inputs\n");

	printf("LL_ANALYZE\n");
	printf("\t stats about the level, integrity checks\n");

	printf("EXPORT\n");
	printf("\t exports level entries\n");

	printf("LEVEL_WIPE_DL & LEVEL_WIPE_ENT\n");
	printf("\t remove draw lists / remove draw lists + entities from level (scrambles the level a bit)\n");

	printf("NSD\n");
	printf("\t prints gool table from the nsd\n");

	printf("EID <EID>\n");
	printf("\t prints the EID in hex form\n");

	printf("GEN_SPAWN\n");
	printf("\t NSD spawn generation for input level, zone and entity ID\n");

	printf("MODEL_REFS & MODEL_REFS_NSF\n");
	printf("\t lists model entry texture references\n");

	printf("ENTITY_USAGE\n");
	printf("\t prints info about type/subtype usage count across nsfs in the folder\n");

	printf("PAYLOAD_INFO\n");
	printf("\t prints info about normal chunk / tpage / entity loading in nsf's cam paths\n");

	printf("\nError messages:\n");
	printf("[ERROR]   error message\n");
	printf("\tan error that could not be handled, the program skipped some action or gave up\n");

	printf("[error]   error message\n");
	printf("\tan error that was handled\n");

	printf("[warning] warning text\n");
	printf("\tjust a warning, something may or may not be wrong\n");
	for (int32_t i = 0; i < 100; i++)
		printf("-");
	printf("\n");
}

// wipes the current screen
void clrscr()
{
	system("@cls||clear");
}

// helper for reading signed 32b
int32_t from_s32(const uint8_t* data)
{
	return *(int32_t*)data;
}

// helper for reading unsigned 32b
uint32_t from_u32(const uint8_t* data)
{
	return *(uint32_t*)data;
}

// helper for reading signed 16b
int32_t from_s16(const uint8_t* data)
{
	return *(int16_t*)(data);
}

// helper for reading unsigned 16b
uint32_t from_u16(const uint8_t* data)
{
	return *(uint16_t*)data;
}

// helper for reading unsigned 8b
uint32_t from_u8(const uint8_t* data)
{
	const uint8_t* p = data;
	uint32_t result = p[0];
	return result;
}

// conversion of eid from string form to u32int form
uint32_t eid_to_int(std::string eid)
{
	uint32_t result = 0;
	const char charset[] =
		"0123456789"
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"_!";

	for (int32_t i = 0; i < 5; i++)
		for (int32_t j = 0; j < 0x40; j++)
			if (charset[j] == eid.c_str()[i])
				result = (result << 6) + j;

	result = 1 + (result << 1);
	return result;
}

// calculates chunk checksum (CRC)
uint32_t nsfChecksum(const uint8_t* data, int32_t size)
{
	uint32_t checksum = 0x12345678;
	for (int32_t i = 0; i < size; i++)
	{
		if (i < CHUNK_CHECKSUM_OFFSET || i >= CHUNK_CHECKSUM_OFFSET + 4)
			checksum += data[i];
		checksum = checksum << 3 | checksum >> 29;
	}
	return checksum;
}

// integer comparison func for qsort
int32_t cmp_func_int(const void* a, const void* b)
{
	return (*(int32_t*)a - *(int32_t*)b);
}

// calculates distance of two 3D points
int32_t point_distance_3D(const int16_t* a, const int16_t* b)
{
	int32_t dx = a[0] - b[0];
	int32_t dy = a[1] - b[1];
	int32_t dz = a[2] - b[2];
	return static_cast<int32_t>(std::sqrt(dx * dx + dy * dy + dz * dz));
}

// fixes path string supplied by the user, if it starts with "
void path_fix(char* fpath)
{
	if (fpath[0] == '\"')
	{
		strcpy(fpath, fpath + 1);
		*(strchr(fpath, '\0') - 1) = '\0';
	}
}

static thread_local std::mt19937 g_rng(std::random_device{}());
void rand_seed(int32_t seed)
{
	srand(seed);
	g_rng.seed(seed);
}

double randfrom(double min, double max)
{
	std::uniform_real_distribution<double> dist(min, max);
	return dist(g_rng);
}

// caps the angle to 0-360
int32_t normalize_angle(int32_t angle)
{
	while (angle < 0)
		angle += 360;
	while (angle >= 360)
		angle -= 360;

	return angle;
}

// converts the ingame yaw value range to regular range
int32_t c2yaw_to_deg(int32_t yaw)
{
	int32_t angle = (yaw * 360) / 4096;
	angle += 90;

	return normalize_angle(angle);
}

// converts 0-360 range angle to ingame angle value
int32_t deg_to_c2yaw(int32_t deg)
{
	int32_t angle = deg - 90;
	int32_t yaw = (angle * 4096) / 360;
	while (yaw < 0)
		yaw += 4096;
	yaw = yaw % 4096;
	return yaw;
}

// calculates angle distance between 2 angles
int32_t angle_distance(int32_t angle1, int32_t angle2)
{
	angle1 = normalize_angle(angle1);
	angle2 = normalize_angle(angle2);

	int32_t diff1 = normalize_angle(angle2 - angle1);
	int32_t diff2 = normalize_angle(angle1 - angle2);

	return min(diff1, diff2);
}

// averages 2 angles
int32_t average_angles(int32_t angle1, int32_t angle2)
{
	// Convert degrees to radians
	double a1 = angle1 * PI / 180.0;
	double a2 = angle2 * PI / 180.0;

	// Convert to Cartesian coordinates
	double x = cos(a1) + cos(a2);
	double y = sin(a1) + sin(a2);

	// Compute the averaged angle in degrees
	return (int32_t)round(atan2(y, x) * 180.0 / PI);
}


// calculates the amount of chunks based on the nsf size
int32_t chunk_count_base(FILE* nsf)
{
	fseek(nsf, 0, SEEK_END);
	int32_t result = ftell(nsf) / CHUNKSIZE;
	rewind(nsf);

	return result;
}

int32_t ask_level_ID()
{
	int32_t level_ID;
	printf("\nWhat ID do you want your level to have? (hex 0 - 3F) [CAN OVERWRITE EXISTING FILES!]\n");
	scanf("%x", &level_ID);
	if (level_ID < 0 || level_ID > 0x3F)
	{
		printf("Invalid ID, defaulting to 1\n");
		level_ID = 1;
	}
	printf("Selected level ID %02X\n", level_ID);

	return level_ID;
}

ChunkType chunk_type(uint8_t* chunk)
{
	if (chunk == NULL)
		return ChunkType(-1);

	return ChunkType(from_u16(chunk + 0x2));
}

int32_t distance_with_penalty(int32_t distance, double backw_penalty)
{
	return int32_t(distance * (1.0 - backw_penalty));
};