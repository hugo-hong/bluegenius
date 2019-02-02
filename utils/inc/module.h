/*********************************************************************************
   Bluegenius - Bluetooth host protocol stack for Linux/android/windows...
   Copyright (C)
   Written 2017 by hugo£¨yongguang hong£© <hugo.08@163.com>
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
#ifndef _UTILS_MODULE_H_
#define _UTILS_MODULE_H_
#include <mutex>
#include <map>

#define MAX_MODULE_DEPENDENCIES			10

class Future;

class Module {
public:
	virtual Future* init() { return NULL; }
	virtual Future* start_up() { return NULL; }
	virtual Future* shut_down() { return NULL; }
	virtual Future* clean_up() { return NULL; }
	virtual void* get_interface() { return NULL; }
	static const char* kMODULE_NAME;
};

class ModuleManager {
public:
	typedef enum {
		MODULE_STATE_NONE = 0,
		MODULE_STATE_INITIALIZED = 1,
		MODULE_STATE_STARTED = 2		
	} module_state_t;
	typedef Future* (*module_lifecycle_fn)(void);
	typedef struct {
		const char* name;
		module_lifecycle_fn init;
		module_lifecycle_fn start_up;
		module_lifecycle_fn shut_down;
		module_lifecycle_fn clean_up;
		const char* dependencies[MAX_MODULE_DEPENDENCIES];
	}module_t;

	ModuleManager();
	~ModuleManager();

	bool module_init(const module_t* module);
	bool module_start_up(const module_t* module);
	void module_shut_down(const module_t* module);
	void module_clean_up(const module_t* module);
	const module_t* get_module(const char* name);
	module_state_t get_module_state(const module_t* module);
	void set_module_state(const module_t* module, module_state_t state);

	static ModuleManager& GetInstance() { return m_instance; }
protected:
	void start(void);
	void stop(void);
	bool invoke_lifecycle_function(module_lifecycle_fn fun);
private:
	static ModuleManager m_instance;
	std::mutex m_mutex;
	std::map<const module_t*, module_state_t> m_modules;
};

#endif // _UTILS_MODULE_H_
