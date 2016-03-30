#ifndef _POET_H
#define _POET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <heartbeats/heartbeat-accuracy-power.h>

#ifdef FIXED_POINT
typedef int32_t real_t;
#else
typedef double real_t;
#endif

/**
 * Setting this environment variable tells POET not to execute the
 * poet_apply_control function.
 * Allows disabling POET at runtime, removing the overhead of calculations.
 * Of course, no system changes will then be made either.
 */
#define POET_DISABLE_CONTROL "POET_DISABLE_CONTROL"

/**
 * Setting this environment variable tells POET not to call the apply function.
 * POET will run all its calculations but not make any system changes.
 */
#define POET_DISABLE_APPLY "POET_DISABLE_APPLY"

typedef struct poet_internal_state poet_state;

/**
 * The apply function format required to be passed to poet_init().
 */
typedef void (* poet_apply_func) (void * states,
                                  unsigned int num_states,
                                  unsigned int id,
                                  unsigned int last_id);

/**
 * The current state function is used to determine the id of the current state
 * of the system before any changes are applied. Should return -1 if the state
 * cannot be determined, 0 otherwise.
 */
typedef int (* poet_curr_state_func) (const void* states,
                                      unsigned int num_states,
                                      unsigned int* curr_state_id);

typedef struct {
  unsigned int id;
  real_t speedup;
  real_t cost;
} poet_control_state_t;

/**
 * Initializes a poet_state struct which is needed to call other functions.
 *
 * A copy of the apply_states struct will be passed to the apply function as
 * the void* parameter when it is called from poet_apply_control(). It is
 * allowed to be NULL, in which case the apply function must know where to
 * access the appropriate data structures to apply system changes.
 *
 * Default values for state variables are located in src/poet_constants.h
 *
 * @param heart
 * @param num_system_states
 * @param control_states
 * @param apply_states
 * @param apply
 * @param current
 * @param buffer_depth
 * @param log_filename
 */
poet_state * poet_init(heartbeat_t * heart,
                       unsigned int num_system_states,
                       poet_control_state_t * control_states,
                       void * apply_states,
                       poet_apply_func apply,
                       poet_curr_state_func current,
                       unsigned int buffer_depth,
                       const char * log_filename);

/**
 * Deallocates memory from the poet_state struct.
 *
 * @param state
 */
void poet_destroy(poet_state * state);

/**
 * Runs POET decision engine and requests system changes by calling the apply
 * function provided in poet_init().
 *
 * @param state
 */
void poet_apply_control(poet_state * state);

#ifdef __cplusplus
}
#endif

#endif
