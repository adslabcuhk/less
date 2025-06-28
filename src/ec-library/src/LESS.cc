#include "LESS.hh"

LESS::LESS(int _n, int _k, int _alpha) : ErasureCode(_n, _k, _alpha, -1)
{
    numGroups = alpha + 1; // num_groups = sub-packetization + 1

    // for given (n,k,alpha), set w, e, f
    if (getAvailPrimElements(n, k, alpha, w, e, f) == false)
    {
        cout << "LESS::LESS() failed to find primitive element" << endl;
        exit(1);
    }

    if (e == 0)
    {
        cout << "LESS::LESS() failed to find available primitive elements in GF(2^" << w << ")" << endl;
        exit(-1);
    }

    // obtain GF elements
    order = (1 << w) - 1;
    getPrimitiveElementPower();

    // element map (debug)
    for (int i = 0; i < 255; i++)
    {
        elementMap[primitiveElementPower[i]] = i;
    }

    /**
     * @brief Construct extended sub-stripes for encoding and decoding
     */

    // init layout
    int symbolId = 0;
    _layout.resize(alpha, vector<int>(n, 0));
    for (int nodeId = 0; nodeId < n; nodeId++)
    {
        for (int sp = 0; sp < alpha; sp++)
        {
            _layout[sp][nodeId] = symbolId++;
        }
    }

    // Step 1: assign nodes to node groups
    _nodeGroups = vector<vector<int>>(numGroups, vector<int>());
    int minNumGroupElements = _n / numGroups;
    int numMaxGroups = n % (numGroups);

    for (int groupId = 0, nodeId = 0; groupId < numGroups; groupId++)
    {
        if (groupId < numMaxGroups)
        {
            for (int i = 0; i < minNumGroupElements + 1; i++)
            {
                _nodeGroups[groupId].push_back(nodeId);
                nodeId++;
            }
        }
        else
        {
            for (int i = 0; i < minNumGroupElements; i++)
            {
                _nodeGroups[groupId].push_back(nodeId);
                nodeId++;
            }
        }
    }

    // // print out the node groups
    // cout << "LESS::LESS() Node groups:" << endl;
    // for (int groupId = 0; groupId < numGroups; groupId++)
    // {
    //     cout << "Group " << groupId << ": ";
    //     for (auto nodeId : _nodeGroups[groupId])
    //     {
    //         cout << nodeId << " ";
    //     }
    //     cout << endl;
    // }

    // Step 2: assign symbols to symbol groups
    _symbolGroups = vector<vector<int>>(numGroups, vector<int>());

    // handle the first numGroups - 1 symbol groups
    for (int groupId = 0; groupId < numGroups - 1; groupId++)
    {
        auto &nodeGroup = _nodeGroups[groupId];
        // add symbols from the same node group
        for (auto nodeId : nodeGroup)
        {
            for (int sp = 0; sp < alpha; sp++)
            {
                _symbolGroups[groupId].push_back(_layout[sp][nodeId]);
            }
        }
        // add symbols from the <groupId>-th sub-stripe
        for (int nodeId = 0; nodeId < n; nodeId++)
        {
            // exclude the nodes from the same node group
            if (find(nodeGroup.begin(), nodeGroup.end(), nodeId) == nodeGroup.end())
            {
                _symbolGroups[groupId].push_back(_layout[groupId][nodeId]);
            }
        }
    }

    // assign the last symbol group
    auto &lastNodeGroup = _nodeGroups[numGroups - 1];

    // put all symbols diagonally
    for (int groupId = 0; groupId < numGroups - 1; groupId++)
    {
        for (auto nodeId : _nodeGroups[groupId])
        {
            _symbolGroups[numGroups - 1].push_back(_layout[groupId][nodeId]);
        }
    }

    // put all symbols from the last node group
    for (auto nodeId : lastNodeGroup)
    {
        for (int sp = 0; sp < alpha; sp++)
        {
            _symbolGroups[numGroups - 1].push_back(_layout[sp][nodeId]);
        }
    }

    // sort the symbol groups
    for (int groupId = 0; groupId < numGroups; groupId++)
    {
        sort(_symbolGroups[groupId].begin(), _symbolGroups[groupId].end());
    }

    // // print out the symbol groups
    // cout << "LESS::LESS() Symbol groups:" << endl;
    // for (int groupId = 0; groupId < numGroups; groupId++)
    // {
    //     cout << "Group " << groupId << ": ";
    //     for (auto symbolId : _symbolGroups[groupId])
    //     {
    //         cout << symbolId << " ";
    //     }
    //     cout << endl;
    // }

    // Step 3: assign coefficients for each symbol for constructing parity
    // check matrix

    // generate node permutation (in vertical order)
    vector<vector<int>> nodeReordering(numGroups, vector<int>());
    for (int nodeId = 0, rid = 0; nodeId < n; nodeId++)
    {
        nodeReordering[rid].push_back(nodeId);
        rid = (rid + 1) % numGroups;
    }
    for (int groupId = 0; groupId < numGroups; groupId++)
    {
        for (auto nodeId : nodeReordering[groupId])
        {
            _nodePermutation.push_back(nodeId);
        }
    }

    // // print node permutation
    // cout << "LESS::LESS() Node permutation:" << endl;
    // for (auto nodeId : _nodePermutation)
    // {
    //     cout << nodeId << " ";
    // }
    // cout << endl;

    // assign coefficients to each symbols: (w * n)
    _coefs4Symbols = vector<vector<int>>(alpha, vector<int>(n, 0));

    for (int nodeId = 0; nodeId < n; nodeId++)
    {
        for (int sp = 0; sp < alpha; sp++)
        {
            _coefs4Symbols[sp][nodeId] = primitiveElementPower[_nodePermutation[nodeId] * alpha + sp];
        }
    }

    // // print out the coefficients
    // cout << "LESS::LESS() Coefficients for encoding:" << endl;
    // for (int i = 0; i < alpha; i++)
    // {
    //     for (int j = 0; j < n; j++)
    //     {
    //         cout << _coefs4Symbols[i][j] << " ";
    //     }
    //     cout << endl;
    // }

    // cout << "symbol element correspondence:" << endl;
    // for (int groupId = 0; groupId < numGroups; groupId++)
    // {
    //     cout << "Group " << groupId << ": ";
    //     for (auto symbolId : _symbolGroups[groupId])
    //     {
    //         int nodeId = symbolId / alpha;
    //         int sp = symbolId % alpha;
    //         cout << _elementMap[_coefs4Symbols[sp][nodeId]] << " ";
    //     }
    //     cout << endl;
    // }

    // // print out the coefficients
    // cout << "LESS::LESS() Coefficients for encoding (corresponding):" << endl;
    // for (int i = 0; i < alpha; i++)
    // {
    //     for (int j = 0; j < n; j++)
    //     {
    //         cout << _elementMap[_coefs4Symbols[i][j]] << " ";
    //     }
    //     cout << endl;
    // }

    // Step 4: construct parity check matrix for each extended sub-stripe
    // we also construct generator matrix for the first alpha extended sub-stripes
    for (int esId = 0; esId < numGroups; esId++)
    {
        vector<int> &symbolGroup = _symbolGroups[esId];

        int as_n = symbolGroup.size();
        int as_k = as_n - r;

        // obtain coefficients for each symbol for encoding
        vector<int> coefs4Encoding(as_n, 0);
        for (int i = 0; i < as_n; i++)
        {
            int symbolNodeId = symbolGroup[i] / alpha;
            int symbolAlpha = symbolGroup[i] % alpha;
            coefs4Encoding[i] = _coefs4Symbols[symbolAlpha][symbolNodeId];
        }

        // construct parity check matrix for the extended sub-stripe
        int *pcMatrix4SubStripe = new int[r * as_n];
        for (int rid = 0; rid < r; rid++)
        {
            for (int cid = 0; cid < as_n; cid++)
            {
                if (rid == 0)
                {
                    pcMatrix4SubStripe[rid * as_n + cid] = 1;
                }
                else
                {
                    pcMatrix4SubStripe[rid * as_n + cid] = galois_single_multiply(pcMatrix4SubStripe[(rid - 1) * as_n + cid], coefs4Encoding[cid], w);
                }
            }
        }
        _ES2pcMatrixMap[esId] = pcMatrix4SubStripe;

        // print parity check matrix
        cout << "LESS::LESS() Parity-check matrix for extended sub-stripe " << esId << ":" << endl;
        jerasure_print_matrix(_ES2pcMatrixMap[esId], r, as_n, w);

        if (esId < alpha)
        {
            // obtain encoding matrix for the extended sub-stripe
            vector<int> from(as_n, 0);
            vector<int> to(as_n, 0);

            // codes: the esId-th sub-blocks of the _m parity blocks
            vector<int> codes;
            for (int nodeId = k; nodeId < n; nodeId++)
            {
                codes.push_back(_layout[esId][nodeId]);
            }

            for (int i = 0; i < as_n; i++)
            {
                if (find(codes.begin(), codes.end(), symbolGroup[i]) != codes.end())
                {
                    to[i] = 1;
                }
                else
                {
                    from[i] = 1;
                }
            }

            int *encodeMatrix4SubStripe = new int[as_k * r];
            if (convertPCMatrixToGenMatrix(as_n, as_k, w, from, to, pcMatrix4SubStripe, encodeMatrix4SubStripe) == false)
            {
                cout << "LESS::LESS() failed to obtain encoding matrix for extended sub-stripe " << esId << endl;
                exit(-1);
            }
            _ES2encodeMatrixMap[esId] = encodeMatrix4SubStripe;

            // // print encoding matrix for extended sub-stripe
            // cout << "LESS::LESS() Encoding matrix for extended sub-stripe " << esId << ":" << endl;
            // jerasure_print_matrix(_ES2encodeMatrixMap[esId], _m, as_k, _fw);
        }
    }

    // Step 5: construct overall parity check matrix and generator matrix
    initParityCheckMatrix();
    if (initEncodingMatrix() == false)
    {
        printf("error: failed to initialize encoding matrix\n");
    }
}

