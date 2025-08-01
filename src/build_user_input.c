#include "macros.h"
// contains mostly functions used to get input from user (config stuff)

/** \brief
 *  Asks the user for built level's ID.
 *
 * \return int32_t                          picked level ID
 */
int32_t build_ask_ID()
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

/** \brief
 *  Asks for path to the list with the permaloaded stuff and type/subtype info.
 *
 * \param fpath char *                   path to the file
 * \return void
 */
void build_ask_list_paths(char fpaths[BUILD_FPATH_COUNT][MAX], int32_t *config)
{
    int32_t remaking_load_lists_flag = config[CNFG_IDX_LL_REMAKE_FLAG];

    printf("\nInput path to file with permaloaded entries:\n");
    scanf(" %[^\n]", fpaths[0]);
    path_fix(fpaths[0]);
    printf("Using %s for permaloaded entries\n", fpaths[0]);

    // if building load lists
    if (remaking_load_lists_flag)
    {
        printf("\nInput path to file with type/subtype dependencies:\n");
        scanf(" %[^\n]", fpaths[1]);
        path_fix(fpaths[1]);
        printf("Using %s for type/subtype dependencies\n", fpaths[1]);

        printf("\nInput path to file with collision dependencies [assumes file is not necessary if path is invalid]:\n");
        scanf(" %[^\n]", fpaths[2]);
        path_fix(fpaths[2]);
        printf("Using %s for collision dependencies\n", fpaths[2]);

        printf("\nInput path to file with music entry dependencies [assumes file is not necessary if path is invalid]:\n");
        scanf(" %[^\n]", fpaths[3]);
        path_fix(fpaths[3]);
        printf("Using %s for music entry dependencies\n\n", fpaths[3]);
    }
}

/** \brief
 *  Lets the user pick the spawn that's to be used as the primary spawn.
 *
 * \param spawns SPAWNS                 struct that contains all found spawns (willy or checkpoint entities' global origins)
 * \return void
 */
void build_ask_spawn(SPAWNS spawns)
{
    int32_t input = 0;
    char temp[100] = "";

    // lets u pick a spawn point
    printf("Pick a spawn:\n");
    for (int32_t i = 0; i < spawns.spawn_count; i++)
        printf("Spawn %2d:\tZone: %s\n", i + 1, eid_conv(spawns.spawns[i].zone, temp));

    scanf("%d", &input);
    input--;
    if (input >= spawns.spawn_count || input < 0)
    {
        printf("No such spawn, defaulting to first one\n");
        input = 0;
    }

    build_swap_spawns(spawns, 0, input);
    printf("Using spawn %2d: Zone: %s\n", input + 1, eid_conv(spawns.spawns[0].zone, temp));
}

void build_ask_draw_distances(int32_t *config)
{
    int32_t temp;

    printf("\nDraw distance 2D horizontal (x-dist) (set 0 to make infinite)\n");
    scanf("%d", &temp);
    config[CNFG_IDX_DRAW_LIST_GEN_CAP_X] = temp;
    printf("Selected %d for horizontal draw distance\n", temp);

    printf("\nDraw distance 2D vertical (y-dist) (set 0 to make infinite)\n");
    scanf("%d", &temp);
    config[CNFG_IDX_DRAW_LIST_GEN_CAP_Y] = temp;
    printf("Selected %d for vertical draw distance\n", temp);

    printf("\nDraw distance 3D sections (xz-dist) (set 0 to make infinite)\n");
    scanf("%d", &temp);
    config[CNFG_IDX_DRAW_LIST_GEN_CAP_XZ] = temp;
    printf("Selected %d for 3D sections draw distance\n", temp);

    printf("\nMax allowed angle distance for 3D sections (default to 90)\n");
    scanf("%d", &temp);
    config[CNFG_IDX_DRAW_LIST_GEN_ANGLE_3D] = temp;
    printf("Selected %d for max allowed angle distance for 3D sections\n", temp);
}

/** \brief
 *  Asks the user what distances to use for load list building.
 *
 * \param config int32_t*                   config array
 * \return void
 */
void build_ask_distances(int32_t *config)
{
    int32_t temp;
    int32_t ans;
    printf("\nSLST distance?      (recommended is approx 7250)\n");
    scanf("%d", &temp);
    config[CNFG_IDX_LL_SLST_DIST_VALUE] = temp;
    printf("Selected %d for SLST distance\n", temp);

    printf("\nNeighbour distance? (recommended is approx 7250)\n");
    scanf("%d", &temp);
    config[CNFG_IDX_LL_NEIGH_DIST_VALUE] = temp;
    printf("Selected %d for neighbour distance\n", temp);

    printf("\nDraw list distance? (recommended is approx 7250)\n");
    scanf("%d", &temp);
    config[CNFG_IDX_LL_DRAW_DIST_VALUE] = temp;
    printf("Selected %d for draw list distance\n", temp);

    printf("\nTransition pre-loading? [0 - none / 1 - textures / 2 - normal chunk entries only / 3 - all]\n");
    scanf("%d", &ans);
    if (ans >= 0 && ans <= 3)
        config[CNFG_IDX_LL_TRNS_PRLD_FLAG] = ans;
    else
        config[CNFG_IDX_LL_TRNS_PRLD_FLAG] = 0;

    switch (config[CNFG_IDX_LL_TRNS_PRLD_FLAG])
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
    config[CNFG_IDX_LL_BACKWARDS_PENALTY] = (int32_t)(PENALTY_MULT_CONSTANT * backw);
    printf("Selected %.2f for backwards loading penalty\n", backw);
}

