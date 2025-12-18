#include "../include.h"
#include "../utils/entry.hpp"
#include "../utils/elist.hpp"
#include "../side_scripts/level_analyze.hpp"
#include "../side_scripts/side_scripts.hpp"

// one of old texture recoloring utils
void texture_recolor_stupid()
{
	char fpath[1000];
	printf("Path to color item:\n");
	scanf(" %[^\n]", fpath);
	path_fix(fpath);

	FILE* file1;
	if ((file1 = fopen(fpath, "rb+")) == NULL)
	{
		printf("[ERROR] Couldn't open file.\n\n");
		return;
	}

	int32_t r_wanted, g_wanted, b_wanted;
	printf("R G B? [hex]\n");
	scanf("%x %x %x", &r_wanted, &g_wanted, &b_wanted);

	fseek(file1, 0, SEEK_END);
	int32_t file_len = ftell(file1);
	uint8_t* buffer = (uint8_t*)try_malloc(file_len * sizeof(uint8_t*));
	rewind(file1);
	fread(buffer, 1, file_len, file1);

	// pseudograyscale of the wanted color
	int32_t sum_wanted = r_wanted + g_wanted + b_wanted;

	for (int32_t i = 0; i < file_len; i += 2)
	{
		uint16_t clr_val = *(uint16_t*)(buffer + i);
		int32_t r = clr_val & 0x1F;
		int32_t g = (clr_val >> 5) & 0x1F;
		int32_t b = (clr_val >> 10) & 0x1F;
		int32_t a = (clr_val >> 15) & 1;

		r *= 8;
		g *= 8;
		b *= 8;

		// get pseudograyscale of the current color
		int32_t sum = r + g + b;

		// get new color
		int32_t r_new = (sum * r_wanted) / sum_wanted;
		int32_t g_new = (sum * g_wanted) / sum_wanted;
		int32_t b_new = (sum * b_wanted) / sum_wanted;

		// clip it at 0xFF
		r_new = min(r_new, 0xFF);
		g_new = min(g_new, 0xFF);
		b_new = min(b_new, 0xFF);

		// print stuff
		printf("old: %2X %2X %2X\n", r, g, b);
		printf("new: %2X %2X %2X\n", r_new, g_new, b_new);

		r_new /= 8;
		g_new /= 8;
		b_new /= 8;

		// write back
		uint16_t clr_out = (a << 15) + (b_new << 10) + (g_new << 5) + (r_new);
		*(uint16_t*)(buffer + i) = clr_out;
	}

	rewind(file1);
	fwrite(buffer, 1, file_len, file1);
	fclose(file1);
	free(buffer);
}

// for recoloring, i use some dumb algorithm that i think does /some/ job and thats about it
void scenery_recolor_main()
{
	char fpath[1000];
	printf("Path to color item:\n");
	scanf(" %[^\n]", fpath);
	path_fix(fpath);

	FILE* file1;
	if ((file1 = fopen(fpath, "rb+")) == NULL)
	{
		printf("[ERROR] Couldn't open file.\n\n");
		return;
	}

	fseek(file1, 0, SEEK_END);
	int32_t color_count = ftell(file1) / 4;
	uint8_t* buffer = (uint8_t*)try_malloc((color_count * 4) * sizeof(uint8_t*));
	rewind(file1);
	fread(buffer, color_count, 4, file1);

	int32_t r_wanted = 0x85;
	int32_t g_wanted = 0x28;
	int32_t b_wanted = 0x6A;

	int32_t sum_wanted = r_wanted + g_wanted + b_wanted;

	for (int32_t i = 0; i < color_count; i++)
	{
		// read current color
		uint8_t r = buffer[4 * i + 0];
		uint8_t g = buffer[4 * i + 1];
		uint8_t b = buffer[4 * i + 2];

		// get pseudograyscale of the current color
		int32_t sum = r + g + b;
		int32_t avg = sum / 3;

		// get new color
		int32_t r_new = (sum * r_wanted) / sum_wanted;
		int32_t g_new = (sum * g_wanted) / sum_wanted;
		int32_t b_new = (sum * b_wanted) / sum_wanted;

		r_new = (r_new + 2 * avg) / 3;
		g_new = (g_new + 2 * avg) / 3;
		b_new = (b_new + 2 * avg) / 3;

		float mult = 0.7f;
		r_new *= mult;
		g_new *= mult;
		b_new *= mult;

		// print stuff
		// printf("old: %2X %2X %2X\n", r, g, b);
		// printf("new: %2X %2X %2X\n", r_new, g_new, b_new);

		// clamp
		r_new = min(r_new, 255);
		g_new = min(g_new, 255);
		b_new = min(b_new, 255);
		r_new = max(0, r_new);
		g_new = max(0, g_new);
		b_new = max(0, b_new);

		// write back
		buffer[4 * i + 0] = r_new;
		buffer[4 * i + 1] = g_new;
		buffer[4 * i + 2] = b_new;
	}

	rewind(file1);
	fwrite(buffer, color_count, 4, file1);
	fclose(file1);
	free(buffer);

	printf("\nDone\n\n");
}

