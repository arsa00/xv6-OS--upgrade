#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int res = 0;
    if(argc == 4)
    {
        if(argv[1][0] == 'S' && argv[1][1] == 'J' && argv[1][2] == 'F')
            res = chsched(SJF, atoi(argv[2]), atoi(argv[3]));
        else
            res = chsched(CFS, atoi(argv[2]), atoi(argv[3]));
    }
    else if(argc > 1)
    {
        if(argv[1][0] == 'S' && argv[1][1] == 'J' && argv[1][2] == 'F')
            res = chsched(SJF, 50, PREEMPTIVE);
        else
            res = chsched(CFS, 0, 0);
    }
    if(argc > 1 && res == 0)
    {
        if(argv[1][0] == 'S' && argv[1][1] == 'J' && argv[1][2] == 'F')
            write(1, "Algorithm successfully changed to SJF\n", 38);
        else
            write(1, "Algorithm successfully changed to CFS\n", 38);
    }
    else write(1, "An error occurred\n", 18);
    exit(0);
}
