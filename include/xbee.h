      
/**@file 
 * @note
 * @brief  
 * 
 * @author   madongfang
 * @date     2015-5-19
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#ifndef _XBEE_H_
#define _XBEE_H_

#define MAX_RF_PACK_LEN 256
#define MAX_AT_PARAM_LEN 40
#define MAX_IO_SAMLE_LEN 10
#define XBEE_ADDR_LEN 8
#define XBEE_NET_ADDR_LEN 2

#define API_AT_CMD 0x08 // AT Command
#define API_AT_CMD_QPV 0x09 //AT Command - Queue Parameter Value
#define API_TRANS_REQ 0x10 // ZigBee Transmit Request
#define API_EXPLCT_ADDR_CMD 0x11 // Explicit Addressing ZigBee Command Frame
#define API_RMT_CMD_REQ 0x17 // Remote AT Command Request
#define API_CREAT_SRC_ROUTE 0x21 // Create Source Route
#define API_AT_CMD_RESP 0x88 // AT Command Response
#define API_MODEM_STATUS 0x8A //Modem Status
#define API_TRANS_STATUS 0x8B // ZigBee Trans Status
#define API_RECV_PACK 0x90 // ZigBee Receive Packet
#define API_EXPLCT_RX_IND 0x91 // ZigBee Explicit Rx Indicator
#define API_IO_RX_IND 0x92 // ZigBee IO Data Sample Rx Indicator
#define API_SENSOR_RD_IND 0x94 // Xbee Sensor Read Indicator
#define API_NODE_ID_IND 0x95 // Node Identification Indicator
#define API_RMT_CMD_RESP 0x97 // Remote Command Response
#define API_OTA_FW_UP_STATUS 0xA0 // Over-the-Air Firmware Update Status
#define API_ROUTE_REC_IND 0xA1 // Route Record Indicator
#define API_MTO_ROUTE_REQ_IND 0xA3 // Many-to-One Route Request Indicator

typedef struct
{
	UINT8 id;
	INT8 cmd[2];
	UINT8 value[MAX_AT_PARAM_LEN];
}AT_CMD;

typedef struct
{
	UINT8 id;
	UINT8 dst_addr[XBEE_ADDR_LEN];
	UINT8 dst_net_addr[XBEE_NET_ADDR_LEN];
	UINT8 broadcast_rad;
	UINT8 options;
	UINT8 data[MAX_RF_PACK_LEN];
}TRANS_REQ;

typedef struct
{
	UINT8 id;
	UINT8 dst_addr[XBEE_ADDR_LEN];
	UINT8 dst_net_addr[XBEE_NET_ADDR_LEN];
	UINT8 options;
	INT8 cmd[2];
	UINT8 value[MAX_AT_PARAM_LEN];
}RMT_CMD_REQ;

typedef struct
{
	UINT8 id;
	INT8 cmd[2];
	UINT8 status;
	UINT8 value[MAX_AT_PARAM_LEN];
}AT_CMD_RESP;

typedef struct
{
	UINT8 status;
}MODEM_STATUS;

typedef struct
{
	UINT8 id;
	UINT8 dst_net_addr[XBEE_NET_ADDR_LEN];
	UINT8 retry_count;
	UINT8 dlvry_status;
	UINT8 dscvry_status;
}TRANS_STATUS;

typedef struct
{
	UINT8 src_addr[XBEE_ADDR_LEN];
	UINT8 src_net_addr[XBEE_NET_ADDR_LEN];
	UINT8 options;
	UINT8 data[MAX_RF_PACK_LEN];
}RECV_PACK;

typedef struct
{
	UINT8 src_addr[XBEE_ADDR_LEN];
	UINT8 src_net_addr[XBEE_NET_ADDR_LEN];
	UINT8 options;
	UINT8 number;
	UINT8 digital_mask[2];
	UINT8 analog_mask;
	UINT8 samples[MAX_IO_SAMLE_LEN];
}IO_RX_IND;

typedef struct
{
	UINT8 id;
	UINT8 src_addr[XBEE_ADDR_LEN];
	UINT8 src_net_addr[XBEE_NET_ADDR_LEN];
	INT8 cmd[2];
	UINT8 status;
	UINT8 value[MAX_AT_PARAM_LEN];
}RMT_CMD_RESP;


typedef struct
{
	UINT8 api_id;
	union
	{
		AT_CMD at_cmd;
		TRANS_REQ trans_req;
		RMT_CMD_REQ rmt_cmd_req;
		AT_CMD_RESP at_cmd_resp;
		MODEM_STATUS modem_status;
		TRANS_STATUS trans_status;
		RECV_PACK recv_pack;
		IO_RX_IND io_rx_ind;
		RMT_CMD_RESP rmt_cmd_resp;
	}api_data;
}API_FRAME;

typedef struct
{
	INT32 frame_len;
	API_FRAME api_frame;
}XBEE_FRAME;

UINT8 xbee_get_modem_status(void);
UINT8 *xbee_get_cmd_data(void);
INT32 xbee_convert_addr_to_str(const UINT8 *p_addr, INT8 *p_str, UINT32 len);
void xbee_convert_str_to_addr(const INT8 *p_str, UINT8 *p_addr);
INT32 xbee_init(void);
INT32 xbee_send_at(const INT8 *p_cmd);
INT32 xbee_send_rf_pack(const void *p_data, INT32 len, const UINT8 *p_dst_addr);
INT32 xbee_recv_rf_pack(void *p_data, INT32 len, UINT8 *p_src_addr);
void xbee_set_print(BOOL need_print);

#endif

