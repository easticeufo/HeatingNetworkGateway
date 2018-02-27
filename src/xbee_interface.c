      
/**@file 
 * @note 
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

#define XBEE_DEBUG DEBUG_PRINT

#define XBEE_SERIAL_FILE "/dev/ttySP0" // uart0

static INT32 xbee_serial_fd = -1;

/**		  
 * @brief xbee串口初始化
 * @param 无
 * @return OK-成功，ERROR-失败
 */
INT32 xbee_uart_init(void)
{
	struct termios options;
	
	memset(&options, 0, sizeof(struct termios));

	if (ERROR == (xbee_serial_fd = open(XBEE_SERIAL_FILE, O_RDWR | O_NOCTTY)))
	{
		DEBUG_PRINT(DEBUG_ERROR, "open serial failed: %s\n", strerror(errno));
		return ERROR;
	}

	if (ERROR == tcgetattr(xbee_serial_fd, &options))
	{
		DEBUG_PRINT(DEBUG_ERROR, "tcgetattr: %s\n", strerror(errno));
		SAFE_CLOSE(xbee_serial_fd);
		return ERROR;
	}
	
	/* 设置波特率 */
	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);

	options.c_cflag &= ~PARENB; // Parity:None
	options.c_cflag &= ~CSTOPB; // Stop bits:1
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8; // Data bits:8

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;
	options.c_iflag &= ~(IXON | IXOFF | IXANY);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_oflag &= ~(ONLCR | OCRNL);
	options.c_iflag &= ~(INLCR | ICRNL | IGNCR);

	if (ERROR == tcsetattr(xbee_serial_fd, TCSANOW, &options))
	{
		DEBUG_PRINT(DEBUG_ERROR, "tcsetattr: %s\n", strerror(errno));
		SAFE_CLOSE(xbee_serial_fd);
		return ERROR;
	}
	
	return OK;
}

INT32 xbee_uart_recvn(void *p_buff, INT32 buff_len)
{
	return readn(xbee_serial_fd, p_buff, buff_len);
}

INT32 xbee_uart_sendn(const void *p_data, INT32 data_len)
{
	return writen(xbee_serial_fd, p_data, data_len);
}

