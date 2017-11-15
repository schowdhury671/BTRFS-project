#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <cstring> 
#include <bits/stdc++.h>
#include "btimpl.h"
#include "data_structure.h"

using namespace std;

#define free_list_size 
#define block_size 20

int number_of_blocks=0;
vector<vector<int>> free_list(1000,std::vector<int>(2));
//int free_list_entries = 0;

// struct super_block* fs;
// struct file_descriptor filedes[MAX_FILEDES]; // file descriptor table
// int num_files = 2;
// int BLOCKS_PER_FILE;
// int total_allocated = 0;
// int total_used = 0;
int sz = 1;
struct btreeWriteNode {
    int val, count;
    int node_id;   // acts as an identifier of the node
    int size;  // denotes the allocated memory space to the node  
    //struct node* parent_dir_ptr;  // stores the pointer to its parent directory
    //char st;
}btreeWriteNode;

struct superblock
{
	int btree_root_loc;
	int free_list_loc;
	int inode_count;
	int file_storage_loc;
	int file_count;
	int free_list_entries;
	int bTreeNodeWriteCount;
}superblock;

std::fstream fs;
int inode_id_count = 0;
int bTreeNodeWriteCount = 0;;
struct btreeNode *root;
struct file_storage storage_info;
map<int,struct file_storage> storage_table;


void write_file_content(char const *disk_name, char const* content, int loc) {
	FILE *fp = fopen(disk_name, "a+");
	fseek(fp, loc, SEEK_SET);
	cout << "content size " << strlen(content) << endl;
	char sb_buff[strlen(content)];
	memset(sb_buff,0,strlen(content));
	memcpy(sb_buff,content,strlen(content));
	fwrite(sb_buff,1,strlen(content),fp);
	fclose(fp);
}

void read_file_content(char const *disk_name, char * content, int loc) {
	FILE *fp = fopen(disk_name, "r+");
	fseek(fp, loc, SEEK_SET);
	char sb_buff[10];
	memset(sb_buff,0,10);
	fread(sb_buff,1,10,fp);
	memcpy(content,sb_buff,10);
	fclose(fp);
}


int free_loc(int size)
{
	//int free_list_size = free_list.size();
	//cout << "here";
	int fl = 0;
	for(int i=0;i<superblock.free_list_entries;i++)
	{
		if(free_list[i][1] - free_list[i][0] > size)
		{
			int temp = free_list[i][0];
			free_list[i][0] += size;
			fl = 1;
			return temp;
		}
		else if(free_list[i][1] - free_list[i][0] == size)
		{
			int temp = free_list[i][0];
			free_list.erase(free_list.begin()+i);
			superblock.free_list_entries--;
			fl = 1;
			return temp;
		}

	}
	if(fl == 0)
	{
		cout<<"Sufficient memory not found\n";
		return -1;	
	}
}

void new_free_loc(int start,int size)
{
	free_list[superblock.free_list_entries][0] = start;
	free_list[superblock.free_list_entries][1] = start + size;
	superblock.free_list_entries++;
}


void initialise_free_list() {
	free_list[0][0] = 5000;   // to be initialised from seek(0)+offset
	free_list[0][1] = 20000;
	for(int i=1;i<superblock.free_list_entries;i++)
	{
		free_list[i][0] = 0;
		free_list[i][1] = 0;
	}
}

void searching_update(struct btreeWriteNode bStruct, int *pos, struct btreeNode *myNode) {
    if (!myNode) {
            return;
    }

    if (bStruct.val < myNode->val[1]) {
            *pos = 0;
    } else {
            for (*pos = myNode->count; (bStruct.val < myNode->val[*pos] && *pos > 1); (*pos)--)
	            if (bStruct.val == myNode->val[*pos]) {
	            		myNode->count = bStruct.count;
	            		myNode->node_id = bStruct.node_id;
	            		myNode->size = bStruct.size;
	                    return;
	            }
    }
    searching_update(bStruct, pos, myNode->link[*pos]);
    return;
}

