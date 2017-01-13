#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "poet.h"
#include "poet_constants.h"
#include "poet_math.h"

#ifdef FIXED_POINT
#pragma message "Compiling fixed point version"
#else
#pragma message "Compiling floating point version"
#endif

/*
##################################################
###########  POET INTERNAL DATATYPES #############
##################################################
*/

// Represents state of kalman filter used in estimate_workload
typedef struct {
  real_t x_hat_minus;
  real_t x_hat;
  real_t p_minus;
  real_t h;
  real_t k;
  real_t p;
} filter_state;

// Container for old speedup values and old errors
typedef struct {
  real_t u;
  real_t uo;
  real_t uoo;
  real_t e;
  real_t eo;
  real_t umax;
} calc_xup_state;

// Container for log records
typedef struct {
  unsigned long tag;
  real_t act_rate;
  filter_state pfs;
  calc_xup_state scs;
  real_t workload;
  int lower_id;
  int upper_id;
  int low_state_iters;
} poet_record;

struct poet_internal_state {
  // log file and log buffer
  FILE * log_file;
  unsigned int buffer_depth;
  poet_record * lb;

  real_t perf_goal;

  // performance filter state
  filter_state pfs;

  // speedup calculation state
  calc_xup_state scs;

  // general
  int current_action;

  int lower_id;
  int upper_id;
  unsigned int last_id;
  int low_state_iters;
  unsigned int period;

  unsigned int num_system_states;
  poet_apply_func apply;
  poet_control_state_t * control_states;
  void * apply_states;
};

/*
##################################################
###########  POET FUNCTION DEFINITIONS  ##########
##################################################
*/

// Allocates and initializes a new poet state variable
poet_state * poet_init(real_t perf_goal,
                       unsigned int num_system_states,
                       poet_control_state_t * control_states,
                       void * apply_states,
                       poet_apply_func apply,
                       poet_curr_state_func current,
                       unsigned int period,
                       unsigned int buffer_depth,
                       const char * log_filename) {
  unsigned int i;

  if (perf_goal <= R_ZERO || num_system_states == 0 || control_states == NULL || period == 0 ||
      (buffer_depth == 0 && log_filename != NULL)) {
    errno = EINVAL;
    return NULL;
  }

  // Allocate memory for state struct
  poet_state * state = (poet_state *) malloc(sizeof(struct poet_internal_state));
  if (state == NULL) {
    return NULL;
  }

  // Remember the performance goal
  state->perf_goal = perf_goal;

  // Remember the period
  state->period = period;

  // Allocate memory for log buffer
  state->buffer_depth = buffer_depth;
  if (buffer_depth > 0) {
    state->lb = malloc(buffer_depth * sizeof(poet_record));
    if (state->lb == NULL) {
      free(state);
      return NULL;
    }
  } else {
    state->lb = NULL;
  }

  // Open log file
  if (log_filename == NULL) {
    state->log_file = NULL;
  } else {
    state->log_file = fopen(log_filename, "w");
    if (state->log_file == NULL) {
      perror(log_filename);
      free(state->lb);
      free(state);
      return NULL;
    }
    fprintf(state->log_file,
            "%16s %16s %16s %16s %16s %16s %16s %16s %16s %16s %16s %16s %16s %16s\n",
            "TAG", "ACTUAL_RATE", "X_HAT_MINUS", "X_HAT", "P_MINUS", "H", "K",
            "P", "SPEEDUP", "ERROR", "WORKLOAD", "LOWER_ID", "UPPER_ID", "LOW_STATE_ITERS");
  }

  // initialize variables used in the performance filter
  state->pfs.x_hat_minus = X_HAT_MINUS_START;
  state->pfs.x_hat = X_HAT_START;
  state->pfs.p_minus = P_MINUS_START;
  state->pfs.h = H_START;
  state->pfs.k = K_START;
  state->pfs.p = P_START;

  // initialize general poet variables
  state->current_action = CURRENT_ACTION_START;
  state->num_system_states = num_system_states;
  state->apply = apply;
  state->control_states = control_states;
  state->apply_states = apply_states; // allowed to be NULL

  state->upper_id = -1;
  state->lower_id = -1;

  // try to get the initial system state
  if (current == NULL || current(state->apply_states, state->num_system_states, &state->last_id)) {
    // default to the highest state id
    state->last_id = state->num_system_states - 1;
  }

  // initialize variables used for calculating speedup
  state->scs.u = state->control_states[state->last_id].speedup;
  state->scs.uo = state->scs.u;
  state->scs.uoo = state->scs.u;
  state->scs.e = E_START;
  state->scs.eo = EO_START;

  state->low_state_iters = 0;

  // Calculate max_speedup
  state->scs.umax = R_ONE;
  for (i = 0; i < state->num_system_states; i++) {
    if (state->control_states[i].speedup >= state->scs.umax) {
      state->scs.umax = state->control_states[i].speedup;
    }
  }

  return state;
}

