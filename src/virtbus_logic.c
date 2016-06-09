#include <virtbus_logic.h>
#include <linkedhash.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
enum read_return_status{
	read_return_pass,
	read_return_close,
};
struct endpoint * alloc_endpoint()
{
	struct endpoint * ep=malloc(sizeof(struct endpoint));
	if(ep){
		memset(ep,0x0,sizeof(struct endpoint));
	}
	return ep;
}
void reset_state_endpoint(struct endpoint *point)
{
	point->state=endpoint_state_init;
	point->msg_buffer_iptr=0;
	point->msg_header_iptr=0;
	point->msg_buffer_pending=0;
	point->msg_header_pending=sizeof(struct message_header);
}
enum read_return_status virtbus_logic_read_data(struct endpoint* point)
{
	int rc;
	switch(point->state)
	{
		case endpoint_state_init:
			rc=read(point->fd_client,point->msg_header_iptr+(char*)&point->msg_header,point->msg_header_pending);
			//printf("recv1:%d\n",rc);
			if(!rc)
				goto error_ret;
			point->msg_header_iptr+=rc;
			point->msg_header_pending-=rc;
			if(!point->msg_header_pending){/*header reception  completed*/
				point->state=endpoint_state_tlv;
				point->msg_buffer_iptr=0;
				point->msg_buffer_pending=point->msg_header.total_length-sizeof(struct message_header);
				//printf("buffer pending:%d\n",point->msg_buffer_pending);
			}
			break;
		case endpoint_state_tlv:
			rc=read(point->fd_client,point->msg_buffer+point->msg_buffer_iptr,point->msg_buffer_pending);
			//printf("recv2:%d\n",rc);
			if(!rc)
				goto error_ret;
			point->msg_buffer_iptr+=rc;
			point->msg_buffer_pending-=rc;
			if(!point->msg_buffer_pending){
				/*here we enter TLV loop logic*/
				//printf("received tlc:%d\n",point->msg_buffer_iptr);
				
				message_iterate_raw(&point->msg_header,point->msg_buffer,NULL,def_callback);
				/*prepare for another round TLV parse*/
				reset_state_endpoint(point);
			}
			break;
		default:
			goto error_ret;
			break;
	}
	return read_return_pass;
	error_ret:
		
		return read_return_close;
}
int start_virtbus_logic()
{

	hashtable_t *socket_hash_tbl=hashtable_create(4096);
	assert(socket_hash_tbl);
	/*1.setup socket framework*/
	int fd_accept;
	char buffer[1024];
	int len;
	struct sockaddr_in server_addr;
	fd_accept=socket(AF_INET,SOCK_STREAM,0);
	if(fd_accept<0)
		return FALSE;
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=INADDR_ANY;
	server_addr.sin_port=htons(VIRTIO_BUS_PORT);
	if(bind(fd_accept,(struct sockaddr*)&server_addr,sizeof(struct sockaddr))<0)
		return FALSE;
	if(listen(fd_accept,64)<0)
		return FALSE;
	/*2.setup epoll framework*/
	int epfd=epoll_create1(EPOLL_CLOEXEC);/*always be the epoll instance*/
	struct epoll_event event;
	struct epoll_event events[MAX_EVENT_NR];
	int idx;
	assert(epfd>=0);
	event.data.fd=fd_accept;
	event.events=EPOLLIN;
	epoll_ctl(epfd,EPOLL_CTL_ADD,fd_accept,&event);
	/*3.enter event loop*/
	int  nfds;
	int fd_client;
	struct sockaddr_in client_addr;
	int client_addr_len;
	int res;
	struct endpoint *tmp_endpoint;
	enum read_return_status ret_status;
	while(1){
		nfds=epoll_wait(epfd,events,MAX_EVENT_NR,100);
		if(nfds<0)
			goto fails;
		for(idx=0;idx<nfds;idx++){
			if(events[idx].data.fd==fd_accept){//new connection arrives
				memset(&client_addr,0x0,sizeof(struct sockaddr_in));
				client_addr_len=sizeof(struct sockaddr_in);
				fd_client=accept(fd_accept,(struct sockaddr*)&client_addr,&client_addr_len);
				printf("new connection:%d\n",fd_client);

				/*add the newly arrived connection into epoll set*/
				event.data.fd=fd_client;
				event.events=EPOLLIN;//|EPOLLET;
				res=epoll_ctl(epfd,EPOLL_CTL_ADD,fd_client,&event);
				if(!res){/*add socket handle into hash table for fast index*/
					tmp_endpoint=alloc_endpoint();
					assert(tmp_endpoint);
					tmp_endpoint->fd_client=fd_client;
					reset_state_endpoint(tmp_endpoint);
					
					res=hashtable_set_key_value(socket_hash_tbl,fd_client,tmp_endpoint);
					assert(!res);
				}
				
			}else if(events[idx].events&EPOLLIN){
				fd_client=events[idx].data.fd;
				tmp_endpoint=hashtable_get_value(socket_hash_tbl,fd_client);
				if(!tmp_endpoint){/*if no endpoint found,this socket must be fake*/
					close(fd_client);
					continue;
				}
				ret_status=virtbus_logic_read_data(tmp_endpoint);
				switch(ret_status)
				{
					case read_return_close:
						/*1.delete fd form epoll set*/
						event.data.fd=tmp_endpoint->fd_client;
						event.events=EPOLLIN;
						epoll_ctl(epfd,EPOLL_CTL_DEL,tmp_endpoint->fd_client,&event);
						/*2.close socket*/
						close(tmp_endpoint->fd_client);
						/*3 release fd from hash table*/
						//assert(hashtable_get_value(socket_hash_tbl,fd_client)==tmp_endpoint);
						res=hashtable_delete_key(socket_hash_tbl,tmp_endpoint->fd_client);
						assert(!res);
						/*4.deallocate what is allocated*/
						free(tmp_endpoint);
						break;
					default:

						break;
				}
				
				printf("read status:%d\n",ret_status);
				
			}else {
				printf("unpreditable:%d\n",events[idx].data.fd);
			}
		}
	}	
	
	return TRUE;
	fails:
		return FALSE;
}
