#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <heartbeats/heartbeat-accuracy-power.h>
#include "poet.h"
#include "poet_config.h"
#include "poet_math.h"

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

const char* HB_LOG_FILE = "/tmp/heartbeat_log.txt";
const char* POET_LOG_FILE = "/tmp/poet_log.txt";
heartbeat_t* heart;

int BIG_NUM1;
int BIG_NUM2_START = 10000000;
int BIG_NUM2 = 10000000;

poet_control_state_t control_states[] = CONTROL_STATES;
poet_cpu_state_t cpu_states[] = CPU_STATES;

void apply(void * states, unsigned int num_states, unsigned int id,
           unsigned int last_id);
void apply2(void * states, unsigned int num_states, unsigned int id,
            unsigned int last_id);

void apply(void * states,
           unsigned int num_states,
           unsigned int id,
           unsigned int last_id) {
  (void) states;
  (void) num_states;
  (void) last_id;
  BIG_NUM2  = BIG_NUM2_START / real_to_db(control_states[id].speedup);
}

double freqs[4] = {2.00, 2.33, 2.67, 3.17};
unsigned int freq_state = 0;

void apply2(void * states,
            unsigned int num_states,
            unsigned int id,
            unsigned int last_id) {
  (void) states;
  (void) num_states;
  (void) last_id;
  char command[4096];
  int i;
  if (freq_state != id) {
    for(i = 0; i < 8; i++) {
      sprintf(command,
  	       "echo %d > /sys/devices/system/cpu/cpu%d/cpufreq/scaling_setspeed",
          (int) (freqs[id] * 1000000), i);
      system(command);
    }
    freq_state = id;
  }
}


int main(int argc, char** argv) {
  if ( argc != 4 ) {
    printf("usage:\n");
    printf("processor_speed_test num_beats target_rate window_size\n");
    return -1;
  }

  if(getenv("HEARTBEAT_ENABLED_DIR") == NULL) {
    fprintf(stderr, "ERROR: need to define environment variable HEARTBEAT_ENABLED_DIR (see README)\n");
    return 1;
  }

  heart = heartbeat_init(atoi(argv[3]),
                         atoi(argv[1]),
                         HB_LOG_FILE,
                         CONST(atof(argv[2])),
                         CONST(atof(argv[2])));
  if (heart == NULL) {
    fprintf(stderr, "Failed to initialize heartbeat\n");
    return 1;
  }

  volatile int dummy = 0;
  BIG_NUM1 = atoi(argv[1]);
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

  int i, j;
  heartbeat_record_t hbr;
  for (i = 0; i < BIG_NUM1; i++) {
    heartbeat(heart, i);
    hb_get_current(heart, &hbr);
    real_t hbr_window_rate = hbr_get_window_rate(&hbr);
    real_t hbr_window_power = hbr_get_window_power(&hbr);
    poet_apply_control(state, i, hbr_window_rate, hbr_window_power);

    for (j = 0; j < BIG_NUM2; j++) {
      dummy = dummy >> 1;
      dummy = dummy - 1;
    }
  }

  poet_destroy(state);
  heartbeat_finish(heart);

  return 0;
}
