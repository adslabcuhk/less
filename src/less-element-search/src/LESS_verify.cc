#include "comb.hh"
#include "progressBar.hh"
#include <vector>
#include "element.hh"
#include <iostream>
#include "LESSMDS.hh"
#include <algorithm>

int main(int argc, char **argv)
{
    if (argc != 6)
    {
        fprintf(stderr, "usage: LESS_verify n k alpha w p\n");
        fprintf(stderr, "       Verify MDS property for (n, k, alpha) LESS.\n");
        fprintf(stderr, "       Galois Field: GF(2^w).\n");
        fprintf(stderr, "       primitive element: p.\n");
        fprintf(stderr, "       w must be one of 8, 16, 32.\n");
        fprintf(stderr, "\n\n");
        return 0;
    }

    int n = 14;
    int k = 10;
    int alpha = 2;
    int w = 8;
    uint32_t p = 2;

    sscanf(argv[1], "%d", &n);
    sscanf(argv[2], "%d", &k);
    sscanf(argv[3], "%d", &alpha);
    sscanf(argv[4], "%d", &w);
    sscanf(argv[5], "%u", &p);
    

    LESS_PCmat pcmat(n, k, alpha);
    pcmat.verify_MDS_property(w, p);

    return 0;
}

