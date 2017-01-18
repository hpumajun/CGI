/**************************************************************************************
 * Copyright 2010-2013 Hangzhou Hikvision Digital Technology Co., Ltd.
 *    Filename: openvpn_client.c
 * Description: openvpn demo
 *     Created: 2016-09-18
 *     Version: V1.0
 *      Author: laoyuliang<laoyuliang@hikvision.com.cn>
 * modification history:
 * 
 **************************************************************************************/
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include "../src/openvpn/syshead.h"
#include "../src/openvpn/init.h"
#include "../src/openvpn/forward.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>  
#include <unistd.h>  
#include <../src/openvpn/socket.h>

#include <netdb.h>
#include <netinet/ip.h>
#include <net/route.h>
#include <linux/icmp.h>
#include <pthread.h>
#include "../src/openvpn/openvpn.h"
#include "openvpn_client.h"
#include "http_get.h"
#include "../src/openvpn/error.h"

/********************Macro define*****************************/

#define UNIX_DOMAIN "/tmp/cgi.domain"

char szuploadcfgfilepath[32] = "/home/" ;
char szcfgfilesavepath[32]   = "/home/config/" ;
/***********************Variable define ***************************/

static struct context vpn_context;	///< openvpn隧道状态信息结构体

/* 外部函数声明 */
extern int openvpn_client (struct context *c, char *config_file);

char img_path[10][256];  
char aasave_path[10][256];

int readFileList(char *basePath)  
{  
      
    DIR *dir;  
    int img_num=0;  
    struct dirent *ptr;  
    //char base[1000];  
  
    if ((dir=opendir(basePath)) == NULL)  
    {  
        perror("Open dir error...");  
        exit(1);  
    }  
  
    while ((ptr=readdir(dir)) != NULL)  
    {  
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    //current dir OR parrent dir  
            continue;  
        else if(ptr->d_type == 8)    //file  
        {  
			if(strstr(ptr->d_name,".new"))
			{
				strcpy(img_path[img_num],basePath); 
                strcat(img_path[img_num],ptr->d_name);  
                printf("file is %s\n",img_path[img_num++]);
                strcpy(aasave_path[img_num],"/home/config/"); 
                memcpy(aasave_path[img_num]+strlen(aasave_path[img_num]),ptr->d_name,strlen(ptr->d_name)-4);
			}
			
		}  
  
        else   
        {  
            continue;  
        }  
    }  
    closedir(dir);  
    return img_num;  
}  

int checkIfFileMatch()
{
    
}

int savecfgfile(uint ifilecnt)
{
    char szcmd[128];
    uint i,uilen;
    int status,ret;
    for(i = 0; i < ifilecnt; i++)
    {
        snprintf(szcmd, sizeof(szcmd),"mv %s %s",img_path[i],aasave_path[i]);
        status = system(szcmd);
        if (status < 0)
		{
			printf("updated %s failed\n",aasave_path[i]);
			ret = UNKNOWN_ERROR;
			break;
		}
        else
        {
           ret =  OK;
        }
    }
    return ret;
}

int checkconfigfile(void)
{
    unsigned int uicnt;
    int ret;
    char *pszpath = szuploadcfgfilepath;
    uicnt = readFileList(pszpath);
    if( uicnt < 5)
    {
        printf("miss some config files\n");
        return MISSFILE;
    }
//    ret = checkIfFileMatch();
    ret = savecfgfile(uicnt);
    return ret;
}


/**@brief		当前openvpn配置开启还是关闭，此函数必须要有，openvpn库中会用到此函数
 * @param[in] 	OPENVPN_T openvpn_cfg openvpn配置文件结构体
 * @param[out] 	无
 * @return 		1-开启，0-关闭
 */
int is_openvpn_enable(void)
{
	return stopenvpn_flag.benable;
}
void updatevpnstatus(int status)
{
    stopenvpn_flag.bisrun = status;
}
/**@brief		启动openvpn客户端服务，openvpn线程启动调用此函数进入循环
 * @param[in] 	OPENVPN_T openvpn_cfg, openvpn配置文件结构体
 * @param[out] 	struct context vpn_context, openvpn隧道状态结构体
 * @return 		无
 */
