/* clang-format off */

/* Original design from:
 * =============================================================================
 * XNU kperf/kpc
 * Available for 64-bit Intel/Apple Silicon, macOS/iOS, with root privileges
 *
 * References:
 * XNU source (since xnu 2422.1.72):
 *   https://github.com/apple/darwin-xnu/blob/main/osfmk/kern/kpc.h
 *   https://github.com/apple/darwin-xnu/blob/main/bsd/kern/kern_kpc.c
 *
 * Created by YaoYuan <ibireme@gmail.com> on 2021.
 * Released into the public domain (unlicense.org).
 * =============================================================================
 */

#ifndef COUNTERS_M1CYCLES_H
#define COUNTERS_M1CYCLES_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>           /* for dlopen() and dlsym() */
#include <mach/mach_time.h>  /* for mach_absolute_time() */
#include <sys/kdebug.h>      /* for kdebug trace decode */
#include <sys/sysctl.h>      /* for sysctl() */
#include <unistd.h>          /* for usleep() */

typedef struct {
  double cycles;
  double branches;
  double missed_branches;
  double instructions;
} counters_performance_counters;

static inline counters_performance_counters counters_pc_zero(void) {
  counters_performance_counters r = {0.0, 0.0, 0.0, 0.0};
  return r;
}

static inline counters_performance_counters counters_pc_sub(
    counters_performance_counters a, counters_performance_counters b) {
  counters_performance_counters r;
  r.cycles = a.cycles - b.cycles;
  r.branches = a.branches - b.branches;
  r.missed_branches = a.missed_branches - b.missed_branches;
  r.instructions = a.instructions - b.instructions;
  return r;
}

typedef float f32;
typedef double f64;
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef size_t usize;

/* -----------------------------------------------------------------------------
 * <kperf.framework> header (reverse engineered)
 * -----------------------------------------------------------------------------
 */

#define KPC_CLASS_FIXED (0)
#define KPC_CLASS_CONFIGURABLE (1)
#define KPC_CLASS_POWER (2)
#define KPC_CLASS_RAWPMU (3)

#define KPC_CLASS_FIXED_MASK (1u << KPC_CLASS_FIXED)
#define KPC_CLASS_CONFIGURABLE_MASK (1u << KPC_CLASS_CONFIGURABLE)
#define KPC_CLASS_POWER_MASK (1u << KPC_CLASS_POWER)
#define KPC_CLASS_RAWPMU_MASK (1u << KPC_CLASS_RAWPMU)

#define KPC_PMU_ERROR (0)
#define KPC_PMU_INTEL_V3 (1)
#define KPC_PMU_ARM_APPLE (2)
#define KPC_PMU_INTEL_V2 (3)
#define KPC_PMU_ARM_V2 (4)

#define KPC_MAX_COUNTERS 32

#define KPERF_SAMPLER_TH_INFO (1U << 0)
#define KPERF_SAMPLER_TH_SNAPSHOT (1U << 1)
#define KPERF_SAMPLER_KSTACK (1U << 2)
#define KPERF_SAMPLER_USTACK (1U << 3)
#define KPERF_SAMPLER_PMC_THREAD (1U << 4)
#define KPERF_SAMPLER_PMC_CPU (1U << 5)
#define KPERF_SAMPLER_PMC_CONFIG (1U << 6)
#define KPERF_SAMPLER_MEMINFO (1U << 7)
#define KPERF_SAMPLER_TH_SCHEDULING (1U << 8)
#define KPERF_SAMPLER_TH_DISPATCH (1U << 9)
#define KPERF_SAMPLER_TK_SNAPSHOT (1U << 10)
#define KPERF_SAMPLER_SYS_MEM (1U << 11)
#define KPERF_SAMPLER_TH_INSCYC (1U << 12)
#define KPERF_SAMPLER_TK_INFO (1U << 13)

#define KPERF_ACTION_MAX (32)
#define KPERF_TIMER_MAX (8)

typedef u64 kpc_config_t;

