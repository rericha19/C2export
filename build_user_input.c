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
    printf("\nInput path to file with permaloaded entries:\n");
    scanf(" %[^\n]",fpaths[0]);
    path_fix(fpaths[0]);

    // if building load lists
    if (config[10]) {
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
    char temp[100];

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
    config[3] = temp;

    printf("\nNeighbour distance? (recommended is approx 7250)\n");
    scanf("%d", &temp);
    config[4] = temp;

    printf("\nDraw list distance? (recommended is approx 7250)\n");
    scanf("%d", &temp);
    config[5] = temp;

    printf("\nPre-load stuff for transitions? (not pre-loading is safer) [0/1]\n");
    scanf("%d", &ans);
    if (ans) {
        config[6] = 1;
        printf("Pre-loading\n\n");
    }
    else {
        config[6] = 0;
        printf("Not pre-loading\n\n");
    }

    printf("Backwards loading penalty? [0 - 0.5]\n");
    float backw;
    scanf("%f", &backw);
    if (backw < 0.0 || backw > 0.5) {
        printf("Invalid, defaulting to 0\n");
        backw = 0;
    }
    config[7] = (int) (PENALTY_MULT_CONSTANT * backw);
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


void build_ask_build_flags(int* ll_flag, int* merge_type) {
    printf("\nRemake load lists? [0/1]\n");
    int ans;
    scanf("%d", &ans);
    if (ans) {
        *ll_flag = 1;
        printf("Will remake load lists\n\n");
    }
    else {
        *ll_flag = 0;
        printf("Won't remake load lists\n\n");
    }

    printf("What merge technique do you want to use?\n");
    printf("[0] - occurence matrix merge\n");
    printf("[1] - a-star merge (wip)\n");
    printf("[2] - relatives & payload merge (deprecate, bad)\n");
    printf("[3] - relative occurence count matrix merge (usually worse than option 0)\n");
    scanf("%d", &ans);
    *merge_type = ans;
    printf("\n");
}
