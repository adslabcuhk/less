#include "element.hh"

/**
* In GF(2^w), the element x^i (2^i) is primitive if and only
*
*                  gcd(i, 2^w-1) = 1.
*
* 2^8  - 1 = 3 * 5 * 17
* 2^16 - 1 = 3 * 5 * 17 * 257
* 2^32 - 1 = 3 * 5 * 17 * 257 * 65537
*/

#define is_PE_GF8x(i) (((i) % 3) && ((i) % 5) && ((i) % 17))
#define is_PE_GF16x(i) (((i) % 3) && ((i) % 5) && ((i) % 17) && ((i) % 257))
#define is_PE_GF32x(i) (((i) % 3) && ((i) % 5) && ((i) % 17) && ((i) % 257) && ((i) % 65537))


bool primitive_element_generator::next()
{

    if (cur_exponent == end_exponent)
    {
        return false;
    }

    if (w == 8)
    {
        while (cur_exponent != end_exponent)
        {
            cur_exponent++;
            cur_element = (uint32_t)galois_single_multiply(cur_element, 2, w);
            if (is_PE_GF8x(cur_exponent) && is_minimum_conjugate(cur_element))
            {
                return true;
            }
        }
        return false;
    }
    else if (w == 16)
    {
        while (cur_exponent != end_exponent)
        {
            cur_exponent++;
            cur_element = (uint32_t)galois_single_multiply(cur_element, 2, w);
            if (is_PE_GF16x(cur_exponent) && is_minimum_conjugate(cur_element))
            {
                return true;
            }
        }
        return false;
    }
    else if (w == 32)
    {
        while(cur_exponent!=end_exponent){
            cur_exponent++;
            cur_element = (uint32_t)galois_single_multiply(cur_element, 2, w);
            if (is_PE_GF32x(cur_exponent)&&is_minimum_conjugate(cur_element)){
                return true;
            }
        }
        return false;
    }
    else
    {
        return false;
    }
}

bool primitive_element_generator::is_minimum_conjugate(uint32_t e)
{
    uint32_t conj = e;
    for (int i = 1; i < w; i++)
    {
        conj = (uint32_t)galois_single_multiply(conj, conj, w);
        if (conj < e)
        {
            return false;
        }
    }
    return true;
}

uint32_t primitive_element_generator::minimum_conjugate(uint32_t e)
{
    uint32_t conj = e;
    uint32_t min = e;
    for (int i = 1; i < w; i++)
    {
        conj = (uint32_t)galois_single_multiply(conj, conj, w);
        if (conj < min)
        {
            min = conj;
        }
    }
    return min;
}