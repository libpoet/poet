#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <heartbeat-pow.h>
#include <energymon-default.h>
#include "poet.h"
#include "poet_config.h"
#include "poet_math.h"

// Modify the following define statements as appropriate for your test platform

#define NUM_CORES 4

#define CONTROL_STATES { \
{ 0 , CONST(1.0) , CONST(1.0) } , \
{ 1 , CONST(1.165) , CONST(1.1) } , \
{ 2 , CONST(1.335) , CONST(1.2) } , \
{ 3 , CONST(1.585) , CONST(1.3) }}

#define CPU_STATES { \
{ 0 , 1000000 , 1 } , \
{ 1 , 1100000 , 1 } , \
{ 2 , 1200000 , 1 } , \
{ 3 , 1300000 , 1 }}

const char* POET_LOG_FILE = "poet.log";
const unsigned int WORK_ITERATIONS = 10000000;

poet_control_state_t control_states[] = CONTROL_STATES;
poet_cpu_state_t cpu_states[] = CPU_STATES;

static void apply(void* states,
                  unsigned int num_states,
                  unsigned int id,
                  unsigned int last_id) {
  (void) states;
  (void) num_states;
  (void) last_id;
  char command[4096];
  unsigned int i;
  int ret;
  for (i = 0; i < NUM_CORES; i++) {
    snprintf(command, sizeof(command),
             "echo %lu > /sys/devices/system/cpu/cpu%u/cpufreq/scaling_setspeed",
             cpu_states[id].freq, i);
    ret = system(command);
    if (ret) {
      fprintf(stderr, "Command returned status %d:\n\t%s\n", ret, command);
    }
  }
}

static inline uint64_t get_time(void) {
  struct timespec ts;
#ifdef __MACH__
  // OS X does not have clock_gettime, use clock_get_time
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts.tv_sec = mts.tv_sec;
  ts.tv_nsec = mts.tv_nsec;
#else
  // CLOCK_REALTIME is always supported, this should never fail
  clock_gettime(CLOCK_REALTIME, &ts);
#endif
  // must use a const or cast a literal - using a simple literal can overflow!
  const uint64_t ONE_BILLION = 1000000000;
  return ts.tv_sec * ONE_BILLION + ts.tv_nsec;
}


int main(int argc, char** argv) {
  if ( argc != 4 ) {
    printf("usage:\n");
    printf("processor_speed_test num_beats target_rate window_size\n");
    return -1;
  }

  uint64_t window_size = atoi(argv[3]);
  heartbeat_pow_context hb;
  heartbeat_pow_record* hb_window_buffer = malloc(window_size * sizeof(heartbeat_pow_record));
  if (hb_window_buffer == NULL) {
    perror("malloc");
    return 1;
  }
  int hb_fd = fileno(stdout);
  if (heartbeat_pow_init(&hb, window_size, hb_window_buffer, hb_fd, NULL)) {
    perror("Failed to initialize heartbeat");
    return 1;
  }
  hb_pow_log_header(hb_fd);
  energymon em;
  if (energymon_get_default(&em) || em.finit(&em)) {
    perror("Failed to initialize energymon");
    return 1;
  }

  volatile int dummy = 0;
  unsigned int num_beats = atoi(argv[1]);
  poet_state * state = poet_init(CONST(atof(argv[2])),
                                 4,
                                 control_states,
                                 cpu_states,
                                 &apply,
                                 NULL,
                                 atoi(argv[3]),
                                 1,
                                 POET_LOG_FILE);
  if (state == NULL) {
    fprintf(stderr, "Failed to initialize poet\n");
    return 1;
  }

  unsigned int i, j;
  uint64_t time_start, time_end, energy_start, energy_end;
  time_end = get_time();
  energy_end = em.fread(&em);
  for (i = 0; i < num_beats; i++) {
    time_start = time_end;
    energy_start = energy_end;
    for (j = 0; j < WORK_ITERATIONS; j++) {
      dummy = dummy >> 1;
      dummy = dummy - 1;
    }
    time_end = get_time();
    energy_end = em.fread(&em);
    heartbeat_pow(&hb, i, 1, time_start, time_end, energy_start, energy_end);
    real_t hb_window_perf = hb_pow_get_window_perf(&hb);
    real_t hb_window_power = hb_pow_get_window_power(&hb);
    poet_apply_control(state, i, hb_window_perf, hb_window_power);
  }

  poet_destroy(state);
  hb_pow_log_window_buffer(&hb, hb_pow_get_log_fd(&hb));
  free(hb_window_buffer);
  em.ffinish(&em);

  return 0;
}
