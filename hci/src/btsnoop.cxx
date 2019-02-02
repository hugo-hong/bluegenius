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