static int (*kpc_cpu_string)(char *buf, usize buf_size);
static u32 (*kpc_pmu_version)(void);
static u32 (*kpc_get_counting)(void);
static int (*kpc_set_counting)(u32 classes);
static u32 (*kpc_get_thread_counting)(void);
static int (*kpc_set_thread_counting)(u32 classes);
static u32 (*kpc_get_config_count)(u32 classes);
static int (*kpc_get_config)(u32 classes, kpc_config_t *config);
static int (*kpc_set_config)(u32 classes, kpc_config_t *config);
static u32 (*kpc_get_counter_count)(u32 classes);
static int (*kpc_get_cpu_counters)(bool all_cpus, u32 classes, int *curcpu,
                                   u64 *buf);
static int (*kpc_get_thread_counters)(u32 tid, u32 buf_count, u64 *buf);
static int (*kpc_force_all_ctrs_set)(int val);
static int (*kpc_force_all_ctrs_get)(int *val_out);
static int (*kperf_action_count_set)(u32 count);
static int (*kperf_action_count_get)(u32 *count);
static int (*kperf_action_samplers_set)(u32 actionid, u32 sample);
static int (*kperf_action_samplers_get)(u32 actionid, u32 *sample);
static int (*kperf_action_filter_set_by_task)(u32 actionid, i32 port);
static int (*kperf_action_filter_set_by_pid)(u32 actionid, i32 pid);
static int (*kperf_timer_count_set)(u32 count);
static int (*kperf_timer_count_get)(u32 *count);
static int (*kperf_timer_period_set)(u32 actionid, u64 tick);
static int (*kperf_timer_period_get)(u32 actionid, u64 *tick);
static int (*kperf_timer_action_set)(u32 actionid, u32 timerid);
static int (*kperf_timer_action_get)(u32 actionid, u32 *timerid);
static int (*kperf_timer_pet_set)(u32 timerid);
static int (*kperf_timer_pet_get)(u32 *timerid);
static int (*kperf_sample_set)(u32 enabled);
static int (*kperf_sample_get)(u32 *enabled);
static int (*kperf_reset)(void);
static u64 (*kperf_ns_to_ticks)(u64 ns);
static u64 (*kperf_ticks_to_ns)(u64 ticks);
static u64 (*kperf_tick_frequency)(void);

/* -----------------------------------------------------------------------------
 * <kperfdata.framework> header (reverse engineered)
 * -----------------------------------------------------------------------------
 */

#define KPEP_ARCH_I386 0
#define KPEP_ARCH_X86_64 1
#define KPEP_ARCH_ARM 2
#define KPEP_ARCH_ARM64 3

typedef struct kpep_event {
  const char *name;
  const char *description;
  const char *errata;
  const char *alias;
  const char *fallback;
  u32 mask;
  u8 number;
  u8 umask;
  u8 reserved;
  u8 is_fixed;
} kpep_event;

typedef struct kpep_db {
  const char *name;
  const char *cpu_id;
  const char *marketing_name;
  void *plist_data;
  void *event_map;
  kpep_event *event_arr;
  kpep_event **fixed_event_arr;
  void *alias_map;
  usize reserved_1;
  usize reserved_2;
  usize reserved_3;
  usize event_count;
  usize alias_count;
  usize fixed_counter_count;
  usize config_counter_count;
  usize power_counter_count;
  u32 architecture;
  u32 fixed_counter_bits;
  u32 config_counter_bits;
  u32 power_counter_bits;
} kpep_db;

typedef struct kpep_config {
  kpep_db *db;
  kpep_event **ev_arr;
  usize *ev_map;
  usize *ev_idx;
  u32 *flags;
  u64 *kpc_periods;
  usize event_count;
  usize counter_count;
  u32 classes;
  u32 config_counter;
  u32 power_counter;
  u32 reserved;
} kpep_config;

typedef enum {
  KPEP_CONFIG_ERROR_NONE = 0,
  KPEP_CONFIG_ERROR_INVALID_ARGUMENT = 1,
  KPEP_CONFIG_ERROR_OUT_OF_MEMORY = 2,
  KPEP_CONFIG_ERROR_IO = 3,
  KPEP_CONFIG_ERROR_BUFFER_TOO_SMALL = 4,
  KPEP_CONFIG_ERROR_CUR_SYSTEM_UNKNOWN = 5,
  KPEP_CONFIG_ERROR_DB_PATH_INVALID = 6,
  KPEP_CONFIG_ERROR_DB_NOT_FOUND = 7,
  KPEP_CONFIG_ERROR_DB_ARCH_UNSUPPORTED = 8,
  KPEP_CONFIG_ERROR_DB_VERSION_UNSUPPORTED = 9,
  KPEP_CONFIG_ERROR_DB_CORRUPT = 10,
  KPEP_CONFIG_ERROR_EVENT_NOT_FOUND = 11,
  KPEP_CONFIG_ERROR_CONFLICTING_EVENTS = 12,
  KPEP_CONFIG_ERROR_COUNTERS_NOT_FORCED = 13,
  KPEP_CONFIG_ERROR_EVENT_UNAVAILABLE = 14,
  KPEP_CONFIG_ERROR_ERRNO = 15,
  KPEP_CONFIG_ERROR_MAX
} kpep_config_error_code;

