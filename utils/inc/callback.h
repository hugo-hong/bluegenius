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
#ifndef _CALLBACK_PROXY_H_
#define _CALLBACK_PROXY_H_
#include <stdlib.h>
#include <stdint.h>
#include <map>

using namespace std;

class ICallback {
public:
  virtual void excute(uint32_t arg1,  uint32_t arg2, void * arg3) = 0;
}

template<class T>
class CCallbackProxy : public ICallback {
  typedef void(T::PFNCALLBACK)(uint32_t, uint32_t, void*);
public:
  CCallbackProxy(T *pObject, PFNCALLBACK pfnCallback) {
    m_pObject = pObject;
    m_pfnCallback = pfnCallback;
  }
  ~CCallbackProxy{
  }
  void excute(uint32_t arg1,  uint32_t arg2, void * arg3) {
    if (m_pObject != NULL && m_pfnCallback != NULL)
      (m_pObject->*m_pfnCallback)(arg1, arg2, arg3);
  } 
protected:
private:
  T *m_pObject;
  PFNCALLBACK m_pfnCallback;
}

class CCallbackHandler {
public:
  CCallbackHandler() {
  }
  ~CCallbackHandler() {
    cleanup();
  }
  void regCallback(int key, ICallback *callback) {
    m_callbacks.insert(make_pair(key, callback));
  }
  void delCallback(int key) {
    map<int, ICallback*>::iterator callback_it = m_callbacks.find(key);
    if (m_callbacks.end() != callback_it) delete callback_it->second;
    m_callbacks.erase(key);
  }
  void invokeMethod(int key, uint32_t arg1,  uint32_t arg2, void * arg3) {
    map<int, ICallback*>::iterator callback_it = m_callbacks.find(key);
    if (m_callbacks.end() != callback_it) {
      callback_it->second->execute(arg1, arg2, arg3);
    }
  }
protected:
  void cleanup() {
    map<int, ICallback*>::iterator callback_it = m_callbacks.begin();
    while (m_callbacks.size() > 0) {
      delete callback_it->second;
      m_callbacks.erase(callback_it);
      callback_it = m_callbacks.begin();
    }
  }
private:
  map<int, ICallback*> m_callbacks;
}

#define REGISTER_CALLBACK(theClass, id, object, memberFxn, handler) \
{ \
  if (object != NULL && membenFxn != NULL) { \
    CCallbackProxy<theClass> *thisCallback = new CCallbackProxy<theClass>(object, memberFxn); \
    handler.regCallback(id, thisCallback); \
  } \
}

#define DEREGISTER_CALLBACK(id, handler) \
{ \
  handler.delCallback(id); \
}

#define INVOKE_CALLBACK(id, arg1, arg2, arg3, handler) \
{ \
  handler.invokeMethod(id, arg1, arg2, arg3); \
}

#endif //_CALLBACK_PROXY_H_