// garbage, probably doesnt work properly
void rotate_main()
{
	time_t rawtime;
	struct tm* timeinfo;
	char time_str[9] = "";
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	sprintf(time_str, "%02d_%02d_%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

	for (uint32_t i = 0; i < strlen(time_str); i++)
		if (!isalnum(time_str[i]))
			time_str[i] = '_';

	double rotation;
	char path[MAX];
	FILE* file;
	uint8_t* entry;
	int32_t filesize;

	scanf("%lf", &rotation);
	if (rotation < 0 || rotation >= 360)
	{
		printf("Input a value in range (0; 360) degrees\n");
		return;
	}

	rotation = 2 * PI * rotation / 360;

	printf("%lf\n\n", rotation);

	printf("Input the path to the ENTRY you want to export:\n");
	scanf(" %[^\n]", path);
	path_fix(path);

	if ((file = fopen(path, "rb")) == NULL)
	{
		printf("[ERROR] File specified could not be opened\n");
		return;
	}
	fseek(file, 0, SEEK_END);
	filesize = ftell(file);
	rewind(file);
	entry = (uint8_t*)try_malloc(filesize); // freed here
	fread(entry, sizeof(uint8_t), filesize, file);
	// if (entry[8] == 7)
	// rotate_zone(entry, path, rotation);
	if (entry[8] == 3)
		rotate_scenery(entry, path, rotation, time_str, filesize);
	free(entry);
	fclose(file);
}

// rotates scenery
void rotate_scenery(uint8_t* buffer, char* filepath, double rotation, char* time, int32_t filesize)
{
	FILE* filenew;
	char* help, fpath[MAX];
	int32_t curr_off, next_off, i;
	uint32_t group, rest, vert;
	curr_off = BYTE * buffer[0x15] + buffer[0x14];
	next_off = BYTE * buffer[0x19] + buffer[0x18];
	int32_t vertcount = (next_off - curr_off) / 6;
	uint32_t** verts = (uint32_t**)try_malloc(vertcount * sizeof(uint32_t*)); // freed here
	for (int32_t i = 0; i < vertcount; i++)
		verts[i] = (uint32_t*)try_malloc(2 * sizeof(uint32_t*)); // freed here

	for (i = curr_off; i < curr_off + 6 * vertcount; i += 2)
	{
		group = 256 * buffer[i + 1] + buffer[i];
		vert = group / 16;
		if (i < (4 * vertcount + curr_off) && (i % 4 == 0))
			verts[(i - curr_off) / 4][0] = vert;

		else if (i >= (4 * vertcount + curr_off))
			verts[(i - curr_off) / 2 - vertcount * 2][1] = vert;
		if (vert > 2048)
			printf("x_index: %d, z_index: %d\n", (i - curr_off) / 4, (i - curr_off) / 2 - vertcount * 2);
	}

	for (int32_t i = 0; i < vertcount; i++)
		rotate_rotate(&verts[i][0], &verts[vertcount - 1 - i][1], rotation);

	for (i = curr_off; i < curr_off + 6 * vertcount; i += 2)
	{
		group = 256 * buffer[i + 1] + buffer[i];
		rest = group % 16;
		vert = group / 16;
		if (i < (4 * vertcount + curr_off) && (i % 4 == 0))
		{
			vert = verts[(i - curr_off) / 4][0];
		}

		else if (i >= (4 * vertcount + curr_off))
		{
			vert = verts[(i - curr_off) / 2 - vertcount * 2][1];
		}

		group = 16 * vert + rest;
		buffer[i + 1] = group / 256;
		buffer[i] = group % 256;
	}

	help = strrchr(filepath, '\\');
	*help = '\0';
	help = help + 1;
	sprintf(fpath, "%s\\%s_%s", filepath, time, help);

	filenew = fopen(fpath, "wb");
	fwrite(buffer, sizeof(uint8_t), filesize, filenew);
	fclose(filenew);
	for (int32_t i = 0; i < vertcount; i++)
		free(verts[i]);
	free(verts);
}

// rotates xy coord by some angle
void rotate_rotate(uint32_t* y, uint32_t* x, double rotation)
{
	int32_t x_t = *x;
	int32_t y_t = *y;

	*x = (int32_t)(x_t * cos(rotation) - y_t * sin(rotation));
	*y = (int32_t)(x_t * sin(rotation) + y_t * cos(rotation));
}

//  Copies texture from one texture chunk to another, doesnt include cluts.
void texture_copy_main()
{
	char fpath[1000];
	printf("Path to source texture:\n");
	scanf(" %[^\n]", fpath);
	path_fix(fpath);
	FILE* file1 = fopen(fpath, "rb");
	uint8_t* texture1 = (uint8_t*)try_malloc(CHUNKSIZE);

	printf("Path to destination texture:\n");
	scanf(" %[^\n]", fpath);
	path_fix(fpath);
	FILE* file2 = fopen(fpath, "rb+");
	uint8_t* texture2 = (uint8_t*)try_malloc(CHUNKSIZE);

	if (file1 == NULL)
	{
		printf("[ERROR] Source texture could not be opened.\n\n");
		return;
	}

	if (file2 == NULL)
	{
		printf("[ERROR] Destination texture could not be opened.\n\n");
		return;
	}

	fread(texture1, CHUNKSIZE, 1, file1);
	fread(texture2, CHUNKSIZE, 1, file2);

	while (1)
	{
		int32_t bpp, src_x, src_y, width, height, dest_x, dest_y;
		printf("bpp src_x src_y width height dest-x dest-y? [set bpp to 0 to end]\n");
		scanf("%d", &bpp);
		if (bpp == 0)
			break;
		scanf(" %x %x %x %x %x %x", &src_x, &src_y, &width, &height, &dest_x, &dest_y);

		switch (bpp)
		{
		case 4:
			for (int32_t i = 0; i < height; i++)
			{
				int32_t offset1 = (i + src_y) * 0x200 + src_x / 2;
				int32_t offset2 = (i + dest_y) * 0x200 + dest_x / 2;
				printf("i: %2d offset1: %5d, offset2: %5d\n", i, offset1, offset2);
				memcpy(texture2 + offset2, texture1 + offset1, (width * 4) / 8);
			}
			break;
		case 8:
			for (int32_t i = 0; i < height; i++)
			{
				int32_t offset1 = (i + src_y) * 0x200 + src_x;
				int32_t offset2 = (i + dest_y) * 0x200 + dest_x;
				printf("i: %2d offset1: %5d, offset2: %5d\n", i, offset1, offset2);
				memcpy(texture2 + offset2, texture1 + offset1, width);
			}
			break;
		default:
			break;
		}
	}

	rewind(file2);
	fwrite(texture2, CHUNKSIZE, 1, file2);

	fclose(file1);
	fclose(file2);

	free(texture1);
	free(texture2);
	printf("Done\n\n");
}

// property printing util
void print_prop_header(uint8_t* arr, int32_t off)
{
	printf("\nheader\t");
	for (int32_t j = 0; j < 8; j++)
	{
		printf("%02X", arr[off + j]);
		if (j % 4 == 3)
			printf(" ");
	}
}

// property printing util
void print_prop_body(uint8_t* arr, int32_t offset, int32_t offset_next)
{
	printf("\ndata\t");

	for (int32_t j = 0; j < offset_next - offset; j++)
	{
		printf("%02X", arr[offset + j]);
		if (j % 4 == 3)
			printf(" ");
		if ((j % 16 == 15) && (j + 1 != offset_next - offset))
			printf("\n\t");
	}

	printf("\n\n");
}

// Prints out properties present in the file.
void prop_main()
{
	char path[MAX] = "";
	printf("Input the path to the file:\n");
	scanf(" %[^\n]", path);
	path_fix(path);

	FILE* file = NULL;
	uint32_t fsize, code, offset, offset_next;
	uint8_t* arr;

	if ((file = fopen(path, "rb")) == NULL)
	{
		printf("[ERROR] File could not be opened\n\n");
		return;
	}

	fseek(file, 0, SEEK_END);
	fsize = ftell(file);
	rewind(file);

	arr = (uint8_t*)try_malloc(fsize); // freed here
	fread(arr, fsize, sizeof(uint8_t), file);

	printf("\n");
	for (int32_t i = 0; i < PROPERTY::count(arr); i++)
	{
		code = from_u16(arr + 0x10 + 8 * i);
		offset = from_u16(arr + 0x12 + 8 * i) + OFFSET;
		offset_next = from_u16(arr + 0x1A + 8 * i) + OFFSET;
		if (i == (PROPERTY::count(arr) - 1))
			offset_next = from_u16(arr);
		printf("0x%03X\t", code);
		switch (code)
		{
		case 0x29:
			printf("camera mode");
			break;
		case 0x4B:
			printf("positions/angles");
			break;
		case 0xC9:
			printf("avg point distance");
			break;
		case 0x103:
			printf("SLST");
			break;
		case 0x109:
			printf("path links");
			break;
		case 0x130:
			printf("FOV");
			break;
		case 0x13B:
		case 0x13C:
			printf("draw list");
			break;
		case 0x162:
			printf("progress thing?????");
			break;
		case 0x16D:
			printf("camera path related?????");
			break;
		case 0x173:
			printf("path number");
			break;
		case 0x174:
			printf("path item number");
			break;
		case 0x176:
			printf("link count");
			break;
		case 0x1A8:
			printf("warp/cam switch");
			break;
		case 0x1AA:
			printf("camera related?????");
			break;
		case 0x208:
		case 0x209:
			printf("load list");
			break;
		case 0x27F:
			printf("update scenery info");
			break;

		case 0x1FA:
			printf("item1 bg color");
			break;
		case 0x142:
			printf("item1 cam distance");
			break;
		default:
			break;
		}

		print_prop_header(arr, 0x10 + 8 * i);
		print_prop_body(arr, offset, offset_next);
	}

	free(arr);
	fclose(file);
	printf("Done \n\n");
}

// command for removing an entity property
void prop_remove_script()
{
	char fpath[1000];
	printf("Path to item:\n");
	scanf(" %[^\n]", fpath);
	path_fix(fpath);

	FILE* file1 = fopen(fpath, "rb");
	if (file1 == NULL)
	{
		printf("[ERROR] File could not be opened\n\n");
		return;
	}

	fseek(file1, 0, SEEK_END);
	int32_t fsize = ftell(file1);
	rewind(file1);

	std::vector<uint8_t> item(fsize);
	fread(item.data(), 1, fsize, file1);
	fclose(file1);

	int32_t prop_code;
	printf("\nWhat prop do you wanna remove? (hex)\n");
	scanf("%x", &prop_code);
	int32_t fsize2 = fsize;
	item = PROPERTY::item_rem_property(prop_code, item, NULL);
	fsize2 = int32_t(item.size());

	char fpath2[1004];
	sprintf(fpath2, "%s-alt", fpath);

	FILE* file2 = fopen(fpath2, "wb");
	fwrite(item.data(), 1, fsize, file2);
	fclose(file2);

	if (fsize == fsize2)
		printf("Seems like no changes were made\n");
	else
		printf("Property removed\n");
	printf("Done. Altered file saved as %s\n\n", fpath2);
}

// command for replacing property data
void prop_replace_script()
{
	char fpath[1000];
	printf("Path to source item:\n");
	scanf(" %[^\n]", fpath);
	path_fix(fpath);

	FILE* file1 = fopen(fpath, "rb");
	if (file1 == NULL)
	{
		printf("[ERROR] File could not be opened\n\n");
		return;
	}

	fseek(file1, 0, SEEK_END);
	int32_t fsize = ftell(file1);
	rewind(file1);

	std::vector<uint8_t> item(fsize);
	fread(item.data(), 1, fsize, file1);
	fclose(file1);

	char fpath2[1000];
	printf("Path to destination item:\n");
	scanf(" %[^\n]", fpath2);
	path_fix(fpath2);

	FILE* file2 = fopen(fpath2, "rb+");
	if (file2 == NULL)
	{
		printf("[ERROR] File could not be opened\n\n");
		return;
	}

	fseek(file2, 0, SEEK_END);
	int32_t fsize2 = ftell(file2);
	rewind(file2);

	std::vector<uint8_t> item2(fsize2);
	fread(item2.data(), 1, fsize2, file2);

	int32_t fsize2_before;
	int32_t fsize2_after;
	int32_t prop_code;
	printf("\nWhat prop do you wanna replace/insert? (hex)\n");
	scanf("%x", &prop_code);

	PROPERTY prop = PROPERTY::get_full(item.data(), prop_code);
	if (prop.length == 0)
	{
		printf("Property wasnt found in the source file\n\n");
		return;
	}

	fsize2_before = fsize2;
	item2 = PROPERTY::item_rem_property(prop_code, item2, NULL);
	fsize2 = int32_t(item2.size());

	fsize2_after = fsize2;
	item2 = PROPERTY::item_add_property(prop_code, item2, &prop);
	fsize2 = int32_t(item2.size());

	rewind(file2);
	fwrite(item2.data(), 1, fsize2, file2);
	fclose(file2);

	if (fsize2_before == fsize2_after)
		printf("Property inserted. ");
	else
		printf("Property replaced. ");
	printf("Done.\n\n");
}

// im not gonna comment the resize stuff cuz it looks atrocious
void resize_main()
{
	time_t rawtime;
	struct tm* timeinfo;
	char time_str[9] = "";
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	sprintf(time_str, "%02d_%02d_%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

	for (uint32_t i = 0; i < strlen(time_str); i++)
		if (!isalnum(time_str[i]))
			time_str[i] = '_';

	char path[MAX] = "";
	double scale[3];
	bool check = true;

	int32_t gamemode;

	scanf("%d %lf %lf %lf", &gamemode, &scale[0], &scale[1], &scale[2]);
	if (gamemode != 2 && gamemode != 3)
	{
		printf("[error] invalid gamemode, defaulting to 2");
		gamemode = 2;
	}
	printf("Input the path to the directory or level whose contents you want to resize:\n");
	scanf(" %[^\n]", path);
	path_fix(path);

	if (!std::filesystem::exists(path))
	{
		printf("[ERROR] %s doesnt exist\n", path);
		return;
	}

	std::error_code ec{};
	if (std::filesystem::is_directory(path, ec)) // opendir returns NULL if couldn't open directory
		resize_folder(path, scale, time_str, gamemode);
	else if (std::filesystem::is_regular_file(path))
		resize_level(path, scale, time_str, gamemode);
	else
	{
		printf("Couldn't open\n");
		check = false;
	}

	if (check)
		printf("\nEntries' dimensions resized\n\n");
}

// yep resizes level
void resize_level(char* filepath, double scale[3], char* time, int32_t game)
{
	std::filesystem::path inPath(filepath);

	if (!std::filesystem::exists(inPath))
	{
		std::printf("[ERROR] Input file not found: %s\n", filepath);
		return;
	}

	// Build output path: same folder, new name with time prefix
	std::string fname = inPath.filename().string();
	std::filesystem::path outPath = inPath.parent_path() / (std::string(time) + "_" + fname);

	// Open input file
	std::ifstream inFile(inPath, std::ios::binary | std::ios::ate);
	if (!inFile)
	{
		std::printf("[ERROR] Could not open input file: %s\n", filepath);
		return;
	}

	std::streamsize totalSize = inFile.tellg();
	if (totalSize <= 0)
	{
		std::printf("[ERROR] Empty or invalid file: %s\n", filepath);
		return;
	}
	inFile.seekg(0, std::ios::beg);

	// Open output file
	std::ofstream outFile(outPath, std::ios::binary);
	if (!outFile)
	{
		std::printf("[ERROR] Could not create output file: %s\n", outPath.string().c_str());
		return;
	}

	const std::size_t chunkSize = CHUNKSIZE;
	std::vector<uint8_t> buffer(chunkSize);

	int32_t chunkCount = static_cast<int32_t>(totalSize / chunkSize);

	for (int32_t i = 0; i < chunkCount; i++)
	{
		if (!inFile.read(reinterpret_cast<char*>(buffer.data()), chunkSize))
		{
			std::printf("[ERROR] Failed reading chunk %d from %s\n", i, filepath);
			break;
		}

		resize_chunk_handler(buffer.data(), game, scale);

		outFile.write(reinterpret_cast<char*>(buffer.data()), chunkSize);
		if (!outFile)
		{
			std::printf("[ERROR] Failed writing chunk %d to %s\n", i, outPath.string().c_str());
			break;
		}
	}
}
// yep resizes model
void resize_model(int32_t fsize, uint8_t* buffer, double scale[3])
{
	int32_t eid = from_u32(buffer + 4);
	int32_t pb10g = eid_to_int("Pb10G");
	int32_t bd10g = eid_to_int("Bd10G");
	int32_t cry1g = eid_to_int("Cry1G");
	int32_t cry2g = eid_to_int("Cry2G");
	int32_t rel1g = eid_to_int("Rel1G");
	int32_t ge10g = eid_to_int("Ge10G");
	int32_t ge30g = eid_to_int("Ge30G");
	int32_t ge40g = eid_to_int("Ge40G");

	if (eid == pb10g || eid == bd10g || eid == cry1g || eid == cry2g || eid == rel1g || eid == ge10g || eid == ge30g || eid == ge40g)
	{
		printf("model Pb10G/Bd10G/Cry1G/Cry2G/Rel1G/Ge10G/Ge30G/Ge40G, skipping\n");
		return;
	}

	int32_t o = ENTRY::get_nth_item_offset(buffer, 0);

	int32_t sx = *(int32_t*)(buffer + o);
	int32_t sy = *(int32_t*)(buffer + o + 4);
	int32_t sz = *(int32_t*)(buffer + o + 8);

	sx = (int32_t)(sx * scale[0]);
	sy = (int32_t)(sy * scale[1]);
	sz = (int32_t)(sz * scale[2]);

	*(int32_t*)(buffer + o) = sx;
	*(int32_t*)(buffer + o + 4) = sy;
	*(int32_t*)(buffer + o + 8) = sz;
}

// yep resizes animation
void resize_animation(int32_t fsize, uint8_t* buffer, double scale[3])
{
	int32_t itemc = ENTRY::get_item_count(buffer);

	for (int32_t i = 0; i < itemc; i++)
	{
		int32_t o = ENTRY::get_nth_item_offset(buffer, i);

		if (*(int32_t*)(buffer + o + 0xC))
		{
			int32_t xyz_iter = 0;
			int32_t end = *(int32_t*)(buffer + o + 0x14);
			for (int32_t j = 0x1C; j < end; j += 4)
			{
				int32_t val = *(int32_t*)(buffer + o + j);
				val = (int32_t)(val * scale[xyz_iter]);
				*(int32_t*)(buffer + o + j) = val;
				xyz_iter = (xyz_iter + 1) % 3;
			}
		}
	}
}

// yep fixes camera distance
void resize_zone_camera_distance(int32_t fsize, uint8_t* buffer, double scale[3])
{
	int32_t cam_c = ENTRY::get_cam_item_count(buffer);

	for (int32_t i = 0; i < cam_c; i += 3)
	{
		int32_t o = ENTRY::get_nth_item_offset(buffer, 2 + i + 1);

		uint8_t* entity = buffer + o;
		for (int32_t j = 0; j < PROPERTY::count(entity); j++)
		{
			int32_t code = from_u16(entity + 0x10 + 8 * j);
			int32_t offset = from_u16(entity + 0x12 + 8 * j) + OFFSET;
			int32_t val_c = from_u16(entity + 0x16 + 8 * j);

			if (code == ENTITY_PROP_CAM_DISTANCE)
			{
				int32_t prop_meta_len = 4;
				if (val_c == 2 || val_c == 3)
					prop_meta_len = 8;

				for (int32_t k = 0; k < val_c * 4; k++)
				{
					int32_t real_off = o + offset + prop_meta_len + (k * 2);

					int32_t val = *(int16_t*)(buffer + real_off);
					val = (int16_t)(val * scale[0]);
					*(int16_t*)(buffer + real_off) = val;
				}
			}
		}
	}
}

// yep resizes chunk
void resize_chunk_handler(uint8_t* chunk, int32_t game, double scale[3])
{
	int32_t offset_start, offset_end;
	uint32_t checksum;
	uint8_t* entry = NULL;
	if (chunk[2] != 0)
		return;

	for (int32_t i = 0; i < chunk[8]; i++)
	{
		offset_start = BYTE * chunk[0x11 + i * 4] + chunk[0x10 + i * 4];
		offset_end = BYTE * chunk[0x15 + i * 4] + chunk[0x14 + i * 4];
		if (!offset_end)
			offset_end = CHUNKSIZE;
		entry = chunk + offset_start;

		ENTRY curr_entry{};
		curr_entry.m_esize = offset_end - offset_start;
		curr_entry.m_data.resize(curr_entry.m_esize);
		memcpy(curr_entry.m_data.data(), entry, curr_entry.m_esize);
		EntryType etype = curr_entry.get_entry_type();

		if (etype == EntryType::Zone)
			resize_zone(offset_end - offset_start, entry, scale, game);
		else if (etype == EntryType::Scenery)
			resize_scenery(offset_end - offset_start, entry, scale, game);
		else if (etype == EntryType::Model)
			resize_model(offset_end - offset_start, entry, scale);
		else if (etype == EntryType::Anim)
			resize_animation(offset_end - offset_start, entry, scale);

		if (etype == EntryType::Zone)
			resize_zone_camera_distance(offset_end - offset_start, entry, scale);
	}

	checksum = nsfChecksum(chunk);

	for (int32_t i = 0; i < 4; i++)
	{
		chunk[0xC + i] = checksum % 256;
		checksum /= 256;
	}
}

// yep resizes folder
void resize_folder(char* path, double scale[3], char* time, int32_t game)
{
	try
	{
		std::filesystem::path basePath(path);
		std::filesystem::path outDir = basePath / time;

		std::filesystem::create_directory(outDir);

		for (const auto& entry : std::filesystem::directory_iterator(basePath))
		{
			if (!entry.is_regular_file())
				continue;

			const std::filesystem::path& fpath = entry.path();
			const std::string fname = fpath.filename().string();

			if (fname.empty() || fname[0] == '.')
				continue;

			char prefix[6] = "";
			std::strncpy(prefix, fname.c_str(), 5);
			prefix[5] = '\0';

			// Only process "scene" or "zone "
			if (std::strcmp(prefix, "scene") == 0 || std::strcmp(prefix, "zone ") == 0)
			{
				// Read file into buffer
				std::ifstream file(fpath, std::ios::binary | std::ios::ate);
				if (!file)
				{
					printf("[ERROR] Could not open file %s\n", fpath.string().c_str());
					continue;
				}
				std::streamsize filesize = file.tellg();
				file.seekg(0, std::ios::beg);

				std::vector<uint8_t> buffer(filesize);
				if (!file.read(reinterpret_cast<char*>(buffer.data()), filesize))
				{
					printf("[ERROR] Failed reading file %s\n", fpath.string().c_str());
					continue;
				}

				// Resize depending on prefix
				if (std::strcmp(prefix, "scene") == 0)
				{
					resize_scenery(static_cast<int32_t>(filesize), buffer.data(), scale, game);
				}
				else if (std::strcmp(prefix, "zone ") == 0)
				{
					resize_zone(static_cast<int32_t>(filesize), buffer.data(), scale, game);
				}

				// Output path
				std::filesystem::path newFile = outDir / fpath.filename();

				// Write buffer back
				std::ofstream filenew(newFile, std::ios::binary);
				if (!filenew)
				{
					printf("[ERROR] Could not write file %s\n", newFile.string().c_str());
					continue;
				}
				filenew.write(reinterpret_cast<const char*>(buffer.data()), filesize);
			}
		}
	}
	catch (const std::exception& e)
	{
		printf("[warning] %s\n%s\n", e.what(), path);
	}
}

// yep resizes zone
void resize_zone(int32_t fsize, uint8_t* buffer, double scale[3], int32_t game)
{
	int32_t itemcount = ENTRY::get_item_count(buffer);
	int32_t* itemoffs = (int32_t*)try_malloc(itemcount * sizeof(int32_t)); // freed here

	for (int32_t i = 0; i < itemcount; i++)
		itemoffs[i] = 256 * buffer[0x11 + i * 4] + buffer[0x10 + i * 4];

	for (int32_t i = 0; i < 6; i++)
	{
		uint32_t coord = from_u32(buffer + itemoffs[1] + i * 4);
		if (coord >= (1LL << 31))
		{
			coord = (uint32_t)((1LL << 32) - coord);
			coord = (uint32_t)coord * scale[i % 3];
			coord = (uint32_t)(1LL << 32) - coord;
			for (int32_t j = 0; j < 4; j++)
			{
				buffer[itemoffs[1] + i * 4 + j] = coord % 256;
				coord = coord / 256;
			}
		}
		else
		{
			coord = (uint32_t)coord * scale[i % 3];
			for (int32_t j = 0; j < 4; j++)
			{
				buffer[itemoffs[1] + i * 4 + j] = coord % 256;
				coord = coord / 256;
			}
		}
	}

	for (int32_t i = 2; i < itemcount; i++)
		if (i > buffer[itemoffs[0] + 0x188] || (i % 3 == 2))
			resize_entity(buffer + itemoffs[i], itemoffs[i + 1] - itemoffs[i], scale);

	free(itemoffs);
}

// yep resizes entity
void resize_entity(uint8_t* item, int32_t itemsize, double scale[3])
{
	for (int32_t i = 0; i < ENTRY::get_item_count(item); i++)
	{
		int32_t code = from_u16(item + 0x10 + 8 * i);
		int32_t offset = from_u16(item + 0x12 + 8 * i) + OFFSET;

		if (code == 0x4B)
		{
			int32_t pathlen = from_u32(item + offset);
			for (int32_t j = 0; j < pathlen * 3; j++)
			{
				double sc = scale[j % 3];
				int32_t val = *(int16_t*)(item + offset + 4 + j * 2);
				val = (int16_t)(val * sc);
				*(int16_t*)(item + offset + 4 + j * 2) = val;
			}
		}
	}
}

// yep resizes scenery
void resize_scenery(int32_t fsize, uint8_t* buffer, double scale[3], int32_t game)
{
	int32_t item1off, curr_off, next_off, group;
	int64_t origin;
	int32_t vertcount;

	item1off = buffer[0x10];
	for (int32_t i = 0; i < 3; i++)
	{
		origin = from_u32(buffer + item1off + 4 * i);
		if (origin >= (1LL << 31))
		{
			origin = (1LL << 32) - origin;
			origin = scale[i] * origin;
			origin = (1LL << 32) - origin;
			for (int32_t j = 0; j < 4; j++)
			{
				buffer[item1off + j + i * 4] = origin % 256;
				origin /= 256;
			}
		}
		else
		{
			origin = scale[i] * origin;
			for (int32_t j = 0; j < 4; j++)
			{
				buffer[item1off + j + i * 4] = origin % 256;
				origin /= 256;
			}
		}
	}

	curr_off = BYTE * buffer[0x15] + buffer[0x14];
	next_off = BYTE * buffer[0x19] + buffer[0x18];
	vertcount = next_off - curr_off / 6;

	for (int32_t i = curr_off; i < next_off; i += 2)
	{
		group = from_s16(buffer + i);
		int16_t vert = group & 0xFFF0;
		int16_t rest = group & 0xF;
		if (game == 2)
		{
			if (i >= 4 * vertcount + curr_off)
				vert *= scale[2];
			else if (i % 4 == 2)
				vert *= scale[1];
			else
				vert *= scale[0];
		}
		if (game == 2 && vert >= 2048)
		{
			vert = 4096 - vert;
			if (i < 4 * vertcount + curr_off)
			{
				if (i % 4 == 0)
					vert = vert * scale[0];
				else
					vert = vert * scale[1];
			}
			else
				vert = vert * scale[2];
			vert = 4096 - vert;
		}
		else
		{
			if (i < 4 * vertcount + curr_off)
			{
				if (i % 4 == 0)
					vert = vert * scale[0];
				else
					vert = vert * scale[1];
			}
			else
				vert = vert * scale[2];
		}

		group = 16 * vert + rest;
		buffer[i + 1] = group / 256;
		buffer[i] = group % 256;
	}
}

// obsolete
void crate_rotation_angle()
{
	int32_t angle;
	scanf("%d", &angle);
	while (angle < 0)
		angle += 360;
	while (angle > 360)
		angle -= 360;

	int32_t game_angle = (0x1000 * angle) / 360;
	int32_t b_value = game_angle >> 8;
	printf("A: %3d\n", game_angle & 0xFF);
	printf("B: %3d", game_angle >> 8);
	while (1)
	{
		b_value += 16;
		if (b_value > 105)
			break;
		printf(" | %3d", b_value);
	}
	printf(" (for rewards)\n\n");
}

// command for printing nsd table
void nsd_gool_table_print_cmd()
{
	char fpath[MAX] = "";
	printf("Input the path to the NSD file:\n");
	scanf(" %[^\n]", fpath);
	path_fix(fpath);

	FILE* file = NULL;
	if ((file = fopen(fpath, "rb")) == NULL)
	{
		printf("[ERROR] File could not be opened\n\n");
		return;
	}

	fseek(file, 0, SEEK_END);
	int32_t filesize = ftell(file);
	rewind(file);

	uint8_t* buffer = (uint8_t*)try_malloc(filesize * sizeof(uint8_t*)); // freed here
	fread(buffer, sizeof(uint8_t), filesize, file);

	int32_t entry_count = from_u32(buffer + C2_NSD_ENTRY_COUNT_OFFSET);
	for (int32_t i = 0; i < 64; i++)
	{
		int32_t eid = from_u32(buffer + C2_NSD_ENTRY_TABLE_OFFSET + 0x10 + 8 * entry_count + 4 * i);
		printf("%2d: %s  ", i, eid2str(eid));
		if (i % 4 == 3)
			putchar('\n');
	}
	fclose(file);
	free(buffer);
	printf("Done.\n\n");
}

// command for generating spawn
void generate_spawn()
{
	ELIST elist{};

	if (elist.read_and_parse_nsf(NULL, 1, NULL))
		return;

	char zone[100] = "";
	printf("\nWhat zone do you wanna spawn in?\n");
	scanf("%s", zone);

	int32_t entity_id;
	int32_t path_to_spawn_on = 0;
	int32_t entity_index = -1;
	uint32_t zone_eid = eid_to_int(zone);
	int32_t elist_index = elist.get_index(zone_eid);

	if (elist_index == -1)
	{
		printf("Zone %5s not found\n\n", zone);
		return;
	}

	int32_t cam_count = elist[elist_index].get_cam_item_count() / 3;
	if (cam_count == 0)
	{
		printf("Zone %5s does not have a camera path\n\n", zone);
		return;
	}
	else if (cam_count > 1)
	{
		printf("Which cam path do you want to spawn on (0 - %d)\n", cam_count - 1);
		scanf("%d", &path_to_spawn_on);
	}

	printf("\nWhat entity's coordinates do you wanna spawn on? (entity has to be in the zone)\n");
	scanf("%d", &entity_id);

	for (int32_t i = 0; i < elist[elist_index].get_entity_count(); i++)
	{
		uint8_t* entity = elist[elist_index].get_nth_entity(i);
		int32_t ID = PROPERTY::get_value(entity, ENTITY_PROP_ID);

		if (ID == entity_id)
			entity_index = i;
	}

	if (entity_index == -1)
	{
		printf("\nEntity with ID %4d not found in %5s\n\n", entity_id, zone);
		return;
	}

	ENTITY_PATH path = elist[elist_index].get_ent_path(2 + 3 * cam_count + entity_index);
	if (path.length() == 0)
	{
		printf("\n Entity doesnt have a position\n");
		return;
	}

	uint8_t* coll_item = elist[elist_index].get_nth_item(1);
	uint32_t zone_x = from_u32(coll_item);
	uint32_t zone_y = from_u32(coll_item + 4);
	uint32_t zone_z = from_u32(coll_item + 8);

	uint32_t x = (zone_x + 4 * path[0]) << 8;
	uint32_t y = (zone_y + 4 * path[1]) << 8;
	uint32_t z = (zone_z + 4 * path[2]) << 8;

	uint8_t arr[0x18] = { 0 };
	*(uint32_t*)arr = zone_eid;
	*(uint32_t*)(arr + 0x4) = path_to_spawn_on;
	*(uint32_t*)(arr + 0x8) = 0;
	*(uint32_t*)(arr + 0xC) = x;
	*(uint32_t*)(arr + 0x10) = y;
	*(uint32_t*)(arr + 0x14) = z;

	printf("\n");
	for (int32_t j = 0; j < 0x18; j++)
	{
		printf("%02X", arr[j]);
		if (j % 4 == 3)
			printf(" ");
		if ((j % 16 == 15) && (j + 1 != 0x18))
			printf("\n");
	}

	printf("\nDone.\n\n");
}

// for recoloring, i use some dumb algorithm that i think does /some/ job and thats about it
void scenery_recolor_main2()
{
	char fpath[1000];
	printf("Path to color item:\n");
	scanf(" %[^\n]", fpath);
	path_fix(fpath);
	float mult;

	FILE* file1;
	if ((file1 = fopen(fpath, "rb+")) == NULL)
	{
		printf("[ERROR] Couldn't open file.\n\n");
		return;
	}

	int32_t r_wanted, g_wanted, b_wanted;
	printf("R G B? [hex]\n");
	scanf("%x %x %x", &r_wanted, &g_wanted, &b_wanted);

	printf("brigtness mutliplicator? (float)\n");
	scanf("%f", &mult);

	fseek(file1, 0, SEEK_END);
	int32_t color_count = ftell(file1) / 4;
	uint8_t* buffer = (uint8_t*)try_malloc((color_count * 4) * sizeof(uint8_t*));
	rewind(file1);
	fread(buffer, color_count, 4, file1);

	// pseudograyscale of the wanted color
	int32_t sum_wanted = r_wanted + g_wanted + b_wanted;

	for (int32_t i = 0; i < color_count; i++)
	{
		// read current color
		uint8_t r = buffer[4 * i + 0];
		uint8_t g = buffer[4 * i + 1];
		uint8_t b = buffer[4 * i + 2];

		// get pseudograyscale of the current color
		int32_t sum = r + g + b;

		// get new color
		int32_t r_new = (sum * r_wanted) / sum_wanted;
		int32_t g_new = (sum * g_wanted) / sum_wanted;
		int32_t b_new = (sum * b_wanted) / sum_wanted;

		r_new *= mult;
		g_new *= mult;
		b_new *= mult;

		// clip it at 0xFF
		r_new = min(r_new, 0xFF);
		g_new = min(g_new, 0xFF);
		b_new = min(b_new, 0xFF);

		// print stuff
		printf("old: %2X %2X %2X\n", r, g, b);
		printf("new: %2X %2X %2X\n", r_new, g_new, b_new);

		// write back
		buffer[4 * i + 0] = r_new;
		buffer[4 * i + 1] = g_new;
		buffer[4 * i + 2] = b_new;
	}

	rewind(file1);
	fwrite(buffer, color_count, 4, file1);
	fclose(file1);
	free(buffer);
}


// script for resizing c3 entity coords to c2 scale?
void c3_ent_resize()
{
	char fpath[1000];
	printf("Path to entity item:\n");
	scanf(" %[^\n]", fpath);
	path_fix(fpath);

	FILE* file1;
	if ((file1 = fopen(fpath, "rb+")) == NULL)
	{
		printf("[ERROR] Couldn't open file.\n\n");
		return;
	}

	fseek(file1, 0, SEEK_END);
	int32_t fsize = ftell(file1);
	double scale = 1.f;
	rewind(file1);

	uint8_t* buffer = (uint8_t*)try_malloc(fsize);
	fread(buffer, fsize, 1, file1);

	uint32_t offset = 0;

	for (int32_t i = 0; i < PROPERTY::count(buffer); i++)
	{
		if ((from_u16(buffer + 0x10 + 8 * i)) == ENTITY_PROP_PATH)
			offset = OFFSET + from_u16(buffer + 0x10 + 8 * i + 2);

		if ((from_u16(buffer + 0x10 + 8 * i)) == 0x30E)
		{
			int32_t off_scale = OFFSET + from_u16(buffer + 0x10 + 8 * i + 2);

			switch (from_u32(buffer + off_scale + 4))
			{
			case 0:
				scale = 0.25;
				break;
			case 1:
				scale = 0.5;
				break;
			case 3:
				scale = 2;
				break;
			case 4:
				scale = 4; //?
				break;
			default:
				scale = 0.25;
				break;
			}

			printf("scale:    %lf\n", scale);
		}
	}

	if (offset)
	{
		int32_t path_len = from_u32(buffer + offset);
		printf("path len: %d\n", path_len);
		for (int32_t i = 0; i < path_len * 3; i++)
		{
			int32_t off_curr = offset + 4 + i * 2;
			*(int16_t*)(buffer + off_curr) = (*(int16_t*)(buffer + off_curr)) * scale;
		}
	}
	else
	{
		printf("[error] no path property found\n");
	}

	rewind(file1);
	fwrite(buffer, 1, fsize, file1);
	free(buffer);
	fclose(file1);
	printf("Done.\n\n");
}


void print_model_refs_util(ENTRY& model)
{
	std::vector<std::string> strs;
	char buffer[128]; // big enough for the formatted string

	int32_t item1off = model.get_nth_item_offset(0);
	int32_t item4off = model.get_nth_item_offset(3);
	int32_t item5off = model.get_nth_item_offset(4);
	int32_t tex_ref_item_size = item5off - item4off;

	printf("\nTexture references: \n");
	printf("TEXTURE CLUT B S-X  Y  W  H\n");

	for (int32_t i = 0; i < tex_ref_item_size; i += 12)
	{
		uint8_t* curr_off = model._data() + item4off + i;
		int32_t seg = from_u8(curr_off + 6) & 0xF;
		int32_t bit = from_u8(curr_off + 6) & 0x80;

		if (bit)
			bit = 8;
		else
			bit = 4;

		int32_t clut = from_u16(curr_off + 2);
		int32_t tex = from_u8(curr_off + 7);

		int32_t startx = min(min(
			from_u8(curr_off + 0),
			from_u8(curr_off + 4)),
			from_u8(curr_off + 8));
		int32_t starty = min(min(
			from_u8(curr_off + 1),
			from_u8(curr_off + 5)),
			from_u8(curr_off + 9));

		int32_t endx = max(max(
			from_u8(curr_off + 0),
			from_u8(curr_off + 4)),
			from_u8(curr_off + 8));
		int32_t endy = max(max(
			from_u8(curr_off + 1),
			from_u8(curr_off + 5)),
			from_u8(curr_off + 9));

		int32_t width = endx - startx + 1;
		int32_t height = endy - starty + 1;

		std::snprintf(buffer, sizeof(buffer),
			" %5s: %04X %d %d-%02X %02X %02X %02X",
			eid2str(from_u32(model._data() + item1off + 0xC + tex)),
			clut, bit, seg, startx, starty, width, height);

		std::string curr_str(buffer);

		if (std::find(strs.begin(), strs.end(), curr_str) == strs.end())
		{
			strs.push_back(std::move(curr_str));
		}
	}

	for (const auto& s : strs)
	{
		printf("%s\n", s.c_str());
	}
}


// prints texture references of a model or all models from a nsf
void print_model_tex_refs_nsf()
{
	ELIST elist{};

	if (elist.read_and_parse_nsf(NULL, 1, NULL))
		return;

	bool printed_something = false;
	printf("Model name? (type \"all\" to print all models' tex refs)\n");
	char ename[6] = "";
	scanf("%5s", ename);

	for (auto& ntry : elist)
	{
		if (ntry.get_entry_type() != EntryType::Model)
			continue;

		if (strcmp(eid2str(ntry.m_eid), ename) == 0 || strcmp(ename, "all") == 0)
		{
			printf("\nModel %s:", ntry.m_ename);
			print_model_refs_util(ntry);
			printed_something = true;
		}
	}

	if (!printed_something)
		printf("No such model was found\n");
	printf("Done.\n\n");
}

// prints texture references of a model from a file
void print_model_tex_refs()
{
	char fpath[1000];
	printf("Path to model entry:\n");
	scanf(" %[^\n]", fpath);
	path_fix(fpath);

	FILE* file1;
	if ((file1 = fopen(fpath, "rb+")) == NULL)
	{
		printf("[ERROR] Couldn't open file.\n\n");
		return;
	}

	fseek(file1, 0, SEEK_END);
	int32_t fsize = ftell(file1);
	if (fsize <= 0)
	{
		printf("[ERROR] File is empty or corrupted.\n\n");
		fclose(file1);
		return;
	}

	rewind(file1);
	ENTRY model{};
	model.m_esize = fsize;
	model.m_data.resize(fsize);
	fread(model._data(), fsize, 1, file1);
	fclose(file1);

	print_model_refs_util(model);
	printf("Done\n\n");
}

// prints all entries?
void print_all_entries_perma()
{
	ELIST elist{};

	if (elist.read_and_parse_nsf(NULL, 1, NULL))
		return;

	for (auto& ntry : elist)
	{
		if (ntry.get_entry_type() != EntryType::Inst)
			printf("%5s\n", ntry.m_ename);
	}

	printf("Done.\n\n");
}

// collects entity usage info from a single nsf
void entity_usage_single_nsf(const char* fpath, DEPENDENCIES* deps, uint32_t* gool_table)
{
	printf("Checking %s\n", fpath);

	ELIST elist{};
	if (elist.read_and_parse_nsf(NULL, 1, fpath))
		return;

	for (int32_t i = 0; i < C3_GOOL_TABLE_SIZE; i++)
	{
		if (gool_table[i] == EID_NONE)
			gool_table[i] = elist.m_gool_table[i];
	}

	int32_t total_entity_count = 0;

	for (auto& ntry : elist)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		int32_t entity_count = ntry.get_entity_count();
		for (int32_t j = 0; j < entity_count; j++)
		{
			uint8_t* entity = ntry.get_nth_entity(j);
			int32_t type = PROPERTY::get_value(entity, ENTITY_PROP_TYPE);
			int32_t subt = PROPERTY::get_value(entity, ENTITY_PROP_SUBTYPE);
			int32_t id = PROPERTY::get_value(entity, ENTITY_PROP_ID);

			if (id == -1)
				continue;
			total_entity_count++;

			bool found_before = false;
			for (auto& d : *deps)
			{
				if (d.type == type && d.subtype == subt)
				{
					d.dependencies.add(total_entity_count);
					found_before = true;
				}
			}

			if (!found_before)
			{
				DEPENDENCY new_dep(type, subt);
				new_dep.dependencies.add(total_entity_count);
				deps->push_back(new_dep);
			}
		}
	}
}

// folder iterator for getting entity usage
void entity_usage_folder_util(const char* dpath, DEPENDENCIES* deps, uint32_t* gool_table)
{
	try
	{
		std::filesystem::path basePath(dpath);

		if (!std::filesystem::exists(basePath) || !std::filesystem::is_directory(basePath))
		{
			printf("[ERROR] Could not open selected directory\n");
			return;
		}

		for (const auto& entry : std::filesystem::directory_iterator(basePath))
		{
			const std::filesystem::path& path = entry.path();
			const std::string fname = path.filename().string();

			// Skip "." and ".."
			if (fname == "." || fname == "..")
				continue;

			if (entry.is_regular_file())
			{
				if (path.extension() == ".NSF")
				{
					std::string fpath = path.string();
					entity_usage_single_nsf(fpath.c_str(), deps, gool_table);
				}
			}
			else if (entry.is_directory())
			{
				std::string subdir = path.string();
				entity_usage_folder_util(subdir.c_str(), deps, gool_table);
			}
		}
	}
	catch (const std::exception& e)
	{
		printf("[warning] %s\n%s\n", e.what(), dpath);
	}
}

// script for getting entity usage from folder
void entity_usage_folder()
{
	printf("Input the path to the folder\n");
	char dpath[MAX] = "";
	DEPENDENCIES deps{};

	uint32_t gool_table[C3_GOOL_TABLE_SIZE];
	for (int32_t i = 0; i < C3_GOOL_TABLE_SIZE; i++)
		gool_table[i] = EID_NONE;

	scanf(" %[^\n]", dpath);
	path_fix(dpath);

	entity_usage_folder_util(dpath, &deps, gool_table);
	std::sort(deps.begin(), deps.end(), [](DEPENDENCY x, DEPENDENCY y)
		{
			return y.dependencies.count() < x.dependencies.count();
		});


	int32_t entity_sum = 0;
	for (auto& d : deps)
		entity_sum += d.dependencies.count();

	printf("\nTotal entity count:\n");
	printf(" %5d entities\n", entity_sum);

	printf("\nEntity type/subtype usage:\n");
	for (auto& d : deps)
		printf(" %2d(%5s)-%2d: %4d entities\n", d.type, eid2str(gool_table[d.type]), d.subtype, d.dependencies.count());

	printf("\nDone.\n\n");
}

// script for printing a property from all cameras in a level
void nsf_props_scr()
{
	ELIST elist{};

	if (elist.read_and_parse_nsf(NULL, 1, NULL))
		return;

	bool printed_something = false;

	int32_t prop_code;
	printf("\nProp code? (hex)\n");
	scanf("%x", &prop_code);

	printf("\n");
	for (auto& ntry : elist)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		int32_t cam_count = ntry.get_cam_item_count();
		for (int32_t j = 0; j < cam_count; j++)
		{
			uint8_t* item = ntry.get_nth_item(2 + j);
			int32_t prop_count = PROPERTY::count(item);

			for (int32_t l = 0; l < PROPERTY::count(item); l++)
			{
				int32_t code = from_u16(item + 0x10 + 8 * l);
				int32_t offset = from_u16(item + 0x12 + 8 * l) + OFFSET;
				int32_t offset_next = from_u16(item + 0x1A + 8 * l) + OFFSET;
				if (l == (prop_count - 1))
					offset_next = from_u16(item);

				if (code == prop_code)
				{
					printed_something = true;
					printf("Zone %5s path %d item %d", ntry.m_ename, j / 3, j % 3);
					print_prop_header(item, 0x10 + 8 * l);
					print_prop_body(item, offset, offset_next);
				}
			}
		}
	}

	if (!printed_something)
		printf("No such prop was found in any camera item\n");
	printf("Done.\n\n");
}

