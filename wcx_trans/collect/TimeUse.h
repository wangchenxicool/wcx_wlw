#ifndef _TIMEUSE_H
#define _TIMEUSE_H

#include <queue>
#include <iostream>
#include <stdint.h>
#include <sys/timeb.h>

class TimeUse 
{
public:
    TimeUse ();
    ~TimeUse ();
    void time_start ();
    float time_used ();
private:
    struct timeb time_starte; 
    struct timeb time_end; 
    float time_use;
};

#endif

