#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include "poet.h"
#include "poet_config.h"

#ifndef POET_CONTROL_STATE_CONFIG_FILE
  #define POET_CONTROL_STATE_CONFIG_FILE "/etc/poet/control_config"
#endif
#ifndef POET_CPU_STATE_CONFIG_FILE
  #define POET_CPU_STATE_CONFIG_FILE "/etc/poet/cpu_config"
#endif

/**
 * Get the current number of CPUs allocated for this process.
 */
static inline unsigned int get_current_cpu_count(void) {
  unsigned int curr_cpu_count = 0;
  long i;
  cpu_set_t* cur_mask;
  long num_configured_cpus = sysconf(_SC_NPROCESSORS_CONF);
  if (num_configured_cpus < 0) {
    fprintf(stderr, "get_current_cpu_count: Failed to get the number of "
            "configured CPUs\n");
    return 0;
  }
  cur_mask = CPU_ALLOC(num_configured_cpus);
  if (cur_mask == NULL) {
    fprintf(stderr, "get_current_cpu_count: Failed to alloc cpu_set_t\n");
    return 0;
  }

  if (sched_getaffinity(getpid(), sizeof(cur_mask), cur_mask)) {
    fprintf(stderr, "get_current_cpu_count: Failed to get CPU affinity\n");
  } else {
    for (i = 0; i < num_configured_cpus; i++) {
      if (CPU_ISSET(i, cur_mask)) {
        curr_cpu_count++;
      }
    }
  }
  CPU_FREE(cur_mask);

  return curr_cpu_count;
}

/**
 * Compare the current CPU governor state with the provided one.
 * Returns -1 on failure.
 */
static inline int cpu_governor_cmp(unsigned int cpu, const char* governor) {
  FILE* fp;
  char buffer[128];
  int governor_cmp = -1;
  size_t len;

  snprintf(buffer, sizeof(buffer),
           "/sys/devices/system/cpu/cpu%u/cpufreq/scaling_governor", cpu);
  fp = fopen(buffer, "r");
  if (fp == NULL) {
    fprintf(stderr, "cpu_governor_cmp: Failed to open %s\n", buffer);
    return -1;
  }

  if (fgets(buffer, sizeof(buffer), fp) != NULL) {
    // replace trailing newline
    len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    governor_cmp = strcmp(governor, buffer);
  }
  fclose(fp);

  return governor_cmp;
}

/**
 * Attempt to get the current frequency of the cpu.
 */
static inline unsigned long get_current_cpu_frequency(unsigned int cpu) {
  FILE* fp;
  char buffer[128];
  unsigned long curr_freq = 0;

  snprintf(buffer, sizeof(buffer),
           "/sys/devices/system/cpu/cpu%u/cpufreq/scaling_cur_freq", cpu);
  fp = fopen(buffer, "r");
  if (fp == NULL) {
    fprintf(stderr, "get_current_cpu_frequency: Failed to open %s\n", buffer);
    return 0;
  }

  if (fgets(buffer, sizeof(buffer), fp) != NULL) {
    curr_freq = strtoul(buffer, NULL, 0);
  }
  fclose(fp);

  return curr_freq;
}

// try to get current CPU configuration state
static int get_cpu_state(const poet_cpu_state_t* states,
                         unsigned int num_states,
                         unsigned int* curr_state_id) {
  int ret = -1;
  unsigned int i;
  unsigned int all_userspace = 1;
  unsigned int curr_cpu_count = get_current_cpu_count();
  if (curr_cpu_count == 0) {
    return ret;
  }
  unsigned long* freqs = malloc(curr_cpu_count * sizeof(unsigned long));
  if (freqs == NULL) {
    return ret;
  }

  // these loops work since we use cores in order starting at cpu0
  for (i = 0; i < curr_cpu_count; i++) {
    freqs[i] = get_current_cpu_frequency(i);
  }
  // cores must be in userspace scaling governor, o/w could get a false reading
  for (i = 0; i < curr_cpu_count; i++) {
    if(cpu_governor_cmp(i, "userspace")) {
      all_userspace = 0;
      break;
    }
  }

  if (all_userspace) {
    for (i = 0; i < num_states; i++) {
      // number of cores and the frequency values must match
      if (states[i].cores == (curr_cpu_count - 1)
          && states[i].freq == freqs[states[i].cores]) {
        *curr_state_id = states[i].id;
        ret = 0;
        break;
      }
    }
  }

  free(freqs);
  return ret;
}

