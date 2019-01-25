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
#ifndef _SINGLETON_H_
#define _SINGLETON_H_
#include <mutex>
//#include <boost/noncopyable>

template<typename T>
class Singleton //: boost::noncopyable
{
public:
	static T& GetInstance() {
		std::call_once(Singleton::ms_onceFlag, Singleton::init);
		return *ms_pInstance;
	}
protected:
  static void init() {
    static T INSTANCE;
	ms_pInstance = &INSTANCE;
  }
private:
  Singleton(const Singleton&){} //no copy
  Singleton& operator=(const Singleton&) {} //no assignment
  ~Singleton(){} 
  
private:
  static T* ms_pInstance;
  static std::once_flag ms_onceFlag;
};

template<typename T> 
T* Singleton<T>::ms_pInstance = NULL;

template<typename T> 
std::once_flag Singleton<T>::ms_onceFlag;

//usage
//MyClass& thiz = Singleton<MyClass>::GetInstance();

#endif //_SINGLETON_H_
