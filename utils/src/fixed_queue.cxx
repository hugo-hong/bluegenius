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
#include <mutex>

#include "utils.h"
#include "eventlock.h"
#include "seqlist.h"
#include "fixed_queue.h"

FixedQueue::FixedQueue(size_t capacity)
    :m_capacity(0)
    ,m_list(NULL)
    ,m_enqueue_evt(NULL)
    ,m_dequeue_evt(NULL)
{
   New(capacity);
}

FixedQueue::~FixedQueue() {
    Free();
}

void FixedQueue::Flush(fixed_queue_free_cb free_cb) {
    while (!IsEmpty()) {
        void* data = TryDequeue();
        if (free_cb) free_cb(data);
    }
}

void FixedQueue::Enqueue(void* data) {
    CHECK(data != NULL);
    
	m_enqueue_evt->Wait();
    m_list->Append(data);     
    m_dequeue_evt->Post();
}

void* FixedQueue::Dequeue() {
    m_dequeue_evt->Wait();
    
    void* ret = NULL;
    ret = m_list->Front();
    m_list->Remove(ret);
    
    m_enqueue_evt->Post();
        
    return ret;
}

bool FixedQueue::TryEnqueue(void* data) {
    if (!m_enqueue_evt->TryWait()) 
		return false; 
    
    m_list->Append(data);
    m_dequeue_evt->Post();
    
    return true;
}

void* FixedQueue::TryDequeue() {
    if (!m_dequeue_evt->TryWait()) return NULL;
    
    void* ret = NULL;
    ret = m_list->Front();
    m_list->Remove(ret);

    m_enqueue_evt->Post();
    
    return ret;
}

void* FixedQueue::PeekFirst() {
    return m_list->IsEmpty() ? NULL : m_list->Front();
}

void* FixedQueue::PeekLast() {
    return m_list->IsEmpty() ? NULL : m_list->Last();
}

void FixedQueue::Push(void* data) {
	CHECK(data != NULL);

	m_enqueue_evt->Wait();
	m_list->Insert(data, NULL);
	m_dequeue_evt->Post();
}

void* FixedQueue::Pull() {
	m_dequeue_evt->Wait();

	void* ret = NULL;
	ret = m_list->Last();
	m_list->Remove(ret);
	m_enqueue_evt->Post();

	return ret;
}

void* FixedQueue::Remove(void* data) {
    bool removed = false;
    
    if (m_list->Contains(data) &&
        m_dequeue_evt->TryWait()) {
        removed = m_list->Remove(data); 
    }    
    if (removed) m_enqueue_evt->Post();

    return removed ? data : NULL;
}

int FixedQueue::GetDequeueFd() {
    return m_dequeue_evt->GetFd();
}

int FixedQueue::GetEnqueueFd() {
    return m_enqueue_evt->GetFd();
}

void FixedQueue::New(size_t capacity) {
    m_capacity = capacity;
    m_list = new SeqList(NULL);
    m_enqueue_evt = new EventLock(capacity);
    m_dequeue_evt = new EventLock(0);
    
    CHECK(m_list != NULL);
    CHECK(m_enqueue_evt != NULL);
    CHECK(m_dequeue_evt != NULL);
}

void FixedQueue::Free() {
    m_capacity = 0;    
    
    if (m_list) {
        delete m_list;
        m_list = NULL;
    }
    if (m_enqueue_evt) {
        delete m_enqueue_evt;
        m_enqueue_evt = NULL;
    }
    if (m_dequeue_evt) {
        delete m_dequeue_evt;
        m_dequeue_evt = NULL;
    }
}
