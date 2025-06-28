#ifndef __PRIMITIVE_ELEMENT__
#define __PRIMITIVE_ELEMENT__

#include <stdlib.h>
#include "jerasure.h"
#include "include.hh"

/**
 * @brief Generate primitive element from GF(2^w)
 * @param w The word size of the Galois Field, w = 8, 16, 32
 * For primitive elements that are conjugate to each other,
 * we only consider the smallest one in terms of the corresponding decimal integer.
 */

class primitive_element_generator
{
private:
    int w;
    uint32_t cur_exponent; // the exponent of the current primitive element over 2
    uint32_t cur_element;  // the current primitive element
    uint32_t end_exponent; // the maximum exponent of the primitive element

public:
    primitive_element_generator(int w) : w(w), cur_exponent(1), cur_element(2)
    {
        if (w == 8)
        {
            end_exponent = 254; // there are 254 elements not in {0, 1} in GF(2^8)
        }
        else if (w == 16)
        {
            end_exponent = 65534; // there are 65534 elements not in {0, 1} in GF(2^16)
        }
        else if (w == 32)
        {
            end_exponent = 4294967294; // there are 4294967294 elements not in {0, 1} in GF(2^32)
        }
        else
        {
            end_exponent = 0; // only consider w = 8, 16, 32
        }
    }
    ~primitive_element_generator() {}
    uint32_t get_element() { return cur_element; } // get the current primitive element
    void clear()
    {
        cur_exponent = 1;
        cur_element = 2;
    }
    bool next(); // generate the next primitive element
    bool is_minimum_conjugate(uint32_t e); // check if the current element is the minimum conjugate
    uint32_t minimum_conjugate(uint32_t e);// get the minimum conjugate of the current element
};

#endif // #ifndef __PRIMITIVE_ELEMENT__