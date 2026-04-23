#include "counters/bench.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

volatile int sink = 0;

static int fib(int n) {
  if (n < 2) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

static void bench_trivial(void *ud) { (void)ud; }

static void bench_simple(void *ud) {
  (void)ud;
  sink++;
}

static void bench_fancy(void *ud) {
  (void)ud;
  volatile int s = 0;
  for (int i = 0; i < 100; ++i) s += i;
  sink += s;
}

static void bench_fib(void *ud) {
  (void)ud;
  volatile int x = fib(20);
  (void)x;
}

typedef struct {
  char *src;
  char *dst;
  size_t size;
} memcpy_ctx;

static void bench_memcpy(void *ud) {
  memcpy_ctx *ctx = (memcpy_ctx *)ud;
  memcpy(ctx->dst, ctx->src, ctx->size);
}

int main(void) {
  if (!counters_has_performance_counters()) {
    printf("Warning: Performance events are not available on this platform. "
           "Maybe use sudo?\n");
  }

  counters_bench_parameter p = counters_bench_parameter_default();

  counters_event_aggregate trivial = counters_bench(bench_trivial, NULL, &p);
  printf("trivial: elapsed_ns=%f total_ns=%f iterations=%d instructions=%f "
         "branches=%f branch_misses=%f\n",
         counters_event_aggregate_elapsed_ns(&trivial),
         counters_event_aggregate_total_elapsed_ns(&trivial),
         counters_event_aggregate_iteration_count(&trivial),
         counters_event_aggregate_instructions(&trivial),
         counters_event_aggregate_branches(&trivial),
         counters_event_aggregate_branch_misses(&trivial));

  counters_event_aggregate simple = counters_bench(bench_simple, NULL, &p);
  printf("simple: elapsed_ns=%f total_ns=%f iterations=%d instructions=%f "
         "branches=%f branch_misses=%f\n",
         counters_event_aggregate_elapsed_ns(&simple),
         counters_event_aggregate_total_elapsed_ns(&simple),
         counters_event_aggregate_iteration_count(&simple),
         counters_event_aggregate_instructions(&simple),
         counters_event_aggregate_branches(&simple),
         counters_event_aggregate_branch_misses(&simple));

  counters_event_aggregate fancy = counters_bench(bench_fancy, NULL, &p);
  printf("fancy: elapsed_ns=%f total_ns=%f iterations=%d instructions=%f "
         "branches=%f branch_misses=%f\n",
         counters_event_aggregate_elapsed_ns(&fancy),
         counters_event_aggregate_total_elapsed_ns(&fancy),
         counters_event_aggregate_iteration_count(&fancy),
         counters_event_aggregate_instructions(&fancy),
         counters_event_aggregate_branches(&fancy),
         counters_event_aggregate_branch_misses(&fancy));

  counters_event_aggregate fib_agg = counters_bench(bench_fib, NULL, &p);
  printf("fib20: elapsed_ns=%f total_ns=%f iterations=%d instructions=%f "
         "branches=%f branch_misses=%f\n",
         counters_event_aggregate_elapsed_ns(&fib_agg),
         counters_event_aggregate_total_elapsed_ns(&fib_agg),
         counters_event_aggregate_iteration_count(&fib_agg),
         counters_event_aggregate_instructions(&fib_agg),
         counters_event_aggregate_branches(&fib_agg),
         counters_event_aggregate_branch_misses(&fib_agg));

  size_t size = 1024 * 1024;
  char *src = (char *)calloc(size, 1);
  char *dst = (char *)calloc(size, 1);
  memcpy_ctx ctx = {src, dst, size};
  counters_event_aggregate mc = counters_bench(bench_memcpy, &ctx, &p);
  double total_ns = counters_event_aggregate_total_elapsed_ns(&mc);
  double speed_gbs = ((double)size *
                      counters_event_aggregate_iteration_count(&mc)) /
                     total_ns;
  printf("memcpy 1MB: elapsed_ns=%f total_ns=%f iterations=%d instructions=%f "
         "branches=%f branch_misses=%f speed=%f GB/s\n",
         counters_event_aggregate_elapsed_ns(&mc), total_ns,
         counters_event_aggregate_iteration_count(&mc),
         counters_event_aggregate_instructions(&mc),
         counters_event_aggregate_branches(&mc),
         counters_event_aggregate_branch_misses(&mc), speed_gbs);
  free(src);
  free(dst);
  return EXIT_SUCCESS;
}
