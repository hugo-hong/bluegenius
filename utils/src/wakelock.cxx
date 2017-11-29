/*********************************************************************************
   Bluegenius - Bluetooth host protocol stack for Linux/android/windows...
   Copyright (C) 
   Written 2017 by hugo（yongguang hong） <hugo.08@163.com>
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
   CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS,
   COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS
   SOFTWARE IS DISCLAIMED.
*********************************************************************************/
#define LOG_TAG "utils_wakelock"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <mutex>
#include <string>

// Wakelock statistics for the "bluetooth_timer"
typedef struct {
  bool is_acquired;
  size_t acquired_count;
  size_t released_count;
  size_t acquired_errors;
  size_t released_errors;
  uint64_t min_acquired_interval_ms;
  uint64_t max_acquired_interval_ms;
  uint64_t last_acquired_interval_ms;
  uint64_t total_acquired_interval_ms;
  uint64_t last_acquired_timestamp_ms;
  uint64_t last_released_timestamp_ms;
  uint64_t last_reset_timestamp_ms;
  int last_acquired_error;
  int last_released_error;
} wakelock_stats_t;

static const clockid_t CLOCK_ID = CLOCK_BOOTTIME;
static const char* WAKE_LOCK_ID = "bluetooth_timer";
static const char* DEFAULT_WAKE_LOCK_PATH = "/sys/power/wake_lock";
static const char* DEFAULT_WAKE_UNLOCK_PATH = "/sys/power/wake_unlock";

static pthread_once_t initialized = PTHREAD_ONCE_INIT;
static ssize_t locked_id_len = -1;
static int wake_lock_fd = INVALID_FD;
static int wake_unlock_fd = INVALID_FD;

static void wakelock_initialize(void) {
  reset_wakelock_stats();

  wake_lock_fd = open(DEFAULT_WAKE_LOCK_PATH, O_RDWR | O_CLOEXEC);
  if (wake_lock_fd == INVALID_FD) {
    LOG_ERROR(LOG_TAG, "%s can't open wake lock %s: %s", __func__,
              DEFAULT_WAKE_LOCK_PATH, strerror(errno));
    CHECK(wake_lock_fd != INVALID_FD);
  }

  wake_unlock_fd = open(DEFAULT_WAKE_UNLOCK_PATH, O_RDWR | O_CLOEXEC);
  if (wake_unlock_fd == INVALID_FD) {
    LOG_ERROR(LOG_TAG, "%s can't open wake unlock %s: %s", __func__,
              DEFAULT_WAKE_UNLOCK_PATH, strerror(errno));
    CHECK(wake_unlock_fd != INVALID_FD);
  }
}

void wakelock_cleanup(void) { 
  initialized = PTHREAD_ONCE_INIT;
}

// Reset the Bluetooth wakelock statistics.
// This function is thread-safe.
static void reset_wakelock_stats(void) {
  std::lock_guard<std::mutex> lock(stats_mutex);

  wakelock_stats.is_acquired = false;
  wakelock_stats.acquired_count = 0;
  wakelock_stats.released_count = 0;
  wakelock_stats.acquired_errors = 0;
  wakelock_stats.released_errors = 0;
  wakelock_stats.min_acquired_interval_ms = 0;
  wakelock_stats.max_acquired_interval_ms = 0;
  wakelock_stats.last_acquired_interval_ms = 0;
  wakelock_stats.total_acquired_interval_ms = 0;
  wakelock_stats.last_acquired_timestamp_ms = 0;
  wakelock_stats.last_released_timestamp_ms = 0;
  wakelock_stats.last_reset_timestamp_ms = now();
}