LESS::~LESS()
{
    // delete matrices for each extended sub-stripe
    for (auto &pcMatrix : _ES2pcMatrixMap)
    {
        delete[] pcMatrix.second;
    }

    for (auto &encodeMatrix : _ES2encodeMatrixMap)
    {
        delete[] encodeMatrix.second;
    }
}

bool LESS::initEncodingMatrix()
{
    vector<int> from(n * alpha, 0);
    vector<int> to(n * alpha, 0);
    for (int i = 0; i < n * alpha; i++)
    {
        if (i < k * alpha)
        {
            from[i] = 1;
        }
        else
        {
            to[i] = 1;
        }
    }

    return convertPCMatrixToGenMatrix(n * alpha, k * alpha, w, from, to, parityCheckMatrix, generatorMatrix);
}

bool LESS::getDecodingMatrix(vector<int> failedIndices, int *&decodingMatrix)
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
    vector<int> from(n * alpha, 0);
    vector<int> to(n * alpha, 0);

    // get the first k available blocks to repair
    for (auto failedIndex : failedIndices)
    {
        for (int symId = 0; symId < alpha; symId++)
        {
            to[failedIndex * alpha + symId] = 1;
        }
    }

    for (int i = 0; i < n; i++)
    {
        if (!to[i * alpha])
        {
            if (numAvailBlocks < k)
            {
                numAvailBlocks++;
                for (int symId = 0; symId < alpha; symId++)
                {
                    from[i * alpha + symId] = 1;
                }
            }
            else
            {
                break;
            }
        }
    }

    decodingMatrix = new int[numFailedBlocks * alpha * k * alpha];
    return convertPCMatrixToGenMatrix(n * alpha, k * alpha, w, from, to, parityCheckMatrix, decodingMatrix);
}

