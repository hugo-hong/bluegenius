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
  typedef void (StateMachine:: *PFN_HandleMessage)(uint32_t, uint32_t, void*);
  typedef struct _SM_State_T {
    int state;
    PFN_HandleMessage pfnProcessMessage;
  }SM_State_T;
public:
  StateMachine(void);
  ~StateMachine(void);
  
  void start();
  void stop();
  void addState(SM_State_T state);
  void removeState(SM_State_T state);
  void setState(int state);
  int getState(void);
  void sendMessage(uint32_t msg_id,  uint32_t len, void * param);
  void deferMessage(uint32_t msg_id,  uint32_t len, void * param);
  
protected:
  void cleanup();
  void doTransition();
  void enterState(int state);
  void exitState(int state);
  void toggleState(int state = SM_INVALID_STATE);
  void processMessage(uint32_t len, void * param);
  void smEngineRun(void);
  static void *smMainThread(void *arg);
  
private:
  list<SM_MSG_T> m_curMessages;
  list<SM_MSG_T> m_deferMessages;
  CallbackHandler m_stateHandler;
}

#endif //_STATE_MACHINE_H_
