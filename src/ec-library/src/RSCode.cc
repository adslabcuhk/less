#include "RSCode.hh"

RSCode::RSCode(int _n, int _k, int _w) : ErasureCode(_n, _k, 1, _w)
{
    e = 2;
    order = (1 << w) - 1;
    getPrimitiveElementPower();
    initParityCheckMatrix();
    if (initEncodingMatrix() == false)
    {
        printf("error: failed to initialize encoding matrix\n");
    }
}

RSCode::~RSCode()
{
}

bool RSCode::initEncodingMatrix()
{
    vector<int> from(n, 0);
    vector<int> to(n, 0);
    for (int i = 0; i < n; i++)
    {
        if (i < k)
        {
            from[i] = 1;
        }
        else
        {
            to[i] = 1;
        }
    }

    return convertPCMatrixToGenMatrix(n, k, w, from, to, parityCheckMatrix, generatorMatrix);
}

bool RSCode::getDecodingMatrix(vector<int> failedIndices, int *&decodingMatrix)
{
    if (failedIndices.size() > n - k)
    {
        printf("error: invalid number of failed blocks: %ld\n", failedIndices.size());
        return false;
    }
    if (decodingMatrix != nullptr)
    {
        printf("Decoding matrix should be initialized as nullptr\n");
        return false;
    }

    int numFailedBlocks = failedIndices.size();
    int numAvailBlocks = 0;
    vector<int> from(n, 0);
    vector<int> to(n, 0);

    // get the first k available blocks to repair
    for (auto failedIndex : failedIndices)
    {
        to[failedIndex] = 1;
    }

    for (int i = 0; i < n; i++)
    {
        if (!to[i])
        {
            if (numAvailBlocks < k)
            {
                numAvailBlocks++;
                from[i] = 1;
            }
            else
            {
                break;
            }
        }
    }

    decodingMatrix = new int[numFailedBlocks * k];
    return convertPCMatrixToGenMatrix(n, k, w, from, to, parityCheckMatrix, decodingMatrix);
}

bool RSCode::encodeData(char **dataPtrs, char **codePtrs, int pktSizeBytes, string ecLib)
{
    int numDataPkts = k * alpha;
    int numCodePkts = r * alpha;
    if (ecLib == "ISA-L")
    {
        // for isa-l, first transfer the mat into char*
        unsigned char *encodingMatrixUC = new unsigned char[numDataPkts * numCodePkts];
        for (int i = 0; i < numDataPkts * numCodePkts; i++)
        {
            encodingMatrixUC[i] = (unsigned char)generatorMatrix[i];
        }

        unsigned char *itable = new unsigned char[32 * numDataPkts * numCodePkts];
        ec_init_tables(numDataPkts, numCodePkts, encodingMatrixUC, itable);
        ec_encode_data(pktSizeBytes, numDataPkts, numCodePkts, itable, (unsigned char **)dataPtrs, (unsigned char **)codePtrs);
        delete[] itable;
        delete[] encodingMatrixUC;
    }
    else if (ecLib == "Jerasure")
    {
        jerasure_matrix_encode(numDataPkts, numCodePkts, w, generatorMatrix, dataPtrs, codePtrs, pktSizeBytes);
    }

    return true;
}

bool RSCode::decodeData(vector<int> failedBlockIds, char **failedPtrs, char **availPtrs, int pktSizeBytes, string ecLib)
{
    int numFailedPkts = failedBlockIds.size() * alpha;
    // always use k for RS code
    int numAvailPkts = k * alpha;

    // get decoding matrix
    int *decodingMatrix = nullptr;
    if (getDecodingMatrix(failedBlockIds, decodingMatrix) == false)
    {
        cout << "error: failed to get decoding matrix" << endl;
        return false;
    }

    if (ecLib == "ISA-L")
    {
        // for isa-l, first transfer the mat into char*
        unsigned char *decodingMatrixUC = new unsigned char[numAvailPkts * numFailedPkts];
        for (int i = 0; i < numAvailPkts * numFailedPkts; i++)
        {
            decodingMatrixUC[i] = (unsigned char)decodingMatrix[i];
        }

        unsigned char *itable = new unsigned char[32 * numAvailPkts * numFailedPkts];
        ec_init_tables(numAvailPkts, numFailedPkts, (unsigned char *)decodingMatrixUC, itable);
        ec_encode_data(pktSizeBytes, numAvailPkts, numFailedPkts, itable, (unsigned char **)availPtrs, (unsigned char **)failedPtrs);
        delete[] itable;
        delete[] decodingMatrixUC;
    }
    else if (ecLib == "Jerasure")
    {
        jerasure_matrix_encode(numAvailPkts, numFailedPkts, w, decodingMatrix, availPtrs, failedPtrs, pktSizeBytes);
    }

    delete[] decodingMatrix;

    return true;
}

void RSCode::initParityCheckMatrix()
{
    for (int i = 0; i < r; i++)
    {
        for (int j = 0; j < n; j++)
        {
            parityCheckMatrix[i * n + j] = primitiveElementPower[(j * i) % order];
        }
    }
}

void RSCode::getPrimitiveElementPower()
{
    primitiveElementPower.resize(order);
    primitiveElementPower[0] = 1;
    for (int i = 1; i < order; i++)
    {
        primitiveElementPower[i] = galois_single_multiply(primitiveElementPower[i - 1], e, w);
    }
}