// makes an empty slst entry for a camera with specified length
void generate_slst()
{
	int32_t cam_len = 0;
	char name[100] = "ABCDE";
	static uint8_t empty_source[] = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	static uint8_t empty_delta[] = { 0x2, 0x0, 0x1, 0x0, 0x2, 0x0, 0x2, 0x0 };
	uint32_t real_len = 0x10;
	uint8_t buffer[10000]{};
	char fpath[500] = "";

	printf("Camera path length?\n");
	scanf("%d", &cam_len);
	if (cam_len <= 1)
	{
		printf("[ERROR] Invalid slst length %d\n\n", cam_len);
		return;
	}

	printf("\nSLST name?\n");
	scanf("%s", name);
	sprintf(fpath, "empty_slst_%5s_len_%d.nsentry", name, cam_len);

	FILE* file = NULL;
	if ((file = fopen(fpath, "wb")) == NULL)
	{
		printf("[ERROR] File %s could not be opened\n\n", fpath);
		return;
	}

	*(uint32_t*)(buffer) = MAGIC_ENTRY;
	*(uint32_t*)(buffer + 0x4) = eid_to_int(name);
	*(uint32_t*)(buffer + 0x8) = int32_t(EntryType::SLST);
	*(uint32_t*)(buffer + 0xC) = cam_len + 1;

	for (int32_t i = 0; i < cam_len + 2; i++)
	{
		*(uint32_t*)(buffer + 0x10 + 4 * i) = (4 * (cam_len + 2)) + 0x10 + (8 * i);
		real_len += 4;
	}

	for (int32_t i = 0; i < 8; i++)
		*(uint8_t*)(buffer + real_len + i) = empty_source[i];
	real_len += 8;

	for (int32_t j = 0; j < (cam_len - 1); j++)
	{
		for (int32_t i = 0; i < 8; i++)
			*(uint8_t*)(buffer + real_len + i) = empty_delta[i];
		real_len += 8;
	}

	for (int32_t i = 0; i < 8; i++)
		*(uint8_t*)(buffer + real_len + i) = empty_source[i];
	real_len += 8;

	fwrite(buffer, real_len, 1, file);
	fclose(file);
	printf("Done. Saved as %s\n\n", fpath);
}

