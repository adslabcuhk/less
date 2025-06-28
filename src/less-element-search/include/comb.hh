#ifndef __COMBINATION__
#define __COMBINATION__

#include "include.hh"

/**
 * @brief Calculate the combination of \binom{n}{r}
 * @param n The total number of elements
 * @param r The number of elements to choose
 */
unsigned long long comb(int n, int r);

/**
 * @brief Generate all the combinations of \binom{n}{r}
 * @param n The total number of elements
 * @param r The number of elements to choose
*/
class generate_combination
{
public:
    generate_combination(int n, int r) : _n(n), _r(r)
    {
        subset = new int[r];
        clear();
        count = 1;
        ncr = comb(n, r);
    }

    ~generate_combination()
    {
        delete[] subset;
    }

    int *cur_subset() // get the current subset
    {
        return subset;
    }

    void clear(); // set the subset to the first combination
    bool next();  // generate the next combination
    void print(); // print the current combination

private:
    int _n;
    int _r;
    int *subset;  // a length r array to store the current combination
    unsigned long long ncr; // the total number of combinations
    unsigned long long count; // the order of  current combination
};

#endif // #ifndef __COMBINATION__