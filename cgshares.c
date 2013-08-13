/*
 * Copyright Chris Procter 2013
 *
 * Author:      chris procter <cprocter@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <libcgroup.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <getopt.h>

struct group {
	unsigned long int shares;
	unsigned int procs;
	char* path;
	struct group* child;
	struct group* next;
};

char * stripslashes(const char * string){
	int len;
	int i;	
	int count=0;
	char * path;

	len = strlen(string);
	for(i=0;i<len;i++){
		if(*(string+i)=='/'){
			count++;
		}else{
			break;
		}
	}

	//if there was a leading slash make sure we keep one
	if(count>0){
		count--;
	}

	len-=count;
	path = malloc(len+1);
	strcpy(path,(string+count));

	for(i=len;i>0;i--){
		if(*(path+i) == '/'){
			*(path+i)='\0';
		}else if(*(path+i) != '\0'){
			break;
		}
	}

	return path;
}


struct group* creategroup(char* path){

	struct group* grp=NULL;
	struct cgroup * group;
	struct cgroup_controller * control;
	pid_t * procs=NULL;
	int proccount=0;
	int64_t value=0;

	group = cgroup_new_cgroup(path);
	cgroup_get_cgroup(group);
	control = cgroup_get_controller(group, "cpu");
	cgroup_get_value_int64(control, "cpu.shares", &value);
	cgroup_get_procs(path,"cpu", &procs, &proccount);

	grp = malloc(sizeof(struct group));
	grp->path = path;
	grp->procs = proccount;
	grp->shares = value;
	grp->child = NULL;
	grp->next = NULL;
	free(group);
	free(control);

	return grp;
}


struct group*  buildtree(char* mountpoint, char * basepath, int flags){
	struct cgroup * group;
	struct cgroup_controller * control;
	struct cgroup_file_info info;
	void *walkhandle;
	int base=0;
	int ret;
	int len;
	int64_t value=0;
	pid_t * procs=NULL;
	int proccount=0;
	char * path;
	struct group* first=NULL;
	struct group* grp=NULL;
	struct group* current=NULL;

	len=strlen(mountpoint);
	ret = cgroup_walk_tree_begin("cpu", basepath,1 , &walkhandle, &info, &base);
	while(ret != ECGEOF ){
		if(info.type == CGROUP_FILE_TYPE_DIR ){
			group = cgroup_new_cgroup((info.full_path+len));
			cgroup_get_cgroup(group);
			control = cgroup_get_controller(group, "cpu");
			cgroup_get_value_int64(control, "cpu.shares", &value);
			path = stripslashes((info.full_path+len));
			cgroup_get_procs(path,"cpu", &procs, &proccount);

			grp = malloc(sizeof(struct group));
			grp->path = path;
			grp->procs = proccount;
			grp->shares = value;
			grp->child = NULL;
			grp->next = NULL;
			if(strcmp(grp->path,basepath) != 0){
				grp->child = buildtree(mountpoint, grp->path, flags);

				if((flags == 1) || (grp->child != NULL || grp->procs != 0)){
					if(first == NULL){
						first = grp;
					}else{
						current->next = grp;	
					}
					current = grp;
				}else{
					free(grp->path);
					free(grp);
				}
			}
			free(group);
			free(control);
		}
		ret = cgroup_walk_tree_next(1, &walkhandle, &info, base);
	}
	ret = cgroup_walk_tree_end(&walkhandle);
	free(walkhandle);
	return first;
}


void calculatecpu(struct group* parent,double pct,int flags){
	struct group* current=NULL;
	int totalshares=0;

	if((flags==1) ||(parent->procs > 0)){
		totalshares=parent->shares;
	}
	current = parent->child;
	while(current != NULL){
		totalshares+=current->shares;
		current=current->next;
	}

	if((flags == 1) || (parent->procs > 0)){
		printf("%-40s %6.2f%% (%d processes)\n",parent->path, ((double)parent->shares/(double)totalshares)*pct,parent->procs);
	}else{
		printf("%-40s %6s  (%d processes)\n",parent->path,"-", parent->procs);
	}
	current = parent->child;
	while(current != NULL){
		calculatecpu(current, ((double)current->shares/(double)totalshares)*pct,flags);
		current=current->next;
	}

}

static struct option longopts[] = {
  { "all",   no_argument,    NULL,   'a'},
  { NULL,  0,  NULL,  0 }
};

int main(int argc,char *argv[]){
	char * mountpoint;
	struct group* parent=NULL;
	char ch;
	int flags=0;

	while((ch = getopt_long(argc, argv, "a",longopts,NULL)) != -1){
		switch(ch){
			case 'a':
					flags=1;
					break;
			default:
					break;
		}
	}

	cgroup_init();

	cgroup_get_subsys_mount_point ("cpu", &mountpoint);

	parent = creategroup("/");
	parent->child = buildtree(mountpoint,"/",flags);
	calculatecpu(parent,100.0,flags);

	return 0;
}

