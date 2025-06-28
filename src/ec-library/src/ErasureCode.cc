#include "ErasureCode.hh"

ErasureCode::ErasureCode(int _n, int _k, int _alpha, int _w) : n(_n), k(_k), r(_n - _k), alpha(_alpha), w(_w)
{
    // init matrices
    parityCheckMatrix = new int[r * alpha * n * alpha];
    memset(parityCheckMatrix, 0, sizeof(int) * r * alpha * n * alpha);
    generatorMatrix = new int[r * alpha * k * alpha];
    memset(generatorMatrix, 0, sizeof(int) * r * alpha * k * alpha);
}

ErasureCode::~ErasureCode()
{
    delete[] parityCheckMatrix;
    delete[] generatorMatrix;
}

bool ErasureCode::convertPCMatrixToGenMatrix(int n, int k, int w, vector<int> from, vector<int> to, int *parityCheckMatrix, int *&generatorMatrix)
{
    int r = n - k;
    int numAvailBlocks = 0;
    int numFailedBlocks = 0;
    for (int i = 0; i < n; i++)
    {
        if (to[i] && from[i])
        {
            printf("error: node %d appears at both from and to\n", i);
            return false;
        }
        if (from[i])
        {
            numAvailBlocks++;
        }
        if (to[i])
        {
            numFailedBlocks++;
        }
    }
    if (numAvailBlocks != k)
    {
        printf("error: invalid number of available nodes: %d\n", numFailedBlocks);
        return false;
    }
    if (numFailedBlocks > r)
    {
        printf("error: invalid number of failed nodes: %d\n", numFailedBlocks);
        return false;
    }

    const int *H = parityCheckMatrix;
    int *H1 = new int[r * r]; // failed
    int *H2 = new int[r * k]; // helper
    int *H1_inverse = new int[r * r];

    // copy by columns
    for (int j = 0, j1 = 0, j2 = 0; j < n; j++)
    {
        if (!from[j])
        {
            for (int i = 0; i < r; i++)
            {
                H1[i * r + j1] = H[i * n + j];
            }
            j1++;
        }
        else
        {
            for (int i = 0; i < r; i++)
            {
                H2[i * k + j2] = H[i * n + j];
            }
            j2++;
        }
    }

    jerasure_invert_matrix(H1, H1_inverse, r, w);
    int *tmpMatrix = jerasure_matrix_multiply(H1_inverse, H2, r, r, r, k, w);

    delete[] H1;
    delete[] H2;
    delete[] H1_inverse;

    if (numFailedBlocks == r)
    {
        memcpy(generatorMatrix, tmpMatrix, sizeof(int) * r * k);
    }
    else
    {
        // less than r failed nodes
        for (int i = 0, x = 0, y = 0; i < n; i++)
        {
            if (!from[i])
            {
                if (to[i])
                { // only copy when to[i] = 1
                    memcpy(generatorMatrix + y * k, tmpMatrix + x * k, k * sizeof(int));
                    y++;
                }
                x++;
            }
        }
    }

    free(tmpMatrix);
    return true;
}