#define WARP_SPAWN_COUNT 32
// modpack warp spawns generation util (for nsd)
void warp_spawns_generate()
{
	ELIST elist{};

	if (elist.read_and_parse_nsf(NULL, 1, NULL))
		return;

	/* older, hardcoded spawns
	int32_t spawn_ids[] = {
		27, 34, 40, 27, df, 40, 36,
		32, 41, 32, 32, 35,
		40, 31, 37, 33, 39,
		37, 30, 29, 28, 29,
		27, 37, 38, 32, 131,
		30, df, df, 37, df,
		df};
	*/

	char path[MAX];
	printf("\nInput the path to the warp spawns file:\n");
	scanf(" %[^\n]", path);
	path_fix(path);

	FILE* file = fopen(path, "r");
	if (!file)
	{
		printf("File could not be opened!\n");
		return;
	}

	char buffer[CHUNKSIZE];
	int32_t count = 0;
	int32_t spawn_ids[WARP_SPAWN_COUNT] = { 0 };

	size_t len = fread(buffer, 1, sizeof(buffer) - 1, file);
	fclose(file);
	buffer[len] = '\0';

	char* token = strtok(buffer, ",\n\r");
	while (token && count < WARP_SPAWN_COUNT)
	{
		spawn_ids[count++] = atoi(token);
		token = strtok(NULL, ",\n\r");
	}

	for (int32_t i = 0; i < WARP_SPAWN_COUNT; i++)
	{
		if (spawn_ids[i] == 0)
			spawn_ids[i] = 26; // default to ID 26
	}

	for (int32_t i = 0; i < WARP_SPAWN_COUNT; i++)
	{
		for (int32_t j = 0; j < elist.count(); j++)
		{
			if (elist[j].get_entry_type() != EntryType::Zone)
				continue;

			uint8_t* curr_zone = elist[j]._data();
			int32_t camc = elist[j].get_cam_item_count();

			for (int32_t k = 0; k < elist[j].get_entity_count(); k++)
			{
				int32_t offset = elist[j].get_nth_item_offset(2 + camc + k);
				int32_t ent_id = PROPERTY::get_value(curr_zone + offset, ENTITY_PROP_ID);
				if (ent_id == spawn_ids[i])
				{
					ENTITY_PATH path = elist[j].get_ent_path(2 + camc + k);

					uint8_t* coll_item = elist[j].get_nth_item(1);
					uint32_t zone_x = from_u32(coll_item);
					uint32_t zone_y = from_u32(coll_item + 4);
					uint32_t zone_z = from_u32(coll_item + 8);

					uint32_t x = (zone_x + 4 * path[0]) << 8;
					uint32_t y = (zone_y + 4 * path[1]) << 8;
					uint32_t z = (zone_z + 4 * path[2]) << 8;

					uint8_t arr[0x18] = { 0 };
					*(uint32_t*)arr = elist[j].m_eid;
					*(uint32_t*)(arr + 0x4) = 0;
					*(uint32_t*)(arr + 0x8) = 0;
					*(uint32_t*)(arr + 0xC) = x;
					*(uint32_t*)(arr + 0x10) = y;
					*(uint32_t*)(arr + 0x14) = z;

					printf("\n");
					for (int32_t j = 0; j < 0x18; j++)
					{
						printf("%02X", arr[j]);
						if (j % 4 == 3)
							printf(" ");
					}
				}
			}
		}
	}

	printf("\nDone.\n\n");
}