bool LESS::encodeData(char **dataPtrs, char **codePtrs, int pktSizeBytes, string ecLib)
{
    /**
     * @brief method 1: use extended sub-stripes for encoding
     */
    char **ptrs = new char *[n * alpha];
    int dataPtrCnt = 0;
    int codePtrCnt = 0;
    for (int i = 0; i < n * alpha; i++)
    {
        int blockId = i / alpha;
        if (blockId < k)
        {
            ptrs[i] = dataPtrs[dataPtrCnt++];
        }
        else
        {
            ptrs[i] = codePtrs[codePtrCnt++];
        }
    }

    for (int sp = 0; sp < alpha; sp++)
    {
        vector<int> &symbolGroup = _symbolGroups[sp];

        // construct RS(as_n, as_k) for the extended sub-stripe
        int as_n = symbolGroup.size();
        int as_k = as_n - r;

        vector<int> data;
        vector<int> codes;

        // codes: the esId-th sub-blocks of the _m parity blocks
        for (int nodeId = k; nodeId < n; nodeId++)
        {
            codes.push_back(_layout[sp][nodeId]);
        }

        for (auto symbolId : symbolGroup)
        {
            if (find(codes.begin(), codes.end(), symbolId) == codes.end())
            {
                data.push_back(symbolId);
            }
        }

        int *encodeMatrix4SubStripe = _ES2encodeMatrixMap[sp];

        // form the pointer for encoding extended sub-stripe
        char **dataPtrs4SubStripe = new char *[as_k];
        for (int i = 0; i < as_k; i++)
        {
            dataPtrs4SubStripe[i] = ptrs[data[i]];
        }

        char **codePtrs4SubStripe = new char *[r * alpha];
        for (int i = 0; i < r; i++)
        {
            codePtrs4SubStripe[i] = ptrs[codes[i]];
        }

        if (ecLib == "ISA-L")
        {
            // for isa-l, first transfer the mat into char*
            unsigned char *encodingMatrixUC = new unsigned char[as_k * r];
            for (int i = 0; i < as_k * r; i++)
            {
                encodingMatrixUC[i] = (unsigned char)encodeMatrix4SubStripe[i];
            }

            unsigned char *itable = new unsigned char[32 * as_k * r];
            ec_init_tables(as_k, r, encodingMatrixUC, itable);
            ec_encode_data(pktSizeBytes, as_k, r, itable, (unsigned char **)dataPtrs4SubStripe, (unsigned char **)codePtrs4SubStripe);
            delete[] itable;
            delete[] encodingMatrixUC;
        }
        else if (ecLib == "Jerasure")
        {
            jerasure_matrix_encode(as_k, r, w, encodeMatrix4SubStripe, dataPtrs4SubStripe, codePtrs4SubStripe, pktSizeBytes);
        }

        delete[] dataPtrs4SubStripe;
        delete[] codePtrs4SubStripe;
    }

    // /**
    //  * @brief method 2: use parity check matrix for encoding
    //  */
    // int numDataPkts = k * alpha;
    // int numCodePkts = r * alpha;
    // if (ecLib == "ISA-L")
    // {
    //     // for isa-l, first transfer the mat into char*
    //     unsigned char *encodingMatrixUC = new unsigned char[numDataPkts * numCodePkts];
    //     for (int i = 0; i < numDataPkts * numCodePkts; i++)
    //     {
    //         encodingMatrixUC[i] = (unsigned char)generatorMatrix[i];
    //     }

    //     unsigned char *itable = new unsigned char[32 * numDataPkts * numCodePkts];
    //     ec_init_tables(numDataPkts, numCodePkts, encodingMatrixUC, itable);
    //     ec_encode_data(pktSizeBytes, numDataPkts, numCodePkts, itable, (unsigned char **)dataPtrs, (unsigned char **)codePtrs);
    //     delete[] itable;
    //     delete[] encodingMatrixUC;
    // }
    // else if (ecLib == "Jerasure")
    // {
    //     jerasure_matrix_encode(numDataPkts, numCodePkts, w, generatorMatrix, dataPtrs, codePtrs, pktSizeBytes);
    // }

    return true;
}

