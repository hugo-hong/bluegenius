/*********************************************************************************
   Bluegenius - Bluetooth host protocol stack for Linux/android/windows...
   Copyright (C)
   Written 2017 by hugo��yongguang hong�� <hugo.08@163.com>
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
#ifndef _BLUEGENIUS_HCI_H_
#define _BLUEGENIUS_HCI_H_

#define HCI_ACL_MAX_SIZE	1024
#define HCI_MAX_FRAME_SIZE (HCI_ACL_MAX_SIZE + 4/*acl data header*/)

/* Message event mask across Host/Controller lib and stack */
#define MSG_EVT_MASK                    0xFF00 /* eq. BT_EVT_MASK */
#define MSG_SUB_EVT_MASK                0x00FF /* eq. BT_SUB_EVT_MASK */

/* Message event ID passed from Host/Controller lib to stack */
#define MSG_HC_TO_STACK_HCI_ERR        0x1300 /* eq. BT_EVT_TO_BTU_HCIT_ERR */
#define MSG_HC_TO_STACK_HCI_ACL        0x1100 /* eq. BT_EVT_TO_BTU_HCI_ACL */
#define MSG_HC_TO_STACK_HCI_SCO        0x1200 /* eq. BT_EVT_TO_BTU_HCI_SCO */
#define MSG_HC_TO_STACK_HCI_EVT        0x1000 /* eq. BT_EVT_TO_BTU_HCI_EVT */
#define MSG_HC_TO_STACK_L2C_SEG_XMIT   0x1900 /* eq. BT_EVT_TO_BTU_L2C_SEG_XMIT */

/* Message event ID passed from stack to vendor lib */
#define MSG_STACK_TO_HC_HCI_ACL        0x2100 /* eq. BT_EVT_TO_LM_HCI_ACL */
#define MSG_STACK_TO_HC_HCI_SCO        0x2200 /* eq. BT_EVT_TO_LM_HCI_SCO */
#define MSG_STACK_TO_HC_HCI_CMD        0x2000 /* eq. BT_EVT_TO_LM_HCI_CMD */

/* Local Bluetooth Controller ID for BR/EDR */
#define LOCAL_BR_EDR_CONTROLLER_ID      0

typedef enum {
	COMMAND_PACKET = 1,
	ACL_PACKET = 2,
	SCO_PACKET = 3,
	EVENT_PACKET = 4
} hci_packet_type_t;

typedef struct {
	uint16_t          event;
	uint16_t          len;
	uint16_t          offset;
	uint16_t          layer_specific;
	uint8_t           data[];
} BT_HDR;
#define BT_HDR_SIZE		(sizeof(BT_HDR))



#endif // _BLUEGENIUS_HCI_H_
