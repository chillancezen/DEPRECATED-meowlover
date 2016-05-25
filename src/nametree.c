#include <nametree.h>

#include <string.h>
struct nametree_node * alloc_nametree_node()
{
	struct nametree_node * node;
	node=malloc(sizeof(struct nametree_node));
	if(node){
		memset(node,0x0,sizeof(struct nametree_node));
	}
	return node;
}
void dealloc_nametree_node(struct nametree_node * node)
{
	if(!node)
		return ;
	free(node);
}
void add_node_to_parent(struct nametree_node *parent,struct nametree_node *son)
{
	if(!parent||!son)
		return;
	if(!parent->first_child){
		parent->first_child=son;
		/*here is sanity clean up*/
		son->next_sibling=NULL;
	}else{
		son->next_sibling=parent->first_child->next_sibling;
		parent->first_child->next_sibling=son;
	}
	son->parent_pointer=parent;
}
int  delete_node_from_parent(struct nametree_node * parent,struct nametree_node * son)
{
	/*first of all ,check whether son is truely the son of the parent*/
	struct nametree_node * tmp_node=parent->first_child;
	if(!tmp_node)
		return 0;
	for(;tmp_node;tmp_node=tmp_node->next_sibling){
		if(tmp_node==son)
			break;
	}
	if(!tmp_node)
		return 0;
	if(son==parent->first_child){
		parent->first_child=son->next_sibling;
	}else{
		for(tmp_node=parent->first_child;tmp_node&&tmp_node->next_sibling!=son;tmp_node=tmp_node->next_sibling);
		//bug_on  tmp_node is null
		tmp_node->next_sibling=tmp_node->next_sibling->next_sibling;
		/*here we only detach the target node from sibling chain ,and we do not care about the children nodes of the target node*/
	}
	return !0;
}
int add_key_to_name_tree(struct nametree_node * root,char * key_string,void * value,struct nametree_node **added_node)
{
	char * lptr,*lptr_next;
	char path[128];
	int nptr;
	int rc;
	struct nametree_node * tmp_node;
	struct nametree_node * parent_node=root,* child_node;
	lptr=key_string;
	do{
		memset(path,0x0,sizeof(path));
		while(*lptr&&*lptr==' ')lptr++;
		if(*lptr++!='/')break;
		while(*lptr&&*lptr==' ')lptr++;
		lptr_next=lptr;
		nptr=0;
		while(*lptr_next&&*lptr_next!='/'&&*lptr_next!=' ') path[nptr++]=*lptr_next++;
		lptr=lptr_next;
		/*enter insertion logic*/
		rc=nametree_son_node_lookup(parent_node,path,&tmp_node);
		if(rc){
			/*already exists*/
			child_node=tmp_node;
		}else{
			tmp_node=alloc_nametree_node();
			if(!tmp_node)
				goto fails;
			initalize_nametree_node(tmp_node,path,value);
			add_node_to_parent(parent_node,tmp_node);
			child_node=tmp_node;
		}
		parent_node=child_node;
		/*leave insertion logic*/
		
	}while(*lptr);
	parent_node->node_value=value;
	*added_node=parent_node;
	
	return 1;
	fails:
		return 0;
}
int lookup_key_in_name_tree(struct nametree_node * root,char * key_string,struct nametree_node **foundnode)
{
	char * lptr,*lptr_next;
	char path[128];
	int nptr;
	int rc;
	struct nametree_node * tmp_node;
	struct nametree_node * parent_node=root,* child_node;
	lptr=key_string;
	do{
		memset(path,0x0,sizeof(path));
		while(*lptr&&*lptr==' ')lptr++;
		if(*lptr++!='/')break;
		while(*lptr&&*lptr==' ')lptr++;
		lptr_next=lptr;
		nptr=0;
		while(*lptr_next&&*lptr_next!='/'&&*lptr_next!=' ') path[nptr++]=*lptr_next++;
		lptr=lptr_next;
		/*enter insertion logic*/
		rc=nametree_son_node_lookup(parent_node,path,&tmp_node);
		if(rc){
			/*already exists*/
			child_node=tmp_node;
		}else{
			goto fails;
		}
		parent_node=child_node;
		/*leave insertion logic*/
		
	}while(*lptr);
	*foundnode=parent_node;
	return 1;
	fails:
		return 0;
}
int delete_key_from_name_tree(struct nametree_node* root, char * key_string)
{
	char * lptr,*lptr_next;
	char path[128];
	int nptr;
	int rc;
	struct nametree_node * tmp_node;
	struct nametree_node * parent_node=root,* child_node;
	struct nametree_node * first_node_without_sibling=NULL;
	struct nametree_node * lpkeep;
	lptr=key_string;
	do{
		memset(path,0x0,sizeof(path));
		while(*lptr&&*lptr==' ')lptr++;
		if(*lptr++!='/')break;
		while(*lptr&&*lptr==' ')lptr++;
		lptr_next=lptr;
		nptr=0;
		while(*lptr_next&&*lptr_next!='/'&&*lptr_next!=' ') path[nptr++]=*lptr_next++;
		lptr=lptr_next;
		/*enter insertion logic*/
		rc=nametree_son_node_lookup(parent_node,path,&tmp_node);
		if(rc){
			/*already exists*/
			child_node=tmp_node;
		}else{
			goto fails;
		}
		if(!first_node_without_sibling&&!child_node->next_sibling&&child_node==child_node->parent_pointer->first_child){
			first_node_without_sibling=child_node;
		}

		if(child_node->parent_pointer->first_child->next_sibling){
			first_node_without_sibling=NULL;
		}
			
		parent_node=child_node;
		/*leave insertion logic*/
		
	}while(*lptr);
	/*now detach them form the whole tree*/
	if(!first_node_without_sibling){/*only single node needs to be deleted*/
		delete_node_from_parent(child_node->parent_pointer,child_node);
		dealloc_nametree_node(child_node);
	}else{
		tmp_node=first_node_without_sibling->parent_pointer;
		while(tmp_node){
			lpkeep=tmp_node;
			tmp_node=tmp_node->first_child;
			delete_node_from_parent(lpkeep->parent_pointer,lpkeep);
			dealloc_nametree_node(lpkeep);
			
		}
	}
	//printf("%s\n",first_node_without_sibling->parent_pointer->node_key);
	return 1;
	fails:
		return 0;
}
int nametree_son_node_lookup(struct nametree_node * parent,char * key,struct nametree_node **foundnode)
{
	struct nametree_node * tmp_node=parent->first_child;
	for(;tmp_node&&strcmp(tmp_node->node_key,key);tmp_node=tmp_node->next_sibling);
	if(tmp_node){
		*foundnode=tmp_node;
		return 1;
	}
	return 0;
}
int nametree_node_match_key(struct nametree_node * node,char * key)
{
	return !strncmp(node->node_key,key,NODE_KEY_LENGTH-1);
}
void initalize_nametree_node(struct nametree_node * node,char * key,void * value)
{
	strncpy(node->node_key,key,NODE_KEY_LENGTH-1);
	node->node_value=value;
}
void dump_nametree(struct nametree_node* node,int level)
{
	struct nametree_node * tmp_node;
	for(tmp_node=node;tmp_node;tmp_node=tmp_node->next_sibling){
		printf("%d:%s\n",level,tmp_node->node_key);
		if(tmp_node->first_child)
			dump_nametree(tmp_node->first_child,level+1);
	}
	#if 0 
	//printf("%d:%s\n",level,node->node_key);
		if(node->first_child){
		printf("%d:%s\n",level+1,node->first_child->node_key);
		dump_nametree(node->first_child,level+1);

		}
	for(tmp_node=node->next_sibling;tmp_node;tmp_node=tmp_node->next_sibling){
		printf("%d:%s\n",level,tmp_node->node_key);
		dump_nametree(tmp_node,level);
		}
	#endif

}