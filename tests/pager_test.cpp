#include "storage/pager.hpp"
#include <cassert>
#include <iostream>
#include <algorithm>
#include <cstdio>

void test_pager_reads_and_writes() {
    std::string test_db = "test.db";

    {
        Pager pager(test_db);

        std::vector<char> write_buffer(PAGE_SIZE, 'A');
        pager.write_page(0, write_buffer);
        std::vector<char> write_buffer2(PAGE_SIZE, 'B');
        pager.write_page(3, write_buffer2);
        std::vector<char> read_buffer;

        pager.read_page(0, read_buffer);
        assert(read_buffer[0] == 'A' && "page 0 should be filled with A");
        assert(read_buffer[4095] == 'A' && "page 0 end should be filled with A");

        pager.read_page(3, read_buffer);
        assert(read_buffer[0] == 'B' && "Page 3 should be filled with B");
        assert(read_buffer[4095] == 'B' && "page 3 end should be filled with B");

        std::cout << "success: pager reads and writes correctly!" << std::endl;
    }

    std::remove(test_db.c_str());
}

int main() {
    std::cout << "running pager tests..." << std::endl;
    test_pager_reads_and_writes();
    std::cout << "all tests passed" << std::endl;
    return 0;
}
