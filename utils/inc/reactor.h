/********************************************************************************
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
#ifndef _REACTOR_H_
#define _REACTOR_H_

typedef void (*ready_cb)(void* context);

class Reactor {
public:
    Reactor();
    ~Reactor();
    
    reactor_status_t Start();
    reactor_status_t RunOnce();
    void Stop();
    reactor_object_t* Register(int fd, void* context, ready_cb pfnRead, ready_cb pfnWrite);
    void Unregister(reactor_object_t* obj);
    
protected:
    void New();
    void Free();
    reactor_status_t Run(int iterations);
    
private:
    int m_epollFd;
    int m_eventFd;
    bool m_isRunning;
    bool m_isObjectRemoved;
    pthread_t m_runThread;
}

#endif //_REACTOR_H_
