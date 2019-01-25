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
#define LOG_TAG "utils_concurrency"

#include "concurrency.h"

Mutex::Mutex()
	:m_mutex() {
	pthread_mutex_init(&m_mutex, nullptr);
}

Mutex::~Mutex() {
	unlock();
	pthread_mutex_destroy(&m_mutex);
}

bool Mutex::lock() {
	if (0 != pthread_mutex_trylock(&m_mutex))
		pthread_mutex_unlock(&m_mutex);

	return (0 == pthread_mutex_trylock(&m_mutex));
}

bool Mutex::unlock() {
	return (0 == pthread_mutex_unlock(&m_mutex));
}

Condition::Condition(Mutex *mutex)
	:m_cond()
	,m_mutex(&(mutex->m_mutex)){
	pthread_cond_init(&m_cond, nullptr);
}

Condition::~Condition() {
}

void Condition::wait() {
	pthread_cond_wait(&m_cond, m_mutex);
}

void Condition::signal() {
	pthread_cond_signal(&m_cond);
}

void Condition::broadcast() {
	pthread_cond_broadcast(&m_cond);
}

ConCurrency::ConCurrency(int count)
	:m_count(count)
	,m_mutex()
	,m_cond(&m_mutex)
{
	//do nothing
}

ConCurrency::~ConCurrency() {
}

void ConCurrency::wait() {
	m_mutex.lock();
	--m_count;
	while (m_count < 0)	{
		m_cond.wait();
	}
	m_mutex.unlock();
}

void ConCurrency::signal() {
	m_mutex.lock();
	if (++m_count >= 0) {
		m_cond.signal();
	}
	m_mutex.unlock();
}

void ConCurrency::broadcast() {
	m_mutex.lock();
	if (++m_count >= 0) {
		m_cond.broadcast();
	}
	m_mutex.unlock();
}