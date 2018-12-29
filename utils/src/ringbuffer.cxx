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
#define LOG_TAG "utils_ringbuffer"

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <mutex>
#include <string>

#include "utils.h"
#include "allocator.h"
#include "ringbuffer.h"

RingBuffer::RingBuffer(const size_t size)
	:m_total(0)
	, m_available(0)
	, m_base(NULL)
	, m_head(NULL)
	, m_tail(NULL) {
	m_base = (uint8_t *)sys_malloc(size);
	CHECK(m_base != NULL);
	m_head = m_tail = m_base;
	m_total = m_available = size;
}

RingBuffer::~RingBuffer() {
	if (m_base != NULL)
		sys_free(m_base);
}

size_t RingBuffer::Insert(const uint8_t *data, size_t len) {
	if (data == NULL || len == 0)
		return 0;
	len = MIN(len, m_available);
	for (size_t i = 0; i != len; ++i) {
		*m_tail++ = *data++;
		if (m_tail >= m_base + m_total)
			m_tail = m_base;
	}
	m_available -= len;
	return len;
}

size_t RingBuffer::Peek(uint8_t *data, size_t len) {
	if (data == NULL || len == 0)
		return 0;

	uint8_t *p = m_base + ((m_head - m_base) % m_total);
	len = MIN(len, GetBufferSize());
	for (size_t i = 0; i < len; ++i) {
		*data++ = *p++;
		if (p >= (m_base + m_total))
			p = m_base;
	}

	return len;
}

size_t RingBuffer::Pop(uint8_t *data, size_t len) {
	len = Peek(data, len);
	m_head += len;
	if (m_head >= m_base + m_total)
		m_head -= m_total;
	m_available += len;

	return len;
}

size_t RingBuffer::Drop(size_t len) {
	len = MIN(len, GetBufferSize());
	m_head += len;
	if (m_head >= m_base + m_total)
		m_head -= m_total;
	m_available += len;

	return len;
}