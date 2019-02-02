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
#ifndef _UTIL_COMMON_H_
#define _UTIL_COMMON_H_
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define UNUSED_ATTR __attribute__((unused))
#define DIM(a)		(sizeof(a) / sizeof((a)[0]))
#define INVALID_FD  (-1)
#define CONCAT(a, b) a##b

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define LOG_TRACE(tag, format, ...) printf("[%s]%s:" format"\r\n", tag, __FUNCTION__, ##__VA_ARGS__)
#define LOG_WARN(tag, format, ...) printf("[WARN][%s]%s:" format"\r\n", tag, __FUNCTION__, ##__VA_ARGS__)
#define LOG_ERROR(tag, format, ...) printf("[ERROR][%s]%s:" format"\r\n", tag, __FUNCTION__, ##__VA_ARGS__)

//retrieves the offset of a member from the beginning of structure
#define OFFSETOF(type, member) ((size_t)(&((type*)0)->member))
//retrieves the member ptr from a structure
#define MEMBER_OF(ptr, type, member) ((typeof( ((type *)0)->member ) *)( (char *)ptr + OFFSETOF(type, member) ))
//retrieves structure ptr from the member ptr
#define CONTAINER_OF(ptr, type, member) ({\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);\
        (type *)( (char *)__mptr - OFFSETOF(type, member) );\
      })

#define CHECK(assertion)                                   \
  if (__builtin_expect(!(assertion), false)) {             \
    LOG_ERROR(0, "%s:%d: %s\n", __FILE__, __LINE__, #assertion); \
    abort();                                               \
  }


// Re-run |fn| system call until the system call doesn't cause EINTR.
#define SYS_NO_INTR(fn) \
	do {                  \
	} while ((fn) == -1 && errno == EINTR)


#endif //_UTIL_COMMON_H_

