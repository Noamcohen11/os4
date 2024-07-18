#include "VirtualMemory.h"
#include "PhysicalMemory.h"

// TODO: REMOVE THIS.
#include <cstdio>
#include <cassert>

#include <iostream>
#include <bitset>

struct VirtualAdressStruct
{
    uint64_t page;
    uint64_t offset;
    uint64_t tables[TABLES_DEPTH];

    VirtualAdressStruct(uint64_t virtualAddress)
    {
        uint64_t page_mask = (PAGE_SIZE - 1);
        offset = virtualAddress & page_mask;
        page = virtualAddress >> OFFSET_WIDTH;

        for (int i = TABLES_DEPTH - 1; i > -1; i--)
        {
            tables[TABLES_DEPTH - 1 - i] = (page >> (OFFSET_WIDTH * i)) & page_mask;
        }
    }

    void printAddress()
    {
        std::cout << "Page: 0b" << std::bitset<64>(page) << "\n";
        std::cout << "Offset: 0b" << std::bitset<OFFSET_WIDTH>(offset) << "\n";
        std::cout << "Tables: \n";
        for (int i = 0; i < TABLES_DEPTH; i++)
        {
            std::cout << "depth " << i << " 0b" << std::bitset<OFFSET_WIDTH>(tables[i]);
            if (i < TABLES_DEPTH - 1)
                std::cout << "\n";
        }
        std::cout << "\n";
    }
};

void __clearFrame(uint64_t frame)
{
    uint64_t start = frame * PAGE_SIZE;
    for (uint64_t i = start; i < start + PAGE_SIZE; i++)
    {
        PMwrite(i, 0);
    }
}

word_t *__DFS(word_t *root = 0, int depth = 0)
{
    if (depth == TABLES_DEPTH)
    {
        return 0;
    }

    word_t *new_root;
    word_t *empty_table;
    bool empty = true;
    for (int i = 0; i < PAGE_SIZE; i++)
    {
        PMread((uint64_t)root * PAGE_SIZE + i, new_root);
        if (new_root != 0)
        {
            empty = false;
            empty_table = __DFS(new_root, depth + 1);
            if (empty_table != 0)
            {
                return empty_table;
            }
        }
    }
    if (empty == true)
    {
        return root;
    }
    std::cout << "finished DFS step\n";
    return 0;
}

/*
 * Initialize the virtual memory
 */
void VMinitialize()
{
    __clearFrame(0);
}

/* reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
VirtualAdressStruct __VMaccess(uint64_t virtualAddress)
{
    VirtualAdressStruct va = VirtualAdressStruct(virtualAddress);
    word_t *curr_address = 0;
    word_t *new_address;
    for (int i = 0; i < TABLES_DEPTH; i++)
    {
        std::cout << "DFS address: " << (uint64_t)curr_address * PAGE_SIZE + va.tables[i] << "\n";
        PMread((uint64_t)curr_address * PAGE_SIZE + va.tables[i], new_address);
        if (new_address == 0)
        {
            new_address = __DFS();
            if (new_address == 0)
            {
                // std::cerr << "No empty frame found" << std::endl;
                return va;
            }
        }
        curr_address = new_address;
    }
    return va;
}

int VMread(uint64_t virtualAddress, word_t *value)
{
    return 1;
}

/* writes a word to the given virtual address
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite(uint64_t virtualAddress, word_t value)
{
    return 1;
}

int main()
{
    VMinitialize();
    uint64_t testAddress = 256;
    VirtualAdressStruct va(testAddress);
    va.printAddress();

    std::cout << "Testing VMaccess with address: " << testAddress << "\n";
    VirtualAdressStruct va_accessed = __VMaccess(testAddress);
    va_accessed.printAddress();

    return 0;
}