void load_b_tree(const char* disk_name,  int loc) {
	int d;
	std::ifstream is(disk_name, std::ifstream::binary);
    if (is) {
    	fs.seekg (0, fs.beg);
    	fs.seekg (loc, fs.beg);
		fs.read ((char *)&bTreeNodeWriteCount,sizeof(int));
		struct btreeWriteNode bStruct;

	cout << "load btree " << bTreeNodeWriteCount;
		for (int i = 1; i <= bTreeNodeWriteCount; ++i)
		{
			//cout << "length" << is.tellg() << endl;
			fs.read ((char *)&bStruct,sizeof(bStruct));
			cout << "\nb tr val " << bStruct.val << endl;

			//is.seekg ((i+1)*sizeof(bStruct), is.beg);

			root = insertion(bStruct.val, root);
			d = 3;
			searching_update(bStruct, &d, root);
		}
		fs.close();
		traversal(root);
	}
}

void bt_traverse_and_write(struct btreeNode *myNode ) {
    int i;
    if (myNode) {
        for (i = 0; i < myNode->count; i++) {
            bt_traverse_and_write(myNode->link[i]);

            struct btreeWriteNode bStruct;
            bStruct.val = myNode->val[i + 1];
            bStruct.node_id = myNode->node_id;
            bStruct.size = myNode->size;
            bStruct.count = myNode->count;
			fs.write ((char *)&bStruct,sizeof(bStruct));
			//outfile.write ((char *)&bStruct.st, sizeof(bStruct.st));
			
			bTreeNodeWriteCount++;
        }
        bt_traverse_and_write(myNode->link[i]);
    }
}

void dump_b_tree(const char* disk_name, int loc) {
	bTreeNodeWriteCount = 0;
	fs.seekp (0, fs.beg);
	fs.seekp (loc+sizeof(int), fs.beg);
	
	bt_traverse_and_write(root);
	fs.seekp (0, fs.beg);
	fs.seekp (loc, fs.beg);
	fs.write ((char *)&bTreeNodeWriteCount,sizeof(bTreeNodeWriteCount));
	fs.close();
}

struct file_storage file_info(int id){

	return (storage_table.find(id))->second;
}

int insert_into_storage_table(struct file_storage storage_info) {
	pair<map<int,struct file_storage>::iterator,bool> ret 
			= storage_table.insert ( pair<int,struct file_storage>(storage_info.inode, storage_info) );
	if (!ret.second)
		return -1;
	else
		return 0;
}

void write_file_storage_details(const char* disk_name, int loc) {
	// outfile.open(disk_name);
	fs.seekp (0, fs.beg);
	fs.seekp (loc, fs.beg);
	for (map<int,struct file_storage>::iterator i = storage_table.begin(); i != storage_table.end(); ++i){
		fs.write ((char *)&i->second, sizeof(i->second));
		cout << (i->second).inode << endl;
	}
}

void load_file_storage_details(const char* disk_name, int loc) {
	std::ifstream is(disk_name, std::ifstream::binary);
    if (is) {
    	fs.seekg (0, fs.beg);
    	fs.seekg (loc, fs.beg);

		struct file_storage f_st;

		for (int i = 1; i <= superblock.file_count; ++i)
		{
			//cout << "length" << is.tellg() << endl;
			fs.read ((char *)&f_st, sizeof(f_st));
			cout << f_st.inode << " " << f_st.block_con << endl;
			if(f_st.inode > superblock.inode_count)
				superblock.inode_count = f_st.inode;
			//is.seekg ((i+1)*sizeof(bStruct), is.beg);
			insert_into_storage_table(f_st);
		}
		fs.close();
	}


}

// writes the free list into memory 
void free_list_write(const char* disk_name, int loc)
{

	cout<<"\nDuring writing\n";

	// outfile.open(disk_name);
	fs.seekp (0, fs.beg);
	fs.seekp (loc, fs.beg);
	cout<<"\nWriting into location "<<loc<<endl;
	// to write
	for(int i=0;i<superblock.free_list_entries;i++)
	{
		for(int j=0;j<2;j++)
		{
			int temp = free_list[i][j];
			fs.write ((char *)&temp,sizeof(int));
			cout << temp << " ";
		}
	}
	cout << endl;
	
}