static const char *kpep_config_error_names[KPEP_CONFIG_ERROR_MAX] = {
    "none",
    "invalid argument",
    "out of memory",
    "I/O",
    "buffer too small",
    "current system unknown",
    "database path invalid",
    "database not found",
    "database architecture unsupported",
    "database version unsupported",
    "database corrupt",
    "event not found",
    "conflicting events",
    "all counters must be forced",
    "event unavailable",
    "check errno"};

static inline const char *kpep_config_error_desc(int code) {
  if (0 <= code && code < KPEP_CONFIG_ERROR_MAX) {
    return kpep_config_error_names[code];
  }
  return "unknown error";
}

static int (*kpep_config_create)(kpep_db *db, kpep_config **cfg_ptr);
static void (*kpep_config_free)(kpep_config *cfg);
static int (*kpep_config_add_event)(kpep_config *cfg, kpep_event **ev_ptr,
                                    u32 flag, u32 *err);
static int (*kpep_config_remove_event)(kpep_config *cfg, usize idx);
static int (*kpep_config_force_counters)(kpep_config *cfg);
static int (*kpep_config_events_count)(kpep_config *cfg, usize *count_ptr);
static int (*kpep_config_events)(kpep_config *cfg, kpep_event **buf,
                                 usize buf_size);
static int (*kpep_config_kpc)(kpep_config *cfg, kpc_config_t *buf,
                              usize buf_size);
static int (*kpep_config_kpc_count)(kpep_config *cfg, usize *count_ptr);
static int (*kpep_config_kpc_classes)(kpep_config *cfg, u32 *classes_ptr);
static int (*kpep_config_kpc_map)(kpep_config *cfg, usize *buf, usize buf_size);
static int (*kpep_db_create)(const char *name, kpep_db **db_ptr);
static void (*kpep_db_free)(kpep_db *db);
static int (*kpep_db_name)(kpep_db *db, const char **name);
static int (*kpep_db_aliases_count)(kpep_db *db, usize *count);
static int (*kpep_db_aliases)(kpep_db *db, const char **buf, usize buf_size);
static int (*kpep_db_counters_count)(kpep_db *db, u8 classes, usize *count);
static int (*kpep_db_events_count)(kpep_db *db, usize *count);
static int (*kpep_db_events)(kpep_db *db, kpep_event **buf, usize buf_size);
static int (*kpep_db_event)(kpep_db *db, const char *name, kpep_event **ev_ptr);
static int (*kpep_event_name)(kpep_event *ev, const char **name_ptr);
static int (*kpep_event_alias)(kpep_event *ev, const char **alias_ptr);
static int (*kpep_event_description)(kpep_event *ev, const char **str_ptr);

/* -----------------------------------------------------------------------------
 * load kperf/kperfdata dynamic library
 * -----------------------------------------------------------------------------
 */

typedef struct {
  const char *name;
  void **impl;
} counters_lib_symbol;

