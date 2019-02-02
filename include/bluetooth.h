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
#ifndef _BLUEGENIUS_BLUETOOTH_H_
#define _BLUEGENIUS_BLUETOOTH_H_
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
* The Bluetooth Hardware Module ID
*/

#define BT_HARDWARE_MODULE_ID "bluetooth"
#define BT_STACK_MODULE_ID "bluetooth"

/** Bluetooth profile interface IDs */
#define BT_PROFILE_HANDSFREE_AG_ID			"handsfree_ag"
#define BT_PROFILE_HANDSFREE_CLIENT_ID		"handsfree_client"
#define BT_PROFILE_ADVANCED_AUDIO_SRC_ID	"a2dp_src"
#define BT_PROFILE_ADVANCED_AUDIO_SINK_ID	"a2dp_sink"
#define BT_PROFILE_HEALTH_ID				"health"
#define BT_PROFILE_SOCKETS_ID				"socket"
#define BT_PROFILE_HIDHOST_ID				"hid_host"
#define BT_PROFILE_HIDDEV_ID				"hid_dev"
#define BT_PROFILE_PAN_ID					"pan"
#define BT_PROFILE_MAP_CLIENT_ID			"map_client"
#define BT_PROFILE_SDP_CLIENT_ID			"sdp"
#define BT_PROFILE_GATT_ID					"gatt"
#define BT_PROFILE_AVRC_TARGET_ID			"avrcp_target"
#define BT_PROFILE_AVRC_CTRL_ID				"avrcp_ctrl"
#define BT_PROFILE_HEARING_AID_ID			"hearing_aid"

/** Bluetooth Device Name */
typedef struct { 
	uint8_t name[256]; 
} __attribute__((packed)) bt_bdname_t;

/** Bluetooth Adapter State */
typedef enum { 
	BT_STATE_OFF,
	BT_STATE_ON
} bt_state_t;

/** Bluetooth Adapter Visibility Modes*/
typedef enum {
	BT_SCAN_MODE_NONE,
	BT_SCAN_MODE_CONNECTABLE,
	BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE
} bt_scan_mode_t;

/** Bluetooth Adapter Discovery state */
typedef enum {
	BT_DISCOVERY_STOPPED,
	BT_DISCOVERY_STARTED
} bt_discovery_state_t;

/** Bluetooth ACL connection state */
typedef enum {
	BT_ACL_STATE_CONNECTED,
	BT_ACL_STATE_DISCONNECTED
} bt_acl_state_t;

/** Bluetooth Error Status */
/** We need to build on this */
typedef enum {
	BT_STATUS_SUCCESS,
	BT_STATUS_FAIL,
	BT_STATUS_NOT_READY,
	BT_STATUS_NOMEM,
	BT_STATUS_BUSY,
	BT_STATUS_DONE, /* request already completed */
	BT_STATUS_UNSUPPORTED,
	BT_STATUS_PARM_INVALID,
	BT_STATUS_UNHANDLED,
	BT_STATUS_AUTH_FAILURE,
	BT_STATUS_RMT_DEV_DOWN,
	BT_STATUS_AUTH_REJECTED,
	BT_STATUS_JNI_ENVIRONMENT_ERROR,
	BT_STATUS_JNI_THREAD_ATTACH_ERROR,
	BT_STATUS_WAKELOCK_ERROR
} bt_status_t;

/** Bluetooth PinKey Code */
typedef struct { 
	uint8_t pin[32]; 
} __attribute__((packed)) bt_pin_code_t;

/** Bluetooth energy info */
typedef struct {
	uint8_t status;
	uint8_t ctrl_state;   /* stack reported state */
	uint64_t tx_time;     /* in ms */
	uint64_t rx_time;     /* in ms */
	uint64_t idle_time;   /* in ms */
	uint64_t energy_used; /* a product of mA, V and ms */
} __attribute__((packed)) bt_activity_energy_info;

typedef struct {
	int32_t app_uid;
	uint64_t tx_bytes;
	uint64_t rx_bytes;
} __attribute__((packed)) bt_uid_traffic_t;

/** Bluetooth 128-bit UUID */
typedef struct {
	uint8_t uuid[16];// 00000000-0000-0000-0000-000000000000
} bt_uuid_t;

/** Bluetooth SDP service record */
typedef struct {
	bt_uuid_t uuid;
	uint16_t channel;
	char name[256];  // what's the maximum length
} bt_service_record_t;

/** Bluetooth Remote Version info */
typedef struct {
	int version;
	int sub_ver;
	int manufacturer;
} bt_remote_version_t;

typedef struct {
	uint16_t version_supported;
	uint8_t local_privacy_enabled;
	uint8_t max_adv_instance;
	uint8_t rpa_offload_supported;
	uint8_t max_irk_list_size;
	uint8_t max_adv_filter_supported;
	uint8_t activity_energy_info_supported;
	uint16_t scan_result_storage_size;
	uint16_t total_trackable_advertisers;
	bool extended_scan_support;
	bool debug_logging_supported;
	bool le_2m_phy_supported;
	bool le_coded_phy_supported;
	bool le_extended_advertising_supported;
	bool le_periodic_advertising_supported;
	uint16_t le_maximum_advertising_data_length;
} bt_local_le_features_t;

#endif //_BLUEGENIUS_BLUETOOTH_H_

