#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <heartbeats/heartbeat-accuracy-power.h>
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

typedef struct {
  int * hb_number;
  real_t * hb_rate;
  real_t * x_hat_minus;
  real_t * x_hat;
  real_t * p_minus;
  real_t * h;
  real_t * k;
  real_t * p;
  real_t * u;
  real_t * e;
  real_t * workload;
} log_buffer;

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

struct poet_internal_state {
  // log file and log buffer
  FILE * log_file;
  int buffer_depth;
  log_buffer * lb;

  // performance filter state
  filter_state * pfs;

  // speedup calculation state
  calc_xup_state * scs;

  // general
  heartbeat_t * heart;
  int current_action;

  int lower_id;
  int upper_id;
  unsigned int last_id;
  int num_hbs;

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
poet_state * poet_init(heartbeat_t * heart,
                       unsigned int num_system_states,
                       poet_control_state_t * control_states,
                       void * apply_states,
                       poet_apply_func apply,
                       poet_curr_state_func current,
                       unsigned int buffer_depth,
                       const char * log_filename) {
  int i;
  FILE* log_file = NULL;

  if (control_states == NULL) {
    return NULL;
  }

  // Open log file
  if (log_filename != NULL) {
    log_file = fopen(log_filename, "w");
    if (log_file == NULL) {
      perror("Failed to open POET log file");
      return NULL;
    }
  }

  // Allocate memory for state struct
  poet_state * state = (poet_state *) malloc(sizeof(struct poet_internal_state));

  // Store log file
  state->log_file = log_file;
  if (state->log_file != NULL) {
    fprintf(state->log_file,
            "%16s %16s %16s %16s %16s %16s %16s %16s %16s %16s %16s %16s %16s %16s\n",
            "HB_NUM", "HB_RATE", "X_HAT_MINUS", "X_HAT", "P_MINUS", "H", "K",
            "P", "SPEEDUP", "ERROR", "WORKLOAD", "LOWER_ID", "UPPER_ID", "NUM_HBS");
  }

  // Allocate memory for log buffer
  state->buffer_depth = buffer_depth;
  state->lb = malloc(sizeof(log_buffer));
  state->lb->hb_number = (int *) malloc(buffer_depth * sizeof(int));
  state->lb->hb_rate = (real_t *) malloc(buffer_depth * sizeof(real_t));
  state->lb->x_hat_minus = (real_t *) malloc(buffer_depth * sizeof(real_t));
  state->lb->x_hat = (real_t *) malloc(buffer_depth * sizeof(real_t));
  state->lb->p_minus = (real_t *) malloc(buffer_depth * sizeof(real_t));
  state->lb->h = (real_t *) malloc(buffer_depth * sizeof(real_t));
  state->lb->k = (real_t *) malloc(buffer_depth * sizeof(real_t));
  state->lb->p = (real_t *) malloc(buffer_depth * sizeof(real_t));
  state->lb->u = (real_t *) malloc(buffer_depth * sizeof(real_t));
  state->lb->e = (real_t *) malloc(buffer_depth * sizeof(real_t));
  state->lb->workload = (real_t *) malloc(buffer_depth * sizeof(real_t));

  // Allocate memory for performance filter state
  filter_state * pfs = (filter_state *) malloc(sizeof(filter_state));

  // Allocate memory for speedup calculation state
  calc_xup_state * scs = (calc_xup_state *) malloc(sizeof(calc_xup_state));

  // initialize variables used in the performance filter
  pfs->x_hat_minus = X_HAT_MINUS_START;
  pfs->x_hat = X_HAT_START;
  pfs->p_minus = P_MINUS_START;
  pfs->h = H_START;
  pfs->k = K_START;
  pfs->p = P_START;
  state->pfs = pfs;

  // initialize general poet variables
  state->heart = heart;
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
  scs->u = state->control_states[state->last_id].speedup;
  scs->uo = scs->u;
  scs->uoo = scs->u;
  scs->e = E_START;
  scs->eo = EO_START;
  state->scs = scs;

  state->num_hbs = 0;

  // Calculate max_speedup
  scs->umax = R_ONE;
  for (i = 0; i < state->num_system_states; i++) {
    if (state->control_states[i].speedup >= scs->umax) {
      scs->umax = state->control_states[i].speedup;
    }
  }

  return state;
}

// Destroys poet state variable
void poet_destroy(poet_state * state) {
  free(state->pfs);
  free(state->scs);
  free(state->lb->hb_number);
  free(state->lb->hb_rate);
  free(state->lb->x_hat_minus);
  free(state->lb->x_hat);
  free(state->lb->p_minus);
  free(state->lb->h);
  free(state->lb->k);
  free(state->lb->p);
  free(state->lb->u);
  free(state->lb->e);
  free(state->lb->workload);
  free(state->lb);
  if (state->log_file != NULL) {
    fclose(state->log_file);
  }
  free(state);
}

static inline void logger(poet_state * state, int period, real_t workload) {
  heartbeat_record_t hbr;
  hb_get_current(state->heart, &hbr);

  int hbr_tag = hbr_get_tag(&hbr);
  int index = ((hbr_tag / period) % state->buffer_depth);
  filter_state * fs = state->pfs;
  calc_xup_state * cxs = state->scs;

  if (state->log_file != NULL) {
    state->lb->hb_number[index] = hbr_tag;
    state->lb->hb_rate[index] = hbr_get_window_rate(&hbr);
    state->lb->x_hat_minus[index] = fs->x_hat_minus;
    state->lb->x_hat[index] = fs->x_hat;
    state->lb->p_minus[index] = fs->p_minus;
    state->lb->h[index] = fs->h;
    state->lb->k[index] = fs->k;
    state->lb->p[index] = fs->p;
    state->lb->u[index] = cxs->u;
    state->lb->e[index] = cxs->e;
    state->lb->workload[index] = workload;

    if (index == state->buffer_depth - 1) {
      int i;
      for (i = 0; i < state->buffer_depth; i++) {
        fprintf(state->log_file, "%16d %16f %16f %16f %16f %16f %16f %16f %16f %16f %16f %16d %16d %16d\n",
                state->lb->hb_number[i],
                real_to_db(state->lb->hb_rate[i]),
                real_to_db(state->lb->x_hat_minus[i]),
                real_to_db(state->lb->x_hat[i]),
                real_to_db(state->lb->p_minus[i]),
                real_to_db(state->lb->h[i]),
                real_to_db(state->lb->k[i]),
                real_to_db(state->lb->p[i]),
                real_to_db(state->lb->u[i]),
                real_to_db(state->lb->e[i]),
                real_to_db(state->lb->workload[i]),
                state->lower_id,
                state->upper_id,
                state->num_hbs);
      }
    }
  }
}


/*
 * Estimates the base workload of the application by estimating
 * either the amount of time (in seconds) or the amount of energy
 * (in joules)  which elapses between heartbeats without any knobs
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
    target_xup = state->scs->u;

    // x represents the percentage of heartbeats spent in the first (lower)
    // configuration
    // Conversely, (1 - x) is the percentage of heartbeats in the second
    // (upper) configuration
    real_t x;

    // If lower rate and upper rate are equal, no need for time division
    if (upper_xup == lower_xup) {
      x = R_ZERO;
      state->num_hbs = 0;
    } else {
      // This equation ensures the time period of the combined rates is equal
      // to the time period of the target rate
      // 1 / Target rate = X / (lower rate) + (1 - X) / (upper rate)
      // Solve for X
      x = div(mult(upper_xup, lower_xup) - mult(target_xup, lower_xup),
              mult(upper_xup, target_xup) - mult(target_xup, lower_xup));

      // Num of hbs (in lower state) = x * (controller period)
      state->num_hbs = real_to_int(mult(r_period, x));
    }
  }
}

/**
 * Check all pairs of states that can achieve the target and choose the pair
 * with the lowest cost. Uses an n^2 algorithm.
 */
static inline void translate_n2_with_time(poet_state * state, int period) {
  int i;
  int j;
  real_t r_period = int_to_real(period);
  real_t target_xup;
  real_t best_cost = BIG_REAL_T;
  int best_lower_id = -1;
  int best_upper_id = -1;
  int best_num_hbs = -1;
  real_t lower_xup;
  real_t upper_xup;
  real_t lower_xup_cost;
  real_t upper_xup_cost;
  real_t r_hbs;
  real_t cost;
  target_xup = state->scs->u;

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
      r_hbs = int_to_real(state->num_hbs);
      cost = mult(div(r_hbs, lower_xup), lower_xup_cost) +
             mult(div(r_period - r_hbs, upper_xup), upper_xup_cost);
      // if this is the best configuration so far, remember it
      if (cost < best_cost) {
        best_lower_id = j;
        best_upper_id = i;
        best_num_hbs = state->num_hbs;
        best_cost = cost;
      }
    }
  }

  // use the best configuration
  state->lower_id = best_lower_id;
  state->upper_id = best_upper_id;
  state->num_hbs = best_num_hbs;
}

