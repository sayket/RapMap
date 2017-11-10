#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <functional>
#include <utility>

int main ()
{
    using namespace boost::interprocess;

    std::cout << "Creating shared memory segment" << std::endl;
    try
    {

        std::cout << "Creating shared memory segment" << std::endl;

        managed_shared_memory segment(open_or_create, "CSE549",65536);

        //Again the map<Key, MappedType>'s value_type is std::pair<const Key, MappedType>, so the allocator must allocate that pair.
        typedef int    KeyType;
        typedef float  MappedType;
        typedef std::pair<const int, float> ValueType;

        //Assign allocator 
        typedef allocator<ValueType, managed_shared_memory::segment_manager> ShmemAllocator;

        //The map
        typedef map<KeyType, MappedType, std::less<KeyType>, ShmemAllocator> MySHMMap;

        //Initialize the shared memory STL-compatible allocator
        ShmemAllocator alloc_inst (segment.get_segment_manager());

        //access the map in SHM through the offset ptr                                                         
        MySHMMap :: iterator iter;

        std::cout << "Finding the Map in shared memory segment" << std::endl;
        
        offset_ptr<MySHMMap> m_pmap = segment.find<MySHMMap>("MyMap").first;

        std::cout << "Accessing the Map in shared memory segment" << std::endl;
        
        iter=m_pmap->begin();
        for(; iter!=m_pmap->end();iter++)
        {
               std::cout<<"\n "<<iter->first<<" "<<iter->second;
        }
    }
    catch(std::exception &e)            
    {
            std::cout<<" error  " << e.what() <<std::endl;
            shared_memory_object::remove("SharedMemoryName");
    }
    return 0;
}
