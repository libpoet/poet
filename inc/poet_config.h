#ifndef _POET_CONFIG_H
#define _POET_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "poet.h"

typedef struct {
  unsigned int id;
  unsigned long freq;
  unsigned int cores;
} poet_cpu_state_t;

/**
 * Change the CPU configuration on the system by setting the frequency and
 * number of cores to be used as configured in the state with the provided id.
 *
 * Compatible with the poet_apply_func definition.
 *
 * @param states - must be a poet_cpu_state_t* (array).
 * @param num_states
 * @param id
 * @param last_id
 */
void apply_cpu_config(void* states,
                      unsigned int num_states,
                      unsigned int id,
                      unsigned int last_id);

/**
 * Read the control states from the file at the provided path and store in the
 * states pointer (states* is assigned). The number of states found is stored
 * in num_states. Returns 0 on success.
 *
 * The caller is responsible for freeing the memory this function allocates.
 *
 * @param path
 * @param states
 * @param num_states
 */
int get_control_states(const char* path,
                       poet_control_state_t** states,
                       unsigned int* num_states);

/**
 * Read the CPU states from the file at the provided path and store in the
 * states pointer (states* is assigned). The number of states found is stored
 * in num_states. Returns 0 on success.
 *
 * The caller is responsible for freeing the memory this function allocates.
 *
 * @param path
 * @param states
 * @param num_states
 */
int get_cpu_states(const char* path,
                   poet_cpu_state_t** states,
                   unsigned int* num_states);

/**
 * Attempt to determine the current system state and return the id.
 * Set curr_state_id if possible and return 0, otherwise return -1.
 *
 * Compatible with the poet_curr_state_func definition.
 *
 * @param states
 * @param num_states
 * @param curr_state_id
 */
int get_current_cpu_state(const void* states,
                          unsigned int num_states,
                          unsigned int* curr_state_id);

#ifdef __cplusplus
}
#endif

#endif
