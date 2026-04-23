#ifndef COUNTERS_LINUX_PERF_EVENTS_H_
#define COUNTERS_LINUX_PERF_EVENTS_H_
#ifdef __linux__

#include <asm/unistd.h>
#include <errno.h>
#include <linux/perf_event.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#define COUNTERS_MAX_LINUX_EVENTS 8

typedef struct {
  int fd;
  bool working;
  bool last_read_scheduled;
  size_t num_events;
  uint64_t ids[COUNTERS_MAX_LINUX_EVENTS];
  int all_fds[COUNTERS_MAX_LINUX_EVENTS];
  size_t all_fds_count;
  uint64_t temp_result[3 + 2 * COUNTERS_MAX_LINUX_EVENTS];
} counters_linux_events;

static inline void counters_linux_events_cleanup_fds(counters_linux_events *ev) {
  for (size_t i = 0; i < ev->all_fds_count; ++i) close(ev->all_fds[i]);
  ev->all_fds_count = 0;
  ev->fd = -1;
}

static inline bool counters_linux_events_try_open(counters_linux_events *ev,
                                                  const int *configs, size_t n) {
  ev->working = true;
  ev->fd = -1;
  ev->all_fds_count = 0;
  ev->num_events = n;
  for (size_t i = 0; i < n; ++i) ev->ids[i] = 0;

  struct perf_event_attr attribs;
  memset(&attribs, 0, sizeof(attribs));
  attribs.type = PERF_TYPE_HARDWARE;
  attribs.size = sizeof(attribs);
  attribs.disabled = 1;
  attribs.exclude_kernel = 1;
  attribs.exclude_hv = 1;
  attribs.sample_period = 0;
  attribs.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID |
                        PERF_FORMAT_TOTAL_TIME_ENABLED |
                        PERF_FORMAT_TOTAL_TIME_RUNNING;

  int group = -1;
  for (size_t i = 0; i < n; ++i) {
    attribs.config = (uint64_t)configs[i];
    int _fd = (int)syscall(__NR_perf_event_open, &attribs, 0, -1, group, 0UL);
    if (_fd == -1) {
      ev->working = false;
      return false;
    }
    ev->all_fds[ev->all_fds_count++] = _fd;
    ioctl(_fd, PERF_EVENT_IOC_ID, &ev->ids[i]);
    if (group == -1) {
      group = _fd;
      ev->fd = _fd;
    }
  }
  return true;
}

static inline bool counters_linux_events_probe_scheduling(counters_linux_events *ev) {
  if (ev->fd == -1) return false;
  if (ioctl(ev->fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) == -1) return false;
  if (ioctl(ev->fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) == -1) return false;

  volatile int sink = 0;
  for (int i = 0; i < 10000; ++i) sink += i;

  if (ioctl(ev->fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP) == -1) return false;
  size_t bytes = (3 + 2 * ev->num_events) * sizeof(uint64_t);
  if (read(ev->fd, ev->temp_result, bytes) == -1) return false;
  uint64_t time_enabled = ev->temp_result[1];
  uint64_t time_running = ev->temp_result[2];
  return time_running > 0 && time_running == time_enabled;
}

static inline void counters_linux_events_init(counters_linux_events *ev,
                                              const int *configs, size_t n) {
  memset(ev, 0, sizeof(*ev));
  ev->fd = -1;

  if (n > COUNTERS_MAX_LINUX_EVENTS) n = COUNTERS_MAX_LINUX_EVENTS;
  size_t cur = n;
  while (cur > 0) {
    if (!counters_linux_events_try_open(ev, configs, cur)) {
      counters_linux_events_cleanup_fds(ev);
      --cur;
      continue;
    }
    if (counters_linux_events_probe_scheduling(ev)) {
      ev->working = true;
      ev->num_events = cur;
      return;
    }
    counters_linux_events_cleanup_fds(ev);
    --cur;
  }
  ev->working = false;
}

static inline void counters_linux_events_destroy(counters_linux_events *ev) {
  counters_linux_events_cleanup_fds(ev);
}

static inline void counters_linux_events_start(counters_linux_events *ev) {
  if (ev->fd != -1) {
    if (ioctl(ev->fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) == -1) {
      ev->working = false;
    }
    if (ioctl(ev->fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) == -1) {
      ev->working = false;
    }
  }
}

static inline void counters_linux_events_end(counters_linux_events *ev,
                                             unsigned long long *results) {
  ev->last_read_scheduled = false;
  if (ev->fd == -1) return;

  if (ioctl(ev->fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP) == -1) {
    ev->working = false;
  }
  size_t bytes = (3 + 2 * ev->num_events) * sizeof(uint64_t);
  if (read(ev->fd, ev->temp_result, bytes) == -1) {
    ev->working = false;
    return;
  }

  uint64_t nr = ev->temp_result[0];
  uint64_t time_enabled = ev->temp_result[1];
  uint64_t time_running = ev->temp_result[2];

  ev->last_read_scheduled = (time_running > 0) && (time_running == time_enabled);

  for (uint64_t i = 0; i < nr; ++i) {
    uint64_t value = ev->temp_result[3 + 2 * i];
    uint64_t id = ev->temp_result[3 + 2 * i + 1];
    results[i] = value;
    if (ev->ids[i] != id) ev->working = false;
  }
}

static inline bool counters_linux_events_is_working(const counters_linux_events *ev) {
  return ev->working;
}

#endif /* __linux__ */
#endif /* COUNTERS_LINUX_PERF_EVENTS_H_ */
