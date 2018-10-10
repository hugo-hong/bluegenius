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
#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_
#include <stdlib.h>
#include <stdint.h>
#include <list>

class Thread;
class FixedQueue;
class CallbackHandler;

using namespace std;

//macro define
#define SM_INVALID_STATE      (-1)

//message type
typedef enum {
  SM_MSG_INIT,
  SM_MSG_PROCESS_MSG,
  SM_MSG_TOGGLE_STATE,
  SM_MSG_STATE_ENTER,
  SM_MSG_STATE_EXIT,
  SM_MSG_DEINIT,
  SM_MSG_MAX
}E_SM_MESSAGES;

typedef struct {
  uint32_t msg_id;
  uint32_t u4Size;
  uint8_t param[];
}SM_MSG_T;

class StateMachine {
public:
  typedef void (StateMachine::*PFNMessageHandler)(uint32_t, uint32_t, void*);
  typedef struct _SM_State_T {
    int state;
	PFNMessageHandler messagehandler;
  }SM_State_T;
public:
  StateMachine(void);
  ~StateMachine(void);
  
  void Start(int priority);
  void Stop();
  void AddState(SM_State_T state);
  void RemoveState(SM_State_T state);
  void TransitionTo(int state);
  int GetState(void);
  void SendMessage(uint32_t msg_id,  uint32_t len, void *param);
  void DeferMessage(uint32_t msg_id,  uint32_t len, void *param);
  
protected:
  void Cleanup();
  void PerformTransition();
  void EnterState(int state);
  void ExitState(int state);

  static void ProcessMessage(void *context);
private:
	int m_curstate;
	int m_desstate;
	
	Thread *m_thread;
	SeqList *m_defermessages;
	CallbackHandler m_statehandler;
};

#endif //_STATE_MACHINE_H_
