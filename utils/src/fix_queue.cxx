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
#include <mutex>

typedef struct fixed_queue_t {
  list_t* list;
  event_t* enqueue_evt;
  event_t* dequeue_evt;
  std::mutex* mutex;
  size_t capacity;

  reactor_object_t* dequeue_object;
  fixed_queue_cb dequeue_ready;
  void* dequeue_context;
} fixed_queue_t;

static void internal_dequeue_ready(void* context);

fixed_queue_t* fixed_queue_new(size_t capacity) {
  fixed_queue_t* ret =
      static_cast<fixed_queue_t*>(sys_calloc(sizeof(fixed_queue_t)));

  ret->mutex = new std::mutex;
  ret->capacity = capacity;

  ret->list = list_new(NULL);
  if (!ret->list) goto error;

  ret->enqueue_evt = event_create(capacity);
  if (!ret->enqueue_evt) goto error;

  ret->dequeue_evt = event_new(0);
  if (!ret->dequeue_evt) goto error;

  return ret;

error:
  fixed_queue_free(ret, NULL);
  return NULL;
}

void fixed_queue_free(fixed_queue_t* queue, fixed_queue_free_cb free_cb) {
  if (!queue) return;

  fixed_queue_unregister_dequeue(queue);

  if (free_cb)
    for (const list_node_t* node = list_begin(queue->list);
         node != list_end(queue->list); node = list_next(node))
      free_cb(list_node(node));

  list_free(queue->list);
  event_free(queue->enqueue_evt);
  event_free(queue->dequeue_evt);
  delete queue->mutex;
  sys_free(queue);
}

void fixed_queue_flush(fixed_queue_t* queue, fixed_queue_free_cb free_cb) {
  if (!queue) return;

  while (!fixed_queue_is_empty(queue)) {
    void* data = fixed_queue_try_dequeue(queue);
    if (free_cb != NULL) {
      free_cb(data);
    }
  }
}

bool fixed_queue_is_empty(fixed_queue_t* queue) {
  if (queue == NULL) return true;

  std::lock_guard<std::mutex> lock(*queue->mutex);
  return list_is_empty(queue->list);
}

size_t fixed_queue_length(fixed_queue_t* queue) {
  if (queue == NULL) return 0;

  std::lock_guard<std::mutex> lock(*queue->mutex);
  return list_length(queue->list);
}

size_t fixed_queue_capacity(fixed_queue_t* queue) {
  CHECK(queue != NULL);

  return queue->capacity;
}

void fixed_queue_enqueue(fixed_queue_t* queue, void* data) {
  CHECK(queue != NULL);
  CHECK(data != NULL);

  event_wait(queue->enqueue_evt);

  {
    std::lock_guard<std::mutex> lock(*queue->mutex);
    list_append(queue->list, data);
  }

  event_post(queue->dequeue_evt);
}

void* fixed_queue_dequeue(fixed_queue_t* queue) {
  CHECK(queue != NULL);

  event_wait(queue->dequeue_sem);

  void* ret = NULL;
  {
    std::lock_guard<std::mutex> lock(*queue->mutex);
    ret = list_front(queue->list);
    list_remove(queue->list, ret);
  }

  event_post(queue->enqueue_evt);

  return ret;
}

bool fixed_queue_try_enqueue(fixed_queue_t* queue, void* data) {
  CHECK(queue != NULL);
  CHECK(data != NULL);

  if (!event_try_wait(queue->enqueue_sem)) return false;

  {
    std::lock_guard<std::mutex> lock(*queue->mutex);
    list_append(queue->list, data);
  }

  event_post(queue->dequeue_evt);
  return true;
}

void* fixed_queue_try_dequeue(fixed_queue_t* queue) {
  if (queue == NULL) return NULL;

  if (!event_try_wait(queue->dequeue_evt)) return NULL;

  void* ret = NULL;
  {
    std::lock_guard<std::mutex> lock(*queue->mutex);
    ret = list_front(queue->list);
    list_remove(queue->list, ret);
  }

  event_post(queue->enqueue_evt);

  return ret;
}

void* fixed_queue_try_peek_first(fixed_queue_t* queue) {
  if (queue == NULL) return NULL;

  std::lock_guard<std::mutex> lock(*queue->mutex);
  return list_is_empty(queue->list) ? NULL : list_front(queue->list);
}

void* fixed_queue_try_peek_last(fixed_queue_t* queue) {
  if (queue == NULL) return NULL;

  std::lock_guard<std::mutex> lock(*queue->mutex);
  return list_is_empty(queue->list) ? NULL : list_back(queue->list);
}

void* fixed_queue_try_remove_from_queue(fixed_queue_t* queue, void* data) {
  if (queue == NULL) return NULL;

  bool removed = false;
  {
    std::lock_guard<std::mutex> lock(*queue->mutex);
    if (list_contains(queue->list, data) &&
        event_try_wait(queue->dequeue_sem)) {
      removed = list_remove(queue->list, data);
      CHECK(removed);
    }
  }

  if (removed) {
    event_post(queue->enqueue_sem);
    return data;
  }
  return NULL;
}

list_t* fixed_queue_get_list(fixed_queue_t* queue) {
  CHECK(queue != NULL);

  // NOTE: Using the list in this way is not thread-safe.
  // Using this list in any context where threads can call other functions
  // to the queue can break our assumptions and the queue in general.
  return queue->list;
}

int fixed_queue_get_dequeue_fd(const fixed_queue_t* queue) {
  CHECK(queue != NULL);
  return event_get_fd(queue->dequeue_evt);
}

int fixed_queue_get_enqueue_fd(const fixed_queue_t* queue) {
  CHECK(queue != NULL);
  return event_get_fd(queue->enqueue_evt);
}

void fixed_queue_register_dequeue(fixed_queue_t* queue, reactor_t* reactor,
                                  fixed_queue_cb ready_cb, void* context) {
  CHECK(queue != NULL);
  CHECK(reactor != NULL);
  CHECK(ready_cb != NULL);

  // Make sure we're not already registered
  fixed_queue_unregister_dequeue(queue);

  queue->dequeue_ready = ready_cb;
  queue->dequeue_context = context;
  queue->dequeue_object =
      reactor_register(reactor, fixed_queue_get_dequeue_fd(queue), queue,
                       internal_dequeue_ready, NULL);
}

void fixed_queue_unregister_dequeue(fixed_queue_t* queue) {
  CHECK(queue != NULL);

  if (queue->dequeue_object) {
    reactor_unregister(queue->dequeue_object);
    queue->dequeue_object = NULL;
  }
}

static void internal_dequeue_ready(void* context) {
  CHECK(context != NULL);

  fixed_queue_t* queue = static_cast<fixed_queue_t*>(context);
  queue->dequeue_ready(queue, queue->dequeue_context);
}
