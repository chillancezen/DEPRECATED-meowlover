
#include<virtbus_client.h>

struct client_endpoint * client_endpoint_alloc_and_init(char * server_ip)
{
	struct client_endpoint * ep=malloc(sizeof(struct client_endpoint));
	int rc;
	if(ep){
		memset(ep,sizeof(struct client_endpoint),0x0);
		
		ep->fd_sock=socket(AF_INET,SOCK_STREAM,0);
		if(!ep->fd_sock)
			goto ret_normal;
		ep->server_addr.sin_family=AF_INET;
		ep->server_addr.sin_port=htons(418);
		ep->server_addr.sin_addr.s_addr=inet_addr(server_ip);
		rc=connect(ep->fd_sock,(struct sockaddr*)&ep->server_addr,sizeof(struct sockaddr_in));
		if(rc)
			goto ret_normal;
		ep->is_connected=TRUE;
		
	}
	ret_normal:
	return ep;
}
void client_endpoint_uninit_and_dealloc(struct client_endpoint*cep)
{
	if(cep->is_connected)
		close(cep->fd_sock);
	free(cep);
}
int client_endpoint_connect_to_virtual_bus(struct client_endpoint *cep,char *bus_name,int32_t cache_line_quantum)
{
	struct message_builder mb;
	struct tlv_header tlv;
	void* value;
	void*dummy;
	/*1.build message*/
	int rc=message_builder_init(&mb,cep->send_buffer,sizeof(cep->send_buffer));
	assert(!rc);
	tlv.type=MESSAGE_TLV_TYPE_JOIN_BUS;
	tlv.length=strlen(bus_name);
	rc=message_builder_add_tlv(&mb,&tlv,bus_name);
	assert(!rc);
	tlv.type=MESSAGE_TLV_TYPE_JOIN_BUS_QUANTUM;
	tlv.length=4;
	rc=message_builder_add_tlv(&mb,&tlv,&cache_line_quantum);
	assert(!rc);
	tlv.type=MESSAGE_TLV_TYPE_JOIN_BUS_END;
	tlv.length=0;
	rc=message_builder_add_tlv(&mb,&tlv,&cache_line_quantum);
	assert(!rc);
	/*2.send the message*/
	
	rc=send_data_with_quantum(cep->fd_sock,mb.buffer,mb.buffer_iptr);
	if(rc!=mb.buffer_iptr)
		return FALSE;
	/*3.recv ack message*/
	rc=recv_tlv_message(cep);
	if(rc)
		return -1;
	tlv.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy);
	if(rc)
		return -1;
	tlv.type=MESSAGE_TLV_TYPE_NB_OF_BLOCKS;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	cep->nr_of_blocks=*(uint32_t*)value;
	cep->is_connected_to_virtual_bus=TRUE;
	return 0;
}
int client_endpoint_read_virtual_bus_raw(struct client_endpoint*cep,int start_block_index,int nr_of_blocks,char*buffer)
{
	struct message_builder builder;
	int dummy;
	void *value;
	struct tlv_header tlv;
	int rc;
	if(cep->is_connected==FALSE||cep->is_connected_to_virtual_bus==FALSE)
		return -1;
	message_builder_init(&builder,cep->send_buffer,sizeof(cep->send_buffer));
	/*1st,bus read started*/
	tlv.type=MESSAGE_TLV_TYPE_READ_BUS_START;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);
	/*2nd,bus read common options*/
	tlv.type=MESSAGE_TLV_TYPE_START_BLOCK_INDEX;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&start_block_index);
	tlv.type=MESSAGE_TLV_TYPE_NB_OF_BLOCKS;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&nr_of_blocks);
	tlv.type=MESSAGE_TLV_TYPE_READ_BUS_END;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);
	/*3rd send them into the network*/
	rc=send_data_with_quantum(cep->fd_sock,builder.buffer,builder.buffer_iptr);
	if(rc!=builder.buffer_iptr)
		return -1;
	/*4th recv the ack message*/
	rc=recv_tlv_message(cep);
	if(rc)
		return -1;
	tlv.type=MESSAGE_TLV_TYPE_DATA;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	memcpy(buffer,value,tlv.length);
	return 0;
}
int client_endpoint_read_virtual_bus_matched(struct client_endpoint *cep,int start_block_index,int nr_of_blocks,char*buffer,uint64_t* target_version,int *is_modified)
{
	struct message_builder builder;
	int dummy;
	void *value;
	struct tlv_header tlv;
	int rc;
	if(cep->is_connected==FALSE||cep->is_connected_to_virtual_bus==FALSE)
		return -1;
	message_builder_init(&builder,cep->send_buffer,sizeof(cep->send_buffer));
	tlv.type=MESSAGE_TLV_TYPE_READ_BUS_START;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);

	tlv.type=MESSAGE_TLV_TYPE_START_BLOCK_INDEX;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&start_block_index);

	tlv.type=MESSAGE_TLV_TYPE_NB_OF_BLOCKS;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&nr_of_blocks);

	tlv.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
	tlv.length=8;
	message_builder_add_tlv(&builder,&tlv,target_version);

	tlv.type=MESSAGE_TLV_TYPE_READ_BUS_END;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);

	rc=send_data_with_quantum(cep->fd_sock,builder.buffer,builder.buffer_iptr);
	if(rc!=builder.buffer_iptr)
		return -1;

	rc=recv_tlv_message(cep);
	if(rc)
		return -1;
	/*1. data not modified*/
	tlv.type=MESSAGE_TLV_TYPE_DATA_NOT_MODIFIED;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(!rc){
		*is_modified=FALSE;
		return 0;
	}
	/*2.data modified*/
	tlv.type=MESSAGE_TLV_TYPE_DATA;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	memcpy(buffer,value,tlv.length);

	tlv.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	*target_version=*(uint64_t*)value;
	*is_modified=TRUE;
		
	//printf(" version length:%d\n",tlv.length);
	return 0;
	

}
int client_endpoint_write_virtual_bus_generic(struct client_endpoint*cep,int start_block_index,int nr_of_blocks,char*buffer,uint64_t *updated_version_number)
{
	struct message_builder builder;
	int dummy;
	void *value;
	struct tlv_header tlv;
	int rc;
	if(cep->is_connected==FALSE||cep->is_connected_to_virtual_bus==FALSE)
		return -1;
	message_builder_init(&builder,cep->send_buffer,sizeof(cep->send_buffer));

	tlv.type=MESSAGE_TLV_TYPE_WRITE_BUS_START;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);

	tlv.type=MESSAGE_TLV_TYPE_START_BLOCK_INDEX;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&start_block_index);

	tlv.type=MESSAGE_TLV_TYPE_NB_OF_BLOCKS;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&nr_of_blocks);

	tlv.type=MESSAGE_TLV_TYPE_DATA;
	tlv.length=CACHE_LINE_SIZE*nr_of_blocks;
	message_builder_add_tlv(&builder,&tlv,buffer);
	
	/*no MESSAGE_TLV_TYPE_TARGET_VERSION section*/

	tlv.type=MESSAGE_TLV_TYPE_WRITE_BUS_END;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);

	rc=send_data_with_quantum(cep->fd_sock,builder.buffer,builder.buffer_iptr);
	if(rc!=builder.buffer_iptr)
		return -1;
	
	rc=recv_tlv_message(cep);
		if(rc)
			return -1;

	tlv.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED;
	tlv.length=0;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	
	tlv.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
	tlv.length=0;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	*updated_version_number=*(uint64_t*)value;
	return 0;
}
int client_endpoint_write_virtual_bus_matched(struct client_endpoint*cep,
													int start_block_index,
													int nr_of_blocks,
													char*buffer,
													int *is_matched,
													uint64_t *target_version)
{
	struct message_builder builder;
	int dummy;
	void *value;
	struct tlv_header tlv;
	int rc;
	if(cep->is_connected==FALSE||cep->is_connected_to_virtual_bus==FALSE)
		return -1;
	message_builder_init(&builder,cep->send_buffer,sizeof(cep->send_buffer));
	
	tlv.type=MESSAGE_TLV_TYPE_WRITE_BUS_START;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);

	tlv.type=MESSAGE_TLV_TYPE_START_BLOCK_INDEX;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&start_block_index);

	tlv.type=MESSAGE_TLV_TYPE_NB_OF_BLOCKS;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&nr_of_blocks);

	tlv.type=MESSAGE_TLV_TYPE_DATA;
	tlv.length=CACHE_LINE_SIZE*nr_of_blocks;
	message_builder_add_tlv(&builder,&tlv,buffer);

	/*add MESSAGE_TLV_TYPE_TARGET_VERSION section*/
	tlv.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
	tlv.length=8;
	message_builder_add_tlv(&builder,&tlv,target_version);

	tlv.type=MESSAGE_TLV_TYPE_WRITE_BUS_END;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);

	rc=send_data_with_quantum(cep->fd_sock,builder.buffer,builder.buffer_iptr);
	if(rc!=builder.buffer_iptr)
		return -1;
	
	rc=recv_tlv_message(cep);
		if(rc)
			return -1;
	tlv.type=MESSAGE_TLV_TYPE_DATA_NOT_MATCHED;
	tlv.length=0;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(!rc){
		*is_matched=FALSE;
		return 0;
	}

	tlv.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED;
	tlv.length=0;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	
	tlv.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
	tlv.length=0;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	*target_version=*(uint64_t*)value;
	*is_matched=TRUE;
	
	return 0;
}
int recv_tlv_message(struct  client_endpoint *cep)
{	
	int rc;
	int tmp;
	cep->recv_buffer_length=0;
	rc=recv_data_with_quantum(cep->fd_sock,cep->recv_buffer,sizeof(struct message_header));
	if(rc!=sizeof(struct message_header))
		return -1;
	cep->recv_buffer_length=rc;
	
	tmp=((struct message_header*)cep->recv_buffer)->total_length-sizeof(struct message_header);
	rc=recv_data_with_quantum(cep->fd_sock,cep->recv_buffer+cep->recv_buffer_length,tmp);
	if(rc!=tmp)
		return -1;
	cep->recv_buffer_length+=rc;
	return 0;
}
