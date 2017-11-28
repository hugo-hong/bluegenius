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
#define LOG_TAG "util_future"

#include "utils/inc/event.h"
#include "utils/inc/future.h"

struct future_t {
  bool ready_call_be_called;
  event_t* event;
  void* result;
}

void future_free(future_t* future) {
  if (!future) return;
  
  event_close(future->event);
  sys_free(future);
}

future_t* future_new(void* value) {
  future_t* future = static_cast<future_t*>sys_calloc(sizeof(future_t));
  CHECK(future != NULL);
  
  future->event = event_create(0);
  future->result = value;
  future->ready_can_be_called = true;
  return future;
}

void future_ready(future_t* future, void* value) {
  CHECK(future != NULL);
  CHECK(future->ready_can_be_called);
  
  future->result = value;
  future->ready_can_be_called = false;
  event_post(future->event);
}

void* future_await(future_t* future) {
  CHECK(future != NULL);
  
  // If the future is immediate, it will not have a event
  if (future->event) event_wait(future->event);
  
  void* result = future->result;
  future_free(future);
  return result;
}
