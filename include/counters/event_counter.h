#ifndef COUNTERS__EVENT_COUNTER_H
#define COUNTERS__EVENT_COUNTER_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "counters/linux-perf-events.h"

#if defined(__APPLE__) && defined(__aarch64__)
#include "counters/apple_arm_events.h"
#endif

#define COUNTERS_NUM_EVENTS 5

enum {
  COUNTERS_CPU_CYCLES = 0,
  COUNTERS_INSTRUCTIONS = 1,
  COUNTERS_BRANCH = 2,
  COUNTERS_BRANCH_MISSES = 3
};

typedef struct {
  double elapsed_ns;
  unsigned long long event_counts[COUNTERS_NUM_EVENTS];
} counters_event_count;

static inline counters_event_count counters_event_count_zero(void) {
  counters_event_count c;
  c.elapsed_ns = 0.0;
  for (int i = 0; i < COUNTERS_NUM_EVENTS; ++i) c.event_counts[i] = 0;
  return c;
}

static inline double counters_event_count_elapsed_sec(const counters_event_count *c) {
  return c->elapsed_ns / 1e9;
}
static inline double counters_event_count_elapsed_ns(const counters_event_count *c) {
  return c->elapsed_ns;
}
static inline double counters_event_count_cycles(const counters_event_count *c) {
  return (double)c->event_counts[COUNTERS_CPU_CYCLES];
}
static inline double counters_event_count_instructions(const counters_event_count *c) {
  return (double)c->event_counts[COUNTERS_INSTRUCTIONS];
}
static inline double counters_event_count_branches(const counters_event_count *c) {
  return (double)c->event_counts[COUNTERS_BRANCH];
}
static inline double counters_event_count_branch_misses(const counters_event_count *c) {
  return (double)c->event_counts[COUNTERS_BRANCH_MISSES];
}

static inline void counters_event_count_add(counters_event_count *dst,
                                            const counters_event_count *other) {
  dst->elapsed_ns += other->elapsed_ns;
  for (int i = 0; i < COUNTERS_NUM_EVENTS; ++i) {
    dst->event_counts[i] += other->event_counts[i];
  }
}

typedef struct {
  bool has_events;
  int iterations;
  counters_event_count total;
  counters_event_count best;
  counters_event_count worst;
} counters_event_aggregate;

static inline void counters_event_aggregate_init(counters_event_aggregate *a) {
  a->has_events = false;
  a->iterations = 0;
  a->total = counters_event_count_zero();
  a->best = counters_event_count_zero();
  a->worst = counters_event_count_zero();
}

static inline void counters_event_aggregate_add(counters_event_aggregate *a,
                                                const counters_event_count *c) {
  if (a->iterations == 0 || c->elapsed_ns < a->best.elapsed_ns) {
    a->best = *c;
  }
  if (a->iterations == 0 || c->elapsed_ns > a->worst.elapsed_ns) {
    a->worst = *c;
  }
  a->iterations++;
  counters_event_count_add(&a->total, c);
}

static inline double counters_event_aggregate_elapsed_sec(const counters_event_aggregate *a) {
  return counters_event_count_elapsed_sec(&a->total) / a->iterations;
}
static inline double counters_event_aggregate_total_elapsed_ns(const counters_event_aggregate *a) {
  return counters_event_count_elapsed_ns(&a->total);
}
static inline double counters_event_aggregate_elapsed_ns(const counters_event_aggregate *a) {
  return counters_event_count_elapsed_ns(&a->total) / a->iterations;
}
static inline double counters_event_aggregate_cycles(const counters_event_aggregate *a) {
  return counters_event_count_cycles(&a->total) / a->iterations;
}
static inline double counters_event_aggregate_instructions(const counters_event_aggregate *a) {
  return counters_event_count_instructions(&a->total) / a->iterations;
}
static inline double counters_event_aggregate_branches(const counters_event_aggregate *a) {
  return counters_event_count_branches(&a->total) / a->iterations;
}
static inline double counters_event_aggregate_branch_misses(const counters_event_aggregate *a) {
  return counters_event_count_branch_misses(&a->total) / a->iterations;
}
static inline double counters_event_aggregate_fastest_elapsed_ns(const counters_event_aggregate *a) {
  return counters_event_count_elapsed_ns(&a->best);
}
static inline double counters_event_aggregate_fastest_cycles(const counters_event_aggregate *a) {
  return counters_event_count_cycles(&a->best);
}
static inline double counters_event_aggregate_fastest_instructions(const counters_event_aggregate *a) {
  return counters_event_count_instructions(&a->best);
}
static inline double counters_event_aggregate_fastest_branches(const counters_event_aggregate *a) {
  return counters_event_count_branches(&a->best);
}
static inline double counters_event_aggregate_fastest_branch_misses(const counters_event_aggregate *a) {
  return counters_event_count_branch_misses(&a->best);
}
static inline int counters_event_aggregate_iteration_count(const counters_event_aggregate *a) {
  return a->iterations;
}

