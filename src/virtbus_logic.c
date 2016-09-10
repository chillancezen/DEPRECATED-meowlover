#include <virtbus_logic.h>
#include <linkedhash.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

int global_epfd;
static struct  nametree_node* virtbus_rootnode;
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
void schedule_epoll_out(struct endpoint*point)
{
	if(point->is_pollout_scheduled==TRUE)
		return;
	struct epoll_event event;
	event.data.fd=point->fd_client;
	event.events=EPOLLOUT|EPOLLIN;
	int res=epoll_ctl(global_epfd,EPOLL_CTL_MOD,point->fd_client,&event);
	assert(!res);
	point->is_pollout_scheduled=TRUE;
}
void close_epoll_out(struct endpoint*point)
{
	if(point->is_pollout_scheduled==FALSE)
		return ;
	struct epoll_event event;
	event.data.fd=point->fd_client;
	event.events=EPOLLIN;
	int res=epoll_ctl(global_epfd,EPOLL_CTL_MOD,point->fd_client,&event);
	assert(!res);
	point->is_pollout_scheduled=FALSE;
	
}

void reset_state_endpoint(struct endpoint *point)
{
	point->state=endpoint_state_init;
	point->msg_buffer_iptr=0;
	point->msg_header_iptr=0;
	point->msg_buffer_pending=0;
	point->msg_header_pending=sizeof(struct message_header);
	//point->msg_send_buffer_iptr=0;
	//point->msg_send_buffer_pending=0;
}

struct message_callback_entry{
	uint16_t message_type;
	void (*callback_entry)(struct tlv_header*,void *,void *);	
};