// util for printing special load list info for single nsf
void special_load_lists_util(const char* fpath)
{
	printf("Checking %s\n", fpath);
	bool printed_something = false;

	ELIST elist{};

	if (elist.read_and_parse_nsf(NULL, 1, fpath))
		return;

	for (auto& curr : elist)
	{
		if (curr.get_entry_type() != EntryType::Zone)
			continue;

		LIST special_entries = curr.get_special_entries_raw();
		if (special_entries.count())
		{
			printed_something = true;
			printf("Zone %5s:\t", curr.m_ename);
			for (int32_t j = 0; j < special_entries.count(); j++)
			{
				printf("%5s ", eid2str(special_entries[j]));
				if (j && j % 8 == 7 && (j + 1) != special_entries.count())
					printf("\n           \t");
			}

			printf("\n");
		}
	}

	if (printed_something)
		printf("\n");
}

bool check_fpath_contains(const char* fpath, std::string name)
{
	if (strstr(fpath, name.c_str()))
		return true;

	return false;
}

// for printing gool type and category list from nsf
void nsd_util_util(const char* fpath)
{
	ELIST elist{};

	const char* filename = strrchr(fpath, '\\');
	if (filename != NULL)
	{
		filename++;
	}
	else
	{
		filename = fpath;
	}

	if (elist.read_and_parse_nsf(NULL, 1, fpath))
		return;

	for (auto& ntry : elist)
	{
		if (ntry.get_entry_type() != EntryType::GOOL)
			continue;

		int32_t off0 = ntry.get_nth_item_offset(0);

		int32_t tpe = from_u32(ntry._data() + off0);
		int32_t cat = from_u32(ntry._data() + off0 + 4) >> 8;

		printf("%s %5s, type %d category %d\n", filename, ntry.m_ename, tpe, cat);
	}
}