// Runs POET decision engine and requests system changes
void poet_apply_control(poet_state * state) {
  if (getenv(POET_DISABLE_CONTROL) != NULL) {
    return;
  }

  heartbeat_record_t hbr;
  hb_get_current(state->heart, &hbr);
  int period = hb_get_window_size(state->heart);
  real_t hbr_window_rate = hbr_get_window_rate(&hbr);
  real_t hbr_window_power = hbr_get_window_power(&hbr);

  if(state->current_action == 0 && hbr_window_power != R_ZERO) {
    // Read current performance data
    real_t act_speed = hbr_window_rate;

    // Estimate the performance workload
    // estimate time between heartbeats given minimum amount of resources
    real_t time_workload = estimate_base_workload(act_speed,
                                                  state->scs->u,
                                                  state->pfs);

    // Get a new goal speedup to apply to the application
    real_t min_speed = hb_get_min_rate(state->heart);
    real_t max_speed = hb_get_max_rate(state->heart);
    real_t speed_goal = mult(max_speed + min_speed, CONST(0.5));
    calculate_xup(act_speed, speed_goal, time_workload, state->scs);

    // printf("\ntarget rate is %f\n", real_to_db(speed_goal));
    // printf("current rate is %f\n", real_to_db(act_speed));
    // printf("calculated speedup is %f\n\n", real_to_db(state->scs->u));

    // Xup is translated into a system configuration
    // A certain amount of time is assigned to each system configuration
    // in order to achieve the requested Xup
    translate_n2_with_time(state, period);

    logger(state, period, time_workload);
  }

  // Check which speedup should be applied, upper or lower
  int config_id = -1;
  if (state->num_hbs > 0) {
    config_id = state->lower_id;
    state->num_hbs--;
  } else if (state->upper_id >= 0) {
    config_id = state->upper_id;
  }

  if (config_id >= 0 && config_id != state->last_id) {
    if (state->apply != NULL && getenv(POET_DISABLE_APPLY) == NULL) {
      state->apply(state->apply_states, state->num_system_states, config_id,
                   state->last_id);
    }
    state->last_id = config_id;
  }

  state->current_action = (state->current_action + 1) % period;
}