/** \brief
 *  Swaps spawns.
 *
 * \param spawns SPAWNS                 struct that contains all spanwns
 * \param spawnA int32_t                    index1
 * \param spawnB int32_t                    index2
 * \return void
 */
void build_swap_spawns(SPAWNS spawns, int32_t spawnA, int32_t spawnB)
{
    SPAWN temp = spawns.spawns[spawnA];
    spawns.spawns[spawnA] = spawns.spawns[spawnB];
    spawns.spawns[spawnB] = temp;
}

void build_ask_build_flags(int32_t *config)
{
    printf("\nRemake load lists? [0/1]\n");
    int32_t ans;
    scanf("%d", &ans);
    if (ans)
    {
        config[CNFG_IDX_LL_REMAKE_FLAG] = 1;
        printf("Will remake load lists\n\n");
    }
    else
    {
        config[CNFG_IDX_LL_REMAKE_FLAG] = 0;
        printf("Won't remake load lists\n\n");
    }

    printf("What merge method do you want to use?\n");
    printf("[0] - occurence count matrix merge (absolute)\n");
    printf("[1] - occurence count matrix merge (relative) [usually worse than option 0]\n");
    printf("[2] - relatives & payload merge [deprecate, bad]\n");
    printf("[3] - state set graph search based merge (A*/DFS) [slow, no guaranteed result]\n");
    printf("[4] - occurence count matrix merge (absolute) with randomness\n");
    int32_t max_ans = 4;
#if COMPILE_WITH_THREADS
    printf("[5] - occurence count matrix merge (absolute) with randomness multithreaded\n");
    max_ans = 5;
#endif // COMPILE_WITH_THREADS

    scanf("%d", &ans);
    config[CNFG_IDX_MERGE_METHOD_VALUE] = ans;
    printf("Selected merge method %d\n", ans);

    if (ans < 0 || ans > max_ans)
    {
        printf(" unknown, defaulting to 4\n");
        config[CNFG_IDX_MERGE_METHOD_VALUE] = 4;
    }
    printf("\n");
}

void build_ask_premerge(int32_t *premerge_type, double *merge_ratio)
{
    int32_t ans;
    printf("\nWhich premerge method do you want to use? (Can speed the process up, but premerging too much might prevent complex levels from building properly)\n");
    printf("[0] - no premerge\n");
    printf("[1] - partial occurence count matrix merge (absolute)\n");
    printf("[2] - partial occurence count matrix merge (relative)\n");
    printf("[3] - place models with their most used animation available\n");
    printf("[4] - place SLSTs with their zone\n");
    printf("[5] - 3 & 4 combined\n");
    scanf("%d", &ans);
    if (ans < 0 || ans > 5)
    {
        printf("Invalid choice, defaulting to [0]\n");
        ans = 0;
    }

    if (ans == 1 || ans == 2)
    {
        printf("\nPartial premerge ratio? [0 - no premerge, 1 - full merge]\n");
        double ans2;
        scanf("%lf", &ans2);
        *merge_ratio = ans2;
    }

    *premerge_type = ans;
    printf("\n");
}

void build_try_second_output(FILE **nsfnew2, FILE **nsd2, int32_t levelID)
{
    char temp[80] = "";
    char path[100] = "";
    char path2[100] = "";

    printf("\nInput path to secondary save folder (use exactly \"-\" (minus sign) to not use sec. save)\n");
    scanf(" %[^\n]", temp);
    path_fix(temp);

    if (!strcmp(temp, "-"))
    {
        printf("Not using secondary output\n");
    }
    else
    {
        sprintf(path, "%s\\S00000%02X.NSF", temp, levelID);
        sprintf(path2, "%s\\S00000%02X.NSD", temp, levelID);

        printf("Secondary file paths:\n%s\n%s\n", path, path2);

        *nsfnew2 = fopen(path, "wb");
        *nsd2 = fopen(path2, "wb");

        if (*nsfnew2 == NULL || *nsd2 == NULL)
        {
            printf("Could not open secondary NSF or NSD\n");
            *nsfnew2 = NULL;
            *nsd2 = NULL;
        }
        else
        {
            printf("Successfully using secondary output\n");
        }
    }
}
