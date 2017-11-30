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

typedef struct fixed_queue_t {
  list_t* list;
  event_t* enqueue_evt;
  event_t* dequeue_evt;
  std::mutex* mutex;
  size_t capacity;

  reactor_object_t* dequeue_object;
  fixed_queue_cb dequeue_ready;
  void* dequeue_context;
} fixed_queue_t;


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

void FixedQueue::Flush() {
}

bool FixedQueue::IsEmpty() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pList->IsEmpty();
}

size_t FixedQueue::GetLength() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pList->Length();
}

void FixedQueue::Enqueue(void* data) {
    CHECK(data != NULL);
    
    m_pEnqueueEvt->Wait();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pList->Append(data);
    }
    m_pDequeueEvt->Post();
}

void* FixedQueue::Dequeue() {
    m_pDequeueEvt->Wait();
    
    void* ret = NULL;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ret = m_pList->Pop();        
    }
    m_pEnqueueEvt->Post();
    
    return ret;
}

bool FixedQueue::TryEnqueue(void* data) {
    if (!m_pEnqueueEvt->TryWait()) return false;        
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pList->Append(data);
    }
    m_pDequeueEvt->Post();
    
    return true;
}

void* FixedQueue::TryDequeue() {
    if (!m_pDequeueEvt->TryWait()) return NULL;
    
    void* ret = NULL;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ret = m_pList->Pop();        
    }
    m_pEnqueueEvt->Post();
    
    return ret;
}

void* FixedQueue::PeekFirst() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pList->IsEmpty() ? NULL : m_pList->Front();
}

void* FixedQueue::PeekLast() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pList->IsEmpty() ? NULL : m_pList->Last();
}

void* FixedQueue::Remove(void* data) {
    bool removed = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_pList->Contains(data) &&
           m_pDequeueEvt->TryWait()) {
            removed = m_pList->Remove(data); 
        }
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

void FixedQueue::RegisterDequeue(fixed_queue_cb ready_cb, void* context) {
    m_pfnDequeuReady = ready_cb;
    m_pDequeueContext = context;
}

void FixedQueue::DeregisterDequeue() {
}


void FixedQueue::New(size_t capacity) {
    m_capacity = capacity;
    m_pList = new List(NULL);
    m_pEnqueueEvt = new Event(capacity);
    m_pDequeueEvt = new Event(0);    
    
    CHECK(m_pList != NULL);
    CHECK(m_pEnqueueEvt != NULL);
    CHECK(m_pDequeueEvt != NULL);
}

void FixedQueue::Free() {
    m_capacity = 0;
    
    DeregisterDequeue();
    
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

static void internal_dequeue_ready(void* context) {
  CHECK(context != NULL);
    
  FixedQueue* queue = static_cast<FixedQueue*>(context);
  queue->dequeue_ready(queue, queue->dequeue_context);
}
