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
#include "seqlist.h"

SeqList::SeqList (list_free_cb pfnFree)
    :m_mutex(NULL)
    ,m_pHead(NULL)
    ,m_pTail(NULL)
    ,m_length(0)
    ,m_pfnFree(NULL)
{
    New(pfnFree);
}

SeqList::~SeqList () {
    Free();
}

bool SeqList::IsEmpty() {
    std::lock_guard<std::mutex> lock(*m_mutex);
    return (m_length == 0);
}

bool SeqList::Contains(void* data) {
    std::lock_guard<std::mutex> lock(*m_mutex);
    
    for (const list_node_t* node = m_pHead; node != NULL; node = node->next) {
        if (node->data == data) return true;
    }
    return false;
}

void* SeqList::Front() {
    std::lock_guard<std::mutex> lock(*m_mutex);
    return (m_pHead == NULL ? NULL : m_pHead->data);
}

void* SeqList::Last() {
    std::lock_guard<std::mutex> lock(*m_mutex);
    return (m_pTail == NULL ? NULL : m_pTail->data);
}

void* SeqList::Remove(void* data) {
    std::lock_guard<std::mutex> lock(*m_mutex);
    
    list_node_t *Prev = m_pHead, *Next = m_pHead;   
    while (Next != NULL) {
        if (Next->data == data) {
            Prev->next = FreeNode(Next);
            if (Next == m_pHead) {
                m_pHead = Prev->next;
            }
            if (Next == m_pTail) {
                m_pTail = Prev;
            }
            return true;
        }
        Prev = Next;
        Next = Next->next;
    }
    
    return false;
}

void SeqList::New(list_free_cb pfnFree) {
    m_pfnFree = pfnFree;
    m_mutex = new std::mutex;
}

void SeqList::Free() {
    clear();    
    if (m_mutex) {
        delete m_mutex;
        m_mutex = NULL;
    }
}

list_node_t* SeqList::FreeNode(list_node_t* node) {
    CHECK(node != NULL);
    
    list_node_t* next = node->next;
    if (m_pfnFree) m_pfnFree(node->data);
    m_length--;
    
    return next;
}
