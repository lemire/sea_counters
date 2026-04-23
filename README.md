# Sea Counters

This project provides a C library to access hardware performance counters (CPU cycles, instructions, etc.) on different platforms (Linux, macOS/Apple Silicon).

## Main Features
- Access to hardware counters via native interfaces (perf events on Linux, kpc on macOS/Apple Silicon)
- Measurement of elapsed time and hardware events for code sections
- Simple and portable C99/C11 interface (header-only)

## Usage

The library is low-level and does not provide the benchmarking code itself.
You are encouraged to build your own benchmarking code on top of it.

As an example, the repository provides a small helper `counters_bench()` to
measure a function pointer. The helper accepts a `counters_bench_parameter`
struct to tune behaviour (pass `NULL` to use the defaults).

```c
#include "counters/bench.h"

static void my_func(void *user_data) {
  /* code to benchmark */
}

int main(void) {
  /* simple usage: defaults */
  counters_event_aggregate agg = counters_bench_function(my_func);

  /* or use parameters to tune the measurement */
  counters_bench_parameter params = counters_bench_parameter_default();
  params.min_repeat = 20;
  params.min_time_ns = 200000000; /* 0.2 s */
  counters_event_aggregate agg2 = counters_bench(my_func, NULL, &params);
  return 0;
}
```

- **Tailoring `counters_bench`**

You can tune the measurement behaviour via the `counters_bench_parameter`
struct.

- `min_repeat`: minimum number of outer iterations (warm-up + measurement).
  Increase when you need more samples or when per-iteration variance is high.
- `min_time_ns`: target minimum warm-up time (nanoseconds). If the warm-up
  time after `min_repeat` is shorter, the outer loop will grow up to
  `max_repeat`. Increase for longer stabilization on complex workloads.
- `max_repeat`: safety cap on the outer loop. Raised if you expect long runs
  or want more samples; keep reasonable to avoid runaway loops.

**This benchmark is not suitable for very short functions.** Each iteration
invokes the callable exactly once through a function pointer, so the
measurement overhead (timer read + indirect call) dominates for
sub-microsecond workloads. For short kernels, wrap multiple repetitions
inside your callable (e.g. an internal loop) and divide the results yourself.

*WARNINGS*:
- It might matter a great deal whether a function is inlineable. Inlining can
  drastically change what is being benchmarked. Here the callable is always
  called through a function pointer, so it will not be inlined into the
  benchmark loop.
- Care should be taken that the call is not optimized away. You can avoid
  such problems by saving results to a `volatile` variable (although not too
  often in a tight loop). You may also want to add synchronization and other
  features.

The `counters_event_aggregate` struct provides aggregate statistics over
multiple `counters_event_count` measurements. Its main accessors are:

- `counters_event_aggregate_elapsed_sec(agg)`: mean elapsed time in seconds
- `counters_event_aggregate_elapsed_ns(agg)`: mean elapsed time in nanoseconds
- `counters_event_aggregate_total_elapsed_ns(agg)`: total elapsed time in nanoseconds
- `counters_event_aggregate_cycles(agg)`: mean CPU cycles
- `counters_event_aggregate_instructions(agg)`: mean instructions
- `counters_event_aggregate_branches(agg)`: mean branches
- `counters_event_aggregate_branch_misses(agg)`: mean branch misses
- `counters_event_aggregate_fastest_elapsed_ns(agg)`: best (minimum) elapsed time
- `counters_event_aggregate_fastest_cycles(agg)`: best cycles
- `counters_event_aggregate_fastest_instructions(agg)`: best instructions
- `counters_event_aggregate_fastest_branches(agg)`: best branches
- `counters_event_aggregate_fastest_branch_misses(agg)`: best branch misses
- `counters_event_aggregate_iteration_count(agg)`: number of iterations

You can use these accessors to analyze performance, for example:

```c
printf("Fastest time (ns): %f\n", counters_event_aggregate_fastest_elapsed_ns(&agg));
printf("Iterations: %d\n", counters_event_aggregate_iteration_count(&agg));
if (counters_has_performance_counters()) {
  double ns = counters_event_aggregate_elapsed_ns(&agg);
  double cyc = counters_event_aggregate_cycles(&agg);
  double inst = counters_event_aggregate_instructions(&agg);
  printf("Mean cycles: %f\n", cyc);
  printf("Mean instructions: %f\n", inst);
  printf(" %f GHz\n", cyc / ns);
  printf(" %f instructions/cycle\n", inst / cyc);
}
```

The performance counters are only available when `counters_has_performance_counters()`
returns true. You may need to run your software with privileged access (sudo)
to get the performance counters.

## CMake

You can add the library as a dependency as follows. Replace `x.y.z` by
the version you want to use.

```cmake
FetchContent_Declare(
  counters
  GIT_REPOSITORY https://github.com/lemire/counters.git
  GIT_TAG vx.y.z
)

FetchContent_MakeAvailable(counters)

target_link_libraries(yourtarget PRIVATE counters::counters)
```

If you use CPM, it is somewhat simpler:

```cmake
include(cmake/CPM.cmake)

CPMAddPackage("gh:lemire/counters#vx.y.z")
target_link_libraries(yourtarget PRIVATE counters::counters)
```

## Citing This Work

If you use this project in a publication or report, please consider citing it.
Replace fields (year, author, url, commit) as appropriate.

```bibtex
@misc{seacounters2026,
  author = {Daniel Lemire},
  title = {{The Sea Counters library: Lightweight performance counters for Linux and macOS (Apple Silicon)}},
  year = {2026},
  howpublished = {GitHub repository},
  note = {https://github.com/lemire/counters}
}
```

## Project Structure
- `include/counters/event_counter.h`: main interface for event measurement
- `include/counters/linux-perf-events.h`: Linux implementation (perf events)
- `include/counters/apple_arm_events.h`: Apple Silicon/macOS implementation
- `include/counters/bench.h`: `counters_bench()` helper and `counters_bench_parameter`
- `CMakeLists.txt`: CMake configuration file
- `README.md`: this documentation and usage examples

Feel free to open an issue or pull request for any improvement or correction.
