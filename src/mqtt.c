      
/**@file 
 * @note 
 * @brief  
 * 
 * @author   madongfang
 * @date     2016-10-16
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include "base_fun.h"
#include "MQTTPacket.h"

#define MQTT_SERVER_ADDR "112.124.41.233"
#define MQTT_SERVER_PORT 61613
#define MQTT_USERNAME "admin"
#define MQTT_PASSWORD "nonadmin"

static INT32 mqtt_sock_fd = -1; ///< 连接mqtt服务器的套接字
static BOOL mqtt_connected = FALSE; ///< 是否已经和mqtt服务器建立连接
static INT8 last_message[400] = {0}; ///< 所订阅的主题最近一次返回的消息

static INT32 transport_getdata(UINT8 *buff, INT32 len)
{
	return readn(mqtt_sock_fd, buff, len);
}

static INT32 mqtt_connect(INT8 *p_client_id, UINT16 keep_alive_interval)
{
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	INT32 len = 0;
	UINT8 session_present = 0;
	UINT8 connack_rc = 0xff;
	INT32 ret = 0;
	UINT8 buff[200] = {0};
	struct sockaddr_in server_addr;

	if (NULL == p_client_id)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(MQTT_SERVER_PORT);
	inet_pton(AF_INET, MQTT_SERVER_ADDR, &server_addr.sin_addr);

	mqtt_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (ERROR == mqtt_sock_fd)
	{
		DEBUG_PRINT(DEBUG_ERROR, "socket failed: %s\n", strerror(errno));
		return ERROR;
	}

	if (connect(mqtt_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "connect failed: %s\n", strerror(errno));
		SAFE_CLOSE(mqtt_sock_fd);
		return ERROR;
	}
	
	/* 发送mqtt connect请求 */
	data.clientID.cstring = p_client_id;
	data.keepAliveInterval = keep_alive_interval; //服务器保持连接时间，超过该时间后，服务器会主动断开连接，单位为秒
	data.cleansession = 1;
	data.MQTTVersion = 3;
	data.username.cstring = MQTT_USERNAME;
	data.password.cstring = MQTT_PASSWORD;
	len = MQTTSerialize_connect(buff, sizeof(buff), &data);
	if (len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "MQTTSerialize_connect error ret=%d\n", len);
		SAFE_CLOSE(mqtt_sock_fd);
		return ERROR;
	}
	if (writen(mqtt_sock_fd, buff, len) < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "mqtt_sock_fd writen failed!\n");
		SAFE_CLOSE(mqtt_sock_fd);
		return ERROR;
	}

	/* 接收并处理mqtt connect响应 */
	if ((ret = MQTTPacket_read(buff, sizeof(buff), transport_getdata)) != CONNACK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "MQTTPacket_read CONNACK failed! ret=%d\n", ret);
		SAFE_CLOSE(mqtt_sock_fd);
		return ERROR;
	}
	if (MQTTDeserialize_connack(&session_present, &connack_rc, buff, sizeof(buff)) != 1 
		|| connack_rc != 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "CONNACK Return Code = 0x%02x\n", connack_rc);
		SAFE_CLOSE(mqtt_sock_fd);
		return ERROR;
	}

	mqtt_connected = TRUE;
	return OK;
}

static void mqtt_disconnect(void)
{
	UINT8 buff[100] = {0};
	INT32 len = 0;
	
	len = MQTTSerialize_disconnect(buff, sizeof(buff));
	if (len > 0)
	{
		if (writen(mqtt_sock_fd, buff, len) < 0)
		{
			DEBUG_PRINT(DEBUG_ERROR, "mqtt_sock_fd writen failed!\n");
		}
	}
	else
	{
		DEBUG_PRINT(DEBUG_ERROR, "MQTTSerialize_disconnect error ret=%d\n", len);
	}	

	SAFE_CLOSE(mqtt_sock_fd);
	
	mqtt_connected = FALSE;
	return;
}

static INT32 mqtt_subscrib(INT8 *p_topic)
{
	MQTTString subscribe_topic = MQTTString_initializer;
	INT32 len = 0;
	INT32 qos = 0;
	UINT8 buff[200] = {0};
	
	if (NULL == p_topic)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	if (!mqtt_connected)
	{
		DEBUG_PRINT(DEBUG_ERROR, "has not connected to mqtt server\n");
		return ERROR;
	}

	/* 发送subscrib请求 */
	subscribe_topic.cstring = p_topic;
	len = MQTTSerialize_subscribe(buff, sizeof(buff), 0, 1, 1, &subscribe_topic, &qos);
	if (len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "MQTTSerialize_subscribe error ret=%d\n", len);
		return ERROR;
	}
	if (writen(mqtt_sock_fd, buff, len) < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "mqtt_sock_fd writen failed!\n");
		return ERROR;
	}

	/* 接收subscrib响应 */
	if (MQTTPacket_read(buff, sizeof(buff), transport_getdata) != SUBACK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "MQTTPacket_read SUBACK failed!\n");
		return ERROR;
	}

	return OK;
}

