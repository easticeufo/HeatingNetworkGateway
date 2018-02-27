
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author   madongfang
 * @date     2016-6-1
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include "base_fun.h"
#include "appweb.h"
#include "cJSON.h"
#include "pack.h"

#define UPGRADE_DIR "/home/user" // �����ļ����Ŀ¼
#define MAX_LOGIN_NUM 32 // ����û���½��

extern void md5(const uint8_t *initial_msg, size_t initial_len, uint8_t *digest);

/**		  
 * @brief �豸������������ͬʱֻ����һ��http���ӿ��Խ����豸����
 */
typedef struct
{
	UINT8 *p_buff; // ���firmware�ļ��Ļ�����
	INT32 buff_len; // ������������С
	INT32 end_idx; // ��ǰ�Ѿ���ŵ��������Ľ�β����λ��+1
	HttpConn *conn; // ӵ�е�ǰ������������http����
}UPGRADE_BUFF;

/**		  
 * @brief �û���½��֤��Ϣ
 */
typedef struct
{
	INT8 ip[16]; // ��¼�Ŀͻ���IP��ַ
	INT8 username[48]; // ��¼���û���
	struct timeval login_time; // ��¼ʱ��
	INT32 random_num; // ��¼ʱ���ɵ������
	INT8 session_id[36]; // ����ip username login_time random_num���ɵĻỰID����������֤
	time_t last_action_time; // ��һ�β�����ʱ�䣬�û���������û�����ʱѡ��ɾ�����û�
}LOGIN_INFO;

static UPGRADE_BUFF upgrade_buff = {NULL, 0, 0, NULL};
static LOGIN_INFO g_login_info[MAX_LOGIN_NUM];

void print_login_info(void)
{
	INT32 i = 0;
	struct tm login_time_tm;
	struct tm last_action_tm;
	INT8 time_str[32] = {0};

	printf("================================\n");
	printf("Login Client Info:\n");
	printf("================================\n");
	
	for (i = 0; i < MAX_LOGIN_NUM; i++)
	{
		if (g_login_info[i].username[0] != '\0')
		{
			printf("login ip: %s\n", g_login_info[i].ip);
			printf("login username: %s\n", g_login_info[i].username);
			localtime_r(&g_login_info[i].login_time.tv_sec, &login_time_tm);
			strftime(time_str, sizeof(time_str), "%FT%T%z", &login_time_tm);
			printf("login time: %s(sec=%ld, usec=%ld)\n", time_str
				, g_login_info[i].login_time.tv_sec, g_login_info[i].login_time.tv_usec);
			printf("login random number: %d\n", g_login_info[i].random_num);
			localtime_r(&g_login_info[i].last_action_time, &last_action_tm);
			strftime(time_str, sizeof(time_str), "%FT%T%z", &last_action_tm);
			printf("last action time: %s(sec=%ld)\n", time_str, g_login_info[i].last_action_time);
			printf("================================\n");
		}
	}
	
	return;
}

/**		  
 * @brief		���õ�¼�û���Ϣ���������Ӧ��session id
 * @param[in] p_ipaddr ��¼�Ŀͻ���IP��ַ
 * @param[in] p_username ��¼�û���
 * @return	���ض�Ӧ�ĵ�¼��Ϣ�ṹ��ָ��
 */
