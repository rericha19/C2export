#include "macros.h"
// contains mostly functions used to get input from user (config stuff)

/** \brief
 *  Asks the user for built level's ID.
 *
 * \return int                          picked level ID
 */
int build_ask_ID() {
    int level_ID;
    printf("\nWhat ID do you want your level to have? (hex 0 - 3F) [CAN OVERWRITE EXISTING FILES!]\n");
    scanf("%x", &level_ID);
    if (level_ID < 0 || level_ID > 0x3F) {
        printf("Invalid ID, defaulting to 1\n");
        level_ID = 1;
    }

    return level_ID;
}


/** \brief
 *  Asks for path to the list with the permaloaded stuff and type/subtype info.
 *
 * \param fpath char*                   path to the file
 * \return void
 */
void build_ask_list_paths(char fpaths[FPATH_COUNT][MAX], int* config) {

    int remaking_load_lists_flag = config[CNFG_IDX_LL_REMAKE_FLAG];

    printf("\nInput path to file with permaloaded entries:\n");
    scanf(" %[^\n]",fpaths[0]);
    path_fix(fpaths[0]);

    // if building load lists
    if (remaking_load_lists_flag) {
        printf("\nInput path to file with type/subtype dependencies:\n");
        scanf(" %[^\n]",fpaths[1]);
        path_fix(fpaths[1]);

        printf("\nInput path to file with collision dependencies:\n");
        scanf(" %[^\n]", fpaths[2]);
        path_fix(fpaths[2]);
        //strcpy(fpaths[2], "collision list.txt");
    }
}


/** \brief
 *  Lets the user pick the spawn that's to be used as the primary spawn.
 *
 * \param spawns SPAWNS                 struct that contains all found spawns (willy or checkpoint entities' global origins)
 * \return void
 */
void build_ask_spawn(SPAWNS spawns) {
    int input;
    char temp[100] = "";

    // lets u pick a spawn point
    printf("Pick a spawn:\n");
    for (int i = 0; i < spawns.spawn_count; i++)
        printf("Spawn %d:\tZone: %s\n", i + 1, eid_conv(spawns.spawns[i].zone, temp));

    scanf("%d", &input);
    input--;
    if (input >= spawns.spawn_count || input < 0) {
        printf("No such spawn, defaulting to first one\n");
        input = 0;
    }

    build_swap_spawns(spawns, 0, input);
}


/** \brief
 *  Asks the user what distances to use for load list building.
 *
 * \param config int*                   config array
 * \return void
 */
void build_ask_distances(int *config) {
    int temp;
    int ans;
    printf("\nSLST distance?      (recommended is approx 7250)\n");
    scanf("%d", &temp);
    config[CNFG_IDX_LL_SLST_DIST_VALUE] = temp;

    printf("\nNeighbour distance? (recommended is approx 7250)\n");
    scanf("%d", &temp);
    config[CNFG_IDX_LL_NEIGH_DIST_VALUE] = temp;

    printf("\nDraw list distance? (recommended is approx 7250)\n");
    scanf("%d", &temp);
    config[CNFG_IDX_LL_DRAW_DIST_VALUE] = temp;

    printf("\nPre-load stuff for transitions? (not pre-loading is safer) [0/1]\n");
    scanf("%d", &ans);
    if (ans) {
        config[CNFG_IDX_LL_TRNS_PRLD_FLAG] = 1;
        printf("Pre-loading\n\n");
    }
    else {
        config[CNFG_IDX_LL_TRNS_PRLD_FLAG] = 0;
        printf("Not pre-loading\n\n");
    }

    printf("Backwards loading penalty? [0 - 0.5]\n");
    float backw;
    scanf("%f", &backw);
    if (backw < 0.0 || backw > 0.5) {
        printf("Invalid, defaulting to 0\n");
        backw = 0;
    }
    config[CNFG_IDX_LL_BACKWARDS_PENALTY] = (int) (PENALTY_MULT_CONSTANT * backw);
}


/** \brief
 *  Swaps spawns.
 *
 * \param spawns SPAWNS                 struct that contains all spanwns
 * \param spawnA int                    index1
 * \param spawnB int                    index2
 * \return void
 */
void build_swap_spawns(SPAWNS spawns, int spawnA, int spawnB) {
    SPAWN temp = spawns.spawns[spawnA];
    spawns.spawns[spawnA] = spawns.spawns[spawnB];
    spawns.spawns[spawnB] = temp;
}


void build_ask_build_flags(int* config) {
    printf("\nRemake load lists? [0/1]\n");
    int ans;
    scanf("%d", &ans);
    if (ans) {
        config[CNFG_IDX_LL_REMAKE_FLAG] = 1;
        printf("Will remake load lists\n\n");
    }
    else {
        config[CNFG_IDX_LL_REMAKE_FLAG] = 0;
        printf("Won't remake load lists\n\n");
    }

    printf("What merge method do you want to use?\n");
    printf("[0] - occurence count matrix merge (absolute)\n");
    printf("[1] - occurence count matrix merge (relative) [usually worse than option 0]\n");
    printf("[2] - relatives & payload merge [deprecate, bad]\n");
    printf("[3] - state set graph search based merge (A*/DFS) [slow, no guaranteed result]\n");
    scanf("%d", &ans);
    config[CNFG_IDX_MERGE_METHOD_VALUE] = ans;
    printf("\n");
}

void build_ask_premerge(int *premerge_type, double *merge_ratio) {
    int ans;
    printf("\nWhich premerge method do you want to use?\n");
    printf("[0] - no premerge\n");
    printf("[1] - partial occurence count matrix merge (absolute)\n");
    printf("[2] - partial occurence count matrix merge (relative)\n");
    printf("[3] - place models with their most used animation available\n");
    printf("[4] - place SLSTs with their zone\n");
    printf("[5] - 3 & 4 combined\n");
    scanf("%d", &ans);
    if (ans < 0 || ans > 5) {
        printf("Invalid choice, defaulting to [0]\n");
        ans = 0;
    }

    if (ans == 1 || ans == 2) {
        printf("\nPartial premerge ratio? [0 - no premerge, 1 - full merge]\n");
        double ans2;
        scanf("%lf", &ans2);
        *merge_ratio = ans2;
    }

    *premerge_type = ans;
    printf("\n");
}