// util for printing fov info for a single level
void fov_stats_util(const char* fpath)
{
	printf("Level: %s\n", fpath);
	ELIST elist{};

	if (elist.read_and_parse_nsf(NULL, 1, fpath))
		return;

	for (auto& ntry : elist)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		int32_t c_count = ntry.get_cam_item_count() / 3;
		for (int32_t j = 0; j < c_count; j++)
		{
			uint8_t* cam = ntry.get_nth_main_cam(j);
			int32_t fov = PROPERTY::get_value(cam, ENTITY_PROP_CAM_FOV);
			printf("Zone %5s : fov %d\n", ntry.m_ename, fov);
		}
	}

	printf("\n");
}

// util function for printing checkpoint, mask and dda info
void checkpoint_stats_util(const char* fpath)
{
	ELIST elist{};

	if (elist.read_and_parse_nsf(NULL, 1, fpath))
		return;

	int32_t cam_count = 0;
	int32_t checks_non_dda = 0;
	int32_t checks_with_dda = 0;
	int32_t masks_non_dda = 0;
	int32_t masks_with_dda = 0;

	for (auto& ntry : elist)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		int32_t c_count = ntry.get_cam_item_count();
		int32_t e_count = ntry.get_entity_count();
		cam_count += (c_count / 3);

		for (int32_t j = 0; j < e_count; j++)
		{
			uint8_t* entity = ntry.get_nth_entity(j);
			int32_t type = PROPERTY::get_value(entity, ENTITY_PROP_TYPE);
			int32_t subt = PROPERTY::get_value(entity, ENTITY_PROP_SUBTYPE);
			int32_t dda_deaths = PROPERTY::get_value(entity, ENTITY_PROP_DDA_DEATHS) / 256;

			if (type == 34 && subt == 4)
			{
				if (dda_deaths)
					checks_with_dda++;
				else
					checks_non_dda++;
			}

			if (type == 34 && subt == 27)
				checks_non_dda++;

			if (type == 34 && subt == 9)
			{
				if (dda_deaths)
					masks_with_dda++;
				else
					masks_non_dda++;
			}

			if (type == 3 && subt == 6)
				masks_non_dda++;

			// snow no alt iron checks
			if (check_fpath_contains(fpath, "S0000015.NSF"))
			{
				if (type == 40 && subt == 27)
					checks_non_dda++;
			}

			// lava checks
			if (check_fpath_contains(fpath, "S000000E.NSF"))
			{
				if (type == 33 && (subt == 0 || subt == 2))
					checks_non_dda++;
			}

			// jet rex alt masks
			if (check_fpath_contains(fpath, "S0000019.NSF"))
			{
				if (type == 45 && subt == 7)
					masks_non_dda++;
				if (type == 36 && subt == 0)
				{
					if (dda_deaths)
						masks_with_dda++;
					else
						masks_non_dda++;
				}
			}

			// dream zone alt iron checks
			if (check_fpath_contains(fpath, "S000001B.NSF"))
			{
				if (type == 60 && subt == 0)
					checks_with_dda++;
			}
		}
	}

	// arab/lava alt masks (done thru args so its just added artificially)
	if (check_fpath_contains(fpath, "S0000013.NSF") || check_fpath_contains(fpath, "S000000E.NSF"))
		masks_non_dda++;

	if (checks_non_dda || checks_with_dda)
	{
		printf("\"%s\",", fpath);

		printf("%3d,", cam_count);
		printf("%3d,", checks_non_dda);
		printf("%3d,", checks_with_dda);

		printf("%3d,", masks_non_dda);
		printf("%3d ", masks_with_dda);

		printf("\n");
	}
}

