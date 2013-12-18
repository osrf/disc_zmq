#include <stdio.h>

#ifdef __MACH__
# include <mach/clock.h>
# include <mach/mach.h>
#endif

#include "timing.h"

void dzmq_get_time_now (struct timespec *time)
{
# ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  time->tv_sec = mts.tv_sec;
  time->tv_nsec = mts.tv_nsec;
# else
  clock_gettime(CLOCK_REALTIME, &time);
# endif
}

long convert_timespec_to_ms(struct timespec * time_spec)
{
    return (time_spec->tv_sec * 1e3) + (time_spec->tv_nsec / 1e6);
}

long dzmq_time_till(struct timespec * last_time, long timer_period)
{
    struct timespec now;
    dzmq_get_time_now(&now);
    long time_since_last = convert_timespec_to_ms(&now) - convert_timespec_to_ms(last_time);
    return timer_period - time_since_last;
}
