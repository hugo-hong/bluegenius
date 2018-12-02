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
#ifndef _UTILS_THREAD_H_
#define _UTILS_THREAD_H_

#include <atomic>
#include <stdbool.h>
#include <stdlib.h>

#define THREAD_NAME_MAX       		(16)//PR_SET_NAME limit max name length 16 bytes
#define DEFAULT_WORK_QUEUE_CAPACITY (128)

typedef void(*thread_fn)(void* context, void* arg);

class Reactor;
class FixedQueue;

class Thread {	 
public:
	Thread(const char* name, size_t size = DEFAULT_WORK_QUEUE_CAPACITY);
	~Thread();
	
	void Post(thread_fn func, void* context, void* arg = NULL);
	void Stop();
	void Join();
	bool SetPriority(int priority);
	bool SetRTPriority(int priority);
	bool IsSelf();	
	Reactor* GetReactor() {return m_reactor;}
	FixedQueue* GetWorkqueue() {return m_workqueue;}
	const char* GetName() {return m_name;}
	
protected:
	void New(const char* name, size_t size);
	void Free();	
	void* Run(void* arg);
	
	static void* RunThread(void* arg);
	static void WorkqueueReady(void* context);
private:
	std::atomic<bool> m_isjoined;
	pthread_t m_thread;
	pid_t m_tid;
	char m_name[THREAD_NAME_MAX + 1];
	Reactor* m_reactor;
	FixedQueue* m_workqueue;
};


#endif //_UTILS_THREAD_H_
