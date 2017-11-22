/*
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
*/ 
#ifndef _THREAD_PROXY_H_
#define _THREAD_PROXY_H_

#include <stdbool.h>
#include <stdlib.h>

#define THREAD_NAME_MAX       (16)//PR_SET_NAME limit max name length 16 bytes

typedef void (*thread_fn)(void* context);


/*********************************************************************
	Name:thread_new
	Para:name,Only THREAD_NAME_MAX bytes from |name| will be assigned
       to the newly-created thread			
	Return: Returns a thread object if successfully, NULL otherwise
	Func:create new thread for given name	
*********************************************************************/
thread_t* thread_new(const char* name);

/*********************************************************************
	Name:thread_free
	Para:thread, the thread object to free      	
	Return: none
	Func: free the given thread.if the thread is running, it is stopped
        and the calling thread will block until thread terminates.
*********************************************************************/
void thread_free(thread_t* thread);

/*********************************************************************
	Name:thread_join
	Para:thread, the thread object to be join      	
	Return: none
	Func: waits for thead stop.
*********************************************************************/
void thread_join(thread_t* thread);

/*********************************************************************
	Name:thread_post
	Para:thread, the thread object;func, the function to be excute; 
        context, func parameter.
	Return: true on success, otherwise false.
	Func: call func with argument context. this function typically does 
        not block unless there are excessive number of functions posted 
        to thread that have not been dispatched yet.
*********************************************************************/
bool thread_post(thread_t* thread, thread_fn func, void* context);

/*********************************************************************
	Name:thread_stop
	Para:thread, the thread object to be stop.
	Return: none.
	Func: requests thread to stop. 
        this function is guaranteed to not block.
*********************************************************************/
void thread_stop(thread_t* thread);

/*********************************************************************
	Name:thread_set_priority
	Para:thread, the thread object;priority, the priority to be set.        
	Return: true on success, otherwise false.
	Func: attempts to set the priority of the gien thread.
*********************************************************************/
bool thread_set_priority(thread_t* thread, int priority);

/*********************************************************************
	Name:thread_set_rt_priority
	Para:thread, the thread object;priority, the priority to be set.        
	Return: true on success, otherwise false.
	Func: attempts to set thread to the real-time SCHED_FIFO priority.
*********************************************************************/
bool thread_set_rt_priority(thread_t* thread, int priority);

#endif //_THREAD_PROXY_H_
