#include "comb.hh"
#include <stdio.h>

unsigned long long comb(int n, int r)
{
    if (r <= 0)
        return -1;
    if (r == 0 || r == n)
        return 1;
    if (r > n - r)
    {
        r = n - r;
    }

    unsigned long long *dp = new unsigned long long[r + 1]();
    dp[0] = 1;

    for (int cn = 1; cn <= n; cn++)
    {
        for (int j = (cn < r) ? (cn) : (r); j > 0; j--)
        {
            dp[j] = dp[j] + dp[j - 1];
        }
    }

    unsigned long long result = dp[r];
    delete[] dp;

    return result;
}

void generate_combination::clear()
{
    for (int i = 0; i < _r; i++)
    {
        subset[i] = i;
    }
}

bool generate_combination::next()
{
    int i = _r - 1;
    while ((i >= 0) && subset[i] == _n - _r + i)
    {
        i--;
    }
    if (i < 0)
        return false;
    subset[i]++;
    for (int j = i + 1; j < _r; j++)
    {
        subset[j] = subset[i] + j - i;
    }
    count++;
    return true;
}

void generate_combination::print()
{
    printf("{");
    printf("%d", subset[0]);
    for (int i = 1; i < _r; i++)
    {
        printf(", %3d", subset[i]);
    }
    printf("}\n");
}