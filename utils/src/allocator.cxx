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
#include <errno.h>
#include <malloc.h>

#include "utils.h"
#include "allocator.h"

inline size_t allocator_resize(size_t size) {
	return((size + 3) & 0xfffffffc);
}

void* sys_malloc(size_t size) {
	size_t real_size = allocator_resize(size);
	return malloc(real_size);  
}

void* sys_calloc(size_t size) {
	size_t real_size = allocator_resize(size);
	return calloc(1, real_size); 
}

void sys_free(void* ptr) {
	CHECK(ptr != NULL);
	free(ptr);
}

void sys_free_and_reset(void** p_ptr) {
	CHECK(p_ptr != NULL);
	sys_free(*p_ptr);
	*p_ptr = NULL;
}

const allocator_t allocator_calloc = {sys_calloc, sys_free};
const allocator_t allocator_malloc = {sys_malloc, sys_free};