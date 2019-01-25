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
#ifndef _UTILS_EVENTBUS_H_
#define _UTILS_EVENTBUS_H_
#include <map>
#include <list>
#include <queue>
#include <vector>
#include <mutex>
#include <typeindex>
#include <typeinfo>

#include "concurrency.h"
#include "threadpool.h"

/**
 * \brief The base event class, all events inherit from this class
 */
class Event {
public:
	enum ThreadMode {
		BACKGROUND,
		MAIN,
		DEFAULT
	};
	Event(void *sender, int id, ThreadMode mode)
		: m_id(id)
		, m_sender(sender)
		, m_mode(mode) {}
	~Event() {}
	
	void *getSender() { return m_sender; }
	int getId() { return m_id; }
	Worker *getWorker() { return m_worker; }
	void setWorker(Worker *worker) { m_worker = worker; }
	ThreadMode getThreadMode() { return m_mode; }
protected:
private:	
	int m_id;
	ThreadMode m_mode;
	void* const m_sender;
	Worker *m_worker;
};

template<typename T>
class IEventListener {
public:
	IEventListener() {
		// An error here indicates you're trying to implement IEventListener with a type that is not derived from Event
		static_assert(std::is_base_of<Event, T>::value, "IEventListener<T>: T must be a class derived from Event");
	}
	~IEventListener() {
	}

	/**
	 * \brief Pure virtual method for implementing the body of the listener
	 *
	 * @param The event instance
	 */
	virtual void onEvent(T& event) = 0;
};

class EventBus {
public:
	EventBus()
		:m_processing(false)
		, m_mutex()
		, m_threadpool(3)
	{
	}
	~EventBus() {}

	template<typename T>
	void addListener(IEventListener<T>* listener) {
		HandlerList_t* handlers = m_handlers[typeid(T)];

		// Create a new collection instance for this type if it hasn't been created yet
		if (handlers == nullptr) {
			handlers = new HandlerList_t();
			m_handlers[typeid(T)] = handlers;
		}

		// Create a new EventPair instance for this registration.
		EventHandler *handler = new EventHandler(this, static_cast<void*>(listener));

		// Add the listener object to the collection
		handlers->push_back(handler);
	}
	template<typename T>
	void removeListener(IEventListener<T>* listener) {
		HandlerList_t* handlers = m_handlers[typeid(T)];
		if (handlers != nullptr) {
			for (auto &handler : *handlers) {
				if (handler->getListener() == listener) {
					handlers->remove(handler);
					delete handler;
					break;
				}
			}

			if (handlers->empty())
				delete handlers;
		}
	}
	
	void dispatchEvent(Event* event) {
		std::lock_guard<std::mutex> lock(m_mutex);

		//add event to queue
		m_events.push(event);

		if (!m_processing)
			scheduleEvent();		
	}

protected:
	class EventHandler : public Task {
	public:
		EventHandler(EventBus* bus, void* const listener)
			:Task()
			,m_bus(bus)
			,m_listener(listener) {}
		~EventHandler() {}
		virtual void run() {
			if (m_bus != nullptr)
				m_bus->processEvent();
		}
		void* const getListener() { return m_listener; }
	protected:
	private:
		EventBus *m_bus;
		void* const m_listener;
	};
	void scheduleEvent() {
		std::lock_guard<std::mutex> lock(m_mutex);
		Event* event = m_events.front();
		if (event == nullptr) return;
		switch (event->getThreadMode())	{
		case Event::ThreadMode::BACKGROUND:
		{
			Worker *worker = event->getWorker();
			if (worker == nullptr) {
				worker = m_threadpool.peek();
				if (worker == nullptr) {
					if (!m_threadpool.push(new Worker()) ||
						(worker = m_threadpool.peek()) == nullptr) {
						//maybe thread pool overflow
						m_threadpool.flush();
						worker = m_threadpool.peek();
					}						
				}
				event->setWorker(worker);
				HandlerList_t* handlers = m_handlers[typeid(event)];
				worker->add_task(event->getId(), handlers->front());
			}
			worker->schedule();
		}
			break;
		case Event::ThreadMode::MAIN:
			processEvent();
			break;
		case Event::ThreadMode::DEFAULT:
			processEvent();
			break;
		}
	}

	/**
	* \brief event handler, run on thread
	*/
	void processEvent() {
		std::lock_guard<std::mutex> lock(m_mutex);

		Event* event = m_events.front();
		if (event == nullptr)
			return;

		HandlerList_t* handlers = m_handlers[typeid(event)];
		// there no listener for this event, do nothing
		if (handlers == nullptr)
			return;

		m_processing = true;
		for (auto &handler : *handlers) {
			if (handler != nullptr)
				static_cast<IEventListener<Event>*>(handler->getListener())->onEvent(*event);
		}	

		//event process complete
		m_events.pop();
		delete event;
		doneEvent();		
	}
	void doneEvent() {
		std::lock_guard<std::mutex> lock(m_mutex);

		m_processing = false;		

		//schedule next event
		if (!m_events.empty())
			scheduleEvent();
	}
private:
	typedef std::list<EventHandler*> HandlerList_t;
	typedef std::map<std::type_index, HandlerList_t*> HandlerMap_t;
	HandlerMap_t m_handlers;
	std::queue<Event*> m_events;
	std::mutex m_mutex;
	bool m_processing;
	ThreadPool m_threadpool;
};

#endif //_UTILS_EVENTBUS_H_
