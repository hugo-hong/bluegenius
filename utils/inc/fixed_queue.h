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
#ifndef _UTILS_FIXED_QUEUE_H_
#define _UTILS_FIXED_QUEUE_H_

class EventLock;
class SeqList;

typedef void(*fixed_queue_free_cb)(void* data);

class FixedQueue {	
public:
    FixedQueue(size_t capacity);
    ~FixedQueue();
    
    void Flush(fixed_queue_free_cb free_cb = NULL);
	bool IsEmpty() { return m_list->IsEmpty(); }
	size_t GetLength() { return m_list->Size(); }
    size_t GetCapcity() {return m_capacity;}
    SeqList* GetList() {return m_list;}
    void Enqueue(void* data);
    void* Dequeue();
    bool TryEnqueue(void* data);
    void* TryDequeue();
    void* PeekFirst();
    void* PeekLast();
	void Push(void* data);
	void* Pull();
    void* Remove(void* data);
    int GetEnqueueFd();
    int GetDequeueFd();
   
protected:
    void New(size_t capacity);
    void Free();    
    
private:
    SeqList *m_list;
	EventLock *m_enqueue_evt;
	EventLock *m_dequeue_evt;
    size_t m_capacity;    
};

#endif //_UTILS_FIXED_QUEUE_H_