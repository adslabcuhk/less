#include "comb.hh"
#include "progressBar.hh"
#include <vector>
#include "element.hh"
#include <iostream>
#include "LESSMDS.hh"
#include <algorithm>

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        fprintf(stderr, "usage: LESS_search n k alpha w \n");
        fprintf(stderr, "   -- A brute-force search method for primitive element in GF(2^w)\n");
        fprintf(stderr, "      to ensure the MDS property of (n, k, alpha) LESS.\n");
        fprintf(stderr, "      w must be one of 8, 16, 32.\n");
        fprintf(stderr, "\n\n");
        return 0;
    }

    int n = 14;
    int k = 10;
    int alpha = 2;
    int w = 8;

    sscanf(argv[1], "%d", &n);
    sscanf(argv[2], "%d", &k);
    sscanf(argv[3], "%d", &alpha);
    sscanf(argv[4], "%d", &w);

    LESS_PCmat pcmat(n, k, alpha);
   
    uint32_t e = 0;
    bool flag = pcmat.search_element(w, e);
    if(flag){
        printf("\n+-------------------------------------------------+\n");
        printf("|    Available primitive element: %-10u      |\n", e);
        printf("|    Galois Fields: GF(2^%-2d)                      |\n", w);
        printf("+-------------------------------------------------+\n");
    }else{
        printf("\n+---------------------------------------------------+\n");
        printf("|    No available primitive element in GF(2^%-2d)     |\n", w);
        printf("|    to ensure (%3d, %3d, %3d) LESS is MDS!!!       |\n", n, k, alpha);
        printf("+---------------------------------------------------+\n");
    }

    return 0;
}