      
/**@file 
 * @note
 * @brief  
 * 
 * @author   madongfang
 * @date     2015-12-5
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#ifndef _MQTT_H_
#define _MQTT_H_

BOOL mqtt_is_connected(void);
INT32 mqtt_send(INT8 *p_message);
INT8 *mqtt_get_last_message(void);

#endif