bool wakelock_acquire(void) {
  pthread_once(&initialized, wakelock_initialize);

  bt_status_t status = BT_STATUS_FAIL;

  if (INVALID_FD == wake_lock_fd)
    status = BT_STATUS_PARM_INVALID;
  else {
     long lock_name_len = strlen(WAKE_LOCK_ID);
     locked_id_len = write(wake_lock_fd, WAKE_LOCK_ID, lock_name_len);
     if (locked_id_len == -1) {
     LOG_ERROR(LOG_TAG, "%s wake lock not acquired: %s", __func__,
              strerror(errno));
     status = BT_STATUS_FAIL;
    } else if (locked_id_len < lock_name_len) {
      // TODO (jamuraa): this is weird. maybe we should release and retry.
      LOG_WARN(LOG_TAG, "%s wake lock truncated to %zd chars", __func__,
             locked_id_len);
    }
  }
   
  update_wakelock_acquired_stats(status);

  if (status != BT_STATUS_SUCCESS)
    LOG_ERROR(LOG_TAG, "%s unable to acquire wake lock: %d", __func__, status);

  return (status == BT_STATUS_SUCCESS);
}

bool wakelock_release(void) {
  pthread_once(&initialized, wakelock_initialize);

  bt_status_t status = BT_STATUS_FAIL;
   
  if (INVALID_FD == wake_unlock_fd) {
     status = BT_STATUS_PARM_INVALID;
  }
  else {
      ssize_t wrote_name_len = write(wake_unlock_fd, WAKE_LOCK_ID, locked_id_len);
   if (wrote_name_len == -1) {
    LOG_ERROR(LOG_TAG, "%s can't release wake lock: %s", __func__,
              strerror(errno));
   } else if (wrote_name_len < locked_id_len) {
    LOG_ERROR(LOG_TAG, "%s lock release only wrote %zd, assuming released",
              __func__, wrote_name_len);
   }

  }



  update_wakelock_released_stats(status);

  return (status == BT_STATUS_SUCCESS);
}

//
// Update the Bluetooth acquire wakelock statistics.
//
// This function should be called every time when the wakelock is acquired.
// |acquired_status| is the status code that was return when the wakelock was
// acquired.
// This function is thread-safe.
//
static void update_wakelock_acquired_stats(bt_status_t acquired_status) {
  const uint64_t now_ms = now();

  std::lock_guard<std::mutex> lock(stats_mutex);

  if (acquired_status != BT_STATUS_SUCCESS) {
    wakelock_stats.acquired_errors++;
    wakelock_stats.last_acquired_error = acquired_status;
  }

  if (wakelock_stats.is_acquired) {
    return;
  }

  wakelock_stats.is_acquired = true;
  wakelock_stats.acquired_count++;
  wakelock_stats.last_acquired_timestamp_ms = now_ms;  
}

//
// Update the Bluetooth release wakelock statistics.
//
// This function should be called every time when the wakelock is released.
// |released_status| is the status code that was return when the wakelock was
// released.
// This function is thread-safe.
//
static void update_wakelock_released_stats(bt_status_t released_status) {
  const uint64_t now_ms = now();

  std::lock_guard<std::mutex> lock(stats_mutex);

  if (released_status != BT_STATUS_SUCCESS) {
    wakelock_stats.released_errors++;
    wakelock_stats.last_released_error = released_status;
  }

  if (!wakelock_stats.is_acquired) {
    return;
  }

  wakelock_stats.is_acquired = false;
  wakelock_stats.released_count++;
  wakelock_stats.last_released_timestamp_ms = now_ms;

  // Compute the acquired interval and update the statistics
  uint64_t delta_ms = now_ms - wakelock_stats.last_acquired_timestamp_ms;
  if (delta_ms < wakelock_stats.min_acquired_interval_ms ||
      wakelock_stats.released_count == 1) {
    wakelock_stats.min_acquired_interval_ms = delta_ms;
  }
  if (delta_ms > wakelock_stats.max_acquired_interval_ms) {
    wakelock_stats.max_acquired_interval_ms = delta_ms;
  }
  wakelock_stats.last_acquired_interval_ms = delta_ms;
  wakelock_stats.total_acquired_interval_ms += delta_ms;

}