void free_list_read(const char* disk_name, int loc)
{

	cout<<"\nDuring reading\n";
	// to read
	//std::ifstream fs(disk_name, std::ifstream::binary);
	fs.seekg (0, fs.beg);
	fs.seekg(loc,fs.beg);

	for(int i=0;i<superblock.free_list_entries;i++)
	{
		for(int j=0;j<2;j++)
		{
			int temp;
			fs.read ((char *)&temp,sizeof(temp));
			
			cout << "temp is " << temp << endl;
			free_list[i][j] = temp;
			// superblock.free_list_entries++;
		}

	}
	
}

void create_new_file(const char* disk_name) {
	int size, st_add;
	string f_content;
	cout << "Enter the block size of new file: ";
	cin >> size;

	//cout << "\nEnter the contents of the file:\t";
	//int x;
	//scanf("%d", &x);
	//getline(cin, f_content);
	//size = f_content.size();

	superblock.file_count++;
	st_add = free_loc(size);
	if (st_add == -1)
	{
		cout << "File Could not be created" << endl;
		return;
	}
	storage_info.start_add = st_add;
	storage_info.block_con = size;
	superblock.inode_count++;
	storage_info.inode = superblock.inode_count;
	root = insertion(storage_info.inode, root);
	//write_file_content(disk_name, f_content.c_str(), st_add);
	
	cout<<"\nInode id of new file is "<<storage_info.inode<<endl;
	insert_into_storage_table(storage_info);
	cout << "File has been successfully created.." << endl;
}

int load_super_block(const char* disk_name) {
	std::ifstream is(disk_name, std::ifstream::binary);
    if (is) {
    	fs.seekg (0, fs.beg);
    	fs.read ((char *)&superblock.btree_root_loc, sizeof(int));
    	fs.read ((char *)&superblock.free_list_loc, sizeof(int));
    	fs.read ((char *)&superblock.inode_count, sizeof(int));
    	fs.read ((char *)&superblock.file_storage_loc, sizeof(int));
    	fs.read ((char *)&superblock.file_count, sizeof(int));

    	return 0;
    }
    else
    	return -1;
}

void write_super_block(const char* disk_name) {
	
	fs.seekp (0, fs.beg);
	//outfile.write ((char *)&superblock, sizeof(superblock));
	fs.write ((char *)&superblock.btree_root_loc,sizeof(int));
	fs.write ((char *)&superblock.free_list_loc,sizeof(int)); 
	fs.write ((char *)&superblock.inode_count,sizeof(int));
	fs.write ((char *)&superblock.file_storage_loc,sizeof(int));
	fs.write ((char *)&superblock.file_count,sizeof(int));
}

void search_file(const char* disk_name) {
	int in_id;
	char content[100];
	cout << "Enter the inode id: ";
	cin >> in_id;
	struct file_storage f_st = file_info(in_id);
	cout << "File information: " << endl;
	cout << "inode id -> " << f_st.inode << endl;
	cout << "start_add -> " << f_st.start_add << endl;
	cout << "block_con -> " << f_st.block_con << endl;
	//read_file_content(disk_name, content, f_st.start_add);
	//cout << "content --> " << content << endl;
	// std::ifstream fs1(disk_name, std::ifstream::binary);
	// char cont;
	// fs1.seekg (f_st.start_add, fs.beg);
 //    fs1.read ((char *)&cont, sizeof(cont));
 //    cout << "content --> " << cont;
}

void write_meta_data(char const *disk_name) {
	// fs.seekg (0, fs.beg);
	ofstream fs1(disk_name,std::ofstream::binary);
	fs1.write ((char *)&superblock.inode_count,sizeof(int));
	fs1.write ((char *)&superblock.file_count,sizeof(int));
	fs1.write ((char *)&superblock.free_list_entries,sizeof(int));

	//Free list write
	fs.seekp (2*sizeof(int), fs1.beg);

	for(int i=0;i<superblock.free_list_entries;i++)
	{
		for(int j=0;j<2;j++)
		{
			int temp = free_list[i][j];
			fs1.write ((char *)&temp,sizeof(int));
			//cout << temp << " ";
		}
	}

	//File storage details  write

	for (map<int,struct file_storage>::iterator i = storage_table.begin(); i != storage_table.end(); ++i) {
		fs1.write ((char *)&i->second, sizeof(i->second));
		//cout << (i->second).inode << endl;
	}

}

