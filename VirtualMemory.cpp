#include "VirtualMemory.h"
#include "PhysicalMemory.h"

// TODO: REMOVE THIS.
#include <cstdio>
#include <cassert>

#include <iostream>
#include <bitset>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

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

struct Victim
{
    uint64_t emptyAddress;
    uint64_t maxFrame;
    uint64_t longestDistnaceAddress;
    uint64_t parentAddress;

    Victim()
    {
        emptyAddress = 0;
        maxFrame = 0;
    }

    Victim(uint64_t emptyAddress, uint64_t parentAddress)
    {
        emptyAddress = emptyAddress;
        parentAddress = parentAddress;
    }

    Victim(uint64_t maxFrame, uint64_t longestDistnace, uint64_t parentAddress)
    {
        maxFrame = maxFrame;
        emptyAddress = 0;
        longestDistnace = longestDistnace;
        parentAddress = parentAddress;
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

uint64_t __get_cylindrical_distance(uint64_t address_a, uint64_t address_b)
{
    uint64_t bigger = (address_a > address_b) ? address_a : address_b;
    uint64_t smaller = (address_a > address_b) ? address_b : address_a;
    uint64_t result1 = NUM_PAGES - (bigger - smaller);
    uint64_t result2 = bigger - smaller;
    return (result1 < result2) ? result1 : result2;
}

Victim __DFS(word_t base_address, word_t root = 0, int depth = 0, uint64_t parentAddress = 0)
{
    if (depth == TABLES_DEPTH)
    {
        return Victim();
    }

    word_t new_root;
    Victim curr_table;
    bool empty = true;
    uint64_t max_distance_address = 0;
    uint64_t max_frame_address = 0;
    uint64_t newParentAddress;

    for (int i = 0; i < PAGE_SIZE; i++)
    {
        std::cout << "read in DFS" << std::endl;
        PMread((uint64_t)root * PAGE_SIZE + i, &new_root);
        if (new_root != 0)
        {
            empty = false;
            curr_table = __DFS(base_address, new_root, depth + 1, (uint64_t)root * PAGE_SIZE + i);
            if (curr_table.emptyAddress != 0)
            {
                return curr_table;
            }
            if (max_distance_address == 0)
            {
                max_distance_address = curr_table.longestDistnaceAddress;
            }
            if (__get_cylindrical_distance(base_address, max_distance_address) <
                __get_cylindrical_distance(base_address, curr_table.longestDistnaceAddress))
            {
                max_distance_address = __get_cylindrical_distance(base_address, curr_table.longestDistnaceAddress);
                newParentAddress = (uint64_t)root * PAGE_SIZE + i;
            }
            max_frame_address = MAX(max_frame_address, curr_table.maxFrame);
        }
    }
    if (empty == true && root != base_address)
    {
        std::cout << "empty table found" << std::endl;
        return Victim((uint64_t)root, parentAddress);
    }
    return Victim(max_frame_address, max_distance_address, newParentAddress);
}

word_t __create_frame(VirtualAdressStruct va, word_t curr_address, Victim victim)
{
    word_t address;
    if (victim.emptyAddress != 0)
    {
        address = victim.emptyAddress;
        PMwrite(victim.parentAddress, 0);
    }
    else if (victim.maxFrame != NUM_FRAMES - 1)
    {
        std::cout << "max_frame_address: " << victim.maxFrame << std::endl;
        address = victim.maxFrame + 1;
    }
    else
    {
        address = victim.longestDistnaceAddress;
        PMwrite(victim.parentAddress, 0);
        PMevict(address, va.page);
    }
    PMwrite(curr_address, address);
    return address;
}

/*
 * Initialize the virtual memory
 */
void VMinitialize()
{
    __clearFrame(0);
}

/* TODO: check address*/
word_t __VMaccess(uint64_t virtualAddress)
{
    VirtualAdressStruct va = VirtualAdressStruct(virtualAddress);
    word_t curr_address = 0;
    word_t new_address;
    Victim victim;
    word_t new_frame;
    for (int i = 0; i < TABLES_DEPTH; i++)
    {
        std::cout << "read in VMACCESS" << std::endl;
        PMread((uint64_t)curr_address * PAGE_SIZE + va.tables[i], &new_address);
        if (new_address == 0)
        {
            victim = __DFS(curr_address);
            new_address = victim.emptyAddress;
            new_frame = __create_frame(va, curr_address, victim);
            if (i < TABLES_DEPTH - 1)
            {
                __clearFrame(i);
            }
            else
            {
                PMrestore(new_frame, va.page);
            }
        }
        curr_address = new_address;
    }
    return new_address;
}

/* reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread(uint64_t virtualAddress, word_t *value)
{
    word_t address = __VMaccess(virtualAddress);
    std::cout << "read in VMread" << std::endl;
    PMread(address, value);
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
    word_t address = __VMaccess(virtualAddress);
    PMwrite(address, value);
    return 1;
}

int main()
{
    VMinitialize();
    uint64_t testAddress = 256;
    VirtualAdressStruct va(testAddress);
    // va.printAddress();

    // std::cout << "Testing VMaccess with address: " << testAddress << "\n";
    VirtualAdressStruct va_accessed = __VMaccess(testAddress);
    va_accessed.printAddress();

    return 0;
}
