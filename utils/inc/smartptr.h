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
#ifndef _UTIL_SMARTPTR_H_
#define _UTIL_SMARTPTR_H_

template<typename T>
class SmartPtr
{
public:
	SmartPtr(T *ptr = nullptr)
		:m_ptr(ptr) {
		if (m_ptr != nullptr)
			m_ref = new size_t(1);
		else
			m_ref = new size_t(0);
	}

	//copy construction
	SmartPtr(const SmartPtr &ptr) {
		if (this != &ptr) {
			this->m_ptr = ptr.m_ptr;
			this->m_ref = ptr.m_ref;
			(*this->m_ref)++;
		}		
	}

	~SmartPtr() {
		if (--(*m_ref) == 0) {
			delete m_ptr;
			delete m_ref;
		}
	}

	T& operator*() {
		return *(this->m_ptr);
	}

	T* operator->() {
		return this->m_ptr;
	}

	SmartPtr<T>& operator=(SmartPtr<T>& sp) {
		if (this->m_ptr != sp.m_ptr) {
			if (--(*m_ref) == 0) {
				delete m_ptr;
				delete m_ref;
			}

			m_ptr = sp.m_ptr;
			m_ref = sp.m_ref;
			(*m_ref)++;
		}

		return *this;
	}

	size_t getRef() {
		return *m_ref;
	}

private:
	T *m_ptr;
	size_t *m_ref;
};


#endif //_UTIL_SMARTPTR_H_