bool LESS::decodeData(vector<int> failedBlockIds, char **failedPtrs, char **availPtrs, int pktSizeBytes, string ecLib)
{
    if (failedBlockIds.size() == 1)
    {
        return decodeSingle(failedBlockIds, failedPtrs, availPtrs, pktSizeBytes, ecLib);
    }
    else
    {
        return decodeMultiple(failedBlockIds, failedPtrs, availPtrs, pktSizeBytes, ecLib);
    }
}

bool LESS::decodeSingle(vector<int> failedBlockIds, char **failedPtrs, char **availPtrs, int pktSizeBytes, string ecLib)
{
    return decodeMultipleWithSubStripes(failedBlockIds, failedPtrs, availPtrs, pktSizeBytes, ecLib);
}

bool LESS::decodeMultiple(vector<int> failedBlockIds, char **failedPtrs, char **availPtrs, int pktSizeBytes, string ecLib)
{
    bool canDecodeWithSubStripes = false;

    // identify failed groups
    vector<int> failedGroups;
    for (auto nodeId : failedBlockIds)
    {
        for (int i = 0; i < numGroups; i++)
        {
            if (find(_nodeGroups[i].begin(), _nodeGroups[i].end(), nodeId) != _nodeGroups[i].end())
            {
                if (find(failedGroups.begin(), failedGroups.end(), i) == failedGroups.end())
                {
                    failedGroups.push_back(i);
                }
                break;
            }
        }
    }

    // print failed blocks
    cout << "LESS::decodeMultiple() failed blocks: ";
    for (auto blockId : failedBlockIds)
    {
        cout << blockId << " ";
    }
    cout << endl;

    // print failed groups
    cout << "LESS::decodeMultiple() failed groups: ";
    for (auto groupId : failedGroups)
    {
        cout << groupId << " ";
    }
    cout << endl;

    // can tolerate at most t = _m / _w failures within one group
    if (failedGroups.size() == 1 && failedBlockIds.size() <= (r / alpha))
    {
        canDecodeWithSubStripes = true;
    }

    if (canDecodeWithSubStripes == true)
    {
        return decodeMultipleWithSubStripes(failedBlockIds, failedPtrs, availPtrs, pktSizeBytes, ecLib);
    }
    else
    {
        return decodeMultipleWithPCMatrix(failedBlockIds, failedPtrs, availPtrs, pktSizeBytes, ecLib);
    }
}