// Destroys poet state variable
void poet_destroy(poet_state * state) {
  if (state != NULL) {
    if (state->log_file != NULL) {
      fclose(state->log_file);
    }
    free(state->lb);
    free(state);
  }
}

// Change the performance goal at runtime.
void poet_set_performance_goal(poet_state * state,
                               real_t perf_goal) {
  if (state != NULL && perf_goal > R_ZERO) {
    state->perf_goal = perf_goal;
  }
}

static inline void logger(const poet_state * state,
                          real_t workload,
                          unsigned long id,
                          real_t perf) {
  unsigned int index;
  unsigned int i;

  if (state->log_file != NULL) {
    index = (id / state->period) % state->buffer_depth;
    state->lb[index].tag = id;
    state->lb[index].act_rate = perf;
    memcpy(&state->lb[index].pfs, &state->pfs, sizeof(filter_state));
    memcpy(&state->lb[index].scs, &state->scs, sizeof(calc_xup_state));
    state->lb[index].workload = workload;
    state->lb[index].lower_id = state->lower_id;
    state->lb[index].upper_id = state->upper_id;
    state->lb[index].low_state_iters = state->low_state_iters;

    if (index == state->buffer_depth - 1) {
      for (i = 0; i < state->buffer_depth; i++) {
        fprintf(state->log_file, "%16lu %16f %16f %16f %16f %16f %16f %16f %16f %16f %16f %16d %16d %16d\n",
                state->lb[i].tag,
                real_to_db(state->lb[i].act_rate),
                real_to_db(state->lb[i].pfs.x_hat_minus),
                real_to_db(state->lb[i].pfs.x_hat),
                real_to_db(state->lb[i].pfs.p_minus),
                real_to_db(state->lb[i].pfs.h),
                real_to_db(state->lb[i].pfs.k),
                real_to_db(state->lb[i].pfs.p),
                real_to_db(state->lb[i].scs.u),
                real_to_db(state->lb[i].scs.e),
                real_to_db(state->lb[i].workload),
                state->lb[i].lower_id,
                state->lb[i].upper_id,
                state->lb[i].low_state_iters);
      }
    }
  }
}

/*
 * Estimates the base workload of the application by estimating
 * either the amount of time (in seconds) or the amount of energy
 * (in joules)  which elapses between iterations without any knobs
 * activated by poet.
 *
 * Uses a Kalman Filter
 */
static inline real_t estimate_base_workload(real_t current_workload,
                                            real_t last_xup,
                                            filter_state * state) {
  real_t _w;

  state->x_hat_minus = state->x_hat;
  state->p_minus = state->p + Q;

  state->h = last_xup;
  state->k = div(mult(state->p_minus, state->h),
                 mult3(state->h, state->p_minus, state->h) + R);
  state->x_hat = state->x_hat_minus + mult(state->k,
                  (current_workload - mult(state->h, state->x_hat_minus)));
  state->p = mult(R_ONE - mult(state->k, state->h), state->p_minus);

  _w = div(R_ONE, state->x_hat);

  return _w;
}

/*
 * Calculates the speedup necessary to achieve the target performance
 */
static inline void calculate_xup(real_t current_rate,
                                 real_t desired_rate,
                                 real_t w,
                                 calc_xup_state * state) {
  // A   = -(-P1*Z1 - P2*Z1 + MU*P1*P2 - MU*P2 + P2 - MU*P1 + P1 + MU)
  // B   = -(-MU*P1*P2*Z1 + P1*P2*Z1 + MU*P2*Z1 + MU*P1*Z1 - MU*Z1 - P1*P2)
  // C   = ((MU - MU*P1)*P2 + MU*P1 - MU)*w
  // D   = ((MU*P1-MU)*P2 - MU*P1 + MU)*w*Z1
  // F   = 1.0/(Z1-1.0)
  real_t A   = -(-mult(P1, Z1) - mult(P2, Z1) + mult3(MU, P1, P2) - mult(MU, P2) + P2 - mult(MU, P1) + P1 + MU);
  real_t B   = -(-mult4(MU, P1, P2, Z1) + mult3(P1, P2, Z1) + mult3(MU, P2, Z1) + mult3(MU, P1, Z1) - mult(MU, Z1) - mult(P1, P2));
  real_t C   = mult(mult(MU - mult(MU, P1), P2) + mult(MU, P1) - MU, w);
  real_t D   = mult3(mult(mult(MU, P1)-MU, P2) - mult(MU, P1) + MU, w, Z1);
  real_t F   = div(R_ONE, Z1 - R_ONE);

  state->e = desired_rate - current_rate;

  // Calculate speedup
  state->u = mult(F , mult(A, state->uo) + mult(B, state->uoo) + mult(C, state->e) + mult(D, state->eo));

  // Speedups less than one have no effect
  if (state->u < R_ONE) {
    state->u = R_ONE;
  }

  // A speedup greater than the maximum is not achievable
  if (state->u > state->umax) {
    state->u = state->umax;
  }

  // Saving old state values
  state->uoo = state->uo;
  state->uo  = state->u;
  state->eo  = state->e;
}