static LOGIN_INFO *set_login_info(const INT8 *p_ipaddr, const INT8 *p_username)
{
	INT32 i = 0;
	INT8 data[128] = {0};
	INT32 len = 0;
	UINT8 md5_result[16] = {0};
	LOGIN_INFO login_info;
	time_t min_last_action_time = 0;
	INT32 idx = 0;
	
	if (NULL == p_ipaddr || NULL == p_username)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return NULL;
	}

	/* ���õ�¼��Ϣ */
	memset(&login_info, 0, sizeof(LOGIN_INFO));
	snprintf(login_info.ip, sizeof(login_info.ip), "%s", p_ipaddr);
	snprintf(login_info.username, sizeof(login_info.username), "%s", p_username);
	gettimeofday(&login_info.login_time, NULL);
	login_info.random_num = random();
	len = snprintf(data, sizeof(data), "%s%s%ld%ld%d", p_ipaddr, p_username
		, login_info.login_time.tv_sec, login_info.login_time.tv_usec
		, login_info.random_num);
	md5((UINT8 *)data, len, md5_result);
	for (i = 0; i < sizeof(md5_result); i++)
	{
		sprintf(login_info.session_id + 2*i, "%02x", md5_result[i]);
	}
	login_info.last_action_time = login_info.login_time.tv_sec;

	/* ����ѡ��һ�����еĵ�¼��Ϣ�����������������е�¼��Ϣ��ѡ���ʱ��δ�����Ľ��е�¼ */
	for (i = 0; i < MAX_LOGIN_NUM; i++)
	{
		if (g_login_info[i].username[0] == '\0')
		{
			idx = i;
			break;
		}
		else
		{
			if (0 == min_last_action_time)
			{
				min_last_action_time = g_login_info[i].last_action_time;
			}
			
			if (min_last_action_time > g_login_info[i].last_action_time)
			{
				min_last_action_time = g_login_info[i].last_action_time;
				idx = i;
			}
		}
	}
	memcpy(&g_login_info[idx], &login_info, sizeof(LOGIN_INFO));

	return &g_login_info[idx];
}

static INT32 api_system_login_process(HttpConn *conn, INT8 *p_body_buff, INT32 buff_len)
{
	cJSON *json = NULL;
	cJSON *json_item = NULL;
	INT8 username[48];
	INT8 password[48];
	LOGIN_INFO *p_login_info = NULL;
	
	if (NULL == conn || NULL == conn->rx || NULL == conn->rx->method 
		|| NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(conn->rx->method, "POST") == 0)
	{
		json = cJSON_Parse(p_body_buff);
		if (NULL == json)
		{
			DEBUG_PRINT(DEBUG_ERROR, "cJSON_Parse error:%s\n", cJSON_GetErrorPtr());
			snprintf(p_body_buff, buff_len, "{\"error\":\"json data format error!\"}");
			return ERROR;
		}

		json_item = cJSON_GetObjectItem(json, "username");
		if (json_item != NULL)
		{
			snprintf(username, sizeof(username), "%s", json_item->valuestring);
		}

		json_item = cJSON_GetObjectItem(json, "password");
		if (json_item != NULL)
		{
			snprintf(password, sizeof(password), "%s", json_item->valuestring);
		}
		cJSON_Delete(json);

		if ('\0' == username[0] || '\0' == password[0])
		{
			DEBUG_PRINT(DEBUG_NOTICE, "no username or password\n");
			snprintf(p_body_buff, buff_len, "{\"error\":\"no username or password!\"}");
			return ERROR;
		}

		if (strcmp("admin", username) == 0 
			&& strcmp("827ccb0eea8a706c4c34a16891f84e7b", password) == 0) // ����Ϊ"12345"��MD5����ֵ
		{
			p_login_info = set_login_info(conn->ip, username);

			if (p_login_info != NULL)
			{
				httpSetCookie(conn, "session_id", p_login_info->session_id, "/api", "", 0, 0);
				snprintf(p_body_buff, buff_len, "{\"error\":\"OK\"}");
			}
			else
			{
				snprintf(p_body_buff, buff_len, "{\"error\":\"exceed max login number!\"}");
				return ERROR;
			}
		}
		else
		{
			DEBUG_PRINT(DEBUG_NOTICE, "username or password is wrong\n");
			snprintf(p_body_buff, buff_len, "{\"error\":\"username or password is wrong!\"}");
			return ERROR;
		}
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", conn->rx->method);
		return ERROR;
	}
	
	return OK;
}

