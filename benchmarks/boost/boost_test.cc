#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/flat_map.hpp>
#include <boost/interprocess/exceptions.hpp>

#include <functional>
#include <utility>
#include <iostream>
#include <string>

#define space_name "MySharedMemory"

int main()
{
    using namespace boost::interprocess;
    // Remove shared memory on construction and destruction

    struct shm_remove
    {
        shm_remove() { shared_memory_object::remove(space_name); }
        ~shm_remove() { shared_memory_object::remove(space_name); }
    } remover;

    typedef int KeyType;
    typedef boost::interprocess::managed_shared_memory::allocator<char>::type char_allocator;
    // typedef boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager> char_allocator;
    // typedef boost::interprocess::basic_string<char, std::char_traits<char>, char_allocator> shm_string;

    struct certificateStorage
    {
        int certificate_id;
        certificateStorage(int _certificate_id, const char *_certificate, const char *_key, const char_allocator &al) : certificate_id(_certificate_id)
        {
        }
    };

#if 1 // STL
    typedef std::pair<const int, certificateStorage> certValueType;
    typedef allocator<certValueType, boost::interprocess::managed_shared_memory::segment_manager> certShmemAllocator;
    typedef map<KeyType, certificateStorage, std::less<KeyType>, certShmemAllocator> certSHMMap;
#else // FLAT_MAP
    typedef std::pair<int, certificateStorage> certValueType; // not const key for flat_map
    typedef allocator<certValueType, boost::interprocess::managed_shared_memory::segment_manager> certShmemAllocator;
    typedef boost::container::flat_map<KeyType, certificateStorage, std::less<KeyType>, certShmemAllocator> certSHMMap;
#endif

    std::cout << "\n\n\nStarting the program.\n\n\n";

    const int numentries = 20;
    const char *elementName = "mymap";
    int size = sizeof(certificateStorage) * numentries + 1000;
    int runningsize = 0;

    std::cout << "SHM size is " << size << " bytes \n";

    try
    {
        managed_shared_memory shm_segment(create_only, space_name /*segment name*/, size);

        certShmemAllocator alloc_inst(shm_segment.get_segment_manager());
        char_allocator ca(shm_segment.get_allocator<char>());

        certSHMMap *mymap = shm_segment.find_or_construct<certSHMMap>(elementName)(std::less<int>(), alloc_inst);

        // mymap->reserve(numentries);

        for (int i = 0; i < numentries; i++)
        {
            std::cout << "Free memory: " << shm_segment.get_free_memory() << "\n";

            certificateStorage thisCert(i, "", "", ca);
            std::cout << "Created object.\n";
            mymap->insert(certValueType(i, thisCert));
            std::cout << "Inserted object. " << i << " size is " << sizeof(thisCert) << " \n";
            runningsize += sizeof(thisCert);
            std::cout << "SHM Current size is " << runningsize << " / " << size << "\n";
        }

        std::cout << "\n\nDone Inserting\nStarting output\n";

        for (int i = 0; i < numentries; i++)
        {
            certificateStorage tmp = mymap->at(i);
            std::cout << "The key is: " << i << " And the value is: " << tmp.certificate_id << "\n";
        }
    }
    catch (boost::interprocess::interprocess_exception &ex)
    {
        std::cout << "\n shm space wont load wont load\n";
        std::cout << "\n Why: " << ex.what() << "\n";
    }
}