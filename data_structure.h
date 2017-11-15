#include <bits/stdc++.h>
using namespace std;



//map<int,struct file_storage> storage_table;

typedef struct file_name
{
	int inode;   // stores the id of the node
	char name[10];  // stores the name of the entry

}file_name;

struct file_storage
{
	int inode;  // stores the id of the node
	int start_add;  // stores the starting address of the node
	int block_con;  // stores the number of blocks consumed
};


// void memory_alloc()
// {
// 	std::ofstream outfile;
// 	int start_loc = free_loc();


// 	outfile.seekp (0, outfile.beg);
// 	outfile.seekp (start_loc, outfile.beg);
// 	//outfile.write ((char *)&bStruct,sizeof(bStruct));
// }

