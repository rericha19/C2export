#include "../include.h"
#include "../utils/elist.hpp"
#include "../utils/entry.hpp"

void ELIST::ask_build_flags()
{
	printf("\nRemake load lists? [0/1]\n");
	int32_t ans;
	scanf("%d", &ans);
	if (ans)
	{
		m_config[Remake_Load_Lists] = 1;
		printf("Will remake load lists\n\n");
	}
	else
	{
		m_config[Remake_Load_Lists] = 0;
		printf("Won't remake load lists\n\n");
	}

	printf("What merge method do you want to use?\n");
	printf("[4] - occurence count matrix merge (absolute) with randomness\n");
	printf("[5] - occurence count matrix merge (absolute) with randomness multithreaded\n");

	scanf("%d", &ans);
	m_config[Chunk_Merge_Method] = ans;
	printf("Selected merge method %d\n", ans);

	if (ans < 4 || ans > 5)
	{
		printf(" unknown, defaulting to 4\n");
		m_config[Chunk_Merge_Method] = 4;
	}
	printf("\n");
}

void ELIST::ask_build_paths(char* perma_path, char* subt_deps_path, char* coll_deps_path, char* mus_deps_path)
{
	printf("\nInput path to file with permaloaded entries:\n");
	scanf(" %[^\n]", perma_path);
	path_fix(perma_path);
	printf("Using %s for permaloaded entries\n", perma_path);

	if (m_config[Remake_Load_Lists])
	{
		printf("\nInput path to file with type/subtype dependencies:\n");
		scanf(" %[^\n]", subt_deps_path);
		path_fix(subt_deps_path);
		printf("Using %s for type/subtype dependencies\n", subt_deps_path);

		printf("\nInput path to file with collision dependencies [assumes file is not necessary if path is invalid]:\n");
		scanf(" %[^\n]", coll_deps_path);
		path_fix(coll_deps_path);
		printf("Using %s for collision dependencies\n", coll_deps_path);

		printf("\nInput path to file with music entry dependencies [assumes file is not necessary if path is invalid]:\n");
		scanf(" %[^\n]", mus_deps_path);
		path_fix(mus_deps_path);
		printf("Using %s for music entry dependencies\n\n", mus_deps_path);
	}
}

// Reads the info from file the user has to provide, first part has permaloaded entries,
// second has a list of type/subtype dependencies
bool ELIST::read_entry_config()
{
	std::string line, token;

	auto read_rest_of_line = [&](std::istringstream& ss, DEPENDENCY& dep)
		{
			while (std::getline(ss, token, ','))
			{
				// Trim leading space
				if (!token.empty() && token[0] == ' ')
					token.erase(0, 1);

				int32_t index = get_index(eid_to_int(token));
				if (index == -1)
				{
					printf("[warning] unknown entry reference in object dependency list, will be skipped:\t %s\n", line.c_str());
					continue;
				}

				dep.dependencies.add(eid_to_int(token));
				if (at(index).get_entry_type() == EntryType::Anim)
					dep.dependencies.copy_in(at(index).get_anim_refs(*this));
			}
		};

	char perma_path[MAX] = "";
	char subt_deps_path[MAX] = "";
	char coll_deps_path[MAX] = "";
	char mus_deps_path[MAX] = "";
	ask_build_paths(perma_path, subt_deps_path, coll_deps_path, mus_deps_path);

	std::ifstream permafile(perma_path);
	if (!permafile.is_open())
	{
		printf("[ERROR] File with permaloaded entries could not be opened\n");
		return false;
	}

	while (std::getline(permafile, line))
	{
		if (line[0] == '#' || line.empty()) continue;

		int32_t index = get_index(eid_to_int(line));
		if (index == -1)
		{
			printf("[ERROR] invalid permaloaded entry, won't proceed :\t%s\n", line.c_str());
			return false;
		}

		m_permaloaded.add(eid_to_int(line));
		if (at(index).get_entry_type() == EntryType::Anim)
			m_permaloaded.copy_in(at(index).get_anim_refs(*this));
	}

	if (!m_config[Remake_Load_Lists])
		return true;

	std::ifstream subtfile(subt_deps_path);
	if (!subtfile.is_open())
	{
		printf("[ERROR] File with type/subtype dependencies could not be opened\n");
		return false;
	}

	while (std::getline(subtfile, line))
	{
		if (line.empty() || line[0] == '#')
			continue;

		std::istringstream ss(line);
		if (!std::getline(ss, token, ',')) continue;
		int32_t type = std::stoi(token);

		if (!std::getline(ss, token, ',')) continue;
		int32_t subtype = std::stoi(token);

		DEPENDENCY n_subt_dep{ type, subtype };
		n_subt_dep.dependencies.add(m_gool_table[type]);
		read_rest_of_line(ss, n_subt_dep);
		m_subt_deps.push_back(n_subt_dep);
	}

	std::ifstream collfile(coll_deps_path);
	if (!collfile.is_open())
	{
		printf("\nFile with collision dependencies could not be opened\n");
		printf("Assuming file is not necessary\n");
	}
	else
	{
		while (std::getline(collfile, line))
		{
			if (line.empty() || line[0] == '#') continue;

			std::istringstream ss(line);
			if (!std::getline(ss, token, ',')) continue;
			int32_t coll_type = std::stoi(token, nullptr, 16);

			DEPENDENCY n_coll_dep(coll_type);
			read_rest_of_line(ss, n_coll_dep);
			m_coll_deps.push_back(n_coll_dep);
		}
	}

	std::ifstream musfile(mus_deps_path);
	if (!musfile.is_open())
	{
		printf("\nFile with music entry dependencies could not be opened\n");
		printf("Assuming file is not necessary\n");
	}
	else
	{
		while (std::getline(musfile, line))
		{
			if (line.empty() || line[0] == '#') continue;

			std::istringstream ss(line);
			if (!std::getline(ss, token, ',')) continue;

			DEPENDENCY n_mus_dep(eid_to_int(token));
			read_rest_of_line(ss, n_mus_dep);
			m_musi_deps.push_back(n_mus_dep);
		}
	}

	return true;
}