// function for recursively iterating over folders and calling callback

void recursive_folder_iter(std::filesystem::path dpath, void(callback)(const char*))
{
	try
	{
		if (!std::filesystem::exists(dpath) || !std::filesystem::is_directory(dpath))
		{
			printf("[ERROR] Could not open selected directory\n");
			return;
		}

		for (const auto& entry : std::filesystem::directory_iterator(dpath))
		{
			const std::filesystem::path& path = entry.path();
			const std::string fname = path.filename().string();

			// Skip "." and ".."
			if (fname == "." || fname == "..")
				continue;

			if (entry.is_regular_file())
			{
				if (path.extension() == ".NSF")
				{
					callback(path.generic_string().c_str());
				}
			}
			else if (entry.is_directory())
			{
				recursive_folder_iter(path, callback);
			}
		}
	}
	catch (const std::exception& e)
	{
		printf("[warning] %s\n%s\n", e.what(), dpath.generic_string().c_str());
	}
}

// command for listing modpack/rebuild special load lists from folder
void special_load_lists_list()
{
	printf("Input the path to the folder\n");
	char dpath[MAX] = "";

	scanf(" %[^\n]", dpath);
	path_fix(dpath);
	recursive_folder_iter(dpath, special_load_lists_util);
	printf("\nDone.\n\n");
}