void openvpn_start(void)
{
	char *openvpn_config    = OPENVPN_CFG_FILE;
    
    error_reset();
	int iret = openvpn_client(&vpn_context, openvpn_config);
    if (0 == iret)
    {
	    OPENVPN_DBG(DEBUG_INFO, "openvpn exit success\n");
    }
    else
    {
        OPENVPN_DBG(DEBUG_INFO, "openvpn_client failed! File:%s,Func:%s,Line:%d\n",__FILE__,__FUNCTION__,__LINE__);
    }
    stopenvpn_flag.benable = DISABLE;
    msg (M_WARN, "stop openvpn client successfully,flag is %d", stopenvpn_flag.benable);

}

/**@brief		停止openvpn客户端服务
 * @param[in] 	OPENVPN_T openvpn_cfg, openvpn配置文件结构体
 * @param[in]	struct context vpn_context,openvpn隧道状态结构体
 * @param[out] 	无
 * @return 		无
 */
void openvpn_stop(void)
{
	stopenvpn_flag.benable = DISABLE;

	if (vpn_context.sig != NULL)
	{
		vpn_context.sig->signal_received = SIGHUP;
	}

	return;
}

/**@brief		重启openvpn客户端服务
 * @param[in] 	OPENVPN_T openvpn_cfg, openvpn配置文件结构体
 * @param[in]	struct context vpn_context, openvpn隧道状态结构体
 * @param[out] 	无
 * @return 		无
 */
void openvpn_restart_client(void)
{
    int ret;
    ret = checkconfigfile();
	if (vpn_context.sig != NULL)
	{
		vpn_context.sig->signal_received = SIGHUP;
	}

	return;
}

/**@brief		获取vpn ip地址
 * @param[in] 	struct context vpn_context, openvpn隧道状态结构体
 * @param[out] 	char *vpn_ip 客户端vpn ip地址，错误则输出0.0.0.0
 * @return 		无
 */
void get_vpn_ip(char *vpn_ip)
{
	struct in_addr local_tun_ia;

	memset(&local_tun_ia, 0 , sizeof(local_tun_ia));

	if (NULL == vpn_ip)
	{
		return;
	}

	if (vpn_context.c1.tuntap != NULL)
	{
		local_tun_ia.s_addr = htonl(vpn_context.c1.tuntap->local);
		strncpy(vpn_ip, inet_ntoa(local_tun_ia), IFNAMESIZE);
	}
	else
	{
		strncpy(vpn_ip, "0,0,0,0", IFNAMESIZE);
	}

	return;
}

/**@brief		获取远程主机地址
 * @param[in] 	struct context vpn_context, openvpn隧道状态结构体
 * @param[out] 	无
 * @return 		服务器主机ip地址
 */
const char *get_vpn_remote(void)
{
	return vpn_context.options.ce.remote;
}

/**@brief		获取端口
 * @param[in] 	struct context vpn_context, openvpn隧道状态结构体
 * @param[out] 	无
 * @return 		openvpn端口号(1024-65536)
 */
unsigned short int get_vpn_port(void)
{
	return vpn_context.options.ce.remote_port;
}

/**@brief		检查tun0隧道是否建立成功
 * @param[in] 	无
 * @param[out] 	无
 * @return	 	0 :tun0未建立链接
 *              1 :tun0已建立链接
 */
int openvpn_get_connection_status(void)
{
	struct ifreq if_req;
	int rv = 0;
	int sockfd = 0;

	memset(&if_req, 0 ,sizeof(if_req));

	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

	if (sockfd < 0)
	{
	    return 0;
	}
	strncpy(if_req.ifr_name, "tun0", sizeof(if_req.ifr_name));
	rv = ioctl(sockfd, SIOCGIFFLAGS, &if_req);
	close(sockfd);
	if (rv == -1)
	{
		return 0;
	}

	return ((if_req.ifr_flags & IFF_UP) && (if_req.ifr_flags & IFF_RUNNING));
}

/**@brief		更新openvpn证书文件
 * @param[in] 	char type, 证书类型
 * @param[in] 	char *p_ypte, 证书数据
 * @param[in] 	int len 证书数据长度
 * @param[out] 	无
 * @return	 	0-更新成功，-1-更新失败
 */
