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
#ifndef _UTILS_WAKELOCK_H_
#define _UTILS_WAKELOCK_H_

class Wakelock : public Singleton<Wakelock> {
public:  
	void PreInit(void);
	bt_status_t Acquire(void);
	bt_status_t Release(void);
	void SetPath(const char *lock_path = NULL, const char *unlock_path = NULL);	

protected:
	void LazyInit();
    
private:
	static const clockid_t kClockId;
	static const char *kWakelockId;
	static const std::string kDefaultWakelockPath;
	static const std::string kDefaultWakeunlockPath;
	static std::once_flag ms_onceFlag;

	std::string m_wakelock_path;
	std::string m_wakeunlock_path;
	int m_wakelock_fd;
	int m_wakeunlock_fd;
	ssize_t m_locked_id_len;
};

#endif //_UTILS_WAKELOCK_H_
