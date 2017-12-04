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
#include "fixed_queue.h"


FixedQueue::FixedQueue(size_t capacity)
    :m_capacity(0)
    ,m_pList(NULL)
    ,m_pEnqueueEvt(NULL)
    ,m_pDequeueEvt(NULL)
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

bool FixedQueue::IsEmpty() {
    return m_pList->IsEmpty();
}

size_t FixedQueue::GetLength() {
    return m_pList->Length();
}

void FixedQueue::Enqueue(void* data) {
    CHECK(data != NULL);
    
    m_pEnqueueEvt->Wait();
    m_pList->Append(data);     
    m_pDequeueEvt->Post();
}

void* FixedQueue::Dequeue() {
    m_pDequeueEvt->Wait();
    
    void* ret = NULL;
    ret = m_pList->Front();
    m_pList->Remove(ret);
    
    m_pEnqueueEvt->Post();
        
    return ret;
}

bool FixedQueue::TryEnqueue(void* data) {
    if (!m_pEnqueueEvt->TryWait()) return false; 
    
    m_pList->Append(data);
    m_pDequeueEvt->Post();
    
    return true;
}

void* FixedQueue::TryDequeue() {
    if (!m_pDequeueEvt->TryWait()) return NULL;
    
    void* ret = NULL;
    ret = m_pList->Front();
    m_pList->Remove(ret);

    m_pEnqueueEvt->Post();
    
    return ret;
}

void* FixedQueue::PeekFirst() {
    return m_pList->IsEmpty() ? NULL : m_pList->Front();
}

void* FixedQueue::PeekLast() {
    return m_pList->IsEmpty() ? NULL : m_pList->Last();
}

void* FixedQueue::Remove(void* data) {
    bool removed = false;
    
    if (m_pList->Contains(data) &&
        m_pDequeueEvt->TryWait()) {
        removed = m_pList->Remove(data); 
    }    
    if (removed) m_pEnqueueEvt->Post();

    return removed ? data : NULL;
}

int FixedQueue::GetDequeueFd() {
    return m_pDequeueEvt->GetFd();
}

int FixedQueue::GetEnqueueFd() {
    return m_pEnqueueEvt->GetFd();
}

void FixedQueue::New(size_t capacity) {
    m_capacity = capacity;
    m_pList = new SeqList(NULL);
    m_pEnqueueEvt = new Event(capacity);
    m_pDequeueEvt = new Event(0);    
    
    CHECK(m_pList != NULL);
    CHECK(m_pEnqueueEvt != NULL);
    CHECK(m_pDequeueEvt != NULL);
}

void FixedQueue::Free() {
    m_capacity = 0;    
    
    if (m_pList) {
        delete m_pList;
        m_pList = NULL;
    }
    if (m_pEnqueueEvt) {
        delete m_pEnqueueEvt;
        m_pEnqueueEvt = NULL;
    }
    if (m_pDequeueEvt) {
        delete m_pDequeueEvt;
        m_pDequeueEvt = NULL;
    }
}
