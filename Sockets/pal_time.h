#include <time.h>
#include <sys/time.h>
#include <sys/timerfd.h>

#define TICS_PER_SECOND 100
#define PAL_TIME_MAX_TV_SEC 0x7fffffff
#define PAL_TIME_MAX_TV_USEC 0x7fffffff


#define pal_tzval timezone

/*
** Number of tv_usec per second.  Some systems actually purport to prefer
** nanosecond values for the tv_usec field.  Somehow, this probably isn't
** quite accurate yet...
*/
#define TV_USEC_PER_SEC 1000000

/*
**
** Types
*/

/*
** Time value in tics
*/
typedef time_t pal_time_t;

/*
** Clock value
*/
typedef clock_t pal_clock_t;

/*
** A time in seconds and microseconds
*/
#define pal_timeval timeval

#define pal_tm tm

/*
**
** Done
*/

pal_time_t
pal_system_uptime ();

pal_time_t
pal_time_current_monotonic (pal_time_t *tp);

pal_time_t
pal_snmp_system_uptime ();

