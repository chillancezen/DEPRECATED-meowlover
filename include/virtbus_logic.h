#ifndef __VIRTBUS_LOGIC
#define __VIRTBUS_LOGIC
#include <virtbus.h>
#include <message.h>
#include <dsm.h>

#define VIRTIO_BUS_PORT 418
#define MAX_EVENT_NR 256

#define MAX_CACHE_LINES 256
#define ENDPOINT_BUFFER_LENGTH (MAX_CACHE_LINES*(1<<CACHE_LINE_BIT)+4096)
/*this is the max buffer length with which we can recive data one time,
I suppose this would be OK*/

int start_virtbus_logic();
enum endpoint_state{
	endpoint_state_init=0,
	endpoint_state_tlv=1
};
struct endpoint{
	int fd_client;
	enum endpoint_state state;
	struct virtual_bus * bus_ptr;
	struct message_header msg_header;
	char msg_buffer[ENDPOINT_BUFFER_LENGTH];
	int msg_header_iptr;
	int msg_buffer_iptr;
	int msg_header_pending;
	int msg_buffer_pending;
};
#endif