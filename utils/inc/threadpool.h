/*********************************************************************************
   Bluegenius - Bluetooth host protocol stack for Linux/android/windows...
   Copyright (C)
   Written 2017 by hugo£¨yongguang hong£© <hugo.08@163.com>
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
#ifndef _UTIL_THREADPOOL_H_
#define _UTIL_THREADPOOL_H_
#include <map>
#include <vector>
#include <pthread.h>

class ConCurrency;

/**
 * \brief The base task class, all tasks inherit from this class
 */
class Task {
public:
	enum Priority {
		LOW,
		NORMAL,
		HIGH
	};
	Task(Priority p = NORMAL, void *arg = nullptr)
		:m_priority(p) {}
	~Task() {}

	virtual void run() = 0;

private:
	Priority m_priority;
};

class Worker {
public:
	enum State {
		CREATING,
		RUNNING,
		SUSPENDED,
		DEAD
	};
	Worker();
	Worker(bool suspend, bool detached, const char *name);
	~Worker();
	
	bool start();
	void stop();
	bool join();
	void suspend();
	void schedule();
	void add_task(int task_id, Task *task);
	void remove_task(int task_id);
	int get_priority();
	void set_priority(int priority);
	State get_state() { return m_state; }
	void set_state(State state) { m_state = state; }
protected:
	void run();
	static void *main_loop(void *arg);

private:
	bool m_detached;
	State m_state;
	pthread_t m_tid;
	ConCurrency m_con;
	std::map<int, Task*> m_tasks;
};

class ThreadPool {
public:
	ThreadPool(int threads);
	~ThreadPool();

	bool push(Worker* worker);
	Worker* peek();
	void flush();

protected:
	void terminate();

private:
	static const int kMaxThreadNum;
	std::vector<std::pair<Worker*, bool>> m_threads;

};



#endif //_UTIL_THREADPOOL_H_
