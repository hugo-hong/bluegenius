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
#define LOG_TAG "bt_snoop"
#include <mutex>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>

#include "utils.h"
#include "future.h"
#include "btsnoop.h"


// The number of of packets per btsnoop file before we rotate to the next
// file. As of right now there are two snoop files that are rotated through.
// The size can be dynamically configured by seting the relevant system
// property
#define DEFAULT_BTSNOOP_SIZE 0xffff

#define BTSNOOP_ENABLE_PROPERTY "persist.bluetooth.btsnoopenable"
#define BTSNOOP_PATH_PROPERTY "persist.bluetooth.btsnooppath"
#define DEFAULT_BTSNOOP_PATH "/data/misc/bluetooth/logs/btsnoop_hci.log"
#define BTSNOOP_MAX_PACKETS_PROPERTY "persist.bluetooth.btsnoopsize"
#define BTSNOOP_FILE_HEADER	"btsnoop\0\0\0\0\1\0\0\x3\xea"

//module name
const char* Module::kMODULE_NAME = "btsnoop_module";
// Epoch in microseconds since 01/01/0000.
const uint64_t BtSnoop::BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;
//module instance
BtSnoop BtSnoop::BTSNOOP_INSTANCE;

static Future* _startup() {
	return BtSnoop::GetInstance().start_up();
}
static Future* _shutdown() {
	return BtSnoop::GetInstance().shut_down();
}

extern const ModuleManager::module_t btsnoop_module = {
	.name = Module::kMODULE_NAME,
	.init = NULL,
	.start_up = _startup,
	.shut_down = _shutdown,
	.clean_up = NULL,
	.dependencies = {NULL}
};

Future* BtSnoop::start_up() {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_logfd = open_snoop_file();
	return NULL;
}

Future* BtSnoop::shut_down() {
	return NULL;
}


void BtSnoop::capture(const BT_HDR* buffer, bool is_received) {
	const uint8_t *p = buffer->data + buffer->offset;

	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_logfd == INVALID_FD)
		return;

	switch (buffer->event & MSG_EVT_MASK) {
	case MSG_STACK_TO_HC_HCI_CMD:
		write_packet(COMMAND_PACKET, (uint8_t*)p, true);
		break;
	case MSG_HC_TO_STACK_HCI_ACL:
	case MSG_STACK_TO_HC_HCI_ACL:
		write_packet(ACL_PACKET, (uint8_t*)p, is_received);
		break;
	case MSG_HC_TO_STACK_HCI_SCO:
	case MSG_STACK_TO_HC_HCI_SCO:
		write_packet(SCO_PACKET, (uint8_t*)p, is_received);
		break;	
	case MSG_HC_TO_STACK_HCI_EVT:
		write_packet(EVENT_PACKET, (uint8_t*)p, false);
		break;
	}
}

int BtSnoop::open_snoop_file() {
	int logfile_fd = m_logfd;

	if (logfile_fd != INVALID_FD) {
		close(logfile_fd);
		logfile_fd = INVALID_FD;
	}

	char log_path[128] = DEFAULT_BTSNOOP_PATH;
	char last_log_path[128 + sizeof(".last")];
	//fill last log file path
	snprintf(last_log_path, sizeof(last_log_path), "%s.last", log_path);
	//covert to last log
	if (rename(log_path, last_log_path) != 0 && errno != ENOENT)
		LOG_ERROR(LOG_TAG, "unable to rename '%s' to '%s' : %s", 
			log_path, last_log_path, strerror(errno));
	//open new log
	mode_t prevmask = umask(0);
	logfile_fd = open(log_path, O_WRONLY | O_CREAT | O_TRUNC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	umask(prevmask);
	if (logfile_fd == INVALID_FD)
		LOG_ERROR(LOG_TAG, "unable to open '%s': %s", log_path, strerror(errno));		
	else
		write(logfile_fd, BTSNOOP_FILE_HEADER, sizeof(BTSNOOP_FILE_HEADER));

	return logfile_fd;
}

void BtSnoop::delete_snoop_file() {
	
	LOG_TRACE(LOG_TAG, "Deleting snoop log if it exists");
	char log_path[128] = DEFAULT_BTSNOOP_PATH;
	char last_log_path[128 + sizeof(".last")];
	//fill last log file path
	snprintf(last_log_path, sizeof(last_log_path), "%s.last", log_path);
	remove(log_path);
	remove(last_log_path);
	
}

void BtSnoop::write_packet(hci_packet_type_t type, uint8_t *packet, bool is_received) {
	int length;
	int flags;
	int drops = 0;
	switch (type)
	{
	case COMMAND_PACKET:
		length = packet[2] + 4;
		flags = 2;
		break;
	case ACL_PACKET:
		length = (packet[3] << 8) + packet[2] + 5;
		flags = is_received;
		break;
	case SCO_PACKET:
		length = packet[2] + 4;
		flags = is_received;
		break;
	case EVENT_PACKET:
		length = packet[1] + 3;
		flags = 3;
		break;
	default:
		break;
	}

	//prepare packet header
	btsnoop_header_t log_header;
	log_header.length_original = htonl(length);
	log_header.length_captured = log_header.length_original;
	log_header.flags = htonl(flags);
	log_header.dropped_packets = htonl(drops);
	log_header.timestamp = htonq(BtSnoop::get_timestamp());
	log_header.type = type;

	//write to file
	if (m_logfd != INVALID_FD) {
		write(m_logfd, &log_header, sizeof(btsnoop_header_t));
		write(m_logfd, packet, length - 1);
	}
}

uint64_t BtSnoop::get_timestamp() {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	//timestamp is in microseconds
	uint64_t timestamp = tv.tv_sec * 1000 * 1000LL;
	timestamp += tv.tv_usec;
	timestamp += BTSNOOP_EPOCH_DELTA;
	return timestamp;
}
