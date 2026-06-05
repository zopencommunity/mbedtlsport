/* Identifier for system-wide realtime clock.  */
# define CLOCK_REALTIME                 0
/* Monotonic system-wide clock.  */
# define CLOCK_MONOTONIC                1
/* High-resolution timer from the CPU.  */
# define CLOCK_PROCESS_CPUTIME_ID       2
/* Thread-specific CPU-time clock.  */
# define CLOCK_THREAD_CPUTIME_ID        3
/* Monotonic system-wide clock, not adjusted for frequency scaling.  */
# define CLOCK_MONOTONIC_RAW            4
/* Identifier for system-wide realtime clock, updated only on ticks.  */
# define CLOCK_REALTIME_COARSE          5
/* Monotonic system-wide clock, updated only on ticks.  */
# define CLOCK_MONOTONIC_COARSE         6
/* Get current value of clock CLOCK_ID and store it in TP.  */
/* Clock ID used in clock and timer functions.  */
#define clockid_t unsigned int
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif
   int clock_gettime (clockid_t __clock_id, struct timespec *__tp);
#ifdef __cplusplus
}
#endif

