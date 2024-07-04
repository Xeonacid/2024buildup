#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "json.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "basefunc.h"
#include "memdb.h"
#include "message.h"
#include "channel.h"
#include "connector.h"
#include "ex_module.h"
#include "connector_value.h"
#include "sys_func.h"

#include "modbus_tcp.h"
#include "modbus_state.h"
#include "modbus_cmd.h"
#include "modbus_monitor.h"

#define MAX_LINE_LEN 1024

static char * server_addr;
static int server_port;
static char * client_addr;
static int client_port;
static struct tcloud_connector_hub * conn_hub;

static BYTE Buf[DIGEST_SIZE*64];
static int index = 0;
static BYTE * ReadBuf=Buf+DIGEST_SIZE*32;
static int readbuf_len;

static void * default_conn = NULL;
struct tcloud_connector * server_conn = NULL;
struct tcloud_connector * client_conn = NULL;
struct tcloud_connector * channel_conn = NULL;

int init_addr=0x1000;

int modbus_monitor_init(void * sub_proc,void * para)
{
    struct init_para * init_para=para;
    int ret;

    conn_hub = get_connector_hub();

    if(conn_hub==NULL)
	    return -EINVAL;
    ret = modbus_monitor_server_init(sub_proc,para);
    if(ret>=0)
        ret=modbus_monitor_client_init(sub_proc,para);
    return ret;

}

int modbus_monitor_client_init(void * sub_proc,void * para)
{
    struct init_para * init_para=para;
    int ret;

    server_conn	= get_connector(CONN_SERVER,AF_INET);
    if((server_conn ==NULL) & IS_ERR(server_conn))
    {
         print_cubeerr("get conn failed!\n");
         return -EINVAL;
    }
 
    Strcpy(Buf,init_para->client_addr);
    Strcat(Buf,":");
    Itoa(init_para->client_port,Buf+Strlen(Buf));

    ret=server_conn->conn_ops->init(server_conn,"modbus_client_side",Buf);
    if(ret<0)
	    return ret;
    conn_hub->hub_ops->add_connector(conn_hub,server_conn,NULL);

    ret=server_conn->conn_ops->listen(server_conn);
    fprintf(stdout,"test client connect listen,return value is %d!\n",ret);

    return 0;
}

int modbus_monitor_server_init(void * sub_proc,void * para)
{
    int ret;
    struct init_para * init_para=para;

    client_conn	= get_connector(CONN_CLIENT,AF_INET);
    if((client_conn ==NULL) & IS_ERR(client_conn))
    {
         print_cubeerr("get conn failed!\n");
         return -EINVAL;
    }
 
    Strcpy(Buf,init_para->server_addr);
    Strcat(Buf,":");
    Itoa(init_para->server_port,Buf+Strlen(Buf));

    ret=client_conn->conn_ops->init(client_conn,"modbus_server_side",Buf);
    if(ret<0)
	    return ret;
    conn_hub->hub_ops->add_connector(conn_hub,client_conn,NULL);

    ret=client_conn->conn_ops->connect(client_conn);
    if(ret<0)
    {
        print_cubeerr("modbus_monitor: client connect error!");
        return ret;
    }
    print_cubeaudit("modbus_monitor: client %p connect succeed!",client_conn);

    return 0;
}

int modbus_monitor_start(void * sub_proc,void * para)
{
    int ret = 0, len = 0, i = 0, j = 0;
    int rc = 0;
    void * recv_msg;
    int type;
    int subtype;

    struct tcloud_connector *recv_conn;
    struct tcloud_connector *temp_conn;
    struct timeval conn_val;
    conn_val.tv_sec=time_val.tv_sec;
    conn_val.tv_usec=time_val.tv_usec;

    for (;;)
    {
        usleep(conn_val.tv_usec);
        ret = conn_hub->hub_ops->select(conn_hub, &conn_val);
    	conn_val.tv_usec = time_val.tv_usec;
        if (ret > 0) {
        do {
            // receive iec104 data
                recv_conn = conn_hub->hub_ops->getactiveread(conn_hub);
                if (recv_conn == NULL)
                    break;
        	    usleep(conn_val.tv_usec);
                if (connector_get_type(recv_conn) == CONN_SERVER)
                {
                    // client send request to server for connect
		            char * peer_addr;
                    channel_conn = recv_conn->conn_ops->accept(recv_conn);
                    if(channel_conn == NULL)
                    {
                        printf("error: server connector accept error %p!\n", channel_conn);
                        continue;
                    }
                    connector_setstate(channel_conn, CONN_CHANNEL_ACCEPT);
                    printf("create a new channel %p!\n", channel_conn);

                    conn_hub->hub_ops->add_connector(conn_hub, channel_conn, NULL);
		            // should add a start message
		            if(channel_conn->conn_ops->getpeeraddr!=NULL)
		            {
			            peer_addr=channel_conn->conn_ops->getpeeraddr(channel_conn);
			            if(peer_addr!=NULL)
				            printf("build channel to %s !\n",peer_addr);	
			            //_channel_set_recv_conn(channel_conn,peer_addr);
                    }
                    // begin connector to server side
                    //ret=client_conn->conn_ops->connect(client_conn);
                    //printf("modbus_monitor: client_conn addr %p\n");
                    //ret=client_conn->conn_ops->connect(client_conn);
                }
                else if (connector_get_type(recv_conn) == CONN_CHANNEL)
                {
                    // client side send transfer data to server 
                    printf("conn peeraddr %s send message\n", recv_conn->conn_peeraddr);
                    rc = 0;
                    // receive modbus data
                    len = recv_conn->conn_ops->read(recv_conn, Buf,256);
                    if (len < 0) {
                        perror("read error");
                        //conn_hub->hub_ops->del_connector(conn_hub, recv_conn);
                    } else if (len == 0) {
                        printf("peer empty!\n");
                        //conn_hub->hub_ops->del_connector(conn_hub, recv_conn);
                    } 
 		            else
		            {
                        // print modbus data
                            printf(" get modbus data %d bytes from client!\n",len);
                            print_bin_data(Buf,len,16);
                        // send modbus data
                            printf(" modbus head is %d!\n",*(UINT16 *)Buf);
	                        ret=client_conn->conn_ops->write(client_conn,Buf,len);
                            printf(" send  modbus  data %d bytes to server!\n",ret);
                            //printf(" iec104 T value %d I value %d!\n",(*(UINT16 *)(Buf+15)),(*(UINT16 *)(Buf+20)));

                    }
                }
                else if (connector_get_type(recv_conn) == CONN_CLIENT)
                {
                    // server side send transfer data to client 
                    printf("conn peeraddr %s send message\n", recv_conn->conn_peeraddr);
                    rc = 0;
                    // receive iec104 data
                    len = recv_conn->conn_ops->read(recv_conn, Buf,256);
                    if (len < 0) {
                        perror("read error");
                        //conn_hub->hub_ops->del_connector(conn_hub, recv_conn);
                    } 
 		            else
		            {
                        // print modbus data
                            printf(" get modbus data %d bytes from server!\n",len);
                            print_bin_data(Buf,len,16);

                            printf(" modbus head value is %d!\n",Buf[0]*128+Buf[1]);

                        // send modbus data
	                        ret=client_conn->conn_ops->write(channel_conn,Buf,len);
                            printf(" send  modbus data %d bytes to client!\n",ret);

                    }
                }
            } while (1);
        }

        // if receive iec104 message, we should send iec104 message
	} 
    return 0;
}
