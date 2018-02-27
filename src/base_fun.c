      
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author  madongfang
 * @date     2016-5-26
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include "base_fun.h"

static INT32 debug_level = DEBUG_ERROR; // ���Դ�ӡ��Ϣ����ȼ�

/**		  
 * @brief ��ȡ���Դ�ӡ��Ϣ����ȼ�
 * @param ��
 * @return ���Դ�ӡ��Ϣ����ȼ�
 */
INT32 get_debug_level(void)
{
	return debug_level;
}

/**		  
 * @brief		���õ��Դ�ӡ��Ϣ����ȼ� 
 * @param[in] level ���Դ�ӡ��Ϣ����ȼ�
 * @return ��
 */
void set_debug_level(INT32 level)
{
	debug_level = level;
}

/**
 * @brief ��ȡ����������ڣ�����һ��������ʾ������,��p_date_buff����NULL����p_date_buff�з��ر������ڵ��ַ���
 * @param[out] p_date_buff �����ڰ��ո�ʽ"build yyyymmdd"���
 * @param[in] buff_len ��ű��������ַ����Ļ���������
 * @return ����һ��������ʾ����,16~31λ��ʾ���,8~15λ��ʾ�·�,0~7λ��ʾ����
 */
UINT32 get_build_date(INT8 *p_date_buff, INT32 buff_len)
{
	INT32 year = 0;
	INT32 month = 0;
	INT32 day = 0;
	INT8 month_name[4] = {0};
	const INT8 *all_mon_names[] 
		= {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	sscanf(__DATE__, "%s%d%d", month_name, &day, &year);

	for (month = 0; month < 12; month++)
	{
		if (strcmp(month_name, all_mon_names[month]) == 0)
		{
			break;
		}
	}
	month++;

	if (p_date_buff != NULL)
	{
		snprintf(p_date_buff, buff_len, "build %d%02d%02d", year, month, day);
	}

	return (UINT32)((year << 16) | (month << 8) | day);
}

/**		  
 * @brief		�����̺߳�����װ  
 * @param[in] p_func �̺߳���ָ��
 * @param[in] arg ���ݸ��̻߳ص������Ĳ���
 * @return	�ɹ�����OK��ʧ�ܷ���ERROR
 */
INT32 thread_create(void *(* p_fun)(void *), void *arg)
{
	pthread_t tid = 0;
	pthread_attr_t attr;
	
	if (OK != pthread_attr_init(&attr))
	{
		DEBUG_PRINT(DEBUG_ERROR, "pthread_attr_init failed\n");
		return ERROR;
	}
	
	/* ���÷����߳����� */
	if (OK != pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
	{
		DEBUG_PRINT(DEBUG_ERROR, "pthread_attr_setdetachstate failed\n");
		pthread_attr_destroy(&attr);
		return ERROR;
	}
	
	if (OK != pthread_create(&tid, &attr, p_fun, arg))
	{
		DEBUG_PRINT(DEBUG_ERROR, "pthread_create failed\n");
		pthread_attr_destroy(&attr);
		return ERROR;
	}
	pthread_attr_destroy(&attr);
	return OK;
}

/**		  
 * @brief		 ��UINT8��ʽ���㻺���������ݵ�У���
 * @param[in]  p_data ���ݻ�����
 * @param[in]  nums ���ݳ���
 * @return	 У���,����������󷵻�ERROR(0xffffffff)
 */
UINT32 checksum_u8(const void *p_data, INT32 nums)
{
	UINT32 checksum = 0;
	const UINT8 *p_buf = NULL;
	INT32 i = 0;

	if (NULL == p_data || nums < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	p_buf = (const UINT8 *)p_data;
	for (i = 0; i < nums; i++)
	{
		checksum += *p_buf;
		p_buf++;
	}
	return checksum;
}

/**		  
 * @brief		 ��ָ�����ȵ����ݣ�����ʱ�����ӶϿ�������ļ�β�򷵻أ���������
 * @param[in]  nums �������ܳ���
 * @param[in]  fd �򿪵�������
 * @param[out] p_buf �����ݻ�����
 * @return	 ������ERROR�����򷵻�ʵ�ʶ��������ݳ���
 */
INT32 readn(INT32 fd, void *p_buf, INT32 nums)
{
	INT32 nleft = nums;
	INT32 nread = 0;
	UINT8 *ptr = (UINT8 *)p_buf;
	INT32 ret = 0;
	fd_set rset;
	struct timeval timeout;

	if (fd < 0 || NULL == p_buf || nums < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	while (nleft > 0)
	{
		FD_ZERO(&rset);
		FD_SET(fd, &rset);
		timeout.tv_sec = 4;
		timeout.tv_usec = 0;
		ret = select(fd + 1, &rset, NULL, NULL, &timeout);

		if (ret < 0)
		{
			if (EINTR == errno)
			{
				DEBUG_PRINT(DEBUG_INFO, "select: %s\n", strerror(errno));
				continue;
			}
			else
			{
				DEBUG_PRINT(DEBUG_WARN, "select failed: %s\n", strerror(errno));
				return ERROR;
			}
		}
		else if (0 == ret)
		{
			DEBUG_PRINT(DEBUG_INFO, "select timeout\n");
			break;
		}
		else
		{
			if ((nread = read(fd, ptr, nleft)) < 0)
			{
				if (EINTR == errno)
				{
					DEBUG_PRINT(DEBUG_INFO, "read: %s\n", strerror(errno));
					nread = 0;
				}
				else
				{
					DEBUG_PRINT(DEBUG_ERROR, "read failed %s\n", strerror(errno));
					return ERROR;
				}
			}
			else if (0 == nread)
			{
				/* ���ӹرջ�����ļ�β */
				break;
			}
			else
			{
				nleft -= nread;
				ptr += nread;
			}
		}
	}
	
	if (nleft > 0)
	{
		DEBUG_PRINT(DEBUG_INFO, "expect read %d bytes,actually read %d bytes\n", nums, (nums - nleft));
	}
	return (nums - nleft);
}

/**		  
 * @brief		 ����дָ�����ȵ�����
 * @param[in]  nums д�����ܳ���
 * @param[in]  fd �򿪵��ļ���������Ҳ�������׽���
 * @param[out] p_buf д���ݻ�����
 * @return	 ������ERROR�����򷵻�ʵ��д������ݳ��ȣ�����ָ��д��ĳ���
 */
INT32 writen(INT32 fd, const void *p_buf, INT32 nums)
{
	INT32 nleft = nums;
	INT32 nwritten = 0;
	const INT8* ptr = (const INT8*)p_buf;
	
	if (fd < 0 || NULL == p_buf || nums < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	while (nleft > 0)
	{
		if ((nwritten = write(fd, ptr, nleft)) <= 0)
		{
			if (nwritten < 0 && EINTR == errno)
			{
				DEBUG_PRINT(DEBUG_INFO, "write: %s\n", strerror(errno));
				nwritten = 0;
			}
			else
			{
				DEBUG_PRINT(DEBUG_ERROR, "write failed: %s\n", strerror(errno));
				return ERROR;
			}
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return (nums);
}

/**		  
 * @brief		 ��ȡ����ӿڵ�mac��ַ����6���ֽڵĶ����Ʒ���
 * @param[in]  p_interface_name ����ӿ�����
 * @param[out] p_mac_addr ����mac��ַ�Ļ�����
 * @param[in]  len ���������ȣ���������6���ֽ�
 * @return	 �ɹ�����OK��������ERROR
 */
INT32 get_mac_addr(const INT8 *p_interface_name, UINT8 *p_mac_addr, INT32 len)
{
	INT32 fd = -1;
	struct ifreq ifr;
	
	if (NULL == p_interface_name || NULL == p_mac_addr || len < 6)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	if (strcmp(p_interface_name, "lo") == 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "interface \"lo\" does not have mac address\n");
		return ERROR;
	}
	strcpy(ifr.ifr_name, p_interface_name);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "socket failed: %s\n", strerror(errno));
		return ERROR;
	}

	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "ioctl SIOCGIFHWADDR failed: %s\n", strerror(errno));
		SAFE_CLOSE(fd);
		return ERROR;
	}

	memcpy(p_mac_addr, ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);
	SAFE_CLOSE(fd);

	return OK;
}

/**		  
 * @brief		 Unix���׽��ֳ�ʼ������һ��·�����ڽ�������
 * @param[in]  p_path Unix���׽��ֵ�ַ
 * @return	 �ɹ����ش������׽��֣�������ERROR
 */
INT32 unix_recv_socket_init(const INT8 *p_path)
{
	INT32 sock_fd = -1;
	struct sockaddr_un sock_addr;
	memset(&sock_addr, 0, sizeof(sock_addr));
	
	sock_fd = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (ERROR == sock_fd)
	{
		DEBUG_PRINT(DEBUG_ERROR, "socket failed: %s\n", strerror(errno));
		return ERROR;
	}

	unlink(p_path);
	sock_addr.sun_family = AF_LOCAL;
	strcpy(sock_addr.sun_path, p_path);
	if (ERROR == bind(sock_fd, (struct sockaddr *)&sock_addr, SUN_LEN(&sock_addr)))
	{
		DEBUG_PRINT(DEBUG_ERROR, "bind failed: %s\n", strerror(errno));
		SAFE_CLOSE(sock_fd);
		return ERROR;
	}

	return sock_fd;
}

/**		  
 * @brief		 ��ȡ�׽����յ������ݱ�
 * @param[in]  fd �򿪵��׽���
 * @param[out] p_buff �����ݻ�����
 * @param[in]  buff_len ����������
 * @return	 �����յ������ݱ��ĳ��ȣ���ʱ����0������ERROR,
 */
INT32 recv_dgram(INT32 fd, void *p_buff, INT32 buff_len)
{
	fd_set rset;
	struct timeval timeout;
	INT32 ret = 0;
	INT32 len = 0;

	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	timeout.tv_sec = 4;
	timeout.tv_usec = 0;

	FOREVER
	{
		ret = select(fd + 1, &rset, NULL, NULL, &timeout);
		if (ret < 0)
		{
			if (EINTR == errno)
			{
				DEBUG_PRINT(DEBUG_NOTICE, "select: %s\n", strerror(errno));
				continue;
			}
			else
			{
				DEBUG_PRINT(DEBUG_ERROR, "select failed: %s\n", strerror(errno));
				return ERROR;
			}
		}
		else if (0 == ret)
		{
			DEBUG_PRINT(DEBUG_INFO, "select timeout\n");
			return 0;
		}
		else
		{
			memset(p_buff, 0, buff_len);
			len = recvfrom(fd, p_buff, buff_len, 0, NULL, NULL);
			if (ERROR == len)
			{
				if (EINTR == errno)
				{
					DEBUG_PRINT(DEBUG_NOTICE, "recvfrom: %s\n", strerror(errno));
					continue;
				}
				else
				{
					DEBUG_PRINT(DEBUG_ERROR, "recvfrom failed %s\n", strerror(errno));
					return ERROR;
				}
			}

			break;
		}
	}

	return len;
}

/**		  
 * @brief		 ����׽��ֽ��ջ������Ƿ�Ϊ��
 * @param[in]  sock_fd �Ѵ򿪵��׽���
 * @return	 TRUE-�գ�FALSE-�ǿ�
 */
BOOL socket_recv_empty(INT32 sock_fd)
{
	fd_set rset;
	struct timeval timeout;

	FD_ZERO(&rset);
	FD_SET(sock_fd, &rset);
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select(sock_fd + 1, &rset, NULL, NULL, &timeout) > 0)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