void ELIST::ask_second_output()
{
	char user_in[80] = "";
	char path[100] = "";
	char path2[100] = "";

	printf("\nInput path to secondary save folder (use exactly \"-\" (minus sign) to not use sec. save)\n");
	scanf(" %[^\n]", user_in);
	path_fix(user_in);

	if (!strcmp(user_in, "-"))
	{
		printf("Not using secondary output\n");
		return;
	}

	sprintf(path, "%s\\S00000%02X.NSF", user_in, m_level_ID);
	sprintf(path2, "%s\\S00000%02X.NSD", user_in, m_level_ID);

	printf("Secondary file paths:\n%s/.NSD\n", path);

	m_nsf_out2.open(path, "wb");
	m_nsd_out2.open(path2, "wb");

	if (!m_nsf_out2.ok() || !m_nsd_out2.ok())
	{
		printf("[warning] Could not open secondary NSF or NSD\n");
		if (m_nsf_out2.f)
		{
			fclose(m_nsf_out2.f);
			m_nsf_out2.f = NULL;
		}
		if (m_nsd_out2.f)
		{
			fclose(m_nsd_out2.f);
			m_nsd_out2.f = NULL;
		}
	}
	else
	{
		printf("Successfully using secondary output\n");
	}
}

void ELIST::ask_draw_distances()
{
	int32_t input;

	printf("\nDraw distance 2D horizontal (x-dist) (set 0 to make infinite)\n");
	scanf("%d", &input);
	m_config[DL_Distance_Cap_X] = input;
	printf("Selected %d for horizontal draw distance\n", input);

	printf("\nDraw distance 2D vertical (y-dist) (set 0 to make infinite)\n");
	scanf("%d", &input);
	m_config[DL_Distance_Cap_Y] = input;
	printf("Selected %d for vertical draw distance\n", input);

	printf("\nDraw distance 3D sections (xz-dist) (set 0 to make infinite)\n");
	scanf("%d", &input);
	m_config[DL_Distance_Cap_XZ] = input;
	printf("Selected %d for 3D sections draw distance\n", input);

	printf("\nMax allowed angle distance for 3D sections (default to 90)\n");
	scanf("%d", &input);
	m_config[DL_Distance_Cap_Angle3D] = input;
	printf("Selected %d for max allowed angle distance for 3D sections\n", input);
}

