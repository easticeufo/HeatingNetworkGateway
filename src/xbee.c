      
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

#include "base_fun.h"
#include "xbee_interface.h"
#include "xbee.h"

#define XBEE_DEBUG DEBUG_PRINT

#define XBEE_RESP_FRAME_SOCK_PATH "/tmp/xbee_resp_frame_sock" ///< 用于接收xbee模块API响应的Unix套接字路径
#define XBEE_RECV_FRAME_SOCK_PATH "/tmp/xbee_recv_frame_sock" ///< 用于接收远程xbee节点发送给本节点数据的Unix套接字路径

static INT32 xbee_resp_frame_sock = -1; ///< 用于接收xbee模块API响应的Unix套接字
static INT32 xbee_recv_frame_sock = -1; ///< 用于接收远程xbee节点发送给本节点数据的Unix套接字

static BOOL prt_xbee_frame = FALSE; ///< 是否打印获取到的xbee api frame数据
static UINT8 modem_status = 0; ///< 当前xbee模块状态
static UINT8 at_cmd_data[MAX_AT_PARAM_LEN] = {0}; ///< xbee模块AT命令返回的值

UINT8 xbee_get_modem_status(void)
{
	return modem_status;
}

UINT8 *xbee_get_cmd_data(void)
{
	return at_cmd_data;
}

