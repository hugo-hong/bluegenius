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
#ifndef _BLUEGENIUS_TRANSPORT_H_
#define _BLUEGENIUS_TRANSPORT_H_

typedef enum {
	BT_TRAN_OP_POWER_CTRL,
	BT_TRAN_OP_FW_CFG,
	BT_TRAN_OP_AUDIO_CFG,
	BT_TRAN_OP_PORT_OPEN,
	BT_TRAN_OP_PORT_CLOSE,
}bt_transport_opcode_t;

/** Power on/off control states */
typedef enum {
	BT_TRAN_PWR_OFF,
	BT_TRAN_PWR_ON,
}bt_transport_power_state_t;

/** Callback result values */
typedef enum {
	BT_TRAN_OP_RESULT_SUCCESS,
	BT_TRAN_OP_RESULT_FAIL,
} bt_transport_op_result_t;

/*
* Bluetooth Host/Controller Transport Interface
*/
typedef struct {
	size_t size;
}bt_transport_interface_t;

#endif // _BLUEGENIUS_TRANSPORT_H_


