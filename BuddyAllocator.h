#ifndef _BuddyAllocator_h_                   // include file only once
#define _BuddyAllocator_h_
#include <iostream>
#include <vector>
using namespace std;
typedef unsigned int uint;

/* declare types as you need */

class BlockHeader{
public:
	// think about what else should be included as member variables
	int block_size;  // size of the block
	BlockHeader* next; // pointer to the next block
	bool isFree;	// indicates whether the block is free or not

	// BlockHeader constructor
	BlockHeader (int _block_size, BlockHeader* _next, bool _isFree):
		block_size(_block_size), next(_next), isFree(_isFree) {}
};

class LinkedList{
	// this is a special linked list that is made out of BlockHeader s. 
public:
	BlockHeader* head;		// you need a head of the list
public:
	void insert (BlockHeader* b){	// adds a block to the list
		// Always add to the beginning of the list!
		b->next = head;
		head = b;
	}

	void remove (BlockHeader* b){  // removes a block from the list
		// if the block is the head
		if(b == head)
		{
			head = head->next;
			b->next = nullptr;
			return;
		}

		// otherwise the element is in the middle of the list of at last. 
		BlockHeader* prev = head;
		for(BlockHeader* curr = head->next; curr != nullptr; curr = curr->next)
		{
			if(curr == b)	// block is found, delete and return.
			{
				prev->next = curr->next;
				curr->next = nullptr;
				return;
			}
			// increase the prev pointer by one
			prev = curr;
		}
	}

	// default constructor, make sure whenever a LinkedList object is constructed, the head points to null
	LinkedList(){
		head = nullptr;
	}
};


class BuddyAllocator{
private:
	/* declare more member variables as necessary */
	vector<LinkedList> FreeList;
	int basic_block_size;
	int total_memory_size;
	BlockHeader* start_pos;	// the start position of the entire block of memory (for getbuddy() to use)

private:
	/* private function you are required to implement
	 this will allow you and us to do unit test */
	
	BlockHeader* getbuddy (BlockHeader * addr); 
	// given a block address, this function returns the address of its buddy 
	
	bool arebuddies (BlockHeader* block1, BlockHeader* block2);
	// checks whether the two blocks are buddies are not
	// note that two adjacent blocks are not buddies when they are different sizes

	BlockHeader* merge (BlockHeader* block1, BlockHeader* block2, int slot);
	// this function merges the two blocks returns the beginning address of the merged block
	// note that either block1 can be to the left of block2, or the other way around

	BlockHeader* split (BlockHeader* block, int slot);
	// splits the given block by putting a new header halfway through the block
	// make sure the two blocks are linked together.
	// also, the original header needs to be corrected
	// Reminder: when a block is split, it is no longer free and should be removed from the empty list


public:
	BuddyAllocator (int _basic_block_size, int _total_memory_length); 
	/* This initializes the memory allocator and makes a portion of 
	   ’_total_memory_length’ bytes available. The allocator uses a ’_basic_block_size’ as 
	   its minimal unit of allocation. The function returns the amount of 
	   memory made available to the allocator. 
	*/ 

	~BuddyAllocator(); 
	/* Destructor that returns any allocated memory back to the operating system. 
	   There should not be any memory leakage (i.e., memory staying allocated).
	*/ 

	void* alloc(int _length); 
	/* Allocate _length number of bytes of free memory and returns the 
		address of the allocated portion. Returns 0 when out of memory. */ 

	void free(void* _a); 
	/* Frees the section of physical memory previously allocated 
	   using alloc(). */ 
   
	void printlist ();
	/* Mainly used for debugging purposes and running short test cases */
	/* This function prints how many free blocks of each size belong to the allocator
	at that point. It also prints the total amount of free memory available just by summing
	up all these blocks.
	Aassuming basic block size = 128 bytes):

	[0] (128): 5
	[1] (256): 0
	[2] (512): 3
	[3] (1024): 0
	....
	....
	 which means that at this point, the allocator has 5 128 byte blocks, 3 512 byte blocks and so on.*/
};

#endif 