INT32 xbee_convert_addr_to_str(const UINT8 *p_addr, INT8 *p_str, UINT32 len)
{
	if (NULL == p_addr || NULL == p_str)
	{
		XBEE_DEBUG(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	return snprintf(p_str, len, "%02X%02X%02X%02X%02X%02X%02X%02X"
		, p_addr[0], p_addr[1], p_addr[2], p_addr[3], p_addr[4], p_addr[5], p_addr[6], p_addr[7]);
}

void xbee_convert_str_to_addr(const INT8 *p_str, UINT8 *p_addr)
{
	if (NULL == p_addr || NULL == p_str)
	{
		XBEE_DEBUG(DEBUG_ERROR, "param error\n");
		return;
	}

	sscanf(p_str, "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx"
		, &p_addr[0], &p_addr[1], &p_addr[2], &p_addr[3]
		, &p_addr[4], &p_addr[5], &p_addr[6], &p_addr[7]);
	return;
}

static UINT8 xbee_check_sum(UINT8 *p_data, UINT32 len)
{
	UINT8 check_sum = 0;
	
	if (NULL == p_data)
	{
		XBEE_DEBUG(DEBUG_ERROR, "param error\n");
		return 0;
	}
	
	while (len-- > 0)
	{
		check_sum += *p_data;
		p_data++;
	}

	return (0xFF - check_sum);
}

static INT32 xbee_send_api_frame(const API_FRAME *p_api_frame, UINT16 len)
{
	UINT8 check_sum = 0;
	UINT8 head[4] = {0};

	if (NULL == p_api_frame)
	{
		XBEE_DEBUG(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	head[0] = 0x7E;
	head[1] = (0xff00 & len) >> 8;
	head[2] = (0xff & len);
	check_sum = xbee_check_sum((UINT8 *)p_api_frame, len);
	xbee_uart_sendn(head, 3);
	xbee_uart_sendn(p_api_frame, len);
	xbee_uart_sendn(&check_sum, 1);

	return OK;
}
	
static INT32 xbee_recv_api_frame(API_FRAME *p_api_frame)
{
	UINT8 head[4] = {0};
	UINT16 frame_len = 0;
	UINT8 check_sum = 0;
	
	if (NULL == p_api_frame)
	{
		XBEE_DEBUG(DEBUG_ERROR, "param error\n");
		return ERROR;
	}
	
	FOREVER
	{
		if (xbee_uart_recvn(&head[0], 1) != 1)
		{
			XBEE_DEBUG(DEBUG_INFO, "xbee_uart_recvn failed\n");
			continue;
		}
		if (head[0] != 0x7E)
		{
			continue;
		}

		if (xbee_uart_recvn(&head[1], 2) != 2)
		{
			XBEE_DEBUG(DEBUG_ERROR, "xbee_uart_recvn failed\n");
			return ERROR;
		}
		frame_len = head[1];
		frame_len = (frame_len << 8) | head[2];
		if (frame_len > sizeof(API_FRAME))
		{
			XBEE_DEBUG(DEBUG_ERROR, "frame len error(len=%d)\n", frame_len);
			return ERROR;
		}

		if (xbee_uart_recvn(p_api_frame, frame_len) != frame_len)
		{
			XBEE_DEBUG(DEBUG_ERROR, "xbee_uart_recvn failed\n");
			return ERROR;
		}
		if (xbee_uart_recvn(&check_sum, 1) != 1)
		{
			XBEE_DEBUG(DEBUG_ERROR, "xbee_uart_recvn failed\n");
			return ERROR;
		}

		if (xbee_check_sum((UINT8 *)p_api_frame, frame_len) == check_sum)
		{
			break;
		}
		else
		{
			XBEE_DEBUG(DEBUG_ERROR, "frame check sum error\n");
			return ERROR;
		}
	}

	return frame_len;
}

static void print_hex_data(const void *p_data, UINT32 len)
{
	UINT8 *ptr = (UINT8 *)p_data;

	while (len-- > 0)
	{
		if (len == 0) // 最后一个字符不打印空格
		{
			printf("0x%02X", *ptr);
		}
		else
		{
			printf("0x%02X ", *ptr);
		}
		ptr++;
	}
	return;
}

static void xbee_print_frame(const API_FRAME *p_api_frame, INT32 frame_len)
{
	INT32 i = 0;
	
	if (NULL == p_api_frame)
	{
		XBEE_DEBUG(DEBUG_ERROR, "param error\n");
	}
	
	printf("\n********************api frame********************\n");
	switch (p_api_frame->api_id)
	{
		case API_AT_CMD_RESP:
			printf("AT Command Response(Frame Type=0x%02X)\n", p_api_frame->api_id);
			printf("Frame ID=%d\n", p_api_frame->api_data.at_cmd_resp.id);
			printf("AT Command=%c%c\n", p_api_frame->api_data.at_cmd_resp.cmd[0], p_api_frame->api_data.at_cmd_resp.cmd[1]);
			printf("Command Status=%d\n", p_api_frame->api_data.at_cmd_resp.status);
			printf("Command Data=");
			print_hex_data(p_api_frame->api_data.at_cmd_resp.value
				, frame_len - (p_api_frame->api_data.at_cmd_resp.value - (UINT8 *)p_api_frame));
			printf("\n");
			break;
		case API_MODEM_STATUS:
			printf("Modem Status(Frame Type=0x%02X)\n", p_api_frame->api_id);
			printf("Status=0x%02X\n", p_api_frame->api_data.modem_status.status);
			break;
		case API_TRANS_STATUS:
			printf("ZigBee Transmit Status(Frame Type=0x%02X)\n", p_api_frame->api_id);
			printf("Frame ID=%d\n", p_api_frame->api_data.trans_status.id);
			printf("16-bit address of destination=%02X%02X\n"
				, p_api_frame->api_data.trans_status.dst_net_addr[0]
				, p_api_frame->api_data.trans_status.dst_net_addr[1]);
			printf("Transmit Retry Count=%d\n", p_api_frame->api_data.trans_status.retry_count);
			printf("Delivery Status=0x%02X\n", p_api_frame->api_data.trans_status.dlvry_status);
			printf("Discovery Status=0x%02X\n", p_api_frame->api_data.trans_status.dscvry_status);
			break;
		case API_RECV_PACK:
			printf("ZigBee Receive Packet(Frame Type=0x%02X)\n", p_api_frame->api_id);
			printf("64-bit Source Address=");
			for (i = 0; i < XBEE_ADDR_LEN; i++)
			{
				printf("%02X", p_api_frame->api_data.recv_pack.src_addr[i]);
			}
			printf("\n");
			printf("16-bit Source Network Address=%02X%02X\n"
				, p_api_frame->api_data.recv_pack.src_net_addr[0]
				, p_api_frame->api_data.recv_pack.src_net_addr[1]);
			printf("Receive Options=0x%02X\n", p_api_frame->api_data.recv_pack.options);
			printf("Received Data=");
			print_hex_data(p_api_frame->api_data.recv_pack.data
				, frame_len - (p_api_frame->api_data.recv_pack.data - (UINT8 *)p_api_frame));
			printf("\n");
			break;
		case API_IO_RX_IND:
			printf("ZigBee IO Data Sample Rx Indicator(Frame Type=0x%02X)\n", p_api_frame->api_id);
			printf("64-bit Source Address=");
			for (i = 0; i < XBEE_ADDR_LEN; i++)
			{
				printf("%02X", p_api_frame->api_data.io_rx_ind.src_addr[i]);
			}
			printf("\n");
			printf("16-bit Source Network Address=%02X%02X\n"
				, p_api_frame->api_data.io_rx_ind.src_net_addr[0]
				, p_api_frame->api_data.io_rx_ind.src_net_addr[1]);
			printf("Receive Options=0x%02X\n", p_api_frame->api_data.io_rx_ind.options);
			printf("Number of Samples=0x%02X\n", p_api_frame->api_data.io_rx_ind.number);
			printf("Digital Channel Mask*=0x%02X%02X\n", p_api_frame->api_data.io_rx_ind.digital_mask[0]
				, p_api_frame->api_data.io_rx_ind.digital_mask[1]);
			printf("Analog Channel Mask**=0x%02X\n", p_api_frame->api_data.io_rx_ind.analog_mask);
			printf("IO Sample Data=");
			print_hex_data(p_api_frame->api_data.io_rx_ind.samples
				, frame_len - (p_api_frame->api_data.io_rx_ind.samples - (UINT8 *)p_api_frame));
			printf("\n");
			break;
		case API_RMT_CMD_RESP:
			printf("Remote Command Response(Frame Type=0x%02X)\n", p_api_frame->api_id);
			printf("Frame ID=%d\n", p_api_frame->api_data.rmt_cmd_resp.id);
			printf("64-bit Source Address=");
			for (i = 0; i < XBEE_ADDR_LEN; i++)
			{
				printf("%02X", p_api_frame->api_data.rmt_cmd_resp.src_addr[i]);
			}
			printf("\n");
			printf("16-bit Source Network Address=%02X%02X\n"
				, p_api_frame->api_data.rmt_cmd_resp.src_net_addr[0]
				, p_api_frame->api_data.rmt_cmd_resp.src_net_addr[1]);
			printf("AT Command=%c%c\n", p_api_frame->api_data.rmt_cmd_resp.cmd[0], p_api_frame->api_data.rmt_cmd_resp.cmd[1]);
			printf("Command Status=%d\n", p_api_frame->api_data.rmt_cmd_resp.status);
			printf("Command Data=");
			print_hex_data(p_api_frame->api_data.rmt_cmd_resp.value
				, frame_len - (p_api_frame->api_data.rmt_cmd_resp.value - (UINT8 *)p_api_frame));
			printf("\n");
			break;
		default:
			printf("Unsupport print this frame(Frame Type=0x%02X)\n", p_api_frame->api_id);
			break;
	}
	printf("***********************end***********************\n");
	
	return;
}

static void *xbee_frame_daemon(void *no_use)
{
	INT32 sock_fd = -1;
	struct sockaddr_un sock_addr;
	XBEE_FRAME xbee_frame;

	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sun_family = AF_LOCAL;
	
	sock_fd	= socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (ERROR == sock_fd)
	{
		XBEE_DEBUG(DEBUG_ERROR, "socket failed: %s\n", strerror(errno));
		return NULL;
	}

	FOREVER
	{
		if ((xbee_frame.frame_len = xbee_recv_api_frame(&xbee_frame.api_frame)) == ERROR)
		{
			XBEE_DEBUG(DEBUG_ERROR, "receive an api frame error\n");
			sleep(2);
			continue;
		}
		
		if (prt_xbee_frame)
		{
			xbee_print_frame(&xbee_frame.api_frame, xbee_frame.frame_len);
		}
		
		switch (xbee_frame.api_frame.api_id)
		{
			case API_MODEM_STATUS:
				modem_status = xbee_frame.api_frame.api_data.modem_status.status;
				break;
			case API_RECV_PACK:
				strcpy(sock_addr.sun_path, XBEE_RECV_FRAME_SOCK_PATH);
				if (sendto(sock_fd, &xbee_frame, sizeof(XBEE_FRAME), MSG_DONTWAIT
					, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == ERROR)
				{
					XBEE_DEBUG(DEBUG_ERROR, "sendto failed: %s\n", strerror(errno));
				}
				break;
			case API_AT_CMD_RESP:
			case API_TRANS_STATUS:
				strcpy(sock_addr.sun_path, XBEE_RESP_FRAME_SOCK_PATH);
				if (sendto(sock_fd, &xbee_frame, sizeof(XBEE_FRAME), MSG_DONTWAIT
					, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == ERROR)
				{
					XBEE_DEBUG(DEBUG_ERROR, "sendto failed: %s\n", strerror(errno));
				}
				break;
			default:
				XBEE_DEBUG(DEBUG_WARN, "do not process this frame(Frame Type=0x%02X)\n"
					, xbee_frame.api_frame.api_id);
				break;
		}
	}
}

INT32 xbee_init(void)
{
	if (xbee_uart_init() != OK)
	{
		XBEE_DEBUG(DEBUG_ERROR, "xbee_uart_init failed\n");
		return ERROR;
	}

	xbee_resp_frame_sock = unix_recv_socket_init(XBEE_RESP_FRAME_SOCK_PATH);
	if (ERROR == xbee_resp_frame_sock)
	{
		XBEE_DEBUG(DEBUG_ERROR, "unix_recv_socket_init xbee_resp_frame_sock failed\n");
		return ERROR;
	}

	xbee_recv_frame_sock = unix_recv_socket_init(XBEE_RECV_FRAME_SOCK_PATH);
	if (ERROR == xbee_recv_frame_sock)
	{
		XBEE_DEBUG(DEBUG_ERROR, "unix_recv_socket_init xbee_recv_frame_sock failed\n");
		SAFE_CLOSE(xbee_resp_frame_sock);
		return ERROR;
	}

	/* 启动xbee串口数据frame守护线程 */
	if(thread_create(xbee_frame_daemon, NULL) != OK)
	{
		XBEE_DEBUG(DEBUG_ERROR, "create thread xbee_frame_daemon error\n");
		SAFE_CLOSE(xbee_resp_frame_sock);
		SAFE_CLOSE(xbee_recv_frame_sock);
		return ERROR;
	}

	return OK;
}

static INT32 str_to_hexstr(const INT8 *str, INT32 str_len, UINT8 *hexstr, INT32 len)
{
	INT32 i = 0;
	INT32 tmp = 0;
	
	if (NULL == str || NULL == hexstr || 0 == len)
	{
		XBEE_DEBUG(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	if (str_len % 2 == 1)
	{
		sscanf(str, "%1x", &tmp);
		str++;
		str_len--;
		hexstr[i] = tmp;
		i++;
	}

	while (str_len > 0 && i < len)
	{
		sscanf(str, "%2x", &tmp);
		str += 2;
		str_len -= 2;
		hexstr[i] = tmp;
		i++;
	}

	return i;
}

INT32 xbee_send_at(const INT8 *p_cmd)
{
	XBEE_FRAME xbee_frame;
	API_FRAME *p_api_frame = &xbee_frame.api_frame;
	AT_CMD *p_at_cmd = NULL;
	AT_CMD_RESP *p_at_cmd_resp = NULL;
	INT32 len = 0;

	if (NULL == p_cmd)
	{
		XBEE_DEBUG(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	len = strlen(p_cmd);
	if (len < 2 || len > (MAX_AT_PARAM_LEN + 2))
	{
		XBEE_DEBUG(DEBUG_ERROR, "cmd len error\n");
		return ERROR;
	}

	/* 发送AT命令帧 */
	p_api_frame->api_id = API_AT_CMD;
	p_at_cmd = &p_api_frame->api_data.at_cmd;
	memset(p_at_cmd, 0, sizeof(AT_CMD));
	p_at_cmd->id = 0x1;
	memcpy(p_at_cmd->cmd, p_cmd, 2);
	len -= 2;
	if ((memcmp(p_at_cmd->cmd, "NI", 2) == 0) || (memcmp(p_at_cmd->cmd, "ND", 2) == 0))
	{
		memcpy(p_at_cmd->value, p_cmd + 2, len);
	}
	else
	{
		len = str_to_hexstr(p_cmd + 2, len, p_at_cmd->value, sizeof(p_at_cmd->value));
		if (ERROR == len)
		{
			XBEE_DEBUG(DEBUG_ERROR, "str_to_hexstr failed\n");
			return ERROR;
		}
	}
	len += (p_at_cmd->value - (UINT8 *)p_api_frame);
	xbee_send_api_frame(p_api_frame, len);

	/* 接收并解析AT响应帧 */
	if (recv_dgram(xbee_resp_frame_sock, &xbee_frame, sizeof(xbee_frame)) <= 0)
	{
		XBEE_DEBUG(DEBUG_ERROR, "recv_dgram failed\n");
		return ERROR;
	}
	if (API_AT_CMD_RESP != p_api_frame->api_id)
	{
		XBEE_DEBUG(DEBUG_ERROR, "receive mq_xbee_resp_frame error(api_id=0x%02X)\n", p_api_frame->api_id);
		return ERROR;
	}
	p_at_cmd_resp = &p_api_frame->api_data.at_cmd_resp;
	if (memcmp(p_at_cmd_resp->cmd, p_cmd, 2) != 0)
	{
		XBEE_DEBUG(DEBUG_ERROR, "xbee_recv_api_frame cmd error\n");
		return ERROR;
	}

	switch (p_at_cmd_resp->status)
	{
		case 0: // OK
			break;
		case 1: // ERROR
			XBEE_DEBUG(DEBUG_ERROR, "xbee_at_cmd error(status=ERROR)\n");
			return ERROR;
		case 2: // Invalid Command
			XBEE_DEBUG(DEBUG_ERROR, "xbee_at_cmd error(status=Invalid Command)\n");
			return ERROR;
		case 3: // Invalid Parameter
			XBEE_DEBUG(DEBUG_ERROR, "xbee_at_cmd error(status=Invalid Parameter)\n");
			return ERROR;
		case 4: // Tx Failure
			XBEE_DEBUG(DEBUG_ERROR, "xbee_at_cmd error(status=Tx Failure)\n");
			return ERROR;
		default:
			XBEE_DEBUG(DEBUG_ERROR, "xbee_at_cmd error(status=unknown)\n");
			return ERROR; 
	}

	len = xbee_frame.frame_len;
	len -= (p_at_cmd_resp->value - (UINT8 *)p_api_frame);
	if (len > MAX_AT_PARAM_LEN)
	{
		XBEE_DEBUG(DEBUG_ERROR, "API_AT_CMD_RESP data len exceed(len=%d)\n", len);
		return ERROR;
	}
	memset(at_cmd_data, 0, sizeof(at_cmd_data));
	memcpy(at_cmd_data, p_at_cmd_resp->value, len);

	return len;
}

INT32 xbee_send_rf_pack(const void *p_data, INT32 len, const UINT8 *p_dst_addr)
{
	XBEE_FRAME xbee_frame;
	API_FRAME *p_api_frame = &xbee_frame.api_frame;
	TRANS_REQ *p_trans_req = NULL;
	TRANS_STATUS *p_trans_status = NULL;

	if ((NULL == p_dst_addr) || (NULL == p_data) || (len < 0))
	{
		XBEE_DEBUG(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	/* 发送一个zigbee数据包 */
	if (len > MAX_RF_PACK_LEN)
	{
		XBEE_DEBUG(DEBUG_ERROR, "RF packet length is exceed(MAX_RF_PACK_LEN=%d, len=%d)\n", MAX_RF_PACK_LEN, len);
		return ERROR;
	}
	p_api_frame->api_id = API_TRANS_REQ;
	p_trans_req = &p_api_frame->api_data.trans_req;
	memset(p_trans_req, 0, sizeof(TRANS_REQ));
	p_trans_req->id = 0x01;
	memcpy(p_trans_req->dst_addr, p_dst_addr, sizeof(p_trans_req->dst_addr));
	p_trans_req->dst_net_addr[0] = 0xFF;
	p_trans_req->dst_net_addr[1] = 0xFE;
	memcpy(p_trans_req->data, p_data, len);
	xbee_send_api_frame(p_api_frame, (p_trans_req->data - (UINT8 *)p_api_frame + len));

	/* 接收发送状态 */
	if (recv_dgram(xbee_resp_frame_sock, &xbee_frame, sizeof(xbee_frame)) <= 0)
	{
		XBEE_DEBUG(DEBUG_ERROR, "recv_dgram failed\n");
		return ERROR;
	}
	if (API_TRANS_STATUS != p_api_frame->api_id)
	{
		XBEE_DEBUG(DEBUG_ERROR, "receive mq_xbee_resp_frame error(api_id=0x%02X)\n", p_api_frame->api_id);
		return ERROR;
	}
	
	p_trans_status = &p_api_frame->api_data.trans_status;
	if (p_trans_status->dlvry_status != 0)
	{
		XBEE_DEBUG(DEBUG_ERROR, "packed send failed(Delivery Status=0x%02X)\n", p_trans_status->dlvry_status);
		return ERROR;
	}

	return OK;
}

INT32 xbee_recv_rf_pack(void *p_data, INT32 len, UINT8 *p_src_addr)
{
	XBEE_FRAME xbee_frame;
	API_FRAME *p_api_frame = &xbee_frame.api_frame;
	RECV_PACK *p_recv_pack = NULL;
	INT32 rf_data_len = 0;
	INT32 ret = 0;

	if ((NULL == p_data) || (len < 0))
	{
		XBEE_DEBUG(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	ret = recv_dgram(xbee_recv_frame_sock, &xbee_frame, sizeof(xbee_frame));
	if (ret < 0)
	{
		XBEE_DEBUG(DEBUG_ERROR, "recv_dgram error\n");
		return ERROR;
	}
	else if (0 == ret)
	{
		XBEE_DEBUG(DEBUG_INFO, "recv_dgram timeout\n");
		return 0;
	}
	
	if (API_RECV_PACK != p_api_frame->api_id)
	{
		XBEE_DEBUG(DEBUG_ERROR, "receive mq_xbee_recv_frame error(api_id=0x%02X)\n", p_api_frame->api_id);
		return ERROR;
	}
	
	p_recv_pack = &p_api_frame->api_data.recv_pack;
	if (NULL != p_src_addr)
	{
		memcpy(p_src_addr, p_recv_pack->src_addr, sizeof(p_recv_pack->src_addr));
	}

	rf_data_len = xbee_frame.frame_len - (p_recv_pack->data - (UINT8 *)p_api_frame);
	if (len < rf_data_len)
	{
		memcpy(p_data, p_recv_pack->data, len);
		return len;
	}
	else
	{
		memcpy(p_data, p_recv_pack->data, rf_data_len);
		return rf_data_len;
	}
}

/**		  
 * @brief		设置是否打印xbee串口收到的一帧数据
 * @param[in] need_print 是否需要打印
 * @return 无
 */
void xbee_set_print(BOOL need_print)
{
	prt_xbee_frame = need_print;
}

