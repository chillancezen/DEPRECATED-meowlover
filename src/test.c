#include <nametree.h>
#include <dsm.h>
#include <virtbus.h>
#include <stdio.h>
int main()
{
#if 0

	//char dst[10][64];
	struct nametree_node * root_node=alloc_nametree_node();
	initalize_nametree_node(root_node,"meeeow",NULL);

	struct nametree_node * sub_node=alloc_nametree_node();
	initalize_nametree_node(sub_node,"cute",NULL);
	add_node_to_parent(root_node,sub_node);


	struct nametree_node * sub_node1=alloc_nametree_node();
	initalize_nametree_node(sub_node1,"cute2",NULL);
	add_node_to_parent(root_node,sub_node1);

	//int rc=delete_node_from_parent(root_node,sub_node);
	
//	int rc=nametree_son_node_lookup(root_node,"cute1",&node);

	struct nametree_node * node;

	add_key_to_name_tree(root_node,"/meeeowworld",NULL,&node);
	#if 1
	add_key_to_name_tree(root_node,"/hello11/hhh",NULL,&node);
	add_key_to_name_tree(root_node,"/hello11/hhh1/hello",NULL,&node);
	add_key_to_name_tree(root_node,"/hello22/hoo",NULL,&node);
	add_key_to_name_tree(root_node,"/hello22/hoo1",NULL,&node);
	#endif
	add_key_to_name_tree(root_node,"/helloworld/hoo1/cute/heloworld",NULL,&node);
	add_key_to_name_tree(root_node,"/helloworld/hoo1/cute2/missant",NULL,&node);
	//int rc=lookup_key_in_name_tree(root_node,"/hello22/hoo",&node);
	//printf("%d:%s\n",rc,node->node_key);
	delete_key_from_name_tree(root_node,"/helloworld/hoo1/cute2/missant1");
	
	dump_nametree(root_node,0);
	//printf("%d %s\n",rc,node->node_key);

	printf("dsm cache lien:%d\n",1<<CACHE_LINE_BIT);
	

	struct dsm_item* arr=alloc_dsm_item_array(256);
	dealloc_dsm_item_array(arr);
	printf("%d\n",arr[12].block_address);
	
	char buffer[128];
	struct dsm_memory * dsm=alloc_dsm_memory("/cute/meeeow",64);
	
	write_dsm_memory_raw(dsm,3,2,"Hello ,virtual isa");
	update_dsm_memory_version_with_max_number(dsm,0,4);
	update_dsm_memory_version_with_max_number(dsm,3,4);
	update_dsm_memory_version_with_max_number(dsm,0,6);
	update_dsm_memory_version_with_specific_value(dsm,0,1,5);
	//updtae_dsm_memory_version_with_max_number(dsm,4,3);
//	updtae_dsm_memory_version_with_max_number(dsm,1,9);
	//int rc=read_dsm_memory_raw(dsm,3,2,buffer);
	
	printf("%d\n",match_version_number(dsm,0,1,5));
	//printf("%s\n",buffer);
	#endif
	char buffer[128];
	uint64_t version=0;
	struct virtual_bus * bus=alloc_virtual_bus("/cute/meeeow",16);
	write_dsm_memory_raw( bus->dsm,0,2,"hello world,meeeow");
	update_dsm_memory_version_with_max_number(bus->dsm,0,2);
	int rc=issue_bus_read_raw(bus,0,1,buffer);
	
	//dealloc_virtual_bus(bus);
	printf("%d %d :%s\n",rc,version,buffer);
	return 0;
}
