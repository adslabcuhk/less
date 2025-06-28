#ifndef __LESSMDS__
#define __LESSMDS__

#include <stdlib.h>
#include "include.hh"

/**
 * @brief  A class to generate the parity-check matrix of (n, k, alpha) LESS
 *         and verify the MDS property of the code.
 * 
 * @param n The total number of blocks
 * @param k The number of data blocks
 * @param alpha sub-packetization
 * 
 * @fn verify_MDS_property(w, e):
 *     Verify the MDS property of the (n, k, alpha) LESS code over GF(2^w)
 *     with the primitive element e.
 *     In this function, we use the class "generate_combination" to enmuerate all the combinations
 * 
 * @fn search_element(w, e):
 *     Search the primitive element in GF(2^w) to ensure the MDS property of the code.
 *     In this function, we use the class "primitive_element_generator" to enmuerate all the primitive elements.
 *
 * @fn get_pcmat_log():
 *     Generate the (n-k)alpha * n alpha parity-check matrix in log form. 
 *     for example, by the log form, the element e^3 will be represented by 3,
 *                                   the element 0 will be represented by -1.
 *     where e is a undetermined primitive element in GF(2^w).
 * @fn get_pcmat(w, e):
 *     Generate the real parity-check matrix by put e into the parity-check matrix in log form.
 *     For the covencience of generating the square matrix, the parity-check matrix is transposed.
 * @fn get_square_mat(int* subset):
 *     Generate the square matrix fromed by n-k out of the n parity-check sub-matrices.
 * @fn g(i): 0<= g(i) <= alpha the group index of the i-th block.
 * @fn h(i): the index of i-th block in the i-th group.
 *
 */

class LESS_PCmat
{
public:
    LESS_PCmat(int n, int k, int alpha);

    ~LESS_PCmat();

    bool verify_MDS_property(int w, uint32_t e);
    bool search_element(int w, uint32_t &e);

    void get_pcmat_log();
    void get_pcmat(int w, uint32_t e);
    uint32_t *get_square_mat(int *subset);

    bool is_invertible(uint32_t *square_mat, int w);
    void print_pcmat_log();
    void print_pcmat();
    void print_square_mat(int *subset);


private:
    int _n;
    int _k;
    int _alpha;

    int *pcmat_log;
    uint32_t *primitive_element_power;
    uint32_t *pcmat; // transpose of pcmat
    uint32_t *square_mat;

    int g(int i);
    int h(int i);

    // temporary variables to avoid repeated calculation
    int _r;  // _n - _k
    int s;   // _n / (_alpha + 1)
    int t;   // _n % (_alpha + 1)
    int rows; // the number of rows of the parity-check matrix
    int cols; // the number of columns of the parity-check matrix
    int order; // (_n * _alpha - 1) * (_r - 1) + 1
    int length; // _alpha * _r * _alpha
};

#endif // #ifndef __LESSMDS__