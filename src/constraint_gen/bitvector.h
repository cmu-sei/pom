//
// <legal>
// Pointer Ownership Model (POM) Source Code Release
// 
// Copyright 2025 Carnegie Mellon University.
// 
// NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
// INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
// UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR
// IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF
// FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS
// OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT
// MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,
// TRADEMARK, OR COPYRIGHT INFRINGEMENT.
// 
// Licensed under a MIT (SEI)-style license, please see license.txt or
// contact permission@sei.cmu.edu for full terms.
// 
// [DISTRIBUTION STATEMENT A] This material has been approved for public
// release and unlimited distribution.  Please see Copyright notice for
// non-US Government use and distribution.
// 
// DM25-1262
// </legal>
//

#ifndef BITVECTOR_H
#define BITVECTOR_H

#include <vector>
#include <cstdlib>
#include <cstddef>

class bitvector {
private:
    std::vector<size_t> data;
    size_t bit_count;
    
    // Number of bits per size_t element (typically 64 on 64-bit systems)
    static constexpr size_t BITS_PER_ELEMENT = sizeof(size_t) * 8;
    
    // Calculate number of size_t elements needed for given number of bits
    size_t required_elements(size_t bits) const {
        return (bits + BITS_PER_ELEMENT - 1) / BITS_PER_ELEMENT;
    }
    
public:
    // Constructor 0
    bitvector() : bit_count(0) {
    }

    // Constructor 1: takes size, initializes all bits to 0
    bitvector(size_t size) : bit_count(size) {
        size_t num_elements = required_elements(size);
        data.resize(num_elements, 0);
    }
    
    // Constructor 2: copy constructor
    bitvector(const bitvector& other) : data(other.data), bit_count(other.bit_count) {
    }
    
    // Assignment operator
    bitvector& operator=(const bitvector& other) {
        bit_count = other.bit_count;
        data = other.data;
        return *this;
    }

    // Equality comparison operator
    bool operator==(const bitvector& other) {
        if (bit_count != other.bit_count) {
            abort();
        }
        size_t data_size = data.size();
        for (size_t i = 0; i < data_size; ++i) {
            if (data[i] != other.data[i]) {
                return false;
            }
        }
        return true;
    }
    
    // Write a bit at the specified position
    void write(size_t pos, bool val) {
        if (pos >= bit_count) {
            abort();
        }
        
        size_t element_index = pos / BITS_PER_ELEMENT;
        size_t bit_index = pos % BITS_PER_ELEMENT;
        
        if (val) {
            data[element_index] |= (size_t(1) << bit_index);
        } else {
            data[element_index] &= ~(size_t(1) << bit_index);
        }
    }
    
    // Read a bit at the specified position
    bool read(size_t pos) const {
        if (pos >= bit_count) {
            abort();
        }
        
        size_t element_index = pos / BITS_PER_ELEMENT;
        size_t bit_index = pos % BITS_PER_ELEMENT;
        
        return (data[element_index] & (size_t(1) << bit_index)) != 0;
    }
    
    // Bitwise OR assignment
    bitvector& operator|=(const bitvector& other) {
        if (bit_count != other.bit_count) {
            abort();
        }
        
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] |= other.data[i];
        }
        
        return *this;
    }
    
    // Bitwise AND assignment
    bitvector& operator&=(const bitvector& other) {
        if (bit_count != other.bit_count) {
            abort();
        }
        
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] &= other.data[i];
        }
        
        return *this;
    }
    
    // Bitwise NOT (negate all bits)
    void negate() {
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = ~data[i];
        }
        
        // Clear any unused bits in the last element; this simplifies
        // comparison for equality.
        if (bit_count % BITS_PER_ELEMENT != 0) {
            size_t last_element_bits = bit_count % BITS_PER_ELEMENT;
            size_t mask = (size_t(1) << last_element_bits) - 1;
            data.back() &= mask;
        }
    }
    
    // Utility method to get the size
    size_t size() const {
        return bit_count;
    }
};

#endif // BITVECTOR_H
