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

#define LOG_TAG "statemachine"

#include "statemachine.h"

StateMachine :: StateMachine() {
	m_pMainThread = NULL;
	
}

StateMachine :: ~StateMachine() {
	
}

void StateMachine :: Start(int priority) {
	m_pMainThread = thread_new("sm_engine");
	if (NULL == m_pMainThread) {
		LOG_ERROR(LOG_TAG, "%s unable to create sm main thread", __func__);
		return;
	}
	
	m_pCurMessageQueue = fixed_queue_new(SIZE_MAX);
	m_pDefMessageQueue = fixed_queue_new(SIZE_MAX);
	
	fixed_queue_register_dequeue(
      m_pCurMessageQueue,
      thread_get_reactor(m_pMainThread),
      StateMachine::ProcessMessage, this);
	  
	fixed_queue_register_dequeue(
      m_pDefMessageQueue,
      thread_get_reactor(m_pMainThread),
      StateMachine::ProcessMessage, this);
	
	thread_set_rt_priority(m_pMainThread, priority);
	thread_post(m_pMainThread, btu_message_loop_run, nullptr);

}

void StateMachine :: Stop(void) {
	
}

void StateMachine :: AddState(SM_State_T state) {
	
}

void StateMachine :: RemoveState(SM_State_T state) {
	
}

void StateMachine :: SetState(int state) {
	
}

void StateMachine :: GetState(int state) {
	
}

void StateMachine :: SendMessage(uint32_t msg_id,  uint32_t len, void * param) {
}

void StateMachine :: DeferMessage(uint32_t msg_id,  uint32_t len, void * param) {
}
	
}