static INT32 api_system_logout_process(HttpConn *conn, INT8 *p_body_buff, INT32 buff_len)
{
	INT32 i = 0;
	const INT8 *p_session_id = NULL;
	
	if (NULL == conn || NULL == conn->rx || NULL == conn->rx->method 
		|| NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(conn->rx->method, "POST") == 0)
	{
		p_session_id = httpGetCookie(conn, "session_id");
		if (NULL == p_session_id)
		{
			DEBUG_PRINT(DEBUG_NOTICE, "no session id in Cookie\n");
			
		}
		else
		{
			for (i = 0; i < MAX_LOGIN_NUM; i++)
			{
				if (g_login_info[i].username[0] != '\0' 
					&& strcmp(p_session_id, g_login_info[i].session_id) == 0)
				{
					httpSetCookie(conn, "session_id", p_session_id, "/api", "", -1, 0); // ɾ��cookie
					httpSetCookie(conn, "session_id", p_session_id, "/websocket", "", -1, 0); // ɾ��cookie

					memset(&g_login_info[i], 0, sizeof(LOGIN_INFO));

					DEBUG_PRINT(DEBUG_NOTICE, "delete matched session id successfully\n");
					break;
				}
			}
		}

	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", conn->rx->method);
		return ERROR;
	}

	snprintf(p_body_buff, buff_len, "{\"error\":\"OK\"}");
	return OK;
}

/**		  
 * @brief		��֤�û���Ϣ
 * @param[in] conn ��ǰappweb����
 * @return	ͨ����֤����OK�����򷵻�ERROR
 */
INT32 check_auth_permission(HttpConn *conn)
{
	const INT8 *p_session_id = NULL;
	INT32 i = 0;

	if (NULL == conn || NULL == conn->ip)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	p_session_id = httpGetCookie(conn, "session_id");

	if (NULL == p_session_id)
	{
		DEBUG_PRINT(DEBUG_NOTICE, "no session id in Cookie\n");
		return ERROR;
	}

	for (i = 0; i < MAX_LOGIN_NUM; i++)
	{
		if (g_login_info[i].username[0] != '\0' 
			&& strcmp(p_session_id, g_login_info[i].session_id) == 0
			&& strcmp(conn->ip, g_login_info[i].ip) == 0)
		{
			g_login_info[i].last_action_time = time(NULL);
			return OK;
		}
	}

	DEBUG_PRINT(DEBUG_WARN, "no matched session id\n");
	return ERROR;
}

/**		  
 * @brief		 ��ȡuri�е�ǰ��һ����ַ�������buff�з��أ����ҷ�����һ�����ʼλ��
 * @param[in]  uri ��ǰ���uri��ʼλ�ã�һ����'/'��ͷ
 * @param[out] buff ���ڷ��ص�ǰ��һ����ַ����Ļ�����
 * @param[in]  buff_len ����������
 * @return	 ������һ�����ʼλ�ã����û����һ���򷵻�NULL
 */
static INT8 *parse_uri_layer(const INT8 *uri, INT8 *buff, INT32	buff_len)
{
	INT8 *ptr = NULL;
	INT32 len = 0;

	if (NULL == uri || NULL == buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return NULL;
	}
	
	if ('/' == uri[0])
	{
		uri++;
	}

	ptr = strchr(uri, '/');
	if (ptr != NULL)
	{
		len = ptr - uri;
		if (len >= buff_len)
		{
			memcpy(buff, uri, buff_len - 1);
			buff[buff_len - 1] = '\0';
		}
		else
		{
			memcpy(buff, uri, len);
			buff[len] = '\0';
		}
	}
	else // �Ѿ������һ��
	{
		strncpy(buff, uri, buff_len - 1);
		buff[buff_len - 1] = '\0';
	}
	
	return ptr;
}