// command for listing checkpoint and mask info from folder
void checkpoint_stats()
{
	printf("Input the path to the folder\n");
	char dpath[MAX] = "";

	scanf(" %[^\n]", dpath);
	path_fix(dpath);

	printf("fpath,cam_count,checks_non_dda,checks_with_dda,masks_non_dda,masks_with_dda\n");
	recursive_folder_iter(dpath, checkpoint_stats_util);
	printf("\nDone.\n\n");
}

// command for listing fov values from folder
void fov_stats()
{
	printf("Input the path to the folder\n");
	char dpath[MAX] = "";

	scanf(" %[^\n]", dpath);
	path_fix(dpath);

	recursive_folder_iter(dpath, fov_stats_util);
	printf("\nDone.\n\n");
}

// command for listing gool types from folder
void nsd_util()
{
	printf("Input the path to the folder\n");
	char dpath[MAX] = "";

	scanf(" %[^\n]", dpath);
	path_fix(dpath);

	recursive_folder_iter(dpath, nsd_util_util);
	printf("\nDone.\n\n");
}

// command for listing draw info from nsf
void draw_util()
{
	ELIST elist{};

	if (elist.read_and_parse_nsf(NULL, 1, NULL))
		return;

	int32_t total = 0;
	int32_t point_count = 0;

	for (auto& ntry : elist)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		int32_t cam_count = ntry.get_cam_item_count() / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			std::vector<LIST> draw_list = ntry.get_expanded_draw_list(2 + 3 * j);
			int32_t path_len = int32_t(draw_list.size());

			for (int32_t k = 0; k < path_len; k++)
			{
				int32_t drawn = draw_list[k].count();
				printf("%s-%d, point %2d/%2d - drawing %d entities\n", ntry.m_ename, j, k, path_len, drawn);

				total += drawn;
				point_count += 1;
			}
		}
	}

	printf("\n");

	// copypaste, cba to optimise
	for (auto& ntry : elist)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		int32_t cam_count = ntry.get_cam_item_count() / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			std::vector<LIST> draw_list = ntry.get_expanded_draw_list(2 + 3 * j);

			int32_t zone_total = 0;
			int32_t zone_points = 0;

			for (auto& sublist : draw_list)
			{
				int32_t drawn = sublist.count();
				zone_total += drawn;
				zone_points += 1;
			}

			printf("%s-%d average entities drawn: %d\n", ntry.m_ename, j, zone_total / zone_points);
		}
	}

	printf("\nAverage entities drawn in level: %d\n", total / point_count);
	printf("\nDone.\n\n");
}

// util for printing file texture pages
void tpage_util_util(const char* fpath)
{
	ELIST elist{};
	const char* filename = strrchr(fpath, '\\');
	if (filename != NULL)
	{
		filename++;
	}
	else
	{
		filename = fpath;
	}

	CHUNKS chunks(CHUNK_LIST_DEFAULT_SIZE);
	if (elist.read_and_parse_nsf(&chunks, 1, fpath))
		return;

	printf("File %s:\n", fpath);
	for (int32_t i = 0; i < elist.m_chunk_count; i++)
	{
		if (chunks[i].get() == NULL)
			continue;
		if (chunk_type(chunks[i].get()) != ChunkType::TEXTURE)
			continue;

		int32_t checksum = from_u32(chunks[i].get() + CHUNK_CHECKSUM_OFFSET);
		printf("\t tpage %s \t%08X\n", eid2str(from_u32(chunks[i].get() + 0x4)), checksum);
	}
	printf("\n");
}

// prints tpage list for files from a folder
void tpage_util()
{
	printf("Input the path to the folder\n");
	char dpath[MAX] = "";

	scanf(" %[^\n]", dpath);
	path_fix(dpath);

	recursive_folder_iter(dpath, tpage_util_util);
	printf("\nDone.\n\n");
}

// for gool_util command, prints gool entries of a file plus their checksum
void gool_util_util(const char* fpath)
{
	ELIST elist{};
	if (elist.read_and_parse_nsf(NULL, 1, fpath))
		return;

	printf("File %s:\n", fpath);
	for (auto& ntry : elist)
	{
		if (ntry.get_entry_type() != EntryType::GOOL)
			continue;

		int32_t g_tpe = from_u32(ntry.get_nth_item(0));
		printf("\t gool %5s-%02d \t%08X\n", ntry.m_ename, g_tpe, nsfChecksum(ntry._data(), ntry.m_esize));
	}

	printf("\n");
}

// command for printing gool entries list from folder
void gool_util()
{
	printf("Input the path to the folder\n");
	char dpath[MAX] = "";

	scanf(" %[^\n]", dpath);
	path_fix(dpath);

	recursive_folder_iter(dpath, gool_util_util);
	printf("\nDone.\n\n");
}

void eid_cmd()
{
	char buf[MAX] = "";
	scanf(" %[^\n]", buf);
	uint32_t eid = eid_to_int(buf);
	printf("%02X %02X %02X %02X\n\n", eid & 0xFF, (eid >> 8) & 0xFF, (eid >> 16) & 0xFF, (eid >> 24) & 0xFF);
}

void cmd_payload_info()
{
	ELIST elist{};

	if (elist.read_and_parse_nsf(NULL, 1, NULL))
		return;

	level_analyze::ll_print_full_payload_info(elist, 1);
	printf("Done.\n\n");
}