int get_current_cpu_state(const void* states,
                          unsigned int num_states,
                          unsigned int* curr_state_id) {
  return get_cpu_state((const poet_cpu_state_t*) states, num_states, curr_state_id);
}

// Set CPU frequency and number of cores using taskset system call
static void apply_cpu_config_taskset(poet_cpu_state_t* cpu_states,
                                     unsigned int num_states,
                                     unsigned int id,
                                     unsigned int last_id) {
  unsigned int i;
  int retvalsyscall = 0;
  char command[4096];

  if (id >= num_states || last_id >= num_states) {
    fprintf(stderr, "apply_cpu_config_taskset: id '%u' or last_id '%u' are not "
            "acceptable values, they must be less than the number of states, "
            "'%u'.\n", id, last_id, num_states);
    return;
  }

  if (cpu_states == NULL) {
    fprintf(stderr, "apply_cpu_config_taskset: cpu_states cannot be null.\n");
    return;
  }

  printf("apply_cpu_config_taskset: Applying state: %u\n", id);

  // only run taskset if number of cores has changed
  if (cpu_states[id].cores != cpu_states[last_id].cores) {
    snprintf(command, sizeof(command),
             "ps -eLf | awk '(/%d/) && (!/awk/) {print $4}' | xargs -n1 taskset -p -c 0-%u",
             getpid(), cpu_states[id].cores);
    printf("apply_cpu_config_taskset: Applying core allocation: %s\n", command);
    retvalsyscall = system(command);
    if (retvalsyscall != 0) {
      fprintf(stderr, "apply_cpu_config_taskset: ERROR running taskset: %d\n",
              retvalsyscall);
    }
  }

  printf("apply_cpu_config_taskset: Applying CPU frequency: %lu\n", cpu_states[id].freq);
  for (i = 0; i <= cpu_states[num_states - 1].cores; i++) {
    snprintf(command, sizeof(command),
             "echo %lu > /sys/devices/system/cpu/cpu%u/cpufreq/scaling_setspeed",
             cpu_states[id].freq, i);
    retvalsyscall = system(command);
    if (retvalsyscall != 0) {
      fprintf(stderr, "apply_cpu_config_taskset: ERROR setting frequencies: %d\n",
              retvalsyscall);
    }
  }
}

void apply_cpu_config(void* states,
                      unsigned int num_states,
                      unsigned int id,
                      unsigned int last_id) {
  apply_cpu_config_taskset((poet_cpu_state_t*) states, num_states, id,
                           last_id);
}

static inline unsigned int get_num_states(FILE* rfile) {
  char line[BUFSIZ];
  unsigned int linenum = 0;
  char id_str[BUFSIZ];
  unsigned int nstates = 0;
  unsigned int nstates_tmp;
  while (fgets(line, BUFSIZ, rfile) != NULL) {
    linenum++;
    if (line[0] == '#') {
      continue;
    }
    if (sscanf(line, "%s", id_str) < 1) {
      fprintf(stderr, "get_num_states: Syntax error, line %u.\n", linenum);
      return 0;
    }
    nstates_tmp = strtoul(id_str, NULL, 0) + 1;
    if (nstates_tmp != nstates + 1) {
      fprintf(stderr, "get_num_states: States are missing or out of order.\n");
      return 0;
    }
    nstates = nstates_tmp;
  }
  return nstates;
}

