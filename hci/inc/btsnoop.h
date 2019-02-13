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
#ifndef _BLUEGENIUS_SNOOP_H_
#define _BLUEGENIUS_SNOOP_H_
#include "bt_hci.h"
#include "module.h"

class BtSnoop : public Module {
public:	

	BtSnoop() : Module() {}
	~BtSnoop() {}

	virtual Future* start_up();
	virtual Future* shut_down();

	void capture(const BT_HDR* buffer, bool is_received);

	static BtSnoop& GetInstance() { return BTSNOOP_INSTANCE; }

protected:	
	int open_snoop_file();
	void delete_snoop_file();
	void write_packet(hci_packet_type_t type, uint8_t *packet, bool is_received);
	static uint64_t get_timestamp();
private:
	int m_logfd;
	std::mutex m_mutex;
	static const uint64_t BTSNOOP_EPOCH_DELTA;
	static BtSnoop BTSNOOP_INSTANCE;

	typedef struct {
		uint32_t length_original;
		uint32_t length_captured;
		uint32_t flags;
		uint32_t dropped_packets;
		uint64_t timestamp;
		uint8_t type;
	}__attribute__((__packed__)) btsnoop_header_t;
};

#endif //_BLUEGENIUS_SNOOP_H_
