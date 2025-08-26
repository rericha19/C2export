#include "../include.h"

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
    const uint8_t *p = data;
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
uint32_t crcChecksum(const uint8_t* data, int32_t size)
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

// calculates chunk checksum
uint32_t nsfChecksum(const uint8_t* data)
{
	return crcChecksum(data, CHUNKSIZE);
}

// integer comparison func for qsort
int32_t cmp_func_int(const void* a, const void* b)
{
	return (*(int32_t*)a - *(int32_t*)b);
}

// calculates distance of two 3D points
int32_t point_distance_3D(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t z1, int16_t z2)
{
	return (int32_t)sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2));
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
// for reading txt files
// copied from stackoverflow
int32_t getdelim(char** linep, int32_t* n, int32_t delim, FILE* fp)
{
	int32_t ch;
	int32_t i = 0;
	if (!linep || !n || !fp)
	{
		errno = EINVAL;
		return -1;
	}
	if (*linep == NULL)
	{
		if (NULL == (*linep = (char*)try_malloc(*n = 128)))
		{
			*n = 0;
			errno = ENOMEM;
			return -1;
		}
	}
	while ((ch = fgetc(fp)) != EOF)
	{
		if (i + 1 >= *n)
		{
			char* buf = (char*)realloc(*linep, *n + 128);
			if (!buf)
			{
				errno = ENOMEM;
				return -1;
			}
			*n += 128;
			*linep = buf;
		}
		(*linep)[i++] = ch;
		if (ch == delim)
			break;
	}
	(*linep)[i] = '\0';
	return !i && ch == EOF ? -1 : i;
}

// for reading text files
int32_t getline(char** linep, int32_t* n, FILE* fp)
{
	return getdelim(linep, n, '\n', fp);
}

double randfrom(double min, double max)
{
	double range = (max - min);
	double div = RAND_MAX / range;
	return min + (rand() / div);
}
