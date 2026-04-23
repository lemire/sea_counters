#ifndef COUNTERS_BENCH_H_
#define COUNTERS_BENCH_H_

#include "counters/event_counter.h"
#include <stddef.h>

/*
 * counters_bench() measures a function pointer `fn` with optional `user_data`.
 *
 * NOTE: This benchmark is NOT suitable for very short functions. Each
 * iteration invokes `fn` exactly once through a function pointer, so the
 * measurement overhead (timer read + indirect call) dominates for
 * sub-microsecond workloads. For short kernels, wrap multiple repetitions
 * inside your callable (e.g. a loop) and divide results yourself.
 */

typedef void (*counters_fn_t)(void *user_data);

typedef struct {
  /* Minimum number of outer iterations (warmup + measurement). */
  size_t min_repeat;
  /* Target minimum warm-up time in nanoseconds. If the total time after
   * min_repeat iterations is less, the loop count grows up to max_repeat. */
  size_t min_time_ns;
  /* Safety cap on the outer loop. */
  size_t max_repeat;
} counters_bench_parameter;

static inline counters_bench_parameter counters_bench_parameter_default(void) {
  counters_bench_parameter p;
  p.min_repeat = 10;
  p.min_time_ns = 400000000; /* 0.4 s */
  p.max_repeat = 1000000;
  return p;
}

static inline counters_event_aggregate counters_bench(counters_fn_t fn,
                                                      void *user_data,
                                                      const counters_bench_parameter *params) {
  counters_bench_parameter p =
      params ? *params : counters_bench_parameter_default();

  static _Thread_local counters_event_collector collector;
  static _Thread_local bool collector_ready = false;
  if (!collector_ready) {
    counters_event_collector_init(&collector);
    collector_ready = true;
  }

  size_t N = p.min_repeat;
  if (N == 0) N = 1;

  counters_event_aggregate warm;
  counters_event_aggregate_init(&warm);
  for (size_t i = 0; i < N; ++i) {
    counters_event_collector_start(&collector);
    fn(user_data);
    counters_event_count c;
    counters_event_collector_end(&collector, &c);
    counters_event_aggregate_add(&warm, &c);
    if ((i + 1 == N) &&
        (counters_event_aggregate_total_elapsed_ns(&warm) < (double)p.min_time_ns) &&
        (N < p.max_repeat)) {
      N *= 10;
    }
  }

  counters_event_aggregate agg;
  counters_event_aggregate_init(&agg);
  for (size_t i = 0; i < N; ++i) {
    counters_event_collector_start(&collector);
    fn(user_data);
    counters_event_count c;
    counters_event_collector_end(&collector, &c);
    counters_event_aggregate_add(&agg, &c);
  }
  agg.has_events = counters_event_collector_has_events(&collector);
  return agg;
}


static inline counters_event_aggregate counters_bench_function(counters_fn_t fn) {
  return counters_bench(fn, NULL, NULL);
}

#endif /* COUNTERS_BENCH_H_ */
