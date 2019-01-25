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
#ifndef _UTILS_CONCURRENCY_H_
#define _UTILS_CONCURRENCY_H_
#include <pthread.h>

class Mutex
{
public:
	Mutex();
	~Mutex();

	bool lock();
	bool unlock();

protected:
	friend class Condition;

private:
	pthread_mutex_t m_mutex;
};

class Condition
{
public:
	explicit Condition(Mutex *);
	~Condition();

	void wait();
	void signal();
	void broadcast();

private:
	pthread_cond_t m_cond;
	pthread_mutex_t *m_mutex;
};

class ConCurrency
{
public:
	explicit ConCurrency(int count);
	~ConCurrency();

	void wait();
	void signal();
	void broadcast();

private:
	int m_count;
	Mutex m_mutex;
	Condition m_cond;
};


#endif //_UTILS_CONCURRENCY_H_
