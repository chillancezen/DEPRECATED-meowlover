#include <stdio.h>
#include <virtbus_logic.h>
#include <message.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <assert.h>
int main()
{
	int fd=socket(AF_INET,SOCK_STREAM,0);
	char buffer[1024];
	
	assert(fd);
	struct sockaddr_in server_addr;
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(418);
	server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
	int rc=connect(fd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr_in));
	assert(!rc);
	struct message_builder mb;
	rc=message_builder_init(&mb,buffer,sizeof(buffer));
	assert(!rc);
	struct tlv_header tlv;
	tlv.type=0x123;
	tlv.length=0;
	rc=message_builder_add_tlv(&mb,&tlv,"hello world");
	tlv.type=0x45f;
	tlv.length=0;
	rc=message_builder_add_tlv(&mb,&tlv,"hello world");
	assert(!rc);
	printf("%d\n",mb.message_header_ptr->total_length);

	rc=send(fd,mb.buffer,mb.message_header_ptr->total_length,0);
	assert(rc==(mb.message_header_ptr->total_length));
	getchar();
	return 0;
}
