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

bool wakelock_acquire(void) {
  pthread_once(&initialized, wakelock_initialize);

  bt_status_t status = BT_STATUS_FAIL;

  if (is_native)
    status = wakelock_acquire_native();
  else
    status = wakelock_acquire_callout();

  update_wakelock_acquired_stats(status);

  if (status != BT_STATUS_SUCCESS)
    LOG_ERROR(LOG_TAG, "%s unable to acquire wake lock: %d", __func__, status);

  return (status == BT_STATUS_SUCCESS);
}
