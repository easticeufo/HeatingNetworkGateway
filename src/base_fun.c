      
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

static INT32 debug_level = DEBUG_ERROR; // 调试打印信息输出等级

/**		  
 * @brief 获取调试打印信息输出等级
 * @param 无
 * @return 调试打印信息输出等级
 */
INT32 get_debug_level(void)
{
	return debug_level;
}

/**		  
 * @brief		设置调试打印信息输出等级 
 * @param[in] level 调试打印信息输出等级
 * @return 无
 */
void set_debug_level(INT32 level)
{
	debug_level = level;
}

/**
 * @brief 获取程序编译日期，返回一个整数表示的日期,若p_date_buff不是NULL，在p_date_buff中返回编译日期的字符串
 * @param[out] p_date_buff 将日期按照格式"build yyyymmdd"存放
 * @param[in] buff_len 存放编译日期字符串的缓冲区长度
 * @return 返回一个整数表示日期,16~31位表示年份,8~15位表示月份,0~7位表示日期
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
 * @brief		创建线程函数封装  
 * @param[in] p_func 线程函数指针
 * @param[in] arg 传递给线程回调函数的参数
 * @return	成功返回OK，失败返回ERROR
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
	
	/* 设置分离线程属性 */
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
 * @brief		 以UINT8格式计算缓冲区内数据的校验和
 * @param[in]  p_data 数据缓冲区
 * @param[in]  nums 数据长度
 * @return	 校验和,输入参数错误返回ERROR(0xffffffff)
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
 * @brief		 读指定长度的数据，若超时或连接断开或读到文件尾则返回，否则阻塞
 * @param[in]  nums 读数据总长度
 * @param[in]  fd 打开的描述符
 * @param[out] p_buf 读数据缓冲区
 * @return	 出错返回ERROR，否则返回实际读到的数据长度
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
				/* 连接关闭或读到文件尾 */
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
 * @brief		 阻塞写指定长度的数据
 * @param[in]  nums 写数据总长度
 * @param[in]  fd 打开的文件描述符，也可用于套接字
 * @param[out] p_buf 写数据缓冲区
 * @return	 出错返回ERROR，否则返回实际写入的数据长度，等于指定写入的长度
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
 * @brief		 获取网络接口的mac地址，以6个字节的二进制返回
 * @param[in]  p_interface_name 网络接口名称
 * @param[out] p_mac_addr 返回mac地址的缓冲区
 * @param[in]  len 缓冲区长度，不能少于6个字节
 * @return	 成功返回OK，出错返回ERROR
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
 * @brief		 Unix域套接字初始化并绑定一个路径用于接收数据
 * @param[in]  p_path Unix域套接字地址
 * @return	 成功返回创建的套接字，出错返回ERROR
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
 * @brief		 读取套接字收到的数据报
 * @param[in]  fd 打开的套接字
 * @param[out] p_buff 读数据缓冲区
 * @param[in]  buff_len 缓冲区长度
 * @return	 返回收到的数据报的长度，超时返回0，错返回ERROR,
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
 * @brief		 检查套接字接收缓冲区是否为空
 * @param[in]  sock_fd 已打开的套接字
 * @return	 TRUE-空，FALSE-非空
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

