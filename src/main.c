
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

extern void *cmd_server(void *no_use);
extern void *web_server(void *no_use);
extern void *upnp_fun(void *no_use);
extern void *trans_server(void *no_use);
extern void *mqtt_daemon(void *no_use);

/**
 * @brief �û����ܳ�ʼ��
 * @param[in] argc �������в����ĸ���
 * @param[in] argv �����в�������
 * @return ��
 */
void user_fun_init(INT32 argc, INT8 *argv[])
{
	/* ����֧���Զ���shell����ķ��� */
	if(thread_create(cmd_server, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread cmd_server error\n");
		return;
	}

	/* ����web���� */
	if(thread_create(web_server, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread web_server error\n");
		return;
	}

	/* ����upnp���� */
	if(thread_create(upnp_fun, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread upnp_fun error\n");
		return;
	}

	/* ����mqtt�ػ��߳� */
	if(thread_create(mqtt_daemon, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread mqtt_daemon error\n");
		return;
	}

	/* ������������ת������ */
	if(thread_create(trans_server, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread cmd_server error\n");
		return;
	}

	return;
}

/**
 * @brief �û�ϵͳ��ʼ��
 * @param[in] argc �������в����ĸ���
 * @param[in] argv �����в�������
 * @return ��
 */
void user_system_init(INT32 argc, INT8 *argv[])
{
	user_fun_init(argc, argv);

	FOREVER
	{
		sleep(100000);
	}
}

void print_usage(INT8 *name)
{
	printf("\nUsage:\n\n"
		"  %s [option] [parameter]\n\n"
		"  Options:\n"
		"    -debug level #set debug print level: 0-DEBUG_NONE 1-DEBUG_ERROR 2-DEBUG_WARN 3-DEBUG_NOTICE 4-DEBUG_INFO"
		"\n\n"
		, name);
	
	return;
}

/**		  
 * @brief		���������
 * @param[in] argc �������в����ĸ���
 * @param[in] argv �����в�������
 * @return OK 
 */
INT32 main(INT32 argc, INT8 *argv[])
{
	INT32 i = 0;

	/* ����ĳЩ�źţ���ֹ���̱���Щ�ź���ֹ */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-debug") == 0)
		{
			i++;
			if (i >= argc)
			{
				print_usage(argv[0]);
				return ERROR;
			}
			set_debug_level(atoi(argv[i]));
		}
	}
	
	user_system_init(argc, argv);
	
	return OK;
}