// Asks the user what distances to use for load list building.
void ELIST::ask_load_distances()
{
	int32_t input;

	printf("\nSLST distance?      (recommended is approx 7250)\n");
	scanf("%d", &input);
	m_config[LL_SLST_Distance] = input;
	printf("Selected %d for SLST distance\n", input);

	printf("\nNeighbour distance? (recommended is approx 7250)\n");
	scanf("%d", &input);
	m_config[LL_Neighbour_Distance] = input;
	printf("Selected %d for neighbour distance\n", input);

	printf("\nDraw list distance? (recommended is approx 7250)\n");
	scanf("%d", &input);
	m_config[LL_Drawlist_Distance] = input;
	printf("Selected %d for draw list distance\n", input);

	printf("\nTransition pre-loading? [0 - none / 1 - textures / 2 - normal chunk entries only / 3 - all]\n");
	scanf("%d", &input);
	if (input >= 0 && input <= 3)
		m_config[LL_Transition_Preloading_Type] = input;
	else
		m_config[LL_Transition_Preloading_Type] = 0;

	switch (m_config[LL_Transition_Preloading_Type])
	{
	default:
	case PRELOADING_NOTHING:
		printf("Not pre-loading.\n\n");
		break;
	case PRELOADING_TEXTURES_ONLY:
		printf("Pre-loading only textures.\n\n");
		break;
	case PRELOADING_REG_ENTRIES_ONLY:
		printf("Pre-loading normal chunk entries, not textures. (entity deps omitted too)\n\n");
		break;
	case PRELOADING_ALL:
		printf("Pre-loading all.\n\n");
		break;
	}

	printf("Backwards loading penalty? [0 - 0.5]\n");
	float backw;
	scanf("%f", &backw);
	if (backw < 0.0 || backw > 0.5)
	{
		printf("Invalid, defaulting to 0\n");
		backw = 0;
	}
	m_config[LL_Backwards_Loading_Penalty_DBL] = double_to_config_int(backw);
	printf("Selected %.2f for backwards loading penalty\n", backw);
}

// asking user parameters for the matrix merges
void ELIST::ask_params_matrix()
{
	int32_t input_int = 0;
	double input_double = 0;

	printf("\nMax payload limit? (usually 21 or 20)\n");
	scanf("%d", &input_int);
	m_config[Rebuild_Payload_Limit] = input_int;
	printf("Max payload limit used: %d\n", m_config[Rebuild_Payload_Limit]);
	printf("\n");

	printf("Max iteration count? (0 for no limit)\n");
	scanf("%d", &input_int);
	if (input_int < 0)
	{
		printf("Negative iteration count not allowed, defaulting to 100");
		input_int = 100;
	}
	m_config[Rebuild_Iteration_Limit] = input_int;
	printf("Max iteration count used: %d\n", m_config[Rebuild_Iteration_Limit]);
	printf("\n");

	printf("Randomness rand_multiplier? (> 1, use 1.5 if not sure)\n");
	scanf("%lf", &input_double);
	if (input_double < 1.0)
	{
		input_double = 1.5;
		printf("Invalid rand_multiplier, defaulting to 1.5\n");
	}
	m_config[Rebuild_Random_Mult_DBL] = double_to_config_int(input_double);
	printf("rand_multiplier used: %.2f\n", input_double);
	printf("\n");

	printf("Seed? (0 for random)\n");
	scanf("%d", &input_int);
	if (input_int == 0)
	{
		time_t raw_time;
		time(&raw_time);
		srand((uint32_t)raw_time);
		input_int = rand();
	}
	m_config[Rebuild_Base_Seed] = input_int;
	printf("Seed used: %d\n", m_config[Rebuild_Base_Seed]);
	printf("\n");

	if (m_config[Chunk_Merge_Method] == 5)
	{
		printf("Thread count? (64 max)\n");
		scanf("%d", &input_int);
		m_config[Rebuild_Thread_Count] = std::clamp(input_int, 1, 64);
		printf("Thread count: %d\n\n", m_config[Rebuild_Thread_Count]);
	}

	m_config[Rebuild_Iteration_Limit] -= m_config[Rebuild_Thread_Count];
}