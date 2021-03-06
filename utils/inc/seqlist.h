/********************************************************************************
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
#ifndef _UTILS_SEQLIST_H_
#define _UTILS_SEQLIST_H_
#include <mutex>
#include <atomic>
#include <stdbool.h>
#include <stdlib.h>

struct list_node_t {
    struct list_node_t* next;
    void* data;
};

typedef void (*list_free_cb)(void* data);

class SeqList {
public:
    SeqList(list_free_cb pfnFree);
    ~SeqList();
    
    void* Front(void);
    void* Last(void);
    bool Remove(void* data);
    bool Append(void* data);   
    bool Insert(void* data, list_node_t* preNode);
    void Clear();
    list_node_t* Begin();
    list_node_t* End();
    size_t Size();
    bool IsEmpty();
    bool Contains(void* data);
    
protected:
    void New(list_free_cb pfnFree);
    void Free();
    list_node_t* FreeNode(list_node_t* node);    
    
private:
    std::mutex* m_mutex;
    list_node_t* m_pHead;
    list_node_t* m_pTail;
    size_t m_length;
    list_free_cb m_pfnFree;

};


#endif //_UTILS_SEQLIST_H_
