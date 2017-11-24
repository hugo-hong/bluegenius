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
  m_curState = SM_INVALID_STATE;
  m_destState = SM_INVALID_STATE;
  m_pMainThread = NULL;
  m_pCurMessageQueue = NULL;
  m_pDefMessageList = NULL;
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
  m_pDefMessageList = list_new(NULL);
  fixed_queue_register_dequeue(
    m_pCurMessageQueue,
    thread_get_reactor(m_pMainThread),
    StateMachine::HandleMessage, this);  

  thread_set_rt_priority(m_pMainThread, priority);
}

void StateMachine :: Stop(void) {
  fixed_queue_unregister_dequeue(m_pCurMessageQueue);
  fixed_queue_free(m_pCurMessageQueue);
  list_free(m_pDefMessageList);
  thread_free(m_pMainThread);
}

void StateMachine :: AddState(SM_State_T state) {
  REGISTER_CALLBACK(StateMachine, state.state, this, state.pfnProcessMessage, m_stateHandler);
}

void StateMachine :: RemoveState(SM_State_T state) {
  DEREGISTER_CALLBACK(state.state, m_stateHandler);
}

void StateMachine :: SetState(int state) {
  m_destState = state;
  if (SM_INVALID_STATE == m_curState) {
    SM_MSG_T *pMsg = (SM_MSG_T*)sys_malloc(sizeof(SM_MSG_T));
    pMsg->msg_id = SM_MSG_TOGGLE_STATE;
    fixed_queue_try_enqueue(m_pCurMessageQueue, pMsg);
  }
  
}

void StateMachine :: GetState(int state) {
  return m_curState;
}

void StateMachine :: SendMessage(uint32_t msg_id,  uint32_t len, void * param) { 
  SM_MSG_T *pMsg = (SM_MSG_T*)sys_malloc(sizeof(SM_MSG_T) + len);;
  
  pMsg->msg_id = msg_id;
  pMsg->u4Size = len;
  if (len > 0 && param != NULL) {
    memcpy(pMsg->param, param, len);    
  }

  fixed_queue_enqueue(m_pCurMessageQueue, pMsg);
}

void StateMachine :: DeferMessage(uint32_t msg_id,  uint32_t len, void * param) {  
  SM_MSG_T *pMsg = (SM_MSG_T*)sys_malloc(sizeof(SM_MSG_T) + len);;
  
  pMsg->msg_id = msg_id;
  pMsg->u4Size = len;
  if (len > 0 && param != NULL) {
    memcpy(pMsg->param, param, len);    
  }
  list_append(m_pDefMessageList, pMsg);
}

void StateMachine :: EnterState(int state) {  
  INVOKE_CALLBACK(state, SM_MSG_STATE_ENTER, 0, NULL, m_stateHandler);
}

void StateMachine :: ExitState(int state) {  
  INVOKE_CALLBACK(state, SM_MSG_STATE_EXIT, 0, NULL, m_stateHandler);
}

void StateMachine :: ToggleState(void) {  
  if (m_destState != INVALID_STATE && 
      m_destState != m_curState) {
    doTransition();
  }
}

void StateMachine :: DoTransition(void) {  
  ExitState(m_curState);
  m_curState = m_destState;
  EnterState(m_destState);
  m_destState = INVALID_STATE;
  ProcessDeferdMessage();
}

void StateMachine :: ProcessDeferdMessage(void) {  
  int len = list_length(m_pDefMessageList);
  for (int = 0; i < len; i++) {
    SM_MSG_T *pMsg = list_front(m_pDefMessageList);
    fixed_queue_enqueue(m_pCurMessageQueue, pMsg);
    list_remove(m_pDefMessageList, pMsg);
  }
}

void StateMachine :: ProcessMessage(SM_MSG_T *pMsg) {  
  CHECK(pMsg != NULL);

  if (SM_MSG_TOGGLE_STATE != pMsg->msg_id) {
    INVOKE_CALLBACK(m_curState, pMsg->msg_id, pMsg->u4Size, pMsg->param, m_stateHandler);
  }
	
  ToggleState();
  
  sys_free(pMsg);
}

void StateMachine :: HandleMessage(fixed_queue_t *queue, void* context) {  
  StateMachine *pInstance = dynamic_cast<StateMachine*>(context);
  CHECK(pInstance != NULL && queue != NULL);
  pInstance->ProcessMessage((SM_MSG_T*)fixed_queue_dequeue(queue));
}