int openvpn_update_openvpn_file(char type, char *p_buf, int len)
{
	int fd = -1;
	int ret = -1;
	char tmp_file[] = "/home/file.tmp";
	char sys_cmd[512] = {0};

	if ((NULL == p_buf) || (len <= 0))
	{
		OPENVPN_DBG(SYS_ERROR, "param is invalid\n");
		return -1;
	}

	//将缓冲区数据写入临时文件
	fd = open(tmp_file, O_CREAT | O_TRUNC | O_RDWR, 0777);
	if (fd < 0)
	{
		OPENVPN_DBG(SYS_ERROR, "create new cert:%s fail. %d\n", tmp_file, errno);
		return -1;
	}

	ret = write(fd, p_buf, len);
	if ((ret <= 0) || (ret != len))
	{
		OPENVPN_DBG(SYS_ERROR, "write tmp file fail, file len=%d, ret=%d\n", len, ret);
		close(fd);
		return -1;
	}
	close(fd);

	switch (type)
	{
		case FILE_TYPE_CA:
			OPENVPN_DBG(DEBUG_INFO, "update openvpn CA file success!\n");
			memset(sys_cmd, 0, sizeof(sys_cmd));
			snprintf(sys_cmd, sizeof(sys_cmd), "mv %s %s", tmp_file, OPENVPN_CA_FILE_FLASH);
			(void)system(sys_cmd);
			break;
		case FILE_TYPE_TA_KEY:
			OPENVPN_DBG(DEBUG_INFO, "update openvpn TA_KEY file success!\n");
			memset(sys_cmd, 0, sizeof(sys_cmd));
			snprintf(sys_cmd, sizeof(sys_cmd), "mv %s %s", tmp_file, OPENVPN_TA_KEY_FLASH);
			(void)system(sys_cmd);
			break;
		case FILE_TYPE_CONFIG:
			OPENVPN_DBG(DEBUG_INFO, "update openvpn CONFIG file success!\n");
			memset(sys_cmd, 0, sizeof(sys_cmd));
			snprintf(sys_cmd, sizeof(sys_cmd), "mv %s %s", tmp_file ,OPENVPN_CFG_FILE_FLASH);
			(void)system(sys_cmd);
			break;
		case FILE_TYPE_KEY:
			OPENVPN_DBG(DEBUG_INFO, "update openvpn KEY file success!\n");
			memset(sys_cmd, 0, sizeof(sys_cmd));
			snprintf(sys_cmd, sizeof(sys_cmd), "mv %s %s", tmp_file, OPENVPN_KEY_FILE_FLASH);
			(void)system(sys_cmd);
			break;
		case FILE_TYPE_CRT:
			OPENVPN_DBG(DEBUG_INFO, "update openvpn CERTIFICATE file success!\n");
			memset(sys_cmd, 0, sizeof(sys_cmd));
			snprintf(sys_cmd, sizeof(sys_cmd), "mv %s %s", tmp_file, CERTIFICATE_FILE_FLASH);
			(void)system(sys_cmd);
			break;
		default:
            break;
			
	}

	return 0;
}


/**@brief		ping命令校验和函数
 * @param[in] 	u_short *ausp_Buffer，校验数据
 * @param[in] 	int ai_BuffLen，校验数据长度
 * @param[out] 	无
 * @return	 	校验和
 */
static u_short CheckSum(u_short *ausp_Buffer, int ai_BuffLen)
{
	int li_WordLeft = 0;
	int li_Sum32Bits = 0;
	u_short *lusp_Word = NULL;
	u_short lus_SumResult = 0;

	li_Sum32Bits = 0;
	lus_SumResult = 0;
	li_WordLeft = ai_BuffLen;
	lusp_Word = ausp_Buffer;

	while (li_WordLeft > 1)  
	{
		li_Sum32Bits = li_Sum32Bits + *lusp_Word;
		lusp_Word++;
		li_WordLeft = li_WordLeft - 2;
	}

	if (li_WordLeft == 1) 
	{
		*(u_char *)(&lus_SumResult) = *(u_char *)lusp_Word ;
		li_Sum32Bits += lus_SumResult;
	}

	li_Sum32Bits = (li_Sum32Bits >> 16) + (li_Sum32Bits & 0xffff);
	li_Sum32Bits += (li_Sum32Bits >> 16);
	lus_SumResult = ~li_Sum32Bits;

	return lus_SumResult;
}

/**@brief		实现ping功能
 * @param[in] 	char *host，主机地址
 * @param[out] 	无
 * @return	 	-1-失败，1-成功
 */
