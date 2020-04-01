#include "BuddyAllocator.h"
#include <iostream>
#include <math.h>
using namespace std;

BuddyAllocator::BuddyAllocator (int _basic_block_size, int _total_memory_length){
  // Initialize basic block size and total memory size
  basic_block_size = _basic_block_size, total_memory_size = _total_memory_length;
  // For now I assume the user input total memory length is always power of 2

  // Based on basic_block_size and total_memory_size, construct the FreeList with the right number of slots.
  // Number of slots is given by (size_t)log2(total_memory_size / basic_block_size) + 1
  FreeList.resize(((long)log2(total_memory_size / basic_block_size) + 1), LinkedList());

  // Request the total memory from the system using malloc and put this 
  // chuck of memory (the pointer to the start position) into the last slot of FreeList.
  start_pos = (BlockHeader*) malloc(total_memory_size);
  FreeList[FreeList.size() - 1].head = start_pos;
  // Create a BlockHeader for this block
  BlockHeader b(total_memory_size, nullptr, true);

  // Move this BlockHeader b to the beginning of the block
  *FreeList[FreeList.size() - 1].head = b;

  // alternative way to construct block header
  // start_pos->block_size = total_memory_size;
  // start_pos->isFree = true;
  // start_pos->next = nullptr;

}

BuddyAllocator::~BuddyAllocator (){
    // since dstructor is always called when all the allocated blocks are freed
    // the only thing it should do is to return the big chunk of memory back to the OS
    delete FreeList[FreeList.size()-1].head;
}

void* BuddyAllocator::alloc(int length) {

    if(length >= total_memory_size - sizeof(BlockHeader))
    {
      return nullptr;
    }

    // Locate the correct slot (be careful for integer division!)
    int i = ceil(log2(ceil((length + sizeof(BlockHeader)) / (double)basic_block_size)));// do reverse calculation

    // start from FreeList[i+i] and stop when a non-empty list is encountered.
    int j = i;
    for(; j < FreeList.size(); ++j)
    {
        if(FreeList[j].head != nullptr)
        {
           break;
        }
    }
    // If the loop terminates without breaking, there are not enough memory to allocate so return 0
    if(j == FreeList.size())
    {
       return nullptr;
    }
    // otherwise, start spliting blocks (j - i) times and then ExecuteAllocation!
    // Special Case when j - i == 0, meaning the current slot has a free block ready to be allocated.
    BlockHeader* split_pointer = FreeList[j].head;  // record the location where split should perform
    for(int k = 0; k < j - i; ++k)
    {
        // recored the returned blockheader
        split_pointer = split(split_pointer, j-k);
    }

    // Now the block is ready to be allocated at position FreeList[i]
    // Record the location of the memory block that will be returned
    char* address = (char*) FreeList[i].head + sizeof(BlockHeader);
    // Mark the isFree in this BH to False
    FreeList[i].head->isFree = false;
    // Remove this block from the free list
    FreeList[i].remove(FreeList[i].head);
    // Return the memory block location
    return address;
}

void BuddyAllocator::free(void* a) {
    // substract sizeof(BlockHeader) from the given pointer as char* to get the actual start position of a block
    BlockHeader* block = (BlockHeader*) ((char*) a - sizeof(BlockHeader));

    // Calculate the corresponding slot according to the blocksize
    int slot = ceil(log2((block->block_size) / basic_block_size));
    // mark the isFree to true and put it back to the corresponding freelist slot
    block->isFree = true;
    FreeList[slot].insert(block);

    // get the buddy of the block
    BlockHeader* buddy = getbuddy(block);
    // keep merging until it merge cannot be done.
    while(buddy && slot < FreeList.size()-1)
    {
        block = merge(block, buddy, slot);
        // update the slot
        slot++;
        // update the buddy
        if(slot < FreeList.size() - 1)
        {
            buddy = getbuddy(block);
        }
    }

    return;
}

BlockHeader* BuddyAllocator::getbuddy (BlockHeader * addr)
// given a block address, this function returns the address of its buddy 
{
    char* start = (char*) start_pos;
    char* buddy = (((uintptr_t)((char*)addr - start)) ^ (addr->block_size)) + start;
    // only return the buddy address if they are buddies. 
    if (arebuddies((BlockHeader*) buddy, addr))
    {
        return (BlockHeader*) buddy;
    }
    // return nullptr if they are not buddies
    return nullptr;
}
    
bool BuddyAllocator::arebuddies (BlockHeader* block1, BlockHeader* block2)
// checks whether the two blocks are buddies are not
// note that two adjacent blocks are not buddies when they are different sizes
{
    // two blocks are buddies if they have the same size and they are both free
  return block1->isFree && block2->isFree && block1->block_size == block2->block_size;
}

BlockHeader* BuddyAllocator::merge (BlockHeader* block1, BlockHeader* block2, int slot)
// this function merges the two blocks returns the beginning address of the merged block
// note that either block1 can be to the left of block2, or the other way around
{
  // remove block1 and 2 from the current slot
  FreeList[slot].remove(block1);
  FreeList[slot].remove(block2);

  // decide which block has the lower address and that is the beginning address.
  BlockHeader* beginning = block1 < block2 ? block1 : block2;
  // set a new size to this entire block
  beginning->block_size = 2 * block1->block_size;

  // add the begining to the lower slot (the slot with bigger memory size)
  FreeList[slot+1].insert(beginning);
  return beginning;
}

// splits the given block by putting a new header halfway through the block
// make sure the two blocks are linked together.
// also, the original header needs to be corrected
// Reminder: when a block is split, it is no longer free and should be removed from the empty list
BlockHeader* BuddyAllocator::split(BlockHeader* block, int slot)
{
    // remove the block from the slot
    FreeList[slot].remove(block);
    // compute the buddy header
    BlockHeader* buddy_header = (BlockHeader*) ((char*)block + (block->block_size / 2));
    // write in data for the buddy header
    buddy_header->block_size = block->block_size / 2;
    buddy_header->isFree = true;
    // correct the original header size
    block->block_size /= 2;

    // add these two new blocks to the upper slot (this also makes sure that the two blocks are connected)
    FreeList[slot-1].insert(block);
    FreeList[slot-1].insert(buddy_header);

    // return the left pointer (block itself)
    return block;
}

void BuddyAllocator::printlist (){
  cout << "Printing the Freelist in the format \"[index] (block size) : # of blocks\"" << endl;
  int64_t total_free_memory = 0;
  for (int i=0; i<FreeList.size(); i++)
  {
    int blocksize = ((1<<i) * basic_block_size); // all blocks at this level are this size
    cout << "[" << i <<"] (" << blocksize << ") : ";  // block size at index should always be 2^i * bbs
    int count = 0;
    BlockHeader* b = FreeList [i].head;
    // go through the list from head to tail and count
    while (b){
      total_free_memory += blocksize;
      count ++;
      // block size at index should always be 2^i * bbs
      // checking to make sure that the block is not out of place
      if (b->block_size != blocksize){
        cerr << "ERROR:: Block is in a wrong list" << endl;
        exit (-1);
      }
      b = b->next;
    }
    cout << count << endl;
    cout << "Amount of available free memory: " << total_free_memory << " byes" << endl;  
  }
}

