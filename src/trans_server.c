      
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author   madongfang
 * @date     2016-10-20
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include "base_fun.h"
#include "xbee.h"
#include "mqtt.h"

/**		  
 * @brief 网关数据转发服务
 * @param no_use 未使用
 * @return 无
 */
void *trans_server(void *no_use)
{
	UINT8 src_addr[XBEE_ADDR_LEN] = {0};
	INT8 addr_str[20] = {0};
	INT8 str[200] = {0};
	INT32 ret = 0;
	INT8 *ptr = NULL;
	time_t time_now = 0;
	struct tm tm_now;
	INT8 send_str[200] = {0};
	
	if (xbee_init() != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "xbee_init failed\n");
		return NULL;
	}

	FOREVER
	{
		memset(str, 0, sizeof(str));
		ret = xbee_recv_rf_pack(str, sizeof(str), src_addr);
		if (ret < 0)
		{
			DEBUG_PRINT(DEBUG_ERROR, "xbee_recv_rf_pack error\n");
			sleep(2);
			continue;
		}
		else if (0 == ret)
		{
			DEBUG_PRINT(DEBUG_INFO, "xbee_recv_rf_pack timeout\n");
			continue;
		}

		xbee_convert_addr_to_str(src_addr, addr_str, sizeof(addr_str));

		DEBUG_PRINT(DEBUG_NOTICE, "receive from xbee %s, pack=%s\n", addr_str, str);
		if ((ptr = strstr(str, "publish:")) != NULL) // 将数据publish到mqtt服务器
		{
			if (mqtt_is_connected())
			{
				if (mqtt_send(ptr + strlen("publish:")) != OK)
				{
					DEBUG_PRINT(DEBUG_ERROR, "mqtt_send failed\n");
				}
			}
			else
			{
				DEBUG_PRINT(DEBUG_WARN, "mqtt server is not connected\n");
			}
		}
		else if (strstr(str, "time") != NULL) // 校时
		{
			time_now = time(NULL);
			localtime_r(&time_now, &tm_now);
			strftime(send_str, sizeof(send_str), "%F %T\r", &tm_now);
			if (xbee_send_rf_pack(send_str, strlen(send_str), src_addr) != OK)
			{
				DEBUG_PRINT(DEBUG_ERROR, "xbee_send_rf_pack failed: destination xbee address=%s\n", addr_str);
			}
		}
		else if (strstr(str, "subscribe") != NULL) // 获取mqtt服务器下发的命令
		{
			snprintf(send_str, sizeof(send_str), "%s\r", mqtt_get_last_message());
			if (xbee_send_rf_pack(send_str, strlen(send_str), src_addr) != OK)
			{
				DEBUG_PRINT(DEBUG_ERROR, "xbee_send_rf_pack failed: destination xbee address=%s\n", addr_str);
			}
		}
		else
		{
			DEBUG_PRINT(DEBUG_WARN, "unknown xbee request pack=%s, xbee addr=%s\n", str, addr_str);
		}
	}

	return NULL;
}