bool LESS::decodeMultipleWithSubStripes(vector<int> failedBlockIds, char **failedPtrs, char **availPtrs, int pktSizeBytes, string ecLib)
{
    // use extended sub-stripes for multiple failures repair
    cout << "LESS::decodeMultipleWithSubStripes() repair using extended sub-stripes" << endl;

    char **ptrs = new char *[n * alpha];
    int failedPtrCnt = 0;
    int availPtrCnt = 0;
    for (int i = 0; i < n * alpha; i++)
    {
        int blockId = i / alpha;
        if (find(failedBlockIds.begin(), failedBlockIds.end(), blockId) != failedBlockIds.end())
        {
            ptrs[i] = failedPtrs[failedPtrCnt++];
        }
        else
        {
            ptrs[i] = availPtrs[availPtrCnt++];
        }
    }

    int residingGroupId = -1;
    for (int i = 0; i < numGroups; i++)
    {
        if (find(_nodeGroups[i].begin(), _nodeGroups[i].end(), failedBlockIds[0]) != _nodeGroups[i].end())
        {
            residingGroupId = i;
            break;
        }
    }

    // obtain available data symbols
    vector<int> data;
    vector<int> codes;
    vector<int> symbolGroup = _symbolGroups[residingGroupId];
    int as_n = symbolGroup.size();
    int as_k = symbolGroup.size() - r;

    // construct decoding matrix for the extended sub-stripe
    vector<int> from(as_n, 0);
    vector<int> to(as_n, 0);

    // sort the nodes in descending order of available sub-packets
    vector<vector<int>> helperNodesSubPkts(n, vector<int>());
    for (int i = 0; i < as_n; i++)
    {
        int symbol = symbolGroup[i];
        int symbolBlockId = symbol / alpha;
        if (find(failedBlockIds.begin(), failedBlockIds.end(), symbolBlockId) != failedBlockIds.end())
        {
            // failed node
            to[i] = 1;
            codes.push_back(symbol);
        }
        else
        {
            // available node
            helperNodesSubPkts[symbolBlockId].push_back(symbol);
        }
    }

    // sort helperNodesSubPkts in descending order of sub-packets
    sort(helperNodesSubPkts.begin(), helperNodesSubPkts.end(), [](const vector<int> &a, const vector<int> &b)
         { return a.size() > b.size(); });

    // choose the first as_k blocks from the sorted list
    for (int i = 0; i < helperNodesSubPkts.size(); i++)
    {
        vector<int> &subPkts = helperNodesSubPkts[i];
        if (subPkts.empty())
        {
            continue;
        }
        for (auto symbol : subPkts)
        {
            if (data.size() < as_k)
            {
                // find the index in the symbolGroup
                auto it = find(symbolGroup.begin(), symbolGroup.end(), symbol);
                int idx = distance(symbolGroup.begin(), it);
                from[idx] = 1;
                data.push_back(symbol);
            }
            else
            {
                break;
            }
        }
    }
    sort(data.begin(), data.end());

    // construct parity check matrix for the extended sub-stripe
    int *pcMatrix4SubStripe = _ES2pcMatrixMap[residingGroupId];

    // print matrix
    cout << "LESS::decodeMultipleWithSubStripes() Parity-check matrix for extended sub-stripe " << residingGroupId << ":" << endl;
    jerasure_print_matrix(pcMatrix4SubStripe, r, as_n, w);

    int *decodeMatrix4SubStripe = new int[as_k * r];
    if (convertPCMatrixToGenMatrix(as_n, as_k, w, from, to, pcMatrix4SubStripe, decodeMatrix4SubStripe) == false)
    {
        cout << "LESS::decodeMultipleWithSubStripes() failed to obtain decoding matrix for extended sub-stripe " << residingGroupId << endl;
        exit(-1);
    }

    // // print encoding matrix for extended sub-stripe
    // cout << "LESS::DecodeSingle() Decoding matrix for extended sub-stripe " << residingGroupId << ":" << endl;
    // jerasure_print_matrix(decodeMatrix4SubStripe, _m, as_k, _fw);

    // used availPtrs
    char **availPtrs4SubStripe = new char *[as_k];
    for (int i = 0; i < as_k; i++)
    {
        availPtrs4SubStripe[i] = ptrs[data[i]];
    }

    int numAvailPkts = as_k;
    int numFailedPkts = failedBlockIds.size() * alpha;
    if (ecLib == "ISA-L")
    {
        // for isa-l, first transfer the mat into char*
        unsigned char *decodingMatrixUC = new unsigned char[numAvailPkts * numFailedPkts];
        for (int i = 0; i < numAvailPkts * numFailedPkts; i++)
        {
            decodingMatrixUC[i] = (unsigned char)decodeMatrix4SubStripe[i];
        }

        unsigned char *itable = new unsigned char[32 * numAvailPkts * numFailedPkts];
        ec_init_tables(numAvailPkts, numFailedPkts, (unsigned char *)decodingMatrixUC, itable);
        ec_encode_data(pktSizeBytes, numAvailPkts, numFailedPkts, itable, (unsigned char **)availPtrs4SubStripe, (unsigned char **)failedPtrs);
        delete[] itable;
        delete[] decodingMatrixUC;
    }
    else if (ecLib == "Jerasure")
    {
        jerasure_matrix_encode(numAvailPkts, numFailedPkts, w, decodeMatrix4SubStripe, availPtrs4SubStripe, failedPtrs, pktSizeBytes);
    }

    delete[] availPtrs4SubStripe;
    delete[] decodeMatrix4SubStripe;
    delete[] ptrs;

    return true;
}