static INT32 api_system_info_process(const INT8 *method, INT8 *p_body_buff, INT32 buff_len)
{
	INT8 build_date[20] = {0};
	
	if (NULL == method || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(method, "GET") == 0)
	{
		get_build_date(build_date, sizeof(build_date));
		snprintf(p_body_buff, buff_len, "{\"version\":\"V1.0.0 %s\"}", build_date);
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
	return OK;
}

static INT32 api_system_time_process(const INT8 *method, INT8 *p_body_buff, INT32 buff_len)
{
	time_t time_now = time(NULL);
	struct tm tm_now;
	INT8 time_str[64] = {0};
	cJSON *json = NULL;
	cJSON *json_item = NULL;
	INT32 year = 0;
	INT32 month = 0;
	INT32 day = 0;
	INT32 hour = 0;
	INT32 min = 0;
	INT32 sec = 0;
	INT8 tz[8] = {0};
	INT32 ret = 0;
	
	if (NULL == method || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	if (strcmp(method, "GET") == 0)
	{
		gmtime_r(&time_now, &tm_now);
		strftime(time_str, sizeof(time_str), "%FT%TZ", &tm_now);
		snprintf(p_body_buff, buff_len, "{\"utcTime\":\"%s\"}", time_str);
	}
	else if (strcmp(method, "PUT") == 0)
	{
		json = cJSON_Parse(p_body_buff);
		if (NULL == json)
		{
			DEBUG_PRINT(DEBUG_ERROR, "cJSON_Parse error:%s\n", cJSON_GetErrorPtr());
			snprintf(p_body_buff, buff_len, "{\"error\":\"json data format error!\"}");
			return ERROR;
		}

		json_item = cJSON_GetObjectItem(json, "utcTime");
		if (json_item != NULL)
		{
			sscanf(json_item->valuestring, "%d-%d-%dT%d:%d:%d%s"
				, &year, &month, &day, &hour, &min, &sec, tz);
			if (sec < 0 || sec > 59 || min < 0 || min > 59 || hour < 0 || hour > 23 
				|| day < 1 || day > 31 || month < 1 || month > 12 || year > 2099)
			{
				DEBUG_PRINT(DEBUG_WARN, "time format error\n");
				snprintf(p_body_buff, buff_len, "{\"error\":\"time format error!\"}");
				return ERROR;
			}
			else
			{
				snprintf(time_str, sizeof(time_str), "date -s \"%04d-%02d-%02d %02d:%02d:%02d\" -u"
					, year, month, day, hour, min, sec);
				ret = system(time_str); // ����ϵͳʱ��
				DEBUG_PRINT(DEBUG_NOTICE, "system() ret=%d\n", ret);
				ret = system("hwclock -w --utc"); // д��RTC����
				DEBUG_PRINT(DEBUG_NOTICE, "system() ret=%d\n", ret);
				snprintf(p_body_buff, buff_len, "{\"error\":\"OK\"}");
			}
		}
		cJSON_Delete(json);
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
	return OK;
}

static INT32 firmware_upgrade(const UINT8 *p_firm_buff, INT32 firm_len)
{
	const FIRMWARE_HEADER *p_firm_header = NULL;
	const UPGRADE_FILE_HEADER *p_file_header = NULL;
	const UINT8 *p_file_buff = NULL;
	INT32 file_num = 0;
	INT32 i = 0;
	INT8 file_path_name[64] = {0};
	INT32 file_fd = 0;
	
	if (NULL == p_firm_buff || firm_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	/* У��firmwareͷ��Ϣ */
	if (firm_len < sizeof(FIRMWARE_HEADER))
	{
		DEBUG_PRINT(DEBUG_ERROR, "firmware header error: firm_len=%d\n", firm_len);
		return ERROR;
	}
	p_firm_header = (const FIRMWARE_HEADER *)p_firm_buff;
	file_num = p_firm_header->file_num;
	if (p_firm_header->magic_number != PACK_MAGIC_NUMBER || file_num < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "firmware header error: magic_number=0x%x file_num=%d\n"
			, p_firm_header->magic_number, file_num);
		return ERROR;
	}

	/* У��upgrade fileͷ��Ϣ */
	if (firm_len < sizeof(FIRMWARE_HEADER) + file_num * sizeof(UPGRADE_FILE_HEADER))
	{
		DEBUG_PRINT(DEBUG_ERROR, "upgrade file header error: firm_len=%d\n", firm_len);
		return ERROR;
	}

	/* ��ȡ��У������ļ����ݣ����������ļ� */
	p_file_header = (const UPGRADE_FILE_HEADER *)(p_firm_buff + sizeof(FIRMWARE_HEADER));
	for (i = 0; i < file_num; i++)
	{
		if (firm_len < (p_file_header[i].start_offset + p_file_header[i].file_len))
		{
			DEBUG_PRINT(DEBUG_ERROR, "upgrade file error: firm_len=%d, i=%d\n", firm_len, i);
			return ERROR;
		}
		p_file_buff = p_firm_buff + p_file_header[i].start_offset;
		if (checksum_u8(p_file_buff, p_file_header[i].file_len) != p_file_header[i].check_sum)
		{
			DEBUG_PRINT(DEBUG_ERROR, "upgrade file error: checksum_u8 failed, i=%d\n", i);
			return ERROR;
		}
		
		snprintf(file_path_name, sizeof(file_path_name), "%s/%s", UPGRADE_DIR, p_file_header[i].file_name);
		file_fd = open(file_path_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
		if (ERROR == file_fd)
		{
			DEBUG_PRINT(DEBUG_ERROR, "open %s failed:%s\n", file_path_name, strerror(errno));
			return ERROR;
		}

		writen(file_fd, p_file_buff, p_file_header[i].file_len);
		SAFE_CLOSE(file_fd);
	}

	return OK;
}

static INT32 api_system_upgrade_process(const INT8 *method, INT8 *p_body_buff, INT32 buff_len)
{
	if (NULL == method || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(method, "POST") == 0)
	{
		if (upgrade_buff.buff_len != upgrade_buff.end_idx)
		{
			snprintf(p_body_buff, buff_len, "{\"error\":\"firmware is incomplete!\"}");
			return ERROR;
		}
		else
		{
			if (firmware_upgrade(upgrade_buff.p_buff, upgrade_buff.buff_len) == OK)
			{
				snprintf(p_body_buff, buff_len, "{\"error\":\"OK\"}");
			}
			else
			{
				snprintf(p_body_buff, buff_len, "{\"error\":\"upgrade failed!\"}");
				return ERROR;
			}
		}
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
	return OK;
}

static BOOL system_can_reboot(void)
{
	if (upgrade_buff.conn != NULL) // ϵͳ����ʱ��������
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

static INT32 api_system_reboot_process(const INT8 *method, INT8 *p_body_buff, INT32 buff_len)
{	
	if (NULL == method || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(method, "POST") == 0)
	{
		if (!system_can_reboot())
		{
			snprintf(p_body_buff, buff_len, "{\"error\":\"can't reboot now!\"}");
			return ERROR;
		}
		else
		{
			sync();

			if (reboot(RB_AUTOBOOT) == ERROR) // �ɹ������˺������᷵��
			{
				DEBUG_PRINT(DEBUG_ERROR, "reboot failed: %s\n", strerror(errno));
				snprintf(p_body_buff, buff_len, "{\"error\":\"reboot failed!\"}");
				return ERROR;
			}
		}
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
	return OK;
}

static INT32 api_system_parse(const INT8 *method, const INT8 *uri, INT8 *p_body_buff, INT32 buff_len)
{
	INT8 layer_str[16] = {0};
	INT8 *p_next_layer = NULL;
	INT32 ret = 0;
	
	if (NULL == method || NULL == uri || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	p_next_layer = parse_uri_layer(uri, layer_str, sizeof(layer_str));
	if (strcmp(layer_str, "info") == 0)
	{
		if (NULL == p_next_layer)
		{
			ret = api_system_info_process(method, p_body_buff, buff_len);
		}
		else
		{
			ret = ERROR;
		}
	}
	else if (strcmp(layer_str, "time") == 0)
	{
		if (NULL == p_next_layer)
		{
			ret = api_system_time_process(method, p_body_buff, buff_len);
		}
		else
		{
			ret = ERROR;
		}
	}
	else if (strcmp(layer_str, "upgrade") == 0)
	{
		if (NULL == p_next_layer)
		{
			ret = api_system_upgrade_process(method, p_body_buff, buff_len);
			
			/* ������ɺ���Ҫ�ͷ�������������Դ */
			SAFE_FREE(upgrade_buff.p_buff);
			upgrade_buff.buff_len = 0;
			upgrade_buff.end_idx = 0;
			upgrade_buff.conn = NULL;
		}
		else
		{
			ret = ERROR;
		}
	}
	else if (strcmp(layer_str, "reboot") == 0)
	{
		if (NULL == p_next_layer)
		{
			ret = api_system_reboot_process(method, p_body_buff, buff_len);
		}
		else
		{
			ret = ERROR;
		}
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "layer_str=%s not found\n", layer_str);
		return ERROR;
	}

	return ret;
}

static INT32 api_main(const INT8 *method, const INT8 *uri, INT8 *p_body_buff, INT32 buff_len)
{
	INT8 layer_str[16] = {0};
	INT8 *p_next_layer = NULL;
	INT32 ret = 0;

	if (NULL == method || NULL == uri || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}
	
	p_next_layer = parse_uri_layer(uri, layer_str, sizeof(layer_str));
	DEBUG_PRINT(DEBUG_NOTICE, "api_main layer_str=%s\n", layer_str);
	if (NULL == p_next_layer)
	{
		DEBUG_PRINT(DEBUG_WARN, "input uri dose not have layer after %s\n", layer_str);
		return ERROR;
	}

	p_next_layer = parse_uri_layer(p_next_layer, layer_str, sizeof(layer_str));
	if (strcmp(layer_str, "system") == 0)
	{
		if (NULL == p_next_layer)
		{
			ret = ERROR;
		}
		else
		{
			ret = api_system_parse(method, p_next_layer, p_body_buff, buff_len);
		}
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "layer_str=%s not found\n", layer_str);
		return ERROR;
	}
	
	return ret;
}

static void put_packet_to_upgrade_buff(HttpConn *conn, HttpPacket *packet)
{
	INT8 *ptr = NULL;
	INT32 len = 0;
	
	if (NULL == conn || NULL == packet || NULL == conn->rx)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return;
	}

	DEBUG_PRINT(DEBUG_NOTICE, "receive upgrade data\n");

	if (NULL == upgrade_buff.conn) // ��һ���յ���������
	{
		upgrade_buff.conn = conn;
		SAFE_FREE(upgrade_buff.p_buff);
		upgrade_buff.buff_len = conn->rx->length;
		upgrade_buff.p_buff = (UINT8 *)malloc(upgrade_buff.buff_len);
		upgrade_buff.end_idx = 0;
	}
	else
	{
		if (upgrade_buff.conn != conn)
		{
			DEBUG_PRINT(DEBUG_WARN, "can't upgrade: other client is upgrading!\n");
			return;
		}
	}

	len = MIN(httpGetPacketLength(packet), upgrade_buff.buff_len - upgrade_buff.end_idx);
	ptr = httpGetPacketStart(packet);
	if (ptr != NULL && len > 0)
	{
		memcpy(upgrade_buff.p_buff + upgrade_buff.end_idx, ptr, len);
		upgrade_buff.end_idx += len;
	}

	return;
}

/**		  
 * @brief Ϊ�˴���/api/system/upgrade�������ܣ���������ͬappweb���е�incoming������ͬ
 * @param q http���յ����ݶ���
 * @param packet ��ǰ�յ������ݰ�
 * @return ��
 */
static void api_incoming(HttpQueue *q, HttpPacket *packet)
{
	if (NULL == q || NULL == q->conn || NULL == q->conn->rx)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return;
	}

	if (q->nextQ->put)
	{
		httpPutPacketToNext(q, packet);
	}
	else
	{
		/* This queue is the last queue in the pipeline */
		if (httpGetPacketLength(packet) > 0)
		{
			if (packet->flags & HTTP_PACKET_SOLO)
			{
				httpPutForService(q, packet, HTTP_DELAY_SERVICE);
			} 
			else
			{
				if (strcmp(q->conn->rx->uri, "/api/system/upgrade") == 0) // �������������������ܴ�����Ҫ�����⴦��
				{
					put_packet_to_upgrade_buff(q->conn, packet);
				}
				else
				{
					httpJoinPacketForService(q, packet, 0);
				}
			}
		} 
		else
		{
			/* Zero length packet means eof */
			httpPutForService(q, packet, HTTP_DELAY_SERVICE);
		}
		HTTP_NOTIFY(q->conn, HTTP_EVENT_READABLE, 0);
	}

	return;
}

static void api_entry(HttpQueue *q)
{
    HttpConn *conn = NULL;
	INT8 http_body[100 * 1024] = {0};
	INT32 len = 0;

	if (NULL == q || NULL == q->conn || NULL == q->conn->rx)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return;
	}
	
	conn = q->conn;

	if (strcmp(conn->rx->method, "OPTIONS") == 0) // PUT����������ʱ���ȷ���OPTIONS����
	{
		DEBUG_PRINT(DEBUG_NOTICE, "API OPTIONS!\n");
		httpFinalize(conn);
		return;
	}

	len = httpRead(conn, http_body, sizeof(http_body));

	DEBUG_PRINT(DEBUG_NOTICE, "apiHandler received method=%s uri=%s http body length=%d body content:\n%s\n"
		, conn->rx->method, conn->rx->uri, len, http_body);

	if (strcmp(conn->rx->uri, "/api/system/login") == 0) // �û���¼�����ҪȨ����֤�������ﵥ������
	{
		if (api_system_login_process(conn, http_body, sizeof(http_body)) == OK)
		{
			httpSetHeader(conn, "Content-Type", "application/json");
			httpSetStatus(conn, HTTP_CODE_OK);
			httpWrite(q, "%s", http_body);
			httpFinalize(conn);
		}
		else
		{
			DEBUG_PRINT(DEBUG_WARN, "api_login_process return ERROR\n");
			httpSetHeader(conn, "Content-Type", "application/json");
			httpSetStatus(conn, HTTP_CODE_UNAUTHORIZED);
			httpWrite(q, "%s", http_body);
			httpFinalize(conn);
		}

		return;
	}
	else if (strcmp(conn->rx->uri, "/api/system/logout") == 0) // �ǳ�����Ҳ�����ﵥ������
	{
		if (api_system_logout_process(conn, http_body, sizeof(http_body)) == OK)
		{
			httpSetHeader(conn, "Content-Type", "application/json");
			httpSetStatus(conn, HTTP_CODE_OK);
			httpWrite(q, "%s", http_body);
			httpFinalize(conn);
		}
		else
		{
			DEBUG_PRINT(DEBUG_WARN, "api_logout_process return ERROR\n");
			httpSetHeader(conn, "Content-Type", "application/json");
			httpSetStatus(conn, HTTP_CODE_BAD_REQUEST);
			httpWrite(q, "%s", http_body);
			httpFinalize(conn);
		}

		return;
	}
	else
	{
		/* �û�Ȩ����֤ */
		if (check_auth_permission(conn) == ERROR)
		{
			DEBUG_PRINT(DEBUG_NOTICE, "unauthorized!\n");
			httpSetHeader(conn, "Content-Type", "application/json");
			httpSetStatus(conn, HTTP_CODE_UNAUTHORIZED);
			httpWrite(q, "{\"error\":\"not login, please login first!\"}");
			httpFinalize(conn);
			return;
		}
	}
	
	if (strcmp(conn->rx->uri, "/api/system/upgrade") == 0) // ����������Ҫ���⴦��
	{
		if (conn != upgrade_buff.conn)
		{
			DEBUG_PRINT(DEBUG_WARN, "other client is upgrading!\n");
			httpSetHeader(conn, "Content-Type", "application/json");
			httpSetStatus(conn, HTTP_CODE_FORBIDDEN);
			httpWrite(q, "{\"error\":\"other client is upgrading!\"}");
			httpFinalize(conn);
			return;
		}
	}
	
	if (api_main(conn->rx->method, conn->rx->uri, http_body, sizeof(http_body)) == OK)
	{
		httpSetHeader(conn, "Content-Type", "application/json");
		httpSetStatus(conn, HTTP_CODE_OK);
		httpWrite(q, "%s", http_body);
		httpFinalize(conn);
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "api_main return ERROR\n");
		httpSetHeader(conn, "Content-Type", "application/json");
		httpSetStatus(conn, HTTP_CODE_BAD_REQUEST);
		httpWrite(q, "%s", http_body);
		httpFinalize(conn);
	}
	
	return;
}

static INT32 api_handler_init(Http *http)
{
    HttpStage   *handler = NULL;

	if (NULL == http)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}
	
	handler = httpCreateHandler(http, "apiHandler", NULL);
    if (NULL == handler)
	{
		DEBUG_PRINT(DEBUG_ERROR, "httpCreateHandler failed!\n");
        return ERROR;
    }

	handler->incoming = api_incoming;
    handler->ready = api_entry;
	
    return OK;
}

/**		  
 * @brief appweb����
 * @param no_use δʹ��
 * @return ��
 */
void *web_server(void *no_use)
{
	Mpr *mpr = NULL;
	MaServer *server = NULL;
	MaAppweb *appweb = NULL;
	INT32 ret = 0;

	memset(&g_login_info, 0, sizeof(g_login_info));
	
	mpr = mprCreate(0, NULL, MPR_USER_EVENTS_THREAD);
	if (NULL == mpr)
	{
		DEBUG_PRINT(DEBUG_ERROR, "mprCreate failed!\n");
		return NULL;
	}

	if (mprStart() < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "mprStart failed!\n");
		mprDestroy();
		return NULL;
	}

	appweb = maCreateAppweb();
	if (NULL == appweb)
	{
		DEBUG_PRINT(DEBUG_ERROR, "maCreateAppweb failed!\n");
		mprDestroy();
		return NULL;
	}
	mprAddRoot(appweb);

	server = maCreateServer(appweb, "default");
	if (NULL == server)
	{
		DEBUG_PRINT(DEBUG_ERROR, "maCreateServer failed!\n");
		mprRemoveRoot(appweb);
		mprDestroy();
		return NULL;
	}

	if (api_handler_init(MPR->httpService) != OK) // �����Զ����api����ģ��
	{
		DEBUG_PRINT(DEBUG_ERROR, "api_handler_init failed!\n");
		mprRemoveRoot(appweb);
		mprDestroy();
		return NULL;
	}

	ret = maParseConfig(server, "appweb.conf", 0); // appweb���ò�����appweb.conf�ļ��ж���
	if (ret != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "maParseConfig failed! Mpr error code=%d\n", ret);
		mprRemoveRoot(appweb);
		mprDestroy();
		return NULL;
	}

	ret = maStartServer(server);
	if (ret != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "maStartServer failed! Mpr error code=%d\n", ret);
		mprRemoveRoot(appweb);
		mprDestroy();
		return NULL;
	}

	ret = mprServiceEvents(-1, 0); // ��������ʱ�ú������᷵��
	
	DEBUG_PRINT(DEBUG_ERROR, "Stopping Appweb! mprServiceEvents ret=%d\n", ret);
	maStopServer(server);
	mprRemoveRoot(appweb);
	mprDestroy();

	return NULL;
}

