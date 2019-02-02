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
#define LOG_TAG "utils_threadpool"
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>

#include <stdexcept>

#include "concurrency.h"
#include "threadpool.h"

const int ThreadPool::kMaxThreadNum = 10;

Worker::Worker()
	:m_con(1) {
	Worker(true, false, nullptr);
}

Worker::Worker(bool suspend, bool detached, const char *name)
	: m_con(1)
	, m_tid()
	, m_detached(detached)
	, m_state(CREATING) {
}

Worker::~Worker() {
}

void* Worker::main_loop(void *arg) {
	Worker *thiz = static_cast<Worker *>(arg);
	bool running = true;

	while (running) {
		thiz->m_con.wait();
		switch (thiz->get_state()) {
		case RUNNING:
			thiz->run();
			break;
		case SUSPENDED:
			break;
		case DEAD:
			running = false;
			break;
		}
	}

	return nullptr;
}

bool Worker::start() {
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if (m_detached) {
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	}
	else {
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	}

	int status = pthread_create(&m_tid, &attr, Worker::main_loop,
		static_cast<void *>(this));
	pthread_attr_destroy(&attr);

	return (0 == status);
}

void Worker::stop() {
	set_state(DEAD);
	m_con.signal();
}

bool Worker::join() {
	return (0 == pthread_join(m_tid, NULL));
}

void Worker::suspend() {
	set_state(SUSPENDED);
	m_con.signal();
}

void Worker::schedule() {
	set_state(RUNNING);
	m_con.signal();
}

void Worker::add_task(int task_id, Task *task) {
	std::map<int, Task*>::iterator task_it = m_tasks.find(task_id);
	if (m_tasks.end() == task_it) {
		m_tasks.insert(std::make_pair(task_id, task));
	}
}

void Worker::remove_task(int task_id) {
	std::map<int, Task*>::iterator task_it = m_tasks.find(task_id);
	if (m_tasks.end() != task_it) {
		m_tasks.erase(task_id);
	}
}

void Worker::run() {
	for (auto &item : m_tasks) {
		if (item.second != nullptr)
			item.second->run();
	}
}

int Worker::get_priority() {
	int policy;
	sched_param param;
	pthread_getschedparam(m_tid, &policy, &param);
	return param.sched_priority;
}

void Worker::set_priority(int priority) {
	int policy;
	sched_param param;
	pthread_getschedparam(m_tid, &policy, &param);
	param.sched_priority = priority;
	pthread_setschedparam(m_tid, policy, static_cast<const struct sched_param*>(&param));
}

ThreadPool::ThreadPool(int threads)
	:m_threads()
	,m_mutex() {
	if (threads > kMaxThreadNum) {
		throw std::runtime_error("The initial threads too large");
	}

	for (int i = 0; i < threads; ++i) {
		Worker *w = new Worker();
		m_threads.push_back(std::pair<Worker*, bool>(w, false));
		w->start();
	}
}

ThreadPool::~ThreadPool() {
	terminate();
}

bool ThreadPool::push(Worker* worker) {
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_threads.size() >= kMaxThreadNum)
		return false;

	m_threads.push_back(std::pair<Worker*, bool>(worker, false));
	return true;
}

Worker* ThreadPool::peek() {
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto &item : m_threads) {
		if (item.second == false) {
			item.second = true;
			return item.first;
		}
	}

	return nullptr;
}

void ThreadPool::flush() {
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto &item : m_threads) {
		item.second = false;
		if (item.first->get_state() == Worker::State::RUNNING) {
			item.first->suspend();						
		}
	}
}

bool ThreadPool::pop(Worker* worker) {
	std::lock_guard<std::mutex> lock(m_mutex);

	for (std::vector<std::pair<Worker*, bool>>::iterator it = m_threads.begin();
		it != m_threads.end(); ++it) {
		if (it->first == worker && it->second == true) {
			delete it->first;
			m_threads.erase(it);
			return true;
		}
	}

	return false;
}

void ThreadPool::terminate() {
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto &item : m_threads) {
		item.second = false;
		if (item.first != nullptr) {
			item.first->stop();
			item.first->join();			
		}		
		delete item.first;
	}
	m_threads.clear();
}