int check_host_status(char *host)
{
	int sock_fd = -1;
	int length = 0;
	struct timeval tv;
	fd_set readfds;
	in_addr_t ip_addr;
	struct sockaddr_in to_addr;
	struct sockaddr_in from_addr;
	struct iphdr *ip_head = NULL;
	struct icmphdr *icmp_head = NULL;
	struct icmp_filter icmp_flt;

	char msg[64] = {0};
	char buf[128] = {0};
	int ret = -1;

	if (NULL == host)
	{
		OPENVPN_DBG(SYS_ERROR, "param err!\n");
		return ERROR;
	}

	memset(&tv, 0, sizeof(tv));
	memset(&from_addr, 0, sizeof(from_addr));
	memset(&icmp_flt, 0, sizeof(icmp_flt));
	memset(&readfds, 0, sizeof(readfds));
	memset(&ip_addr, 0, sizeof(ip_addr));

	sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); 
	if (-1 == sock_fd)
	{
		OPENVPN_DBG(SYS_ERROR, "create socket fail, %d\n", errno);
		return ERROR;
	}

	icmp_flt.data = ~((1<<ICMP_TIME_EXCEEDED) | (1<<ICMP_DEST_UNREACH));
	icmp_flt.data = icmp_flt.data & ~(1<<ICMP_ECHOREPLY); 

	if (setsockopt(sock_fd, SOL_RAW, ICMP_FILTER, (char *)&icmp_flt, sizeof(icmp_flt)) < 0)
	{
		OPENVPN_DBG(SYS_ERROR, "setsockopt fail, %d\n", errno);
	}

	memset(&to_addr, '\0', sizeof(to_addr));

	to_addr.sin_family = AF_INET;
	ip_addr = inet_addr(host);
	to_addr.sin_port = htons(0);
	to_addr.sin_addr.s_addr = ip_addr;

	memset(msg, '\0', sizeof(msg));
	icmp_head = (struct icmphdr *)msg;

	icmp_head->type = ICMP_ECHO;
	icmp_head->code = 0;
	icmp_head->un.echo.id = 0xf09;
	icmp_head->un.echo.sequence = 0xf001;
	icmp_head->checksum = CheckSum((u_short *) msg, 64);

	tv.tv_sec = 5;
	tv.tv_usec = 0;    
	FD_ZERO(&readfds);
	FD_SET(sock_fd, &readfds);

	if (sendto(sock_fd, msg, sizeof(msg), 0, (struct sockaddr *)&to_addr, sizeof(to_addr)) < 0)
	{
		OPENVPN_DBG(SYS_ERROR, "sendto fail, %d\n", errno);
		close(sock_fd);
		return ERROR;
	}

	ret = select(sock_fd + 1, &readfds, NULL, NULL, &tv);
	if (ret < 0)
	{
		OPENVPN_DBG(SYS_ERROR, "select fail, %d\n", errno);
		close(sock_fd);
		return ERROR;
	} 
	else if (0 == ret)
	{
		OPENVPN_DBG(SYS_ERROR, "select timeout, %d\n", errno);
		close(sock_fd);
		return ERROR;
	}

	if (FD_ISSET(sock_fd, &readfds))
	{ 
		length = sizeof(from_addr);
		if (recvfrom(sock_fd, buf, sizeof(buf), 0, (struct sockaddr *)&from_addr, &length) != -1)
		{
			fflush(stdout); 
			ip_head = (struct iphdr *)buf;
			icmp_head = (struct icmphdr *)(buf + sizeof(struct iphdr));

			if ((ip_head->saddr == to_addr.sin_addr.s_addr) && (icmp_head->type == ICMP_ECHOREPLY)) 
			{
				close(sock_fd);
				return OK;
			}
		}
		else
		{
			OPENVPN_DBG(SYS_ERROR, "recvfrom fail, %d\n", errno);
			close(sock_fd);
			return ERROR;
		}
	}  
	else
	{
		close(sock_fd);
		return ERROR;
	}

	return ERROR;
}

/**@brief		通过ping命令检查连接状态
 * @param[in] 	char *host，主机地址
 * @param[out] 	无
 * @return	 	-1-失败，0-成功
 */
int ping(char *host)
{
	int retval_ini = 0;

	retval_ini = check_host_status(host);
	if (OK == retval_ini)
	{
		OPENVPN_DBG(DEBUG_INFO, "The host %s is alive.\n",  host);
		return 0;
	}

	return -1;
}