/* Example file:
  #id   speedup     powerup
  0     1           1
  1     1.206124137 1.084785357
  2     1.387207669 1.196666697
 */
int get_control_states(const char* path,
                       poet_control_state_t** cstates,
                       unsigned int* num_states) {
  poet_control_state_t * states;
  FILE * rfile;
  char line[BUFSIZ];
  unsigned int linenum = 0;
  char argA[BUFSIZ];
  char argB[BUFSIZ];
  char argC[BUFSIZ];
  unsigned int id;

  if (cstates == NULL) {
    fprintf(stderr, "get_control_states: cstates cannot be NULL.\n");
    return -1;
  }

  if (path == NULL) {
    path = POET_CONTROL_STATE_CONFIG_FILE;
  }

  rfile = fopen(path, "r");
  if (rfile == NULL) {
    fprintf(stderr, "get_control_states: Could not open file %s\n", path);
    return -1;
  }

  *num_states = get_num_states(rfile);
  if (*num_states == 0) {
    fclose(rfile);
    return -1;
  }
  rewind(rfile);

  // allocate the space
  states = (poet_control_state_t *) malloc(*num_states * sizeof(poet_control_state_t));
  if (states == NULL) {
    fprintf(stderr, "get_control_states: malloc failed.\n");
    return -1;
  }

  // now iterate again to get the lines and fill in the data structure
  while (fgets(line, BUFSIZ, rfile) != NULL) {
    linenum++;
    if (line[0] == '#') {
      continue;
    }

    if (sscanf(line, "%s %s %s", argA, argB, argC) < 3) {
      fprintf(stderr, "get_control_states: Syntax error, line %u\n", linenum);
      fclose(rfile);
      free(states);
      return -1;
    }
    id = strtoul(argA, NULL, 0);
    states[id].id = id;
    states[id].speedup = atof(argB);
    states[id].cost = atof(argC);
  }

  fclose(rfile);
  *cstates = states;
  return 0;
}

/* Example file:
  #id   freq    cores
  0     300000  0
  1     350000  2
  2     400000  1
 */
int get_cpu_states(const char* path,
                   poet_cpu_state_t** cstates,
                   unsigned int* num_states) {
  poet_cpu_state_t * states;
  FILE * rfile;
  char line[BUFSIZ];
  unsigned int linenum = 0;
  char argA[BUFSIZ];
  char argB[BUFSIZ];
  char argC[BUFSIZ];
  unsigned int id;

  if (cstates == NULL) {
    fprintf(stderr, "get_cpu_states: cstates cannot be NULL.\n");
    return -1;
  }

  if (path == NULL) {
    path = POET_CPU_STATE_CONFIG_FILE;
  }

  rfile = fopen(path, "r");
  if (rfile == NULL) {
    fprintf(stderr, "get_cpu_states: Could not open file %s\n", path);
    return -1;
  }

  *num_states = get_num_states(rfile);
  if (*num_states == 0) {
    fclose(rfile);
    return -1;
  }
  rewind(rfile);

  // allocate the space
  states = (poet_cpu_state_t *) malloc(*num_states * sizeof(poet_cpu_state_t));
  if (states == NULL) {
    fprintf(stderr, "get_cpu_states: malloc failed.\n");
    return -1;
  }

  // now iterate again to get the lines and fill in the data structure
  while (fgets(line, BUFSIZ, rfile) != NULL) {
    linenum++;
    if (line[0] == '#') {
      continue;
    }

    if (sscanf(line, "%s %s %s", argA, argB, argC) < 3) {
      fprintf(stderr, "get_cpu_states: Syntax error, line %u\n", linenum);
      fclose(rfile);
      free(states);
      return -1;
    }
    id = strtoul(argA, NULL, 0);
    states[id].id = id;
    states[id].freq = strtoul(argB, NULL, 0);
    states[id].cores = strtoul(argC, NULL, 0);
  }

  fclose(rfile);
  *cstates = states;
  return 0;
}
