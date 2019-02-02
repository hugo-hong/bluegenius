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
#define LOG_TAG "utils_module" 
#include <dlfcn.h>

#include "utils.h"
#include "future.h"
#include "module.h"

ModuleManager ModuleManager::m_instance;

ModuleManager::ModuleManager() {
	start();
}

ModuleManager::~ModuleManager() {
	stop();
}

bool ModuleManager::module_init(const module_t* module) {
	CHECK(module != NULL);
	CHECK(get_module_state(module) == MODULE_STATE_NONE);

	if (!invoke_lifecycle_function(module->init)) {
		LOG_ERROR(LOG_TAG, "failed to init module %s", module->name);
		return false;
	}

	set_module_state(module, MODULE_STATE_INITIALIZED);

	return true;
}

bool ModuleManager::module_start_up(const module_t* module) {
	CHECK(module != NULL);
	CHECK(get_module_state(module) == MODULE_STATE_INITIALIZED ||
		module->init == NULL);

	LOG_TRACE(LOG_TAG, "Startup module %s", module->name);
	if (!invoke_lifecycle_function(module->start_up)) {
		LOG_ERROR(LOG_TAG, "failed to startup module %s", module->name);
		return false;
	}

	set_module_state(module, MODULE_STATE_STARTED);

	return true;
}

void ModuleManager::module_shut_down(const module_t* module) {
	CHECK(module != NULL);
	module_state_t state = get_module_state(module);
	CHECK(state <= MODULE_STATE_STARTED);

	// Only something to do if the module was actually started
	if (state < MODULE_STATE_STARTED) return;

	LOG_TRACE(LOG_TAG, "Shutdown module %s", module->name);
	if (!invoke_lifecycle_function(module->shut_down)) {
		LOG_ERROR(LOG_TAG, "failed to shutdown module %s", module->name);
		return;
	}

	set_module_state(module, MODULE_STATE_INITIALIZED);
}

void ModuleManager::module_clean_up(const module_t* module) {
	CHECK(module != NULL);
	module_state_t state = get_module_state(module);
	CHECK(state <= MODULE_STATE_INITIALIZED);

	// Only something to do if the module was actually initialized
	if (state < MODULE_STATE_INITIALIZED) return;

	LOG_TRACE(LOG_TAG, "Cleanup module %s", module->name);
	if (!invoke_lifecycle_function(module->clean_up)) {
		LOG_ERROR(LOG_TAG, "failed to cleanup module %s", module->name);
		return;
	}

	set_module_state(module, MODULE_STATE_NONE);
}

const ModuleManager::module_t * ModuleManager::get_module(const char* name) {
	module_t* module = (module_t*)dlsym(RTLD_DEFAULT, name);
	return module;
}

ModuleManager::module_state_t ModuleManager::get_module_state(const module_t* module) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_modules.find(module);
	return (it != m_modules.end()) ? it->second : MODULE_STATE_NONE;
}

void ModuleManager::set_module_state(const module_t* module, module_state_t state) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_modules[module] = state;
}

void ModuleManager::start(void) {
}

void ModuleManager::stop(void) {
	m_modules.clear();
}

bool ModuleManager::invoke_lifecycle_function(module_lifecycle_fn fun) {
	// A NULL lifecycle function means it isn't needed, so assume success
	if (NULL == fun) return true;

	Future *future = fun();

	// A NULL future means synchronous success
	if (NULL == future) return true;

	// Asynchronus fall back to the future
	return (bool)future->Await();
}
