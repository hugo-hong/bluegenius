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
#include <unistd.h>

// The number of of packets per btsnoop file before we rotate to the next
// file. As of right now there are two snoop files that are rotated through.
// The size can be dynamically configured by seting the relevant system
// property
#define DEFAULT_BTSNOOP_SIZE 0xffff

#define BTSNOOP_ENABLE_PROPERTY "persist.bluetooth.btsnoopenable"
#define BTSNOOP_PATH_PROPERTY "persist.bluetooth.btsnooppath"
#define DEFAULT_BTSNOOP_PATH "/data/misc/bluetooth/logs/btsnoop_hci.log"
#define BTSNOOP_MAX_PACKETS_PROPERTY "persist.bluetooth.btsnoopsize"

typedef enum {
  CommandPacket = 1,
  AclPacket = 2,
  ScoPacket = 3,
  EventPacket = 4
} packet_type_t;

// Epoch in microseconds since 01/01/0000.
static const uint64_t BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;

static int logfile_fd = INVALID_FD;
static std::mutex btsnoop_mutex;

// Module lifecycle functions

static future_t* start_up(void) {
  std::lock_guard<std::mutex> lock(btsnoop_mutex);

  if (!is_btsnoop_enabled()) {
    delete_btsnoop_files();
  } else {
    open_next_snoop_file();    
  }

  return NULL;
}

static future_t* shut_down(void) {
  std::lock_guard<std::mutex> lock(btsnoop_mutex);

  if (!is_btsnoop_enabled()) {
    delete_btsnoop_files();
  }

  if (logfile_fd != INVALID_FD) close(logfile_fd);
  logfile_fd = INVALID_FD;
  
  return NULL;
}

EXPORT_SYMBOL extern const module_t btsnoop_module = {
    .name = BTSNOOP_MODULE,
    .init = NULL,
    .start_up = start_up,
    .shut_down = shut_down,
    .clean_up = NULL,
    .dependencies = {STACK_CONFIG_MODULE, NULL}};

// Interface functions
static void capture(const BT_HDR* buffer, bool is_received) {
  uint8_t* p = const_cast<uint8_t*>(buffer->data + buffer->offset);

  std::lock_guard<std::mutex> lock(btsnoop_mutex);
  uint64_t timestamp_us = time_gettimeofday_us();
  btsnoop_mem_capture(buffer, timestamp_us);

  if (logfile_fd == INVALID_FD) return;

  switch (buffer->event & MSG_EVT_MASK) {
    case MSG_HC_TO_STACK_HCI_EVT:
      btsnoop_write_packet(kEventPacket, p, false, timestamp_us);
      break;
    case MSG_HC_TO_STACK_HCI_ACL:
    case MSG_STACK_TO_HC_HCI_ACL:
      btsnoop_write_packet(kAclPacket, p, is_received, timestamp_us);
      break;
    case MSG_HC_TO_STACK_HCI_SCO:
    case MSG_STACK_TO_HC_HCI_SCO:
      btsnoop_write_packet(kScoPacket, p, is_received, timestamp_us);
      break;
    case MSG_STACK_TO_HC_HCI_CMD:
      btsnoop_write_packet(kCommandPacket, p, true, timestamp_us);
      break;
  }
}

static const btsnoop_t interface = {capture};

const btsnoop_t* btsnoop_get_interface() {
  return &interface;
}

      length_he = packet[1] + 3;
      flags = 3;
      break;
  }

  btsnoop_header_t header;
  header.length_original = htonl(length_he);
  header.length_captured = header.length_original;
  header.flags = htonl(flags);
  header.dropped_packets = 0;
  header.timestamp = htonll(timestamp_us + BTSNOOP_EPOCH_DELTA);
  header.type = type; 

  if (logfile_fd != INVALID_FD) {
    packet_counter++;
    if (packet_counter > packets_per_file) {
      open_next_snoop_file();
    }

    iovec iov[] = {{&header, sizeof(btsnoop_header_t)},
                   {reinterpret_cast<void*>(packet), length_he - 1}};
    TEMP_FAILURE_RETRY(writev(logfile_fd, iov, 2));
  }
}