void message_join_bus_entry(struct tlv_header*tlv,void * value,void * arg)
{
	struct endpoint * point=(struct endpoint *)arg;
	struct virtual_bus *vbus=NULL;
	int rc;
	struct nametree_node *found_node;
	struct message_builder builder;
	struct tlv_header tlv_tmp;
	int dummy;
	
	switch(tlv->type)
	{
		case MESSAGE_TLV_TYPE_JOIN_BUS:
			memcpy(point->bus_name,value,tlv->length);
			point->bus_name[tlv->length]='\0';
			break;
		case MESSAGE_TLV_TYPE_JOIN_BUS_QUANTUM:
			point->bus_quantum=*(int32_t*)value;
			break;
		case MESSAGE_TLV_TYPE_JOIN_BUS_END:/*check whether the bus exists,if not ,create one*/
			rc=lookup_key_in_name_tree(virtbus_rootnode,point->bus_name,&found_node);
			if(rc){/*virtual created by previous node*/
				vbus=(struct virtual_bus*)found_node->node_value;
				printf("[x]virtual bus found:%s\n",point->bus_name);
			}else{
				vbus=alloc_virtual_bus(point->bus_name,point->bus_quantum);
				printf("[x]virtual bus newly created:%s\n",point->bus_name);
				if(vbus)
					add_key_to_name_tree(virtbus_rootnode,point->bus_name,vbus,&found_node);
			}
			if(!point->bus_ptr&&vbus)/*this is newly added bus node,increase the reference count*/
				vbus->ref_counnt++;
			
			point->bus_ptr=vbus;
			/*ack*/
			message_builder_init(&builder,point->msg_send_buffer,sizeof(point->msg_send_buffer));
			tlv_tmp.type=point->bus_ptr?MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED:MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL;
			tlv_tmp.length=0;
			//printf("result %d\n",tlv_tmp.type==MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED);
			message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
			if(point->bus_ptr){
				tlv_tmp.type=MESSAGE_TLV_TYPE_NB_OF_BLOCKS;
				tlv_tmp.length=4;
				message_builder_add_tlv(&builder,&tlv_tmp,&point->bus_ptr->dsm->nr_of_items);
			}
			
			point->msg_send_buffer_pending=builder.message_header_ptr->total_length;
			point->msg_send_buffer_iptr=0;
			schedule_epoll_out(point);
			break;
		
	}
}
void message_common_options_entry(struct tlv_header*tlv,void * value,void * arg)
{
	struct endpoint * point=(struct endpoint *)arg;
	switch(tlv->type)
	{

		case MESSAGE_TLV_TYPE_START_BLOCK_INDEX:
			point->start_block_index=*(uint32_t*)value;
			break;
		case MESSAGE_TLV_TYPE_NB_OF_BLOCKS:
			point->nr_of_blocks=*(uint32_t*)value;
			break;
		case MESSAGE_TLV_TYPE_TARGET_VERSION:
			point->target_version=*(uint64_t*)value;
			point->is_gona_be_matched=TRUE;
			break;
		case MESSAGE_TLV_TYPE_LOCK_BUS:
			/*because it's scheduled only on single CPU/thread,no lock needed */
			point->is_gona_lock_bus=TRUE;
			break;
		case MESSAGE_TLV_TYPE_DATA:
			point->data_length=tlv->length;
			point->data_ptr=value;
			puts("data init");
			break;
		default:

			break;
	}

}
void message_read_bus_entry(struct tlv_header*tlv,void * value,void * arg)
{
	struct endpoint * point=(struct endpoint *)arg;
	struct message_builder builder;
	struct tlv_header tlv_tmp;
	char msg_temp_buffer[ENDPOINT_BUFFER_LENGTH];
	int rc;
	int dummy=0;
	switch(tlv->type)
	{
		case MESSAGE_TLV_TYPE_READ_BUS_START:
			/*reset status fields*/
			point->start_block_index=0;
			point->nr_of_blocks=0;
			point->target_version=0;
			point->is_gona_be_matched=FALSE;
			point->is_gona_lock_bus=FALSE;
			break;
		case MESSAGE_TLV_TYPE_READ_BUS_END:
			message_builder_init(&builder,point->msg_send_buffer,sizeof(point->msg_send_buffer));
			if(!point->bus_ptr){/*bus error:MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL*/
				tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL;
				tlv_tmp.length=0;
				message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
			} else if(point->is_gona_be_matched){/*need to match the version number*/
				rc=issue_bus_read_matched(point->bus_ptr,point->start_block_index,point->nr_of_blocks,msg_temp_buffer,&point->target_version);
				if(rc==-VIRTUAL_BUS_MATCHED_VERSION){
					tlv_tmp.type=MESSAGE_TLV_TYPE_DATA_NOT_MODIFIED;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
				}else if(rc==VIRTUAL_BUS_OK){
					tlv_tmp.type=MESSAGE_TLV_TYPE_DATA;
					tlv_tmp.length=point->nr_of_blocks*CACHE_LINE_SIZE;
					message_builder_add_tlv(&builder,&tlv_tmp,msg_temp_buffer);
					tlv_tmp.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
					tlv_tmp.length=sizeof(uint64_t);
					message_builder_add_tlv(&builder,&tlv_tmp,&point->target_version);
				}else{/*this is never expected*/
					tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
				}
			}else{/*read cache line blocks raw*/
				rc=issue_bus_read_raw(point->bus_ptr,point->start_block_index,point->nr_of_blocks,msg_temp_buffer);
				if(rc==VIRTUAL_BUS_OK){
					tlv_tmp.type=MESSAGE_TLV_TYPE_DATA;
					tlv_tmp.length=point->nr_of_blocks*CACHE_LINE_SIZE;
					message_builder_add_tlv(&builder,&tlv_tmp,msg_temp_buffer);
				}else{
					tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
				}
			}
			
			/*schedule to send the data back to client*/
			point->msg_send_buffer_pending=builder.message_header_ptr->total_length;
			point->msg_send_buffer_iptr=0;
			schedule_epoll_out(point);
			break;
		default:
			break;
	}
}
void message_write_bus_entry(struct tlv_header*tlv,void * value,void * arg)
{
	struct endpoint * point=(struct endpoint *)arg;
	struct message_builder builder;
	struct tlv_header tlv_tmp;
	int dummy;
	int rc;

	switch(tlv->type)
	{
		case MESSAGE_TLV_TYPE_WRITE_BUS_START:
			point->start_block_index=0;
			point->nr_of_blocks=0;
			point->target_version=0;
			point->is_gona_be_matched=FALSE;
			point->is_gona_lock_bus=FALSE;
			point->data_length=0;
			point->data_ptr=NULL;
			puts("write_start");
			break;
		case MESSAGE_TLV_TYPE_WRITE_BUS_END:
			message_builder_init(&builder,point->msg_send_buffer,sizeof(point->msg_send_buffer));
			if(!point->bus_ptr||!point->data_ptr){
				puts("write init eerror");
				tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL;
				tlv_tmp.length=0;
				message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
			}else if(point->is_gona_be_matched){
				puts("write match");
				rc=issue_bus_write_matched(point->bus_ptr,point->start_block_index,point->nr_of_blocks,point->data_ptr, &point->target_version);
				if(rc==-VIRTUAL_BUS_VERSION_NOT_MATCH){/*version number checking fails*/
					tlv_tmp.type=MESSAGE_TLV_TYPE_DATA_NOT_MATCHED;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
				}else if(rc==VIRTUAL_BUS_OK){/*write bus OK,return the updated version*/
					tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);

					tlv_tmp.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
					tlv_tmp.length=8;
					message_builder_add_tlv(&builder,&tlv_tmp,&point->target_version);
				}else{/*error happens*/
					tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
				}
			}else{ 
				puts("write generic");
				rc=issue_bus_write_generic(point->bus_ptr,point->start_block_index,point->nr_of_blocks,point->data_ptr,&point->target_version);
				if(rc==VIRTUAL_BUS_OK){
					tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
					tlv_tmp.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
					tlv_tmp.length=8;
					message_builder_add_tlv(&builder,&tlv_tmp,&point->target_version);
				}else{
					tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
				}
			}
			
			point->msg_send_buffer_pending=builder.message_header_ptr->total_length;
			point->msg_send_buffer_iptr=0;
			schedule_epoll_out(point);
			break;
		default:
			break;
	}
}

