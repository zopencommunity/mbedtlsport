/*
 * clock_gettime_zos.c
 *
 * Implementation of clock_gettime() for z/OS USS using the STCKF
 * (Store Clock Fast) instruction. Provides CLOCK_REALTIME semantics.
 *
 * The TOD clock epoch is 1900-01-01 00:00:00 UTC.
 * The Unix epoch is    1970-01-01 00:00:00 UTC.
 * Delta = 70 years = 17 leap years + 53 non-leap years
 *       = (17 * 366 + 53 * 365) * 86400
 *       = 2,208,988,800 seconds.
 *
 * STCKF stores a 64-bit value where bit 51 ticks every microsecond.
 * Therefore the value is (microseconds_since_1900 << 12), i.e.
 * 1 second == 2^12 * 1,000,000 == 4,096,000,000 TOD units.
 */

#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <builtins.h>
#include <clock_gettime_zos.h>

/* Seconds between 1900-01-01 and 1970-01-01 (UTC, no leap seconds). */
#define TOD_EPOCH_DELTA_SECS 2208988800ULL

/* TOD units per second: bit 51 = 1 microsecond, so 2^12 units/usec. */
#define TOD_UNITS_PER_SEC 4096000000ULL /* (1ULL << 12) * 1000000ULL */
#define TOD_UNITS_PER_NSEC_NUM 4096ULL  /* nanoseconds = (fraction * 4096) / 1000 ... */
#define NSEC_PER_SEC 1000000000L

/*
 * Issue the STCKF instruction. Condition code is set but we ignore it
 * here; CC 0 means the clock is in the normal "set" state. STCKF does
 * not require alignment of the storage operand, unlike STCK.
 */
static inline unsigned long long int read_tod_stckf ()
{
   unsigned long long int tod;
   //__asm(" STCKF %0" : "=m"(time_of_day) : : );
   __stckf (&tod);
   return tod;
}

/*
 * This function implements clock_gettime() for z/OS USS using the STCKF instruction.
 * Only needed if LE does not support.
 * Function is available in zoslib.
 * This function is not provided in z/OS 2.4, but is available in z/OS 2.5 and later.
 *
 * CLOCK_REALTIME. Its time represents seconds and nanoseconds since the Epoch.
 *  .
 *
 */


int clock_gettime (clockid_t clk_id, struct timespec *time_spec)
{
   uint64_t time_of_day = 0;
   uint64_t secs_since_1900 = 0;
   uint64_t fraction = 0;
   uint64_t nanoseconds = 0;

   if (time_spec == NULL)
   {
      errno = EFAULT;
      return -1;
   }

   if (clk_id != CLOCK_REALTIME && clk_id != CLOCK_MONOTONIC)
   {
      errno = EINVAL;
      return -1;
   }

   time_of_day = read_tod_stckf ();


   /* Split into whole seconds since 1900 and fractional TOD units. */
   secs_since_1900 = time_of_day / TOD_UNITS_PER_SEC;

   fraction = time_of_day - (secs_since_1900 * TOD_UNITS_PER_SEC);
   /*
    * Convert fractional TOD units to nanoseconds.
    *   1 TOD unit = 1 / 4096 microsecond
    *              = 1000 / 4096 nanoseconds
    * So nanoseconds = fraction * 1000 / 4096 = (fraction * 125) / 512
    * (125/512 is the exact reduced form; avoids overflow since
    *  fraction < 4096000000 and fraction * 125 < 2^59.)
    */
   nanoseconds = (fraction * 125ULL) / 512ULL;

   if (clk_id == CLOCK_REALTIME)
   {
      time_spec->tv_sec = (time_t) (secs_since_1900 - TOD_EPOCH_DELTA_SECS);
   }
   else
   {
      time_spec->tv_sec = (time_t) secs_since_1900;
   }

   time_spec->tv_nsec = (long) nanoseconds;

   return 0;
}