typedef struct {
  counters_event_count count;
  struct timespec start_clock;
  bool initialized;
#if defined(__linux__)
  counters_linux_events linux_events;
#elif defined(__APPLE__) && defined(__aarch64__)
  counters_apple_events apple_events;
  counters_performance_counters diff;
#endif
} counters_event_collector;

static inline void counters_event_collector_init(counters_event_collector *c) {
  memset(c, 0, sizeof(*c));
  c->count = counters_event_count_zero();
  c->initialized = true;
#if defined(__linux__)
  int configs[] = {
      PERF_COUNT_HW_CPU_CYCLES,
      PERF_COUNT_HW_INSTRUCTIONS,
      PERF_COUNT_HW_BRANCH_INSTRUCTIONS,
      PERF_COUNT_HW_BRANCH_MISSES,
  };
  counters_linux_events_init(&c->linux_events, configs,
                             sizeof(configs) / sizeof(configs[0]));
#elif defined(__APPLE__) && defined(__aarch64__)
  c->diff = counters_pc_zero();
  counters_apple_events_setup(&c->apple_events);
#endif
}

static inline void counters_event_collector_destroy(counters_event_collector *c) {
#if defined(__linux__)
  counters_linux_events_destroy(&c->linux_events);
#else
  (void)c;
#endif
}

static inline bool counters_event_collector_has_events(counters_event_collector *c) {
#if defined(__linux__)
  return counters_linux_events_is_working(&c->linux_events);
#elif defined(__APPLE__) && defined(__aarch64__)
  return counters_apple_events_setup(&c->apple_events);
#else
  (void)c;
  return false;
#endif
}

static inline void counters_event_collector_start(counters_event_collector *c) {
#if defined(__linux__)
  counters_linux_events_start(&c->linux_events);
#elif defined(__APPLE__) && defined(__aarch64__)
  if (counters_event_collector_has_events(c)) {
    c->diff = counters_apple_events_get(&c->apple_events);
  }
#endif
  clock_gettime(CLOCK_MONOTONIC, &c->start_clock);
}

static inline void counters_event_collector_end(counters_event_collector *c,
                                                counters_event_count *out) {
  struct timespec end_clock;
  clock_gettime(CLOCK_MONOTONIC, &end_clock);
#if defined(__linux__)
  counters_linux_events_end(&c->linux_events, c->count.event_counts);
#elif defined(__APPLE__) && defined(__aarch64__)
  if (counters_event_collector_has_events(c)) {
    counters_performance_counters end = counters_apple_events_get(&c->apple_events);
    c->diff = counters_pc_sub(end, c->diff);
  }
  c->count.event_counts[0] = (unsigned long long)c->diff.cycles;
  c->count.event_counts[1] = (unsigned long long)c->diff.instructions;
  c->count.event_counts[2] = (unsigned long long)c->diff.branches;
  c->count.event_counts[3] = (unsigned long long)c->diff.missed_branches;
#endif
  double ns = (double)(end_clock.tv_sec - c->start_clock.tv_sec) * 1e9 +
              (double)(end_clock.tv_nsec - c->start_clock.tv_nsec);
  c->count.elapsed_ns = ns;
  *out = c->count;
}

static inline bool counters_has_performance_counters(void) {
  counters_event_collector c;
  counters_event_collector_init(&c);
  bool ok = counters_event_collector_has_events(&c);
  counters_event_collector_destroy(&c);
  return ok;
}

#endif