/**@brief		根据路由表获取服务器IP，接口为tun0
 * @param[in] 	char *ip, IP缓冲区
 * @param[in] 	int len, IP缓冲区大小
 * @param[out] 	char *ip, IP缓冲区，格式为点分十进制
 * @return 		0成功，-1失败
 */
int get_server_ip_by_route(char *ip, int len)
{
	char if_name[64] = {0};
	unsigned int d = 0;
	unsigned int g = 0;
	unsigned int m = 0;
	int r = 0;
	int flgs = 0;
	int ref = 0;
	int use = 0;
	int metric = 0;
	int mtu = 0;
	int win = 0;
	int ir = 0;
	struct in_addr mask;
	struct in_addr dst;
	struct in_addr gw;
	int ret = -1;
	FILE *fp = NULL;

	memset(if_name, 0, sizeof(if_name));
	memset(&mask, 0, sizeof(mask));
	memset(&dst, 0, sizeof(dst));
	memset(&gw, 0, sizeof(gw));

	fp = fopen("/proc/net/route", "r");
	if(NULL == fp)
	{
		OPENVPN_DBG(SYS_ERROR, "open file failed %d\n", errno);
		return -1;
	}

	//跳过第一行
	if (fscanf(fp, "%*[^\n]\n") < 0)
	{
		fclose(fp);
		return -1;
	}

	while (1)		///< 此处类似读取文本信息，这里对route表进行逐行扫描信息
	{
		r = fscanf(fp, "%63s%x%x%X%d%d%d%lx%d%d%d\n",
				if_name, &d, &g, &flgs, &ref, &use, &metric, &m, &mtu, &win, &ir);
		if (r != 11)		///< ipc最多网卡eth0,lo,wlan0,tun0四张网卡，在进行第5次扫描时自动退出，调用该函数出有10s延时
		{
			//printf("fscanf error and r=%d,errno=%d\n", r, errno);
			ret = -1;
			break;
		}
		
		if (!(flgs & RTF_UP))		///< 扫描route表中name网卡是否已经工作，不工作则扫描下一张网卡 
		{
			continue;
		}

		mask.s_addr = m;
		dst.s_addr  = d;
		gw.s_addr   = g;

		/* client to server mode，route gateway is not 0:0:0:0, mask is 255:255:255:255  */
		if ((strcmp(if_name, "tun0") == 0) && (0xffffffff == m) && (0 != g))
		{
			inet_ntop(AF_INET, &dst, ip, len);
			ret = 0;
			break;
		}
	}

	fclose(fp);

	return ret;
}


/**@brief		检查vpn与服务器连接状态，定时ping服务器，超过3次ping不通认为与服务器断开，需要重启openvpn服务
 * @param[in] 	无
 * @param[out] 	无
 * @return 		无
 */
void openvpn_check_connection_task(void)
{
	char serverip[32] = {0};
	unsigned char lost_ping_count = 0;	///< ping丢失包数

	
	while (1)
	{
        if (DISABLE== stopenvpn_flag.benable)
        {
            msg (M_WARN | M_ERRNO, "Note: received vpn stop signal, stop to check, flag is %d",stopenvpn_flag.benable);
            break;
        }
		if (openvpn_get_connection_status() > 0)
		{
			memset(serverip, 0, sizeof(serverip));
			if (0 == get_server_ip_by_route(serverip, sizeof(serverip)))
			{
				if (ping(serverip) < 0)
				{
					lost_ping_count++;
					OPENVPN_DBG(SYS_ERROR, "ping vpn server: %s fail, count=%d\n", serverip, lost_ping_count);
					if (lost_ping_count >= 3)
					{
						OPENVPN_DBG(SYS_ERROR, "vpn lost connection, restart openvpn\n");
						openvpn_restart_client();
						lost_ping_count = 0;
						sleep(5);		//openvpn重启等待几秒
					}
				}
				else
				{
					lost_ping_count = 0;
				}
			}
		}
		
		sleep(10);
	}
}

