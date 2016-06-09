#ifndef __MESSAGE
#define __MESSAGE
#include<stdint.h>
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
	MESSAGE_TLV_TYPE_JOIN_BUS=MESSAGE_TLV_TYPE_BUS_BASE,/*before anything ,endpoint must join the bus*/
	MESSAGE_TLV_TYPE_ACK_BUS,/*ack endpoit the result of the issued bus operation*/
	
	MESSAGE_TLV_TYPE_END/*indicator of ending*/
};
#define MESSAGE_VERSION 0x1
int message_builder_init(struct message_builder * builder,char * buffer,int buffer_length);
int message_builder_add_tlv(struct message_builder *builder,struct tlv_header* tlv,void * value);
int message_iterate(struct message_builder *builder,void * argument,void (*callback)(struct tlv_header*,void *,void *));
int message_iterate_raw(struct message_header *header,void *tlv_start,void * argument,void (*callback)(struct tlv_header* _tlv,void *_value,void *_arg));
void def_callback(struct tlv_header*tlv,void * value,void * arg);

#endif


