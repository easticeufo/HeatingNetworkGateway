      
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

#ifndef _XBEE_INTERFACE_H_
#define _XBEE_INTERFACE_H_

INT32 xbee_uart_init(void);
INT32 xbee_uart_recvn(void *p_buff, INT32 buff_len);
INT32 xbee_uart_sendn(const void *p_data, INT32 data_len);

#endif