void read_meta_data(char const *disk_name) {
	std::ifstream fs1(disk_name, std::ifstream::binary);
	//fs.seekg (0, fs.beg);
    fs1.read ((char *)&superblock.inode_count, sizeof(int));
    fs1.read ((char *)&superblock.file_count, sizeof(int));
    fs1.read ((char *)&superblock.free_list_entries, sizeof(int));
	fs.seekg (2*sizeof(int), fs.beg);
	for(int i=0;i<superblock.free_list_entries;i++)
	{
		for(int j=0;j<2;j++)
		{
			int temp;
			fs1.read ((char *)&temp,sizeof(temp));
			free_list[i][j] = temp;
		}

	}
	// cout<<"\nprinting entries of free list\n";
	// for(int i=0;i<superblock.free_list_entries;i++)
	// {
	// 	for(int j=0;j<2;j++)
	// 		cout<<free_list[i][j]<<" ";
	// 	cout<<endl;
	// }
	// cout << "end";
	struct file_storage f_st;
	for (int i = 1; i <= superblock.file_count; ++i)
	{
		fs1.read ((char *)&f_st, sizeof(f_st));
		//cout << "inode read " << f_st.inode << endl;
		root = insertion(f_st.inode, root);
		if(f_st.inode > superblock.inode_count)
			superblock.inode_count = f_st.inode;
		insert_into_storage_table(f_st);
	}
}

void list_all_files() {
	for (map<int,struct file_storage>::iterator i = storage_table.begin(); i != storage_table.end(); ++i) {
		struct file_storage f_st = file_info(i->first);
		cout << "File information: " << endl;
		cout << "inode id -> " << f_st.inode << endl;
		cout << "starting address -> " << f_st.start_add << endl;
		cout << "block consumed -> " << f_st.block_con << endl;
		cout << endl;
	}
}

void delete_file_info (int ind) {
	//storage_table.erase(storage_table.find(ind));
}

void delete_file() {
	int ind;
	cout << "Enter the inode id of the file you want to be deleted: ";
	cin >> ind;
	//delete_file_info(ind);
	auto it = storage_table.find(ind);
	superblock.file_count--;
	new_free_loc((it->second).start_add, (it->second).block_con);
	storage_table.erase(it);
	cout << "File successfully deleted..." << endl;
}

int main(int argc, char const *argv[])
{
	int ch;
	system("clear");
	
	// initialise_free_list();
	// superblock.inode_count = 0;
	// superblock.file_count = 0;
	// superblock.free_list_entries = 1;
	// superblock.btree_root_loc = 1000;
	// superblock.free_list_loc = 2000;
	// superblock.file_storage_loc = 3000;
	// write_meta_data(argv[1]);

	read_meta_data(argv[1]);
	
	while(1){
		cout << endl;
		cout << "Option  | Action" << endl;
		cout << "  1     | Create a file" << endl;
		cout << "  2     | Delete a file" << endl;
		cout << "  3     | Search a file" << endl;
		cout << "  4     | List all the files" << endl;
		cout << "  0     | Exit" << endl;
		cout << "\nEnter your choice: ";
		cin >> ch;
		int flag = 0;
		switch(ch) {
			case 1:
				create_new_file(argv[1]);
				break;

			case 2:
				delete_file();
				break;

			case 3:
				search_file(argv[1]);
				break;

			case 4:
				list_all_files();
				break;

			case 0:
				flag = 1;
				break;

			default:
				cout << "Invalid choice" ;
				break;
		}
		if(flag == 1)
			break;
	}
	
	write_meta_data(argv[1]);

	return 0;
}