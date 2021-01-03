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
void build_ask_list_paths(char fpaths[FPATH_COUNT][MAX]) {
    printf("\nInput the path to the file with permaloaded entries:\n");
    scanf(" %[^\n]",fpaths[0]);
    path_fix(fpaths[0]);

    printf("\nInput the path to the file with type/subtype dependencies:\n");
    scanf(" %[^\n]",fpaths[1]);
    path_fix(fpaths[1]);

    //strcpy(fpaths[1], "complete entity list manni.txt");
    strcpy(fpaths[2], "collision list.txt");
}


/** \brief
 *  Lets the user pick the spawn that's to be used as the primary spawn.
 *
 * \param spawns SPAWNS                 struct that contains all found spawns (willy or checkpoint entities' global origins)
 * \return void
 */
void build_ask_spawn(SPAWNS spawns) {
    int input;
    // lets u pick a spawn point
    printf("\nPick a spawn (type it second time if it doesnt work):\n");
    for (int i = 0; i < spawns.spawn_count; i++)
        printf("Spawn %d:\tZone: %s\n", i + 1, eid_conv(spawns.spawns[i].zone, NULL));

    scanf("%d", &input);
    input--;
    if (input >= spawns.spawn_count || input < 0) {
        printf("No such spawn, defaulting to first one\n");
        input = 0;
    }

    if (input)
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
    char ans;
    printf("\nSLST distance?      (recommended is approx 7250)\n");
    scanf("%d", &temp);
    config[3] = temp;

    printf("\nNeighbour distance? (recommended is approx 7250)\n");
    scanf("%d", &temp);
    config[4] = temp;

    printf("\nDraw list distance? (recommended is approx 7250)\n");
    scanf("%d", &temp);
    config[5] = temp;

    printf("\nPre-load stuff for transitions? (not pre-loading is safer) [y/n]\n");
    scanf("%c", &ans);
    scanf("%c", &ans);
    if (ans == 'y' || ans == 'Y') {
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