static INT32 mqtt_publish(INT8 *p_topic, UINT8 *p_payload, INT32 payloadlen)
{
	MQTTString topic_string = MQTTString_initializer;
	INT32 len = 0;
	UINT8 buff[400] = {0};

	if (NULL == p_topic || NULL == p_payload)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	if (!mqtt_connected)
	{
		DEBUG_PRINT(DEBUG_ERROR, "has not connected to mqtt server\n");
		return ERROR;
	}

	topic_string.cstring = p_topic;
	len = MQTTSerialize_publish(buff, sizeof(buff), 0, 0, 0, 0, topic_string, p_payload, payloadlen);
	if (len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "MQTTSerialize_publish error ret=%d\n", len);
		return ERROR;
	}
	
	if (writen(mqtt_sock_fd, buff, len) < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "mqtt_sock_fd writen failed!\n");
		return ERROR;
	}
	
	return OK;
}

void *mqtt_daemon(void *no_use)
{
	fd_set rset;
	struct timeval timeout;
	INT32 ret = 0;
	INT32 pack_type = 0;
	UINT8 buff[400] = {0};
	UINT8 dup = 0;
	INT32 qos = 0;
	UINT8 retained = 0;
	UINT16 msgid = 0;
	MQTTString received_topic = MQTTString_initializer;
	UINT8 *p_payload = NULL;
	INT32 payload_len = 0;
	UINT8 mac_addr[6] = {0};
	INT8 mac_str[16] = {0};

	if (get_mac_addr("eth0", mac_addr, sizeof(mac_addr)) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "can not get mac address!\n");
		return NULL;
	}
	snprintf(mac_str, sizeof(mac_str), "%02X%02X%02X%02X%02X%02X"
		, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	
	FOREVER
	{
		if (mqtt_connect(mac_str, 0) != OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "mqtt_connect failed!\n");
			sleep(4);
			continue;
		}

		if (mqtt_subscrib("ServerToNode01") != OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "mqtt_subscrib failed!\n");
			mqtt_disconnect();
			sleep(4);
			continue;
		}

		FOREVER
		{
			FD_ZERO(&rset);
			FD_SET(mqtt_sock_fd, &rset);
			timeout.tv_sec = 4;
			timeout.tv_usec = 0;
			ret = select(mqtt_sock_fd + 1, &rset, NULL, NULL, &timeout);
			if (ret < 0)
			{
				if (EINTR == errno)
				{
					DEBUG_PRINT(DEBUG_NOTICE, "mqtt_sock_fd select: %s\n", strerror(errno));
					continue;
				}
				else
				{
					DEBUG_PRINT(DEBUG_ERROR, "mqtt_sock_fd select failed: %s\n", strerror(errno));
					break;
				}
			}
			else if (0 == ret)
			{
				DEBUG_PRINT(DEBUG_INFO, "mqtt_sock_fd select timeout\n");
				continue;
			}
			else
			{
				pack_type = MQTTPacket_read(buff, sizeof(buff), transport_getdata);
				if (pack_type < 0)
				{
					DEBUG_PRINT(DEBUG_ERROR, "MQTTPacket_read failed!\n");
					break;
				}

				switch (pack_type)
				{
					case PUBLISH:
						DEBUG_PRINT(DEBUG_NOTICE, "MQTTPacket_read PUBLISH\n");
						if (MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &received_topic
							, &p_payload, &payload_len, buff, sizeof(buff)) == 1)
						{
							memset(last_message, 0, sizeof(last_message));
							memcpy(last_message, p_payload, payload_len);
						}
						else
						{
							DEBUG_PRINT(DEBUG_ERROR, "MQTTDeserialize_publish error\n");
						}
						break;
					default:
						DEBUG_PRINT(DEBUG_WARN, "MQTTPacket_read packet type=%d\n", pack_type);
						break;
				}
			}
		}

		mqtt_disconnect();
	}
}

INT8 *mqtt_get_last_message(void)
{
	return last_message;
}

BOOL mqtt_is_connected(void)
{
	return mqtt_connected;
}

INT32 mqtt_send(INT8 *p_message)
{
	return mqtt_publish("NodeToServer01", (UINT8 *)p_message, strlen(p_message));
}