void msgprocess(int type,int fd)
{
 	pthread_t client_id = 0;
	pthread_t check_id  = 0;
    int err;
    char status = '1';
    if( CHECKSTATUS == type )
    {
        write(fd,status,sizeof(status));
    }
    else if (VPN_START == type)
    {
        printf("======================\n");
        if (DISABLE == stopenvpn_flag.benable)
        {
            stopenvpn_flag.benable  = ENABLE;
            err = pthread_create(&client_id, NULL, (void*)&openvpn_start, NULL);
        	if (err != 0)
        	{
        		printf("create openvpn start task failed!\n");
        		return ;
        	}
            else
            {
                printf("openvpn thread id is %d\n",client_id);
            }
            err = pthread_create(&check_id, NULL, (void*)&openvpn_check_connection_task, NULL);
        	if (err != 0)
        	{
        		printf("create openvpn check connection task failed!\n");
        		return ;
        	}
            else
            {
                printf("openvpn_check_connection thread id is %d\n",client_id);
            }
        }
        else
        {
            printf("openvpn is still in running!\n");
        }
    }
    else if (VPN_STOP == type)
    {
        /** if we have not start openvpn**/
        if(DISABLE == stopenvpn_flag.benable)
        {
            printf("openvpn is NOT in running! No need to stop\n");
        }
        else
        {
            openvpn_stop();
        }
    }
    else if (VPN_RESTART == type)
    {
        if(DISABLE == stopenvpn_flag.benable)
        {
            printf("openvpn is NOT in running! No need to restart\n");
        }
        
        openvpn_restart_client();
    }
    else
    {
        printf("unknow cmd\n");
    }
    return ;
}


int recvcgicmd()
{
    socklen_t clt_addr_len;
    int listen_fd;
    int com_fd;
    int ret;
    int i,cmd;
    static char recv_buf = '0';
    int len;
    struct sockaddr_un clt_addr;
    struct sockaddr_un srv_addr;

    listen_fd=socket(PF_UNIX,SOCK_STREAM,0);
    if(listen_fd<0)
    {
        perror("cannot create communication socket");
        return 1;
    }

    //set server addr_param
    srv_addr.sun_family=AF_UNIX;
    strncpy(srv_addr.sun_path,UNIX_DOMAIN,sizeof(srv_addr.sun_path)-1);
    unlink(UNIX_DOMAIN);
    //bind sockfd & addr
    ret=bind(listen_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));
    if(ret==-1)
    {
        perror("cannot bind server socket");
        close(listen_fd);
        unlink(UNIX_DOMAIN);
        return 1;
    }
    //listen sockfd
    ret=listen(listen_fd,5);
    if(ret==-1)
    {
        perror("cannot listen the client connect request");
        close(listen_fd);
        unlink(UNIX_DOMAIN);
        return 1;
    }
    //have connect request use accept
    len=sizeof(clt_addr);
    while(1)
    {
        printf("=========prepare to accept client's connection==========\n");
        com_fd=accept(listen_fd,(struct sockaddr*)&clt_addr,&len);
        if(com_fd<0)
        {
            perror("cannot accept client connect request");
            close(listen_fd);
            unlink(UNIX_DOMAIN);
            return 1;
        }
        //read and printf sent client info
        printf("=====info=====\n");

        int num=read(com_fd,&recv_buf,sizeof(recv_buf));
        cmd = (int)recv_buf;
        printf("Message from client (%d)) :%d\n",num,cmd);
        msgprocess(cmd,com_fd);
        close(com_fd);
    }
    close(listen_fd);
    unlink(UNIX_DOMAIN);
    return 0;
}


/* 主函数 */
int openvpn_routine(void)
{
 	pthread_t client_id = 0;
	int err = -1;
	char remote_vpn_ip[IFNAMESIZE] = {0};
    stopenvpn_flag.bisrun = DISABLE;
    stopenvpn_flag.benable = DISABLE ;
    err = pthread_create(&client_id, NULL, (void*)&recvcgicmd, NULL);
    if (err != 0)
	{
		printf("create recvcgicmd start task failed!\n");
		return -1;
	}
    printf("recvcgicmd thread id is %d\n",client_id);
}

void vpnstartbyserialcmd(void)
{
   	pthread_t client_id = 0;
	int err = -1;
//    stopenvpn_flag.benable = ENABLE;
    err = pthread_create(&client_id, NULL, (void*)&openvpn_start, NULL);
    if (err != 0)
	{
		printf("create recvcgicmd start task failed!\n");
		return -1;
	}
}
