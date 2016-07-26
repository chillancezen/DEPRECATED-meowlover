#ifndef __MESSAGE
#define __MESSAGE
#include<stdint.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

struct message_header {
	uint8_t version_number;
	uint8_t reserved;
	uint16_t tlv_number;
	uint32_t total_length;
}__attribute__((packed));


struct tlv_header{
	uint16_t type;
	uint16_t length;
}__attribute__((packed));

struct message_builder{
	uint8_t initialized;
	uint8_t * buffer;
	uint32_t buffer_length;
	uint32_t buffer_iptr;
	struct message_header * message_header_ptr;
	
};
#define MESSAGE_TLV_TYPE_BUS_BASE 0x100
enum ACK_CODE{
	ACK_CODE_OK=1
};
enum MESSAGE_TLV_TYPE
{
	/*string*/MESSAGE_TLV_TYPE_JOIN_BUS=MESSAGE_TLV_TYPE_BUS_BASE,/*before anything ,endpoint must join the bus*/
	/*int32*/MESSAGE_TLV_TYPE_JOIN_BUS_QUANTUM,	
	/*NULL*/MESSAGE_TLV_TYPE_JOIN_BUS_END,
	
	/*NULL*/MESSAGE_TLV_TYPE_READ_BUS_START,
	/*NULL*/MESSAGE_TLV_TYPE_READ_BUS_END,/*it must be */

	/*NULL*/MESSAGE_TLV_TYPE_WRITE_BUS_START,/*must be set before common entries*/
	/*NULL*/MESSAGE_TLV_TYPE_WRITE_BUS_END,/*must be set after common entries*/
	
	/*int32*/MESSAGE_TLV_TYPE_START_BLOCK_INDEX,
	/*int32*/MESSAGE_TLV_TYPE_NB_OF_BLOCKS,
	/*int64*/MESSAGE_TLV_TYPE_TARGET_VERSION,
	/*NULL*/MESSAGE_TLV_TYPE_LOCK_BUS,
	
	/*NULL*/MESSAGE_TLV_TYPE_ACK_BUS,/*ack endpoit the result of the issued bus operation*/
	/*NULL*/MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED,
	/*NULL*/MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL,
	/*NULL*/MESSAGE_TLV_TYPE_DATA,
	/*NULL*/MESSAGE_TLV_TYPE_DATA_NOT_MODIFIED,
	/*NULL*/MESSAGE_TLV_TYPE_DATA_NOT_MATCHED,
	
	MESSAGE_TLV_TYPE_END/*indicator of ending*/
};
#define MESSAGE_VERSION 0x1
int message_builder_init(struct message_builder * builder,char * buffer,int buffer_length);
int message_builder_add_tlv(struct message_builder *builder,struct tlv_header* tlv,void * value);
int message_iterate(struct message_builder *builder,void * argument,void (*callback)(struct tlv_header*,void *,void *));
int message_parse_raw(struct message_header *header,void* tlv_start,struct tlv_header * tlv,void**value);
int message_iterate_raw(struct message_header *header,void *tlv_start,void * argument,void (*callback)(struct tlv_header* _tlv,void *_value,void *_arg));
void def_callback(struct tlv_header*tlv,void * value,void * arg);
int send_data_with_quantum(int fd,char * buffer, int length);
int recv_data_with_quantum(int fd,char *buffer,int length);

#endif


