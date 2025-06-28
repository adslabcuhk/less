#ifndef __RS_CODE_HH__
#define __RS_CODE_HH__

#include "ErasureCode.hh"

class RSCode : public ErasureCode
{
public:
    uint32_t e;                        // primitive element in GF(2^w)
    int order;                         // GF order
    vector<int> primitiveElementPower; // primitive element power

    RSCode(int _n, int _k, int _w = 8);
    ~RSCode();

    bool initEncodingMatrix();

    bool getDecodingMatrix(vector<int> failedIndices, int *&decodingMatrix);

    bool encodeData(char **dataPtrs, char **codePtrs, int pktSizeBytes, string ecLib);

    bool decodeData(vector<int> failedBlockIds, char **failedPtrs, char **availPtrs, int pktSizeBytes, string ecLib);

private:
    void initParityCheckMatrix();

    void getPrimitiveElementPower();
};

#endif // __RS_CODE_HH__