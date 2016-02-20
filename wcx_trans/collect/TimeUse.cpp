#include "TimeUse.h"

TimeUse::TimeUse ()
{
}

TimeUse::~TimeUse ()
{
}

void TimeUse::time_start ()
{
    ftime (&time_starte);
}

float TimeUse::time_used ()
{
    ftime (&time_end);
    time_use = (time_end.time - time_starte.time) * 1000 + (time_end.millitm - time_starte.millitm);
    return time_use;
}
