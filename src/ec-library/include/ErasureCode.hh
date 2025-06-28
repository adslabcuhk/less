#ifndef __ERASURE_CODE_HH__
#define __ERASURE_CODE_HH__

#include "include.hh"

class ErasureCode
{
public:
    int n;     // number of blocks
    int k;     // number of data blocks
    int r;     // number of parity blocks. r = n - k
    int alpha; // sub-packetization
    int w;     // Galois field width GF(2^w); w could be 8, 16, 32 for Jerasure

    int *parityCheckMatrix; // parity check matrix (size: (r * alpha) * (n * alpha))
    int *generatorMatrix;   // generator (encoding) matrix (size: (r * alpha) * (k * alpha))

    ErasureCode(int _n, int _k, int _alpha, int _w);
    virtual ~ErasureCode();

    /**
     * @brief Initialize encoding matrix / generator matrix.
     *
     * @return true if generatorMatrix is successfully initialized
     * @return false
     */
    virtual bool initEncodingMatrix() = 0;

    /**
     * @brief Get the decoding matrix with failed block indices
     *
     * @param failedIndices failed block indices (0 to n - 1)
     * @param decodingMatrix decoding matrix. Size: (numFailedSymbols * alpha) *
     * (k * alpha)
     * @return true
     * @return false
     */
    virtual bool getDecodingMatrix(vector<int> failedIndices, int *&decodingMatrix) = 0;

    /**
     * @brief Encode data blocks into coded blocks
     *
     * @param dataPtrs data pointers
     * @param codePtrs coded pointers
     * @param pktSizeBytes packet size in bytes
     * @param ecLib EC library ("Jerasure", "ISA-L")
     * @return true
     * @return false
     */
    virtual bool encodeData(char **dataPtrs, char **codePtrs, int pktSizeBytes, string ecLib) = 0;

    /**
     * @brief Decode data blocks from coded blocks
     *
     * @param failedBlockIds failed block indices
     * @param failedPtrs failed packet pointers
     * @param availPtrs available packet pointers
     * @param pktSizeBytes packet size in bytes
     * @param ecLib EC library ("Jerasure", "ISA-L")
     * @return true
     * @return false
     */
    virtual bool decodeData(vector<int> failedBlockIds, char **failedPtrs, char **availPtrs, int pktSizeBytes, string ecLib) = 0;

    /**
     * @brief Convert the parity check matrix to generator matrix
     *
     * @param n
     * @param k
     * @param w
     * @param from indicators of available blocks (size: n). 1 indicates an
     * retrieved available block
     * @param to indicators of failed blocks (size: n). 1 indicates a failed block
     * @param parityCheckMatrix: parity check matrix (size: (r * alpha) * (n * alpha))
     * @param generatorMatrix: generator matrix (size: (numFailedSymbols *
     * alpha) * (k * alpha))
     *
     * @return true if successfully converted the parity check matrix
     * @return false
     */
    static bool convertPCMatrixToGenMatrix(int n, int k, int w, vector<int> from, vector<int> to, int *parityCheckMatrix, int *&generatorMatrix);
};

#endif // __ERASURE_CODE_HH__