/*
 * Calculate the time division between the two system configuration states
 */
static inline void calculate_time_division(poet_state * state,
                                           real_t r_period) {
  // Ensure that translate returned valid configuration state ids
  if (!(state->upper_id == -1 && state->lower_id == -1)) {
    if (state->upper_id == -1) {
      state->upper_id = state->lower_id;
    } else if (state->lower_id == -1) {
      state->lower_id = state->upper_id;
    }

    // Calculate the time division between the upper and lower state
    real_t lower_xup;
    real_t upper_xup;
    real_t target_xup;
    upper_xup = state->control_states[state->upper_id].speedup;
    lower_xup = state->control_states[state->lower_id].speedup;
    target_xup = state->scs.u;

    // x represents the percentage of iterations spent in the first (lower)
    // configuration
    // Conversely, (1 - x) is the percentage of iterations in the second
    // (upper) configuration
    real_t x;

    // If lower rate and upper rate are equal, no need for time division
    if (upper_xup == lower_xup) {
      x = R_ZERO;
    } else {
      // This equation ensures the time period of the combined rates is equal
      // to the time period of the target rate
      // 1 / Target rate = X / (lower rate) + (1 - X) / (upper rate)
      // Solve for X
      x = div(mult(upper_xup, lower_xup) - mult(target_xup, lower_xup),
              mult(upper_xup, target_xup) - mult(target_xup, lower_xup));
    }
    // Num of iterations (in lower state) = x * (controller period)
    state->low_state_iters = real_to_int(mult(r_period, x));
  }
}

/**
 * Check all pairs of states that can achieve the target and choose the pair
 * with the lowest cost. Uses an n^2 algorithm.
 */
static inline void translate_n2_with_time(poet_state * state) {
  unsigned int i;
  unsigned int j;
  real_t r_period = int_to_real(state->period);
  real_t target_xup;
  real_t best_cost = BIG_REAL_T;
  int best_lower_id = -1;
  int best_upper_id = -1;
  int best_low_state_iters = -1;
  real_t lower_xup;
  real_t upper_xup;
  real_t lower_xup_cost;
  real_t upper_xup_cost;
  real_t r_low_state_iters;
  real_t cost;
  target_xup = state->scs.u;

  for (i = 0; i < state->num_system_states; i++) {
    upper_xup = state->control_states[i].speedup;
    upper_xup_cost = state->control_states[i].cost;
    if (upper_xup < target_xup) {
      continue;
    }
    state->upper_id = i;
    for (j = 0; j < state->num_system_states; j++) {
      lower_xup = state->control_states[j].speedup;
      lower_xup_cost = state->control_states[j].cost;
      if (lower_xup > target_xup) {
        continue;
      }
      state->lower_id = j;
      // find time for both states
      calculate_time_division(state, r_period);
      // find cost of this state combination
      r_low_state_iters = int_to_real(state->low_state_iters);
      cost = mult(div(r_low_state_iters, lower_xup), lower_xup_cost) +
             mult(div(r_period - r_low_state_iters, upper_xup), upper_xup_cost);
      // if this is the best configuration so far, remember it
      if (cost < best_cost) {
        best_lower_id = j;
        best_upper_id = i;
        best_low_state_iters = state->low_state_iters;
        best_cost = cost;
      }
    }
  }

  // use the best configuration
  state->lower_id = best_lower_id;
  state->upper_id = best_upper_id;
  state->low_state_iters = best_low_state_iters;
}

// Runs POET decision engine and requests system changes
void poet_apply_control(poet_state * state,
                        unsigned long id,
                        real_t perf,
                        real_t pwr) {
  (void) pwr;
  if (state == NULL || getenv(POET_DISABLE_CONTROL) != NULL) {
    return;
  }

  if (state->current_action == 0) {
    // Estimate the performance workload
    // estimate time between iterations given minimum amount of resources
    real_t time_workload = estimate_base_workload(perf,
                                                  state->scs.u,
                                                  &state->pfs);

    // Get a new goal speedup to apply to the application
    calculate_xup(perf, state->perf_goal, time_workload, &state->scs);

    // Xup is translated into a system configuration
    // A certain amount of time is assigned to each system configuration
    // in order to achieve the requested Xup
    translate_n2_with_time(state);

    logger(state, time_workload, id, perf);
  }

  // Check which speedup should be applied, upper or lower
  int config_id = -1;
  if (state->low_state_iters > 0) {
    config_id = state->lower_id;
    state->low_state_iters--;
  } else if (state->upper_id >= 0) {
    config_id = state->upper_id;
  }

  if (config_id >= 0 && (unsigned int) config_id != state->last_id) {
    if (state->apply != NULL && getenv(POET_DISABLE_APPLY) == NULL) {
      state->apply(state->apply_states, state->num_system_states, config_id,
                   state->last_id);
    }
    state->last_id = config_id;
  }

  state->current_action = (state->current_action + 1) % state->period;
}