bool LESS::decodeMultipleWithPCMatrix(vector<int> failedBlockIds, char **failedPtrs, char **availPtrs, int pktSizeBytes, string ecLib)
{
    int numFailedPkts = failedBlockIds.size() * alpha;
    // always use k for conventional repair
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

bool LESS::getAvailPrimElements(int n, int k, int alpha, int &w, uint32_t &e, uint32_t &f)
{
    // no available primitive elements: k <= 1; n - k == 1
    if (k <= 1 || n <= k + 1)
    {
        f = 0;
        w = 0;
        e = 0;
        return false;
    }

    // reset w, f, e
    w = 0;
    f = 0;
    e = 0;

    if (n - k == 2)
    {
        if (n * alpha <= ((1 << 8) - 1))
        {
            w = 8;
            f = 0b100011101; // x^8 + x^4 + x^3 + x^2 + 1
            e = 2;
        }
        else if (n * alpha <= ((1 << 16) - 1))
        {
            w = 16;
            f = 0b10000000000101101; // x^{16} + x^5 + x^3 + x^2 + 1
            e = 2;
        }
    }
    else if (n - k == 3)
    {
        if (alpha == 2)
        {
            if (n <= 44)
            {
                f = 0b111001111; // x^8 + x^7 + x^6 + x^3 + x^2 + x + 1
                w = 8;
            }
            else if (n <= 127)
            {
                f = 0b10000000000101101; // x^{16} + x^5 + x^3 + x^2 + 1
                w = 16;
            }
        }
        else if (alpha == 3)
        {
            if (n <= 40)
            {
                f = 0b100101101; // x^8 + x^5 + x^3 + x^2 + 1
                w = 8;
            }
            else if (n <= 127)
            {
                f = 0b10000000000101101; // x^{16} + x^5 + x^3 + x^2 + 1
                w = 16;
            }
        }
    }
    else if (n - k == 4)
    {
        if (alpha == 2)
        {
            if (n <= 23)
            {
                f = 0b100101011; // x^8 + x^5 + x^3 + x + 1
                w = 8;
            }
            else if (n <= 127)
            {
                f = 0b10000001101110001; // x^{16} + x^9 + x^8 + x^6 + x^5 + x^4 + 1
                w = 16;
            }
        }
        else if (alpha == 3)
        {
            if (n <= 17)
            {
                f = 0b100011101; // x^8 + x^4 + x^3 + x^2 + 1
                w = 8;
            }
            else if (n <= 127)
            {
                f = 0b10001001011110011; // x^{16} + x^{12} + x^9 + x^7 + x^6 + x^5 + x^4 + x + 1
                w = 16;
            }
        }
        else if (alpha == 4)
        {
            if (n <= 16)
            {
                f = 0b100101101; // x^8 + x^5 + x^3 + x^2 + 1
                w = 8;
            }
            else if (n <= 127)
            {
                f = 0b10000011101111001; // x^{16} + x^{10} + x^9 + x^8 + x^6 + x^5 + x^4 + x^3 + 1
                w = 16;
            }
        }
    }
    if (f)
    {
        e = findRoot(f, w);
        // printf("r = %d, w = %d:\n   n = %d\n    w = %2d, f(x) = ", r, w, n, w);
        // print_polynomial(f);
        // printf("    available element: %d\n\n", e);
    }

    return e != 0;
}

uint32_t LESS::findRoot(uint32_t f, int w)
{
    for (uint32_t root = 1;; root++)
    {
        if (root == 0 || root > ((1 << w) - 1))
        {
            return 0;
        }
        if (polynomialAssignment(root, f, w) != 0)
        {
            continue;
        }
        return root;
    }
}

uint32_t LESS::polynomialAssignment(uint32_t x, uint32_t f, int w)
{
    uint32_t fx = 0;
    for (int i = 31; i >= 0; i--)
    {
        fx = galois_single_multiply(fx, x, w);
        // fx *= x;
        // fx = gf_mult(fx, x, w);
        fx ^= ((f >> i) & (1));
    }
    return fx;
}

void LESS::getPrimitiveElementPower()
{
    primitiveElementPower.resize(order);
    primitiveElementPower[0] = 1;
    for (int i = 1; i < order; i++)
    {
        primitiveElementPower[i] = galois_single_multiply(primitiveElementPower[i - 1], e, w);
    }
}

void LESS::initParityCheckMatrix()
{
    // int rows = r * alpha;
    // int cols = n * alpha;

    // int *pcMatrixWithoutPermutate = new int[rows * cols];
    // memset(pcMatrixWithoutPermutate, 0, rows * cols * sizeof(int));

    // for (int x = 0; x < rows; x++)
    // {
    //     for (int y = 0; y < cols; y++)
    //     {
    //         // int a = y / alpha / g;
    //         int b = y / alpha % numGroups; // group id
    //         int j = y % alpha; // the j-th sub-mtx in col
    //         int i = x / r; // the i-th sub-mtx in row
    //         int t = x % r;
    //         if (i == j || i == b)
    //         { // diagonal or
    //             pcMatrixWithoutPermutate[x * cols + y] = primitiveElementPower[(y * t) % order];
    //         }
    //     }
    // }

    // // print matrix
    // Util::printGFMatrix(pcMatrixWithoutPermutate, rows, cols, w);

    // // my implementation

    // int *pcMatrixWithoutPermutate2 = new int[rows * cols];
    // memset(pcMatrixWithoutPermutate2, 0, rows * cols * sizeof(int));

    // // H_i of H
    // for (int i = 0; i < n; i++) {
    //     // z-th row of H_i
    //     int g_i = i % numGroups; // the group id
    //     for (int z = 0; z < alpha; z++) {
    //         // j-th col of H_i
    //         for (int j = 0; j < alpha; j++) {
    //             if (z == j || z == g_i) {
    //                 // assign as column vector v_ij
    //                 int order_idx = (i * alpha + j) % order;
    //                 for (int bidx = 0; bidx < r; bidx++) {
    //                     int ridx = z * r + bidx;
    //                     int cidx = i * alpha + j;
    //                     pcMatrixWithoutPermutate2[ridx * n * alpha + cidx] = primitiveElementPower[(order_idx * bidx) % order];
    //                 }
    //             }
    //         }
    //     }
    // }

    // // print matrix
    // Util::printGFMatrix(pcMatrixWithoutPermutate2, rows, cols, w);

    int s = n / numGroups; // minimum number of elements in a group
    int t = n % numGroups; // number of items in the last group

    // H_i of H
    for (int i = 0; i < n; i++)
    {
        int g_i = -1; // the group id
        if (i < t * (s + 1))
        {
            g_i = i / (s + 1);
        }
        else
        {
            g_i = (i - t * (s + 1)) / s + t;
        }
        int h_i = -1; // the i-th sub-mtx in row
        if (i < t * (s + 1))
        {
            h_i = i % (s + 1);
        }
        else
        {
            h_i = (i - t * (s + 1)) % s;
        }
        // z-th row of H_i
        for (int z = 0; z < alpha; z++)
        {
            // j-th col of H_i
            for (int j = 0; j < alpha; j++)
            {
                if (z == j || z == g_i)
                {
                    // assign as column vector v_ij
                    int order_idx = ((h_i * (alpha + 1) + g_i) * alpha + j) % order;
                    for (int bidx = 0; bidx < r; bidx++)
                    {
                        int ridx = z * r + bidx;
                        int cidx = i * alpha + j;
                        parityCheckMatrix[ridx * n * alpha + cidx] = primitiveElementPower[(order_idx * bidx) % order];
                    }
                }
            }
        }
    }

    // // print matrix
    // printf("Parity Check Matrix of LESS(%d, %d, %d):\n", n, k, alpha);
    // Util::printGFMatrix(parityCheckMatrix, rows, cols, w);
}