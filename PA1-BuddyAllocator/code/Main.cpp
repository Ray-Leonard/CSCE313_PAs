#include "Ackerman.h"
#include "BuddyAllocator.h"
#include <unistd.h>
#include <string>

void easytest(BuddyAllocator* ba){
  // be creative here
  // know what to expect after every allocation/deallocation cycle

  // here are a few examples
  ba->printlist();
  // allocating a byte
  char * mem = (char *) ba->alloc (1);
  char* mem1 = (char *) ba->alloc(256);
  char* mem2 = (char *) ba->alloc(256);
  char* mem3 = (char *) ba->alloc(2345);
  char* mem4 = (char *) ba->alloc(25656);
  // ba->alloc(1);
  // ba->alloc(1);
  // now print again, how should the list look now
  ba->printlist ();

  ba->free (mem); // give back the memory you just allocated
  ba->free (mem1);
  ba->free (mem2);
  ba->free (mem3);
  ba->free (mem4);
  ba->printlist(); // shouldn't the list now look like as in the beginning

}

int unit_translator(string size)
{
  // extract the unit from the size string (last element)
  char unit = size[size.size()-1];
  // error checking
  if(isdigit(unit))
  {
    cout << "Please specify the unit of your memory size!" << endl;
    cout << "--------- HELP ---------" << endl;
    cout << "Type the following units right after the numeric value:" << endl;
    cout << "B or b for Byte" << endl;
    cout << "K or k for KB" << endl;
    cout << "M or m for MB" << endl;
    cout << "All other units will not be accepted!" << endl;
    exit(1);
  }
  // extract the original size value from the size string (everything except from the last element)
  int num_size = stoi(size.substr(0, size.size()-1));
  
  int scale = 1;  // the scale that will be computed from unit

  // do the conversion!
  if('B' == unit || 'b' == unit)
  {
    scale = 1;
  }
  else if('K' == unit || 'K' == unit)
  {
    scale = 1024;
  }
  else if('M' == unit || 'm' == unit)
  {
    scale = 1024 * 1024;
  }

  num_size *= scale;

  return num_size;
}


int main(int argc, char ** argv) {

  int basic_block_size = 128;
  int memory_length = 512 * 1024; // 512 * 1024 = 512KB

  int opt;
  while((opt = getopt(argc, argv, "b:s:")) != -1)
  {
    switch(opt)
    {
      case 'b':
      basic_block_size = unit_translator(optarg);
      break;

      case 's':
      memory_length = unit_translator(optarg);
      break;

      default:
      cout << "Invalid command line argument!" << endl;
      return 0;
    }
  }

  // create memory manager
  BuddyAllocator * allocator = new BuddyAllocator(basic_block_size, memory_length);
  
  // stress-test the memory manager, do this only after you are done with small test cases
  Ackerman* am = new Ackerman ();
  am->test(allocator); // this is the full-fledged test. 
  
  // destroy memory manager
  delete allocator;
}
