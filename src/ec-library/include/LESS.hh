#ifndef __LESS_HH__
#define __LESS_HH__

#include "ErasureCode.hh"
#include "Util.hh"

/**
 * @brief LESS implementation based on parity check matrix
 * In our implementation, we only implement the repair that always retrieve k
 * blocks (i.e., based on parity check matrix)
 *
 */

class LESS : public ErasureCode
{
public:
    uint32_t e;                        // primitive element in GF(2^w)
    uint32_t f;                        // primitive root element (in uint32_t)
    int order;                         // GF order
    vector<int> primitiveElementPower; // primitive element power
    map<int, int> elementMap;          // element map (for debugging)
    int numGroups;                     // number of groups

    LESS(int _n, int _k, int _alpha);
    ~LESS();

    bool initEncodingMatrix();

    bool getDecodingMatrix(vector<int> failedIndices, int *&decodingMatrix);

    bool encodeData(char **dataPtrs, char **codePtrs, int pktSizeBytes, string ecLib);

    bool decodeData(vector<int> failedBlockIds, char **failedPtrs, char **availPtrs, int pktSizeBytes, string ecLib);

private:
    vector<vector<int>> _layout;        // layout (w * n)
    vector<vector<int>> _nodeGroups;    // node groups: _numGroups
    vector<vector<int>> _symbolGroups;  // symbol groups: _numGroups, each corresponds to one extended sub-stripe
    vector<vector<int>> _coefs4Symbols; // coefficient for each symbol for encoding
    vector<int> _nodePermutation;       // node permutation

    map<int, int> _elementMap; // element map (for debugging)

    map<int, int *> _ES2pcMatrixMap; // map: parity check matrix of each extended sub-stripe

    map<int, int *> _ES2encodeMatrixMap; // map: encoding matrix of each extended sub-stripe

    void initParityCheckMatrix();

    void getPrimitiveElementPower();

    bool decodeSingle(vector<int> failedBlockIds, char **failedPtrs, char **availPtrs, int pktSizeBytes, string ecLib); // decode single failure

    bool decodeMultiple(vector<int> failedBlockIds, char **failedPtrs, char **availPtrs, int pktSizeBytes, string ecLib); // decode multiple failures

    bool decodeMultipleWithSubStripes(vector<int> failedBlockIds, char **failedPtrs, char **availPtrs, int pktSizeBytes, string ecLib); // decode multiple failures

    bool decodeMultipleWithPCMatrix(vector<int> failedBlockIds, char **failedPtrs, char **availPtrs, int pktSizeBytes, string ecLib); // decode multiple failures

    /**
     * @brief Get Available Prim Elements for LESS with (n,k,alpha).
     *
     * @param n
     * @param k
     * @param alpha
     * @param w return field width
     * @param e return primitive element
     * @param f return polynomial f (in uint32_t)
     * @return true
     * @return false
     */
    static bool
    getAvailPrimElements(int n, int k, int alpha, int &w, uint32_t &e, uint32_t &f);

    /**
     * @brief find root with primitive polynomial f
     *
     * @param f polynomial
     * @param w field width
     * @return uint32_t root
     */
    static uint32_t findRoot(uint32_t f, int w);

    /**
     * @brief polynomial assignment
     *
     * @param x
     * @param f
     * @param w
     * @return uint32_t
     */
    static uint32_t polynomialAssignment(uint32_t x, uint32_t f, int w);
};

#endif // __LESS_HH__