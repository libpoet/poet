#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "poet_config.h"
#include "poet.h"
#include "poet_math.h"

poet_control_state_t * cstates_speed;
poet_cpu_state_t * cpu_states;

int main() {
  unsigned int nctl_states;
  unsigned int ncpu_states;
  unsigned int curr_state_id;
  unsigned int i;

  if (get_control_states("../config/default/control_config",
                         &cstates_speed,
                         &nctl_states)) {
    fprintf(stderr, "Failed to get control states.\n");
    return 1;
  }
  printf("get_control_state size: %u\n", nctl_states);
  for (i = 0; i < nctl_states; i++) {
    printf("%u, %f, %f\n",
           cstates_speed[i].id, real_to_db(cstates_speed[i].speedup), real_to_db(cstates_speed[i].cost));
  }

  if (get_cpu_states("../config/default/cpu_config",
                     &cpu_states,
                     &ncpu_states)) {
    fprintf(stderr, "Failed to get CPU states.\n");
    return 1;
  }
  printf("get_cpu_state size: %u\n", ncpu_states);
  for (i = 0; i < ncpu_states; i++) {
    printf("%u, %lu, %u\n", cpu_states[i].id, cpu_states[i].freq, cpu_states[i].cores);
  }

  if (nctl_states != ncpu_states) {
    fprintf(stderr, "Error: got different number of states for control and cpu.\n");
    return 1;
  }

  if (get_current_cpu_state(cpu_states, ncpu_states, &curr_state_id)) {
    printf("Failed to get current CPU state.\n");
  } else {
    printf("Current state id: %u\n", curr_state_id);
  }
  for (i = 0; i < nctl_states; i++) {
    printf("Applying configuration %u\n", i);
    apply_cpu_config(cpu_states, ncpu_states, i, i == 0 ? nctl_states - 1 : i - 1);
    sleep(1);
    if (get_current_cpu_state(cpu_states, ncpu_states, &curr_state_id)) {
      fprintf(stderr, "Failed to get current CPU state after setting configuration %u\n", i);
    } else {
      printf("New state id: %u\n", curr_state_id);
    }
    sleep(1);
  }

  free(cstates_speed);
  free(cpu_states);

  return 0;
}
