#include "LESSMDS.hh"

#include "jerasure.h"
#include <string.h>
#include <stdio.h>
#include "comb.hh"
#include "progressBar.hh"
#include "element.hh"

/**
 * pegf8[] is the list of primitive elements in GF(2^8)
 * pegf16[] is the list of primitive elements in GF(2^16)
 * (remove the conjugate elements)
 */
#include "pelist.hh"

LESS_PCmat::LESS_PCmat(int n, int k, int alpha) : _n(n), _k(k), _alpha(alpha)
{
    _r = n - k;
    s = _n / (_alpha + 1);
    t = _n % (_alpha + 1);

    rows = _r * _alpha;
    cols = _n * _alpha;
    order = (_n * _alpha - 1) * (_r - 1) + 1;

    pcmat_log = new int[rows * cols];
    primitive_element_power = new uint32_t[order];
    pcmat = new uint32_t[rows * cols];
    square_mat = new uint32_t[rows * rows];
    length = _alpha * _r * _alpha;

    get_pcmat_log();
}

LESS_PCmat::~LESS_PCmat()
{
    delete[] pcmat_log;
    delete[] primitive_element_power;
    delete[] pcmat;
    delete[] square_mat;
}

void LESS_PCmat::get_pcmat_log()
{
    for (int x = 0; x < rows; x++)
    {
        int z = x / _r; // block row
        int u = x % _r; // row in block
        for (int y = 0; y < cols; y++)
        {
            int i = y / _alpha; // block column
            int j = y % _alpha; // column in block
            int gi = g(i);
            int hi = h(i);
            if (z == j || z == gi)
            {
                pcmat_log[x * cols + y] = ((hi * (_alpha + 1) + gi) * _alpha + j) * u;
            }
            else
            {
                pcmat_log[x * cols + y] = -1;
            }
        }
    }
}

void LESS_PCmat::get_pcmat(int w, uint32_t e) // transpose of pcmat
{
    int a = 0;
    primitive_element_power[0] = 1;
    for (int i = 1; i < order; i++)
    {
        primitive_element_power[i] = galois_single_multiply(primitive_element_power[i - 1], e, w);
    }
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            a = pcmat_log[i * cols + j];
            if (a == -1)
            {
                pcmat[j * rows + i] = 0;
            }
            else
            {
                pcmat[j * rows + i] = primitive_element_power[a];
            }
        }
    }
}

uint32_t *LESS_PCmat::get_square_mat(int *subset)
{
    for (int i = 0; i < _r; i++)
    {
        memcpy(square_mat + (i * length), pcmat + (subset[i] * length), length * sizeof(int));
    }
    return square_mat;
}

bool LESS_PCmat::is_invertible(uint32_t *square_mat, int w)
{
    return jerasure_invertible_matrix((int *)square_mat, rows, w);
}

void LESS_PCmat::print_pcmat_log()
{
    for (int i = 0; i < _alpha; i++)
    {
        printf("        ");
        for (int j = 0; j < cols; j++)
        {
            int a = pcmat_log[(i * _r + 1) * cols + j];
            if (a == -1)
            {
                printf("     ");
            }
            else
            {
                printf("%5d", a);
            }
        }
        printf("\n");
    }
}

void LESS_PCmat::print_pcmat()
{
    for (int i = 0; i < _alpha; i++)
    {
        printf("        ");
        for (int j = 0; j < cols; j++)
        {
            uint32_t a = pcmat[j * rows + i * _r + 1];
            if (a == 0)
            {
                printf("     ");
            }
            else
            {
                printf("%5u", a);
            }
        }
        printf("\n");
    }
}

void LESS_PCmat::print_square_mat(int *subset)
{
    // for (int i = 0; i < _alpha; i++)
    // {
    //     printf("        ");
    //     for (int j = 0; j < _r; j++)
    //     {
    //         for (int u = 0; u < _alpha; u++)
    //         {
    //             int a = pcmat_log[(i * _r + 1) * cols + subset[j]*_alpha + u];
    //             if (a == -1)
    //             {
    //                 printf("     ");
    //             }
    //             else
    //             {
    //                 printf("%5d", a);
    //             }
    //         }
    //     }
    //     printf("\n");
    // }

    for (int i = 0; i < _alpha; i++)
    {
        printf("        ");
        for (int j = 0; j < _r; j++)
        {
            for (int u = 0; u < _alpha; u++)
            {
                uint32_t a = pcmat[(j * _alpha + u) * rows + i * _r + 1];
                if (a == 0)
                {
                    printf("     ");
                }
                else
                {
                    printf("%5u", a);
                }
            }
        }
        printf("\n");
    }
}

bool LESS_PCmat::verify_MDS_property(int w, uint32_t e)
{
    get_pcmat(w, e);

    generate_combination g(_n, _r);
    unsigned long long total = comb(_n, _r);
    ProgressBar bar(total);
    printf(">>> Verifying MDS property for LESS:\n");
    printf("        (n, k, alpha)  =  (%d, %d, %d)\n", _n, _k, _alpha);
    // printf("    n-k = %d, alpha = %d, n = %d\n", _r, _alpha, _n);
    printf("        primitive element: %u in GF(2^%d)\n", e, w);
    // printf("     total number of combinations: %lld\n", total);
    do
    {
        uint32_t *M = get_square_mat(g.cur_subset());
        if (!is_invertible(M, w))
        {
            printf("\n        Non-MDS!!!\n\n");
            // g.print();
            // print_square_mat(g.cur_subset());
            return false;
        }
        bar.increment();
    } while (g.next());

    printf("        MDS!!!\n\n");

    return true;
}

bool LESS_PCmat::search_element(int w, uint32_t &e)
{
    if (w == 8)
    {
        uint32_t *primitive_element_list = pegf8;
        for (int i = 0; i < 16; i++)
        {
            e = primitive_element_list[i];
            if (verify_MDS_property(w, e))
            {
                return true;
            }
        }
        e = 0;
        return false;
    }
    else if (w == 16)
    {
        uint32_t *primitive_element_list = pegf16;
        for (int i = 0; i < 2048; i++)
        {
            e = primitive_element_list[i];
            if (verify_MDS_property(w, e))
            {
                return true;
            }
        }
        e = 0;
        return false;
    }
    else if (w == 32)
    {
        primitive_element_generator peg(32);
        do
        {
            e = peg.get_element();
            if (verify_MDS_property(w, e))
            {
                return true;
            }
        } while (peg.next());
        e = 0;
        return false;
    }
    else
    {
        e = 0;
        return false;
    }
}

int LESS_PCmat::g(int i)
{
    if (i < t * (s + 1))
    {
        return i / (s + 1);
    }
    else
    {
        return (i - t * (s + 1)) / (s) + t;
    }
}
int LESS_PCmat::h(int i)
{
    if (i < t * (s + 1))
    {
        return i % (s + 1);
    }
    else
    {
        return (i - t * (s + 1)) % (s);
    }
}