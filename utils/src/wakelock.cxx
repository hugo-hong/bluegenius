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

#include "bluetooth.h"
#include "utils.h"
#include "singleton.h"
#include "wakelock.h"

const clockid_t Wakelock::kClockId = CLOCK_BOOTTIME;
const char *Wakelock::kWakelockId = "bluetooth_timer";
const std::string Wakelock::kDefaultWakelockPath = "/sys/power/wake_lock";
const std::string Wakelock::kDefaultWakeunlockPath = "/sys/power/wake_unlock";
std::once_flag Wakelock::ms_onceFlag;


void Wakelock::PreInit(void) {
	m_wakelock_path = "";
	m_wakeunlock_path = "";
	m_wakelock_fd = INVALID_FD;
	m_wakeunlock_fd = INVALID_FD;
	m_locked_id_len = -1;
}

bt_status_t Wakelock::Acquire(void) {
	std::call_once(Wakelock::ms_onceFlag, [](Wakelock *thiz) {
		CHECK(thiz != NULL);
		thiz->LazyInit();
	}, this);
	if (m_wakelock_fd == INVALID_FD) {
		LOG_ERROR(LOG_TAG, "unable to acquired lock, invalid fd");
		return BT_STATUS_PARM_INVALID;
	}
	if (m_wakeunlock_fd == INVALID_FD) {
		LOG_ERROR(LOG_TAG, "unable to release lock, invalid fd");
		return BT_STATUS_PARM_INVALID;
	}

	ssize_t lock_name_len = strlen(kWakelockId);
	m_locked_id_len = write(m_wakelock_fd, kWakelockId, lock_name_len);
	if (m_locked_id_len == -1) {
		LOG_ERROR(LOG_TAG, "wake lock not acquired: %s", strerror(errno));
		return BT_STATUS_FAIL;
	}
	else if (m_locked_id_len < lock_name_len) {
		// TODO (jamuraa): this is weird. maybe we should release and retry.
		LOG_WARN(LOG_TAG, "wake lock truncated to %zd chars", m_locked_id_len);
	}

	return BT_STATUS_SUCCESS;
}

bt_status_t Wakelock::Release(void) {
	std::call_once(Wakelock::ms_onceFlag, [](Wakelock *thiz) {
		CHECK(thiz != NULL);
		thiz->LazyInit();
	}, this);

	if (m_wakeunlock_fd == INVALID_FD) {
		LOG_ERROR(LOG_TAG, "%s lock not released, invalid fd", __func__);
		return BT_STATUS_PARM_INVALID;
	}

	ssize_t wrote_name_len = write(m_wakeunlock_fd, kWakelockId, m_locked_id_len);
	if (wrote_name_len == -1) {
		LOG_ERROR(LOG_TAG, "can not release wake lock: %s", strerror(errno));
		return BT_STATUS_FAIL;
	}
	else if (wrote_name_len < m_locked_id_len) {
		// TODO (jamuraa): this is weird. maybe we should release and retry.
		LOG_WARN(LOG_TAG, "wake lock release only %zd chars", m_locked_id_len);
	}

	return BT_STATUS_SUCCESS;
}

void Wakelock::SetPath(const char *lock_path, const char *unlock_path) {
	if (lock_path != NULL) m_wakelock_path = lock_path;
	if (unlock_path != NULL) m_wakeunlock_path = unlock_path;
}

void Wakelock::LazyInit() {
	if (m_wakelock_path.empty())
		m_wakelock_path = kDefaultWakelockPath;
	if (m_wakeunlock_path.empty())
		m_wakeunlock_path = kDefaultWakeunlockPath;

	m_wakelock_fd = open(m_wakelock_path.c_str(), O_RDWR | O_CLOEXEC);
	m_wakeunlock_fd = open(m_wakeunlock_path.c_str(), O_RDWR | O_CLOEXEC);
}