struct message_callback_entry cb_entries[]={
		{MESSAGE_TLV_TYPE_JOIN_BUS,message_join_bus_entry},
		{MESSAGE_TLV_TYPE_JOIN_BUS_QUANTUM,message_join_bus_entry},
		{MESSAGE_TLV_TYPE_JOIN_BUS_END,message_join_bus_entry},
		
		{MESSAGE_TLV_TYPE_START_BLOCK_INDEX,message_common_options_entry},
		{MESSAGE_TLV_TYPE_NB_OF_BLOCKS,message_common_options_entry},
		{MESSAGE_TLV_TYPE_TARGET_VERSION,message_common_options_entry},
		{MESSAGE_TLV_TYPE_LOCK_BUS,message_common_options_entry},
		{MESSAGE_TLV_TYPE_DATA,message_common_options_entry},
		
		{MESSAGE_TLV_TYPE_READ_BUS_START,message_read_bus_entry},
		{MESSAGE_TLV_TYPE_READ_BUS_END,message_read_bus_entry},

		{MESSAGE_TLV_TYPE_WRITE_BUS_START,message_write_bus_entry},
		{MESSAGE_TLV_TYPE_WRITE_BUS_END,message_write_bus_entry},
		
		{MESSAGE_TLV_TYPE_END,NULL}
};
void endpoint_message_callback(struct tlv_header*tlv,void * value,void * arg)
{
	
	
	int idx=0;
	/*struct endpoint * point=(struct endpoint *)arg;*/
	for(idx=0;cb_entries[idx].message_type!=MESSAGE_TLV_TYPE_END;idx++){
		if(cb_entries[idx].message_type==tlv->type){
			if(cb_entries[idx].callback_entry)
				cb_entries[idx].callback_entry(tlv,value,arg);
			break;
		}
	}
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
				message_iterate_raw(&point->msg_header,point->msg_buffer,point,endpoint_message_callback);
				/*prepare for another round TLV parse*/
				reset_state_endpoint(point);
				/*test sending data*/
				#if 0
				strcpy(point->msg_send_buffer,"hello world");
				point->msg_send_buffer_pending=11;
				schedule_epoll_out(point);
				#endif
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
	/*0.allocate gloval resource*/
	virtbus_rootnode=alloc_nametree_node();
	assert(virtbus_rootnode);
	initalize_nametree_node(virtbus_rootnode,"meeeow",NULL);
	
	/*take care of following code clip, adding these avoids bugs which is brought by deleting node*/
	struct nametree_node * sub_node=alloc_nametree_node();
	initalize_nametree_node(sub_node,"cute",NULL);
	add_node_to_parent(virtbus_rootnode,sub_node);
	
	
	hashtable_t *socket_hash_tbl=hashtable_create(4096);
	assert(socket_hash_tbl);
	/*1.setup socket framework*/
	int fd_accept;
//	char buffer[1024];
//	int len;
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
	global_epfd=epfd;/*we make this global*/
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
				fd_client=accept(fd_accept,(struct sockaddr*)&client_addr,(socklen_t *)&client_addr_len);
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
						close_epoll_out(tmp_endpoint);//out first removed
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

						/*5.release virtual_bus*/
						
						if(tmp_endpoint->bus_ptr){
							tmp_endpoint->bus_ptr->ref_counnt--;
							if(tmp_endpoint->bus_ptr->ref_counnt<=0){
								res=delete_key_from_name_tree(virtbus_rootnode,tmp_endpoint->bus_name);
								printf("[x]release virtual bus:%s\n",tmp_endpoint->bus_name);
								dealloc_virtual_bus(tmp_endpoint->bus_ptr);
							}
							
						}
						
						free(tmp_endpoint);
						break;
					default:
						
						break;
				}
				
				printf("read status:%d\n",ret_status);
				
			}else if(events[idx].events&EPOLLOUT){ 
				fd_client=events[idx].data.fd;
				tmp_endpoint=hashtable_get_value(socket_hash_tbl,fd_client);
				if(!tmp_endpoint){
					close(fd_client);
					continue;
				}
				/*if there are some data pending,ok,we just send as possible as we can ,but once all data is sent,
				we should close the EPOLLOUT as soon as possible */
				if(tmp_endpoint->msg_send_buffer_pending){
					res=write(tmp_endpoint->fd_client,tmp_endpoint->msg_send_buffer+tmp_endpoint->msg_send_buffer_iptr,tmp_endpoint->msg_send_buffer_pending);
					tmp_endpoint->msg_send_buffer_iptr+=res;
					tmp_endpoint->msg_send_buffer_pending-=res;
				}
				if(!tmp_endpoint->msg_send_buffer_pending){
					printf("no data sent,close epoll out\n");
					close_epoll_out(tmp_endpoint);
				}
			}
		}
	}	
	
	return TRUE;
	fails:
		return FALSE;
}
