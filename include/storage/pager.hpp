#ifndef PAGER_HPP
#define PAGER_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>

constexpr size_t PAGE_SIZE = 4096;

class Pager {
private:
    std::fstream file;
    std::string filename;

public:
    Pager(const std::string& db_file) : filename(db_file) {
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

        if (!file.is_open()) {
            file.clear();
            std::ofstream new_file(filename, std::ios::out | std::ios::binary);
            new_file.close();
            file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("error: failed to open or create database file");
            }
        }
    }

    ~Pager() {
        if (file.is_open()) {
            file.close();
        }
    }

    void read_page(uint32_t page_num, std::vector<char>& buffer) {
        if (buffer.size() != PAGE_SIZE) {
            buffer.resize(PAGE_SIZE);
        }

        // Move the file cursor to: page_num * 4096
        file.seekg(page_num * PAGE_SIZE, std::ios::beg);
        
        // Read the bytes. If the file is smaller than where we are reading,
        // it will just leave the buffer empty/partially filled, which is fine for a new page.
        file.read(buffer.data(), PAGE_SIZE);
        file.clear(); // Clear any EOF flags so we can keep reading/writing later
    }

    // Write 4096 bytes from memory onto the disk
    void write_page(uint32_t page_num, const std::vector<char>& buffer) {
        if (buffer.size() != PAGE_SIZE) {
            throw std::invalid_argument("Buffer must be exactly PAGE_SIZE bytes.");
        }

        // Move the file cursor to: page_num * 4096
        file.seekp(page_num * PAGE_SIZE, std::ios::beg);
        
        // Write the data
        file.write(buffer.data(), PAGE_SIZE);
        file.flush(); // Ensure the OS actually writes it to the disk
    }
};

#endif // PAGER_HPP