#define counters_lib_nelems(x) (sizeof(x) / sizeof((x)[0]))
#define counters_lib_symbol_def(name) \
  { #name, (void **)&name }

static const counters_lib_symbol counters_lib_symbols_kperf[] = {
    counters_lib_symbol_def(kpc_pmu_version),
    counters_lib_symbol_def(kpc_cpu_string),
    counters_lib_symbol_def(kpc_set_counting),
    counters_lib_symbol_def(kpc_get_counting),
    counters_lib_symbol_def(kpc_set_thread_counting),
    counters_lib_symbol_def(kpc_get_thread_counting),
    counters_lib_symbol_def(kpc_get_config_count),
    counters_lib_symbol_def(kpc_get_counter_count),
    counters_lib_symbol_def(kpc_set_config),
    counters_lib_symbol_def(kpc_get_config),
    counters_lib_symbol_def(kpc_get_cpu_counters),
    counters_lib_symbol_def(kpc_get_thread_counters),
    counters_lib_symbol_def(kpc_force_all_ctrs_set),
    counters_lib_symbol_def(kpc_force_all_ctrs_get),
    counters_lib_symbol_def(kperf_action_count_set),
    counters_lib_symbol_def(kperf_action_count_get),
    counters_lib_symbol_def(kperf_action_samplers_set),
    counters_lib_symbol_def(kperf_action_samplers_get),
    counters_lib_symbol_def(kperf_action_filter_set_by_task),
    counters_lib_symbol_def(kperf_action_filter_set_by_pid),
    counters_lib_symbol_def(kperf_timer_count_set),
    counters_lib_symbol_def(kperf_timer_count_get),
    counters_lib_symbol_def(kperf_timer_period_set),
    counters_lib_symbol_def(kperf_timer_period_get),
    counters_lib_symbol_def(kperf_timer_action_set),
    counters_lib_symbol_def(kperf_timer_action_get),
    counters_lib_symbol_def(kperf_sample_set),
    counters_lib_symbol_def(kperf_sample_get),
    counters_lib_symbol_def(kperf_reset),
    counters_lib_symbol_def(kperf_timer_pet_set),
    counters_lib_symbol_def(kperf_timer_pet_get),
    counters_lib_symbol_def(kperf_ns_to_ticks),
    counters_lib_symbol_def(kperf_ticks_to_ns),
    counters_lib_symbol_def(kperf_tick_frequency),
};

static const counters_lib_symbol counters_lib_symbols_kperfdata[] = {
    counters_lib_symbol_def(kpep_config_create),
    counters_lib_symbol_def(kpep_config_free),
    counters_lib_symbol_def(kpep_config_add_event),
    counters_lib_symbol_def(kpep_config_remove_event),
    counters_lib_symbol_def(kpep_config_force_counters),
    counters_lib_symbol_def(kpep_config_events_count),
    counters_lib_symbol_def(kpep_config_events),
    counters_lib_symbol_def(kpep_config_kpc),
    counters_lib_symbol_def(kpep_config_kpc_count),
    counters_lib_symbol_def(kpep_config_kpc_classes),
    counters_lib_symbol_def(kpep_config_kpc_map),
    counters_lib_symbol_def(kpep_db_create),
    counters_lib_symbol_def(kpep_db_free),
    counters_lib_symbol_def(kpep_db_name),
    counters_lib_symbol_def(kpep_db_aliases_count),
    counters_lib_symbol_def(kpep_db_aliases),
    counters_lib_symbol_def(kpep_db_counters_count),
    counters_lib_symbol_def(kpep_db_events_count),
    counters_lib_symbol_def(kpep_db_events),
    counters_lib_symbol_def(kpep_db_event),
    counters_lib_symbol_def(kpep_event_name),
    counters_lib_symbol_def(kpep_event_alias),
    counters_lib_symbol_def(kpep_event_description),
};

#define counters_lib_path_kperf "/System/Library/PrivateFrameworks/kperf.framework/kperf"
#define counters_lib_path_kperfdata \
  "/System/Library/PrivateFrameworks/kperfdata.framework/kperfdata"

static bool counters_lib_inited = false;
static bool counters_lib_has_err = false;
static char counters_lib_err_msg[256];

static void *counters_lib_handle_kperf = NULL;
static void *counters_lib_handle_kperfdata = NULL;

static inline void counters_lib_deinit(void) {
  counters_lib_inited = false;
  counters_lib_has_err = false;
  if (counters_lib_handle_kperf) dlclose(counters_lib_handle_kperf);
  if (counters_lib_handle_kperfdata) dlclose(counters_lib_handle_kperfdata);
  counters_lib_handle_kperf = NULL;
  counters_lib_handle_kperfdata = NULL;
  for (usize i = 0; i < counters_lib_nelems(counters_lib_symbols_kperf); i++) {
    const counters_lib_symbol *symbol = &counters_lib_symbols_kperf[i];
    *symbol->impl = NULL;
  }
  for (usize i = 0; i < counters_lib_nelems(counters_lib_symbols_kperfdata); i++) {
    const counters_lib_symbol *symbol = &counters_lib_symbols_kperfdata[i];
    *symbol->impl = NULL;
  }
}

static inline bool counters_lib_init(void) {
  if (counters_lib_inited) return !counters_lib_has_err;

  counters_lib_handle_kperf = dlopen(counters_lib_path_kperf, RTLD_LAZY);
  if (!counters_lib_handle_kperf) {
    snprintf(counters_lib_err_msg, sizeof(counters_lib_err_msg),
             "Failed to load kperf.framework, message: %s.", dlerror());
    counters_lib_deinit();
    counters_lib_inited = true;
    counters_lib_has_err = true;
    return false;
  }
  counters_lib_handle_kperfdata = dlopen(counters_lib_path_kperfdata, RTLD_LAZY);
  if (!counters_lib_handle_kperfdata) {
    snprintf(counters_lib_err_msg, sizeof(counters_lib_err_msg),
             "Failed to load kperfdata.framework, message: %s.", dlerror());
    counters_lib_deinit();
    counters_lib_inited = true;
    counters_lib_has_err = true;
    return false;
  }

  for (usize i = 0; i < counters_lib_nelems(counters_lib_symbols_kperf); i++) {
    const counters_lib_symbol *symbol = &counters_lib_symbols_kperf[i];
    *symbol->impl = dlsym(counters_lib_handle_kperf, symbol->name);
    if (!*symbol->impl) {
      snprintf(counters_lib_err_msg, sizeof(counters_lib_err_msg),
               "Failed to load kperf function: %s.", symbol->name);
      counters_lib_deinit();
      counters_lib_inited = true;
      counters_lib_has_err = true;
      return false;
    }
  }
  for (usize i = 0; i < counters_lib_nelems(counters_lib_symbols_kperfdata); i++) {
    const counters_lib_symbol *symbol = &counters_lib_symbols_kperfdata[i];
    *symbol->impl = dlsym(counters_lib_handle_kperfdata, symbol->name);
    if (!*symbol->impl) {
      snprintf(counters_lib_err_msg, sizeof(counters_lib_err_msg),
               "Failed to load kperfdata function: %s.", symbol->name);
      counters_lib_deinit();
      counters_lib_inited = true;
      counters_lib_has_err = true;
      return false;
    }
  }

  counters_lib_inited = true;
  counters_lib_has_err = false;
  return true;
}

#define EVENT_NAME_MAX 8
typedef struct {
  const char *alias;
  const char *names[EVENT_NAME_MAX];
} counters_event_alias;

static const counters_event_alias counters_profile_events[] = {
    {"cycles",
     {
         "FIXED_CYCLES",
         "CPU_CLK_UNHALTED.THREAD",
         "CPU_CLK_UNHALTED.CORE",
     }},
    {"instructions",
     {
         "FIXED_INSTRUCTIONS",
         "INST_RETIRED.ANY"
     }},
    {"branches",
     {
         "INST_BRANCH",
         "BR_INST_RETIRED.ALL_BRANCHES",
         "INST_RETIRED.ANY",
     }},
    {"branch-misses",
     {
         "BRANCH_MISPRED_NONSPEC",
         "BRANCH_MISPREDICT",
         "BR_MISP_RETIRED.ALL_BRANCHES",
         "BR_INST_RETIRED.MISPRED",
     }},
};

static inline kpep_event *counters_get_event(kpep_db *db, const counters_event_alias *alias) {
  for (usize j = 0; j < EVENT_NAME_MAX; j++) {
    const char *name = alias->names[j];
    if (!name) break;
    kpep_event *ev = NULL;
    if (kpep_db_event(db, name, &ev) == 0) {
      return ev;
    }
  }
  return NULL;
}

typedef struct {
  kpc_config_t regs[KPC_MAX_COUNTERS];
  usize counter_map[KPC_MAX_COUNTERS];
  u64 counters_0[KPC_MAX_COUNTERS];
  u64 counters_1[KPC_MAX_COUNTERS];
  bool init;
  bool worked;
} counters_apple_events;

#define counters_apple_ev_count \
  (sizeof(counters_profile_events) / sizeof(counters_profile_events[0]))

static inline bool counters_apple_events_setup(counters_apple_events *self) {
  if (self->init) {
    return self->worked;
  }
  self->init = true;

  if (!counters_lib_init()) {
    printf("Error: %s\n", counters_lib_err_msg);
    self->worked = false;
    return false;
  }

  int force_ctrs = 0;
  if (kpc_force_all_ctrs_get(&force_ctrs)) {
    self->worked = false;
    return false;
  }
  int ret;
  kpep_db *db = NULL;
  if ((ret = kpep_db_create(NULL, &db))) {
    printf("Error: cannot load pmc database: %d.\n", ret);
    self->worked = false;
    return false;
  }

  kpep_config *cfg = NULL;
  if ((ret = kpep_config_create(db, &cfg))) {
    printf("Failed to create kpep config: %d (%s).\n", ret,
           kpep_config_error_desc(ret));
    self->worked = false;
    return false;
  }
  if ((ret = kpep_config_force_counters(cfg))) {
    printf("Failed to force counters: %d (%s).\n", ret,
           kpep_config_error_desc(ret));
    self->worked = false;
    return false;
  }

  kpep_event *ev_arr[counters_apple_ev_count] = {0};
  for (usize i = 0; i < counters_apple_ev_count; i++) {
    const counters_event_alias *alias = counters_profile_events + i;
    ev_arr[i] = counters_get_event(db, alias);
    if (!ev_arr[i]) {
      printf("Cannot find event: %s.\n", alias->alias);
      self->worked = false;
      return false;
    }
  }

  for (usize i = 0; i < counters_apple_ev_count; i++) {
    kpep_event *ev = ev_arr[i];
    if ((ret = kpep_config_add_event(cfg, &ev, 0, NULL))) {
      printf("Failed to add event: %d (%s).\n", ret,
             kpep_config_error_desc(ret));
      self->worked = false;
      return false;
    }
  }

  u32 classes = 0;
  usize reg_count = 0;
  if ((ret = kpep_config_kpc_classes(cfg, &classes))) {
    printf("Failed get kpc classes: %d (%s).\n", ret,
           kpep_config_error_desc(ret));
    self->worked = false;
    return false;
  }
  if ((ret = kpep_config_kpc_count(cfg, &reg_count))) {
    printf("Failed get kpc count: %d (%s).\n", ret,
           kpep_config_error_desc(ret));
    self->worked = false;
    return false;
  }
  if ((ret = kpep_config_kpc_map(cfg, self->counter_map, sizeof(self->counter_map)))) {
    printf("Failed get kpc map: %d (%s).\n", ret,
           kpep_config_error_desc(ret));
    self->worked = false;
    return false;
  }
  if ((ret = kpep_config_kpc(cfg, self->regs, sizeof(self->regs)))) {
    printf("Failed get kpc registers: %d (%s).\n", ret,
           kpep_config_error_desc(ret));
    self->worked = false;
    return false;
  }

  if ((ret = kpc_force_all_ctrs_set(1))) {
    printf("Failed force all ctrs: %d.\n", ret);
    self->worked = false;
    return false;
  }
  if ((classes & KPC_CLASS_CONFIGURABLE_MASK) && reg_count) {
    if ((ret = kpc_set_config(classes, self->regs))) {
      printf("Failed set kpc config: %d.\n", ret);
      self->worked = false;
      return false;
    }
  }

  if ((ret = kpc_set_counting(classes))) {
    printf("Failed set counting: %d.\n", ret);
    self->worked = false;
    return false;
  }
  if ((ret = kpc_set_thread_counting(classes))) {
    printf("Failed set thread counting: %d.\n", ret);
    self->worked = false;
    return false;
  }

  self->worked = true;
  return true;
}

static inline counters_performance_counters counters_apple_events_get(
    counters_apple_events *self) {
  static bool warned = false;
  int ret;
  counters_performance_counters r = {0.0, 0.0, 0.0, 0.0};
  if ((ret = kpc_get_thread_counters(0, KPC_MAX_COUNTERS, self->counters_0))) {
    if (!warned) {
      printf("Failed get thread counters before: %d.\n", ret);
      warned = true;
    }
    r.cycles = 1.0;
    r.branches = 1.0;
    r.missed_branches = 1.0;
    r.instructions = 1.0;
    return r;
  }
  r.cycles = (double)self->counters_0[self->counter_map[0]];
  r.branches = (double)self->counters_0[self->counter_map[2]];
  r.missed_branches = (double)self->counters_0[self->counter_map[3]];
  r.instructions = (double)self->counters_0[self->counter_map[1]];
  return r;
}

#endif
