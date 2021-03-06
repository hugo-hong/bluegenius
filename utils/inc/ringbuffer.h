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
#ifndef _UTILS_RINGBUFFER_H_
#define _UTILS_RINGBUFFER_H_

class RingBuffer {
public:  
	RingBuffer(const size_t size);
	~RingBuffer();

	size_t Insert(const uint8_t *data, size_t len);
	size_t Peek(uint8_t *data, size_t len);
	size_t Pop(uint8_t *data, size_t len);
	size_t Drop(size_t len);

protected:
	size_t GetAvailable() { return m_available; }
	size_t GetBufferSize() { return (m_total - m_available); }

private:
	size_t m_total;
	size_t m_available;
	uint8_t *m_base;
	uint8_t *m_head;
	uint8_t *m_tail;
};

#endif //_UTILS_RINGBUFFER_H_
