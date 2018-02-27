
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
 * @brief 用户功能初始化
 * @param[in] argc 命令行中参数的个数
 * @param[in] argv 命令行参数数组
 * @return 无
 */
void user_fun_init(INT32 argc, INT8 *argv[])
{
	/* 启动支持自定义shell命令的服务 */
	if(thread_create(cmd_server, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread cmd_server error\n");
		return;
	}

	/* 启动web服务 */
	if(thread_create(web_server, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread web_server error\n");
		return;
	}

	/* 启动upnp服务 */
	if(thread_create(upnp_fun, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread upnp_fun error\n");
		return;
	}

	/* 启动mqtt守护线程 */
	if(thread_create(mqtt_daemon, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread mqtt_daemon error\n");
		return;
	}

	/* 启动网关数据转发服务 */
	if(thread_create(trans_server, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread cmd_server error\n");
		return;
	}

	return;
}

/**
 * @brief 用户系统初始化
 * @param[in] argc 命令行中参数的个数
 * @param[in] argv 命令行参数数组
 * @return 无
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
 * @brief		主函数入口
 * @param[in] argc 命令行中参数的个数
 * @param[in] argv 命令行参数数组
 * @return OK 
 */
INT32 main(INT32 argc, INT8 *argv[])
{
	INT32 i = 0;

	/* 忽略某些信号，防止进程被这些信号终止 */
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

