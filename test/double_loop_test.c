#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <heartbeats/heartbeat-accuracy-power.h>
#include "poet.h"
#include "poet_config.h"
#include "poet_math.h"

const char* HB_LOG_FILE = "/tmp/heartbeat_log.txt";
const char* POET_LOG_FILE = "/tmp/poet_log.txt";
heartbeat_t* heart;

int BIG_NUM1;
int BIG_NUM2_START = 10000000;
int BIG_NUM2 = 10000000;

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
  unsigned int s_nstates;
  poet_control_state_t * s_control_states;
  get_control_states("config/default/control_config",
                     &s_control_states,
                     &s_nstates);
  poet_state * state = poet_init(CONST(atof(argv[2])),
                                 s_nstates, s_control_states, NULL,
                                 NULL,
                                 NULL,
                                 atoi(argv[3]), 1, POET_LOG_FILE);
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
  free(s_control_states);
  heartbeat_finish(heart);

  return 0;
}
