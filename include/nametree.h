/*
nametree which could offer convenience to rapidly search linux filesystem file-path like object,
the whole tree is organised by common tree ,and presented child-sibling binary tree.
author:jie(Linc) zheng from BJTU&Juniper networks inc.
*/
#ifndef __NAMETREE
#define __NAMETREE
#include <malloc.h>
#include <string.h>
#define NODE_KEY_LENGTH 64 
struct nametree_node{
	char node_key[NODE_KEY_LENGTH];
	void * node_value;
	struct nametree_node * parent_pointer;
	struct nametree_node * first_child;
	struct nametree_node * next_sibling;
};
struct nametree_node * alloc_nametree_node();
void dealloc_nametree_node(struct nametree_node * node);
void dump_nametree(struct nametree_node* node,int level);
void initalize_nametree_node(struct nametree_node * node,char * key,void * value);
int nametree_node_match_key(struct nametree_node * node,char * key);


/*tree  structure operation*/
void add_node_to_parent(struct nametree_node *parent,struct nametree_node *son);
int  delete_node_from_parent(struct nametree_node * parent,struct nametree_node * son);
int nametree_son_node_lookup(struct nametree_node * parent,char * key,struct nametree_node **foundnode);
int add_key_to_name_tree(struct nametree_node * root,char * key_string,void * value,struct nametree_node **added_node);
int lookup_key_in_name_tree(struct nametree_node * root,char * key_string,struct nametree_node **foundnode);
int delete_key_from_name_tree(struct nametree_node* root, char * key_string);

#endif
