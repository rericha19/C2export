#include "macros.h"

void rotate_main()
{
    double rotation;
    char path[MAX];

    scanf("%lf",&rotation);
    if (rotation <= 0 || rotation >= 360)
    {
        printf("Input a value in range (0; 360) degrees\n");
        return ;
    }

    printf("Input the path to the directory or level whose contents you want to export:\n");
    scanf(" %[^\n]",path);
}

void rotate_scenery()
{

}

void rotate_zone()
{

}
