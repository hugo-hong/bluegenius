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

#include "utils.h"
#include "callback.h"
#include "thread.h"
#include "seqlist.h"
#include "fixed_queue.h"
#include "reactor.h"
#include "allocator.h"
#include "statemachine.h"

StateMachine :: StateMachine() 
	: m_curstate(SM_INVALID_STATE)
	, m_desstate(SM_INVALID_STATE)	
	, m_thread(NULL)
	, m_defermessages(NULL)
{    
}

StateMachine :: ~StateMachine() {	
}

void StateMachine :: Start(int priority) {
	m_thread = new Thread("sm_engine");
	m_defermessages = new SeqList(NULL);
	CHECK(m_thread != NULL && m_defermessages != NULL);
	m_thread->SetPriority(priority);
   

}

void StateMachine :: Stop(void) {

}

void StateMachine :: AddState(SM_State_T state) {
  REGISTER_CALLBACK(StateMachine, state.state, this, state.messagehandler, m_statehandler);
}

void StateMachine :: RemoveState(SM_State_T state) {
  DEREGISTER_CALLBACK(state.state, m_statehandler);
}

void StateMachine ::TransitionTo(int state) {
	m_desstate = state;   
}

int StateMachine :: GetState(void) {
  return m_curstate;
}

void StateMachine :: SendMessage(uint32_t msg_id,  uint32_t len, void * param) { 
	SM_MSG_T *msg = (SM_MSG_T*)sys_malloc(sizeof(SM_MSG_T) + len);;
  
	msg->msg_id = msg_id;
	msg->u4Size = len;
	if (len > 0 && param != NULL) {
		memcpy(msg->param, param, len);
	}
	m_thread->Post((thread_fn)StateMachine::ProcessMessage, msg);
}

void StateMachine :: DeferMessage(uint32_t msg_id,  uint32_t len, void * param) {  
	SM_MSG_T *msg = (SM_MSG_T*)sys_malloc(sizeof(SM_MSG_T) + len);;
  
	msg->msg_id = msg_id;
	msg->u4Size = len;
	if (len > 0 && param != NULL) {
		memcpy(msg->param, param, len);
	}
	m_defermessages->Append(msg);
}

void StateMachine :: EnterState(int state) {  
	INVOKE_CALLBACK(state, SM_MSG_STATE_ENTER, 0, NULL, m_statehandler);
}

void StateMachine :: ExitState(int state) {  
	INVOKE_CALLBACK(state, SM_MSG_STATE_EXIT, 0, NULL, m_statehandler);
}

void StateMachine::Cleanup(void) {

}

void StateMachine ::PerformTransition(void) {
	ExitState(m_curstate);
	m_curstate = m_desstate;
	EnterState(m_desstate);
	m_desstate = SM_INVALID_STATE;
  
}



void StateMachine :: ProcessMessage(void *context) {
	CHECK(context != NULL);

  
}


