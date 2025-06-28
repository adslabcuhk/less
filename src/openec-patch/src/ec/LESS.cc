#include "LESS.hh"

LESS::LESS(int n, int k, int w, int opt, vector<string> param)
{
    _n = n;
    _k = k;
    _w = w; // sub-packetization
    _opt = opt;
    _m = n - k;
    _numGroups = _w + 1; // num_groups = sub-packetization + 1

    // init ECDAG symbols layout
    initLayout();

    // obtain primitive elements (assign fw, e, f)
    if (getAvailPrimElements(_n, _k, _w, _fw, _e, _f) == false)
    {
        cout << "LESS::LESS() failed to find primitive element" << endl;
        exit(1);
    }

    if (_e == 0)
    {
        cout << "LESS::LESS() failed to find available primitive elements in GF(2^" << _fw << ")" << endl;
        exit(-1);
    }

    // obtain primitive elements power
    _order = (1 << _fw) - 1;
    getPrimElementsPower(_order, _e, _fw);

    // element map (for debugging)
    for (int i = 0; i < 255; i++)
    {
        _elementMap[_primElementPower[i]] = i;
    }

    // print primitive element
    // cout << "LESS::LESS() Primitive element (root): " << _e << endl;

    // // print primElementPower
    // cout << "LESS::LESS() Primitive elements power:" << endl;
    // for (int i = 0; i < _order; i++) {
    //     cout << _primElementPower[i] << " ";
    // }
    // cout << endl;

    /**
     * @brief Construct extended sub-stripes for encoding and decoding
     */

    // Step 1: assign nodes to node groups
    _nodeGroups = vector<vector<int>>(_numGroups, vector<int>());
    int minNumGroupElements = _n / _numGroups;
    int numMaxGroups = n % (_numGroups);

    for (int groupId = 0, nodeId = 0; groupId < _numGroups; groupId++)
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
    // for (int groupId = 0; groupId < _numGroups; groupId++) {
    //     cout << "Group " << groupId << ": ";
    //     for (auto nodeId : _nodeGroups[groupId]) {
    //         cout << nodeId << " ";
    //     }
    //     cout << endl;
    // }

    // Step 2: assign symbols to symbol groups
    _symbolGroups = vector<vector<int>>(_numGroups, vector<int>());

    // handle the first _numGroups - 1 symbol groups
    for (int groupId = 0; groupId < _numGroups - 1; groupId++)
    {
        auto &nodeGroup = _nodeGroups[groupId];
        // add symbols from the same node group
        for (auto nodeId : nodeGroup)
        {
            for (int alpha = 0; alpha < _w; alpha++)
            {
                _symbolGroups[groupId].push_back(_layout[alpha][nodeId]);
            }
        }
        // add symbols from the <groupId>-th sub-stripe
        for (int nodeId = 0; nodeId < _n; nodeId++)
        {
            // exclude the nodes from the same node group
            if (find(nodeGroup.begin(), nodeGroup.end(), nodeId) == nodeGroup.end())
            {
                _symbolGroups[groupId].push_back(_layout[groupId][nodeId]);
            }
        }
    }

    // assign the last symbol group
    auto &lastNodeGroup = _nodeGroups[_numGroups - 1];

    // put all symbols diagonally
    for (int groupId = 0; groupId < _numGroups - 1; groupId++)
    {
        for (auto nodeId : _nodeGroups[groupId])
        {
            _symbolGroups[_numGroups - 1].push_back(_layout[groupId][nodeId]);
        }
    }

    // put all symbols from the last node group
    for (auto nodeId : lastNodeGroup)
    {
        for (int alpha = 0; alpha < _w; alpha++)
        {
            _symbolGroups[_numGroups - 1].push_back(_layout[alpha][nodeId]);
        }
    }

    // sort the symbol groups
    for (int groupId = 0; groupId < _numGroups; groupId++)
    {
        sort(_symbolGroups[groupId].begin(), _symbolGroups[groupId].end());
    }

    // // print out the symbol groups
    // cout << "LESS::LESS() Symbol groups:" << endl;
    // for (int groupId = 0; groupId < _numGroups; groupId++) {
    //     cout << "Group " << groupId << ": ";
    //     for (auto symbolId : _symbolGroups[groupId]) {
    //         cout << symbolId << " ";
    //     }
    //     cout << endl;
    // }

    // Step 3: assign coefficients for each symbol for constructing parity
    // check matrix

    // generate node permutation (in vertical order)
    vector<vector<int>> nodeReordering(_numGroups, vector<int>());
    for (int nodeId = 0, rid = 0; nodeId < _n; nodeId++)
    {
        nodeReordering[rid].push_back(nodeId);
        rid = (rid + 1) % _numGroups;
    }
    for (int groupId = 0; groupId < _numGroups; groupId++)
    {
        for (auto nodeId : nodeReordering[groupId])
        {
            _nodePermutation.push_back(nodeId);
        }
    }

    // // print node permutation
    // cout << "LESS::LESS() Node permutation:" << endl;
    // for (auto nodeId : _nodePermutation) {
    //     cout << nodeId << " ";
    // }
    // cout << endl;

    // assign coefficients to each symbols: (w * n)
    _coefs4Symbols = vector<vector<int>>(_w, vector<int>(_n, 0));

    for (int nodeId = 0; nodeId < _n; nodeId++)
    {
        for (int alpha = 0; alpha < _w; alpha++)
        {
            _coefs4Symbols[alpha][nodeId] = _primElementPower[_nodePermutation[nodeId] * _w + alpha];
        }
    }

    // // print out the coefficients
    // cout << "LESS::LESS() Coefficients for encoding:" << endl;
    // for (int i = 0; i < _w; i++) {
    //     for (int j = 0; j < _n; j++) {
    //         cout << _coefs4Symbols[i][j] << " ";
    //     }
    //     cout << endl;
    // }

    // cout << "symbol element correspondence:" << endl;
    // for (int groupId = 0; groupId < _numGroups; groupId++) {
    //     cout << "Group " << groupId << ": ";
    //     for (auto symbolId : _symbolGroups[groupId]) {
    //         int nodeId = symbolId / _w;
    //         int alpha = symbolId % _w;
    //         cout << _elementMap[_coefs4Symbols[alpha][nodeId]] << " ";
    //     }
    //     cout << endl;
    // }

    // // print out the coefficients
    // cout << "LESS::LESS() Coefficients for encoding (corresponding):" << endl;
    // for (int i = 0; i < _w; i++) {
    //     for (int j = 0; j < _n; j++) {
    //         cout << _elementMap[_coefs4Symbols[i][j]] << " ";
    //     }
    //     cout << endl;
    // }

    // Step 4: construct parity check matrix for each extended sub-stripe
    // we also construct generator matrix for the first alpha extended sub-stripes
    for (int esId = 0; esId < _numGroups; esId++)
    {
        vector<int> &symbolGroup = _symbolGroups[esId];

        int as_n = symbolGroup.size();
        int as_k = as_n - _m;

        // obtain coefficients for each symbol for encoding
        vector<int> coefs4Encoding(as_n, 0);
        for (int i = 0; i < as_n; i++)
        {
            int symbolNodeId = symbolGroup[i] / _w;
            int symbolAlpha = symbolGroup[i] % _w;
            coefs4Encoding[i] = _coefs4Symbols[symbolAlpha][symbolNodeId];
        }

        // construct parity check matrix for the extended sub-stripe
        int *pcMatrix4SubStripe = new int[_m * as_n];
        for (int rid = 0; rid < _m; rid++)
        {
            for (int cid = 0; cid < as_n; cid++)
            {
                if (rid == 0)
                {
                    pcMatrix4SubStripe[rid * as_n + cid] = 1;
                }
                else
                {
                    pcMatrix4SubStripe[rid * as_n + cid] = galois_single_multiply(pcMatrix4SubStripe[(rid - 1) * as_n + cid], coefs4Encoding[cid], _fw);
                }
            }
        }
        _ES2pcMatrixMap[esId] = pcMatrix4SubStripe;

        // print parity check matrix
        cout << "LESS::LESS() Parity-check matrix for extended sub-stripe " << esId << ":" << endl;
        jerasure_print_matrix(_ES2pcMatrixMap[esId], _m, as_n, _fw);

        if (esId < _w)
        {
            // obtain encoding matrix for the extended sub-stripe
            int *from = new int[as_n];
            int *to = new int[as_n];
            memset(from, 0, as_n * sizeof(int));
            memset(to, 0, as_n * sizeof(int));

            // codes: the esId-th sub-blocks of the _m parity blocks
            vector<int> codes;
            for (int nodeId = _k; nodeId < _n; nodeId++)
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

            int *encodeMatrix4SubStripe = new int[as_k * _m];
            if (getGenMatrixFromPCMatrix(as_n, as_k, _fw, pcMatrix4SubStripe, encodeMatrix4SubStripe, from, to) == false)
            {
                cout << "LESS::LESS() failed to obtain encoding matrix for extended sub-stripe " << esId << endl;
                exit(-1);
            }
            _ES2encodeMatrixMap[esId] = encodeMatrix4SubStripe;

            delete[] from;
            delete[] to;

            // // print encoding matrix for extended sub-stripe
            // cout << "LESS::LESS() Encoding matrix for extended sub-stripe " << esId << ":" << endl;
            // jerasure_print_matrix(_ES2encodeMatrixMap[esId], _m, as_k, _fw);
        }
    }

    /**
     * @brief Construct generator matrix for the code
     */
    genEncodingMatrix();

    cout << "LESS::LESS() Parity-check matrix:" << endl;
    jerasure_print_matrix(_pcMatrix, _m * _w, _n * _w, 8);

    // cout << "LESS::LESS() Parity-check matrix (corresponding):" << endl;
    // for (int i = 0; i < _m * _w; i++) {
    //     if (i % _m != 1) {
    //         continue;
    //     }
    //     for (int j = 0; j < _n * _w; j++) {
    //         int a = _pcMatrix[i * (_n * _w) + j];
    //         if (a == 0)
    //         {
    //             printf("--- ");
    //         }
    //         else
    //             printf("%03d ", _elementMap[a]);
    //     }
    //     cout << endl;
    // }

    // cout << "LESS::LESS() Generator matrix:" << endl;
    // jerasure_print_matrix(_encodeMatrix, _m * _w, _k * _w, 8);
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

    // delete parity check matrix and encoding matrix
    delete[] _pcMatrix;
    delete[] _encodeMatrix;
}

ECDAG *LESS::Encode()
{
    ECDAG *ecdag = new ECDAG();

    /**
     * @brief method 1: use extended sub-stripes for encoding
     */
    for (int alpha = 0; alpha < _w; alpha++)
    {
        vector<int> &symbolGroup = _symbolGroups[alpha];

        // construct RS(as_n, as_k) for the extended sub-stripe
        int as_n = symbolGroup.size();
        int as_k = as_n - _m;

        vector<int> data;
        vector<int> codes;

        // codes: the esId-th sub-blocks of the _m parity blocks
        for (int nodeId = _k; nodeId < _n; nodeId++)
        {
            codes.push_back(_layout[alpha][nodeId]);
        }

        for (auto symbolId : symbolGroup)
        {
            if (find(codes.begin(), codes.end(), symbolId) == codes.end())
            {
                data.push_back(symbolId);
            }
        }

        int *encodeMatrix4SubStripe = _ES2encodeMatrixMap[alpha];

        // encode the extended sub-stripe
        for (int i = 0; i < codes.size(); i++)
        {
            int code = codes[i];
            vector<int> coef(encodeMatrix4SubStripe + i * as_k, encodeMatrix4SubStripe + (i + 1) * as_k);
            ecdag->Join(code, data, coef);
        }
    }

    /**
     * @brief method 2: use parity check matrix for encoding
     */
    // vector<int> data;
    // vector<int> codes;

    // // data: (k * w)
    // for (int nodeId = 0; nodeId < _k; nodeId++) {
    //     for (int alpha = 0; alpha < _w; alpha++) {
    //         data.push_back(_layout[alpha][nodeId]);
    //     }
    // }

    // for (int nodeId = _k; nodeId < _n; nodeId++) {
    //     for (int alpha = 0; alpha < _w; alpha++) {
    //         codes.push_back(_layout[alpha][nodeId]);
    //     }
    // }

    // for (int i = 0; i < codes.size(); i++) {
    //     int code = codes[i];
    //     vector<int> coef(_encodeMatrix + i * _k * _w, _encodeMatrix + (i + 1) * _k * _w);
    //     ecdag->Join(code, data, coef);
    // }

    // if (codes.size() > 1) {
    //     int vidx = ecdag->BindX(codes);
    //     ecdag->BindY(vidx, data[0]);
    // }

    return ecdag;
}

ECDAG *LESS::Decode(vector<int> from, vector<int> to)
{
    if (to.size() == _w)
    {
        int failedNodeId = to[0] / _w;
        return decodeSingle(from, to);
    }
    else
    {
        return decodeMultiple(from, to);
    }
}

void LESS::Place(vector<vector<int>> &group)
{
}

void LESS::genParityCheckMatrix()
{
    // int rows = _m * _w;
    // int cols = _n * _w;

    // _pcMatrix = new int[rows * cols];
    // memset(_pcMatrix, 0, rows * cols * sizeof(int));

    // int *pcMatrixWithoutPermutate = new int[rows * cols];
    // memset(pcMatrixWithoutPermutate, 0, rows * cols * sizeof(int));

    // for (int x = 0; x < rows; x++)
    // {
    //     for (int y = 0; y < cols; y++)
    //     {
    //         // int a = y / _w / g;
    //         int b = y / _w % _numGroups; // group id
    //         int j = y % _w; // the j-th sub-mtx in col
    //         int i = x / _m; // the i-th sub-mtx in row
    //         int t = x % _m;
    //         if (i == j || i == b)
    //         { // diagonal or
    //             pcMatrixWithoutPermutate[x * cols + y] = _primElementPower[(y * t) % _order];
    //         }
    //     }
    // }

    // // // print pcMatrixWithoutPermutate
    // // cout << "LESS::genParityCheckMatrix() Parity-check matrix without permutation:" << endl;
    // // jerasure_print_matrix(pcMatrixWithoutPermutate, rows, cols, _fw);

    // // convert the parity check matrix based on node permutation
    // for (int nodeId = 0; nodeId < _n; nodeId++) {
    //     int permNodeId = _nodePermutation[nodeId];
    //     for (int alpha = 0; alpha < _w; alpha++) {
    //         for (int rid = 0; rid < rows; rid++) {
    //             _pcMatrix[rid * cols + nodeId * _w + alpha] = pcMatrixWithoutPermutate[rid * cols + permNodeId * _w + alpha];
    //         }
    //     }
    // }

    // // print pcmatrix after permute
    // cout << "LESS::genParityCheckMatrix() Parity-check matrix after permutation:" << endl;
    // jerasure_print_matrix(_pcMatrix, rows, cols, _fw);

    // construct parity check matrix
    _pcMatrix = new int[_m * _w * _n * _w];
    memset(_pcMatrix, 0, _m * _w * _n * _w * sizeof(int));

    int s = _n / _numGroups; // minimum number of elements in a group
    int t = _n % _numGroups; // number of items in the last group

    // H_i of H
    for (int i = 0; i < _n; i++)
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
        for (int z = 0; z < _w; z++)
        {
            // j-th col of H_i
            for (int j = 0; j < _w; j++)
            {
                if (z == j || z == g_i)
                {
                    // assign as column vector v_ij
                    int order_idx = ((h_i * (_w + 1) + g_i) * _w + j) % _order;
                    for (int bidx = 0; bidx < _m; bidx++)
                    {
                        int ridx = z * _m + bidx;
                        int cidx = i * _w + j;
                        _pcMatrix[ridx * _n * _w + cidx] = _primElementPower[(order_idx * bidx) % _order];
                    }
                }
            }
        }
    }

    // // print parity check matrix
    // cout << "LESS::genParityCheckMatrix() Parity-check matrix:" << endl;
    // jerasure_print_matrix(_pcMatrix, _m * _w, _n * _w, _fw);
}

void LESS::genEncodingMatrix()
{
    genParityCheckMatrix();
    if (convertPCMatrix2EncMatrix(_n, _k, _w) == false)
    {
        cout << "LESS::genEncodingMatrix() failed to obtain encoding matrix" << endl;
        exit(-1);
    }
}

bool LESS::convertPCMatrix2EncMatrix(int n, int k, int w)
{
    int *from = new int[n * w];
    int *to = new int[n * w];
    memset(from, 0, n * w * sizeof(int));
    memset(to, 0, n * w * sizeof(int));

    // the first k*w symbols are data symbols, next (n-k)*w symbols are parity
    // symbols
    for (int nodeId = 0; nodeId < k; nodeId++)
    {
        for (int alpha = 0; alpha < w; alpha++)
        {
            from[nodeId * w + alpha] = 1;
        }
    }

    for (int nodeId = k; nodeId < n; nodeId++)
    {
        for (int alpha = 0; alpha < w; alpha++)
        {
            to[nodeId * w + alpha] = 1;
        }
    }

    _encodeMatrix = new int[k * w * n * w];
    bool ret = getGenMatrixFromPCMatrix(n * w, k * w, _fw, _pcMatrix, _encodeMatrix, from, to);

    delete[] from;
    delete[] to;

    return ret;
}

bool LESS::getGenMatrixFromPCMatrix(int n, int k, int fw, const int *pcMatrix, int *genMatrix, const int *from, const int *to)
{
    int m = n - k;
    int numFailedSymbols = 0;
    int numAvailSymbols = 0;
    for (int i = 0; i < n; i++)
    {
        if (to[i])
        {
            numFailedSymbols++;
        }
        if (from[i])
        {
            numAvailSymbols++;
        }
        if (to[i] == 1 && from[i] == 1)
        {
            cout << "LESS::getGenMatrixFromPCMatrix() numFailedSymbols node " << i << " cannot be used for decoding" << endl;
            return false;
        }
    }
    if (numFailedSymbols > m)
    {
        cout << "LESS::getGenMatrixFromPCMatrix() Too many failed symbols to decode: " << numFailedSymbols << ", max allowed: " << m << endl;
        return false;
    }
    if (numAvailSymbols != k)
    {
        cout << "LESS::getGenMatrixFromPCMatrix() Invalid number of available symbols: " << numAvailSymbols << ", k: " << k << std::endl;
        return false;
    }

    int *H1 = new int[m * m]; // number of failed nodes
    int *H2 = new int[m * k]; // number of helper nodes
    int *H1_inverse = new int[m * m];

    // copy the matrix to corresponding sub-matrix
    for (int j = 0, j1 = 0, j2 = 0; j < n; j++)
    {
        if (!from[j])
        {
            for (int i = 0; i < m; i++)
            {
                H1[i * m + j1] = pcMatrix[i * n + j];
            }
            j1++;
        }
        else
        {
            for (int i = 0; i < m; i++)
            {
                H2[i * k + j2] = pcMatrix[i * n + j];
            }
            j2++;
        }
    }

    // inverse H1
    if (jerasure_invert_matrix(H1, H1_inverse, m, fw) == -1)
    {
        cout << "LESS::getGenMatrixFromPCMatrix() failed to invert H1" << endl;
        exit(-1);
    }

    // multiply H1_inverse and H2
    int *tmpMatrix = jerasure_matrix_multiply(H1_inverse, H2, m, m, m, k, fw);
    delete[] H1;
    delete[] H2;
    delete[] H1_inverse;

    // tmpMatrix has exactly m rows
    if (numFailedSymbols == m)
    {
        memcpy(genMatrix, tmpMatrix, m * k * sizeof(int));
    }
    else
    {
        // only needs numFailedSymbols < m rows

        for (int x = 0, y = 0, z = 0; x < n; x++)
        {
            if (!from[x])
            {
                if (to[x])
                {
                    memcpy(genMatrix + z * k, tmpMatrix + y * k, k * sizeof(int));
                    z++;
                }
                y++;
            }
        }
    }

    free(tmpMatrix);
    return true;
}

bool LESS::genDecodingMatrix(vector<int> &availSymbols, vector<int> &failedSymbols, int *decodeMatrix)
{
    int *from = new int[_n * _w];
    int *to = new int[_n * _w];
    memset(to, 0, _n * _w * sizeof(int));

    for (int symbolId = 0; symbolId < _n * _w; symbolId++)
    {
        from[symbolId] = 1;
    }

    // mark failed / available symbols
    for (auto failedSymbol : failedSymbols)
    {
        to[failedSymbol] = 1;
        from[failedSymbol] = 0;
    }

    // only needs to retrieve the first _k * _w available symbols
    int numRedundantSymbols = _m * _w - failedSymbols.size();
    for (int i = _n * _w - 1; (i >= 0) && (numRedundantSymbols > 0); i--)
    {
        if (from[i] == 1 && to[i] == 0)
        {
            from[i] = 0;
            numRedundantSymbols--;

            // in place update availSymbols
            availSymbols.erase(std::remove(availSymbols.begin(), availSymbols.end(), i), availSymbols.end());
        }
    }

    // // print from and to
    // cout << "LESS::genDecodingMatrix() from:" << endl;
    // for (int i = 0; i < _n * _w; i++) {
    //     cout << from[i] << " ";
    // }
    // cout << endl;

    // cout << "LESS::genDecodingMatrix() to:" << endl;
    // for (int i = 0; i < _n * _w; i++) {
    //     cout << to[i] << " ";
    // }
    // cout << endl;

    // cout << "LESS::LESS() Parity-check matrix (corresponding):" << endl;
    // for (int i = 0; i < _m * _w; i++) {
    //     if (i % _m != 1) {
    //         continue;
    //     }
    //     for (int j = 0; j < _n * _w; j++) {
    //         int a = _pcMatrix[i * (_n * _w) + j];
    //         if (a == 0)
    //         {
    //             printf("--- ");
    //         }
    //         else
    //             printf("%03d ", _elementMap[a]);
    //     }
    //     cout << endl;
    // }

    bool ret = getGenMatrixFromPCMatrix(_n * _w, _k * _w, _fw, _pcMatrix, decodeMatrix, from, to);

    delete[] from;
    delete[] to;

    return ret;
}

ECDAG *LESS::decodeSingle(vector<int> &availSymbols, vector<int> &failedSymbols)
{
    // use extended sub-stripes for single failure repair
    return decodeMultipleWithSubStripes(availSymbols, failedSymbols);
}

ECDAG *LESS::decodeMultiple(vector<int> &availSymbols, vector<int> &failedSymbols)
{
    bool canDecodeWithSubStripes = false;

    // identify failed nodes
    vector<int> failedNodes;
    for (auto symbolId : failedSymbols)
    {
        int nodeId = symbolId / _w;
        if (find(failedNodes.begin(), failedNodes.end(), nodeId) == failedNodes.end())
        {
            failedNodes.push_back(nodeId);
        }
    }

    // identify failed groups
    vector<int> failedGroups;
    for (auto nodeId : failedNodes)
    {
        for (int i = 0; i < _numGroups; i++)
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

    // print failed nodes
    cout << "LESS::decodeMultiple() failed nodes: ";
    for (auto nodeId : failedNodes)
    {
        cout << nodeId << " ";
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
    if (failedGroups.size() == 1 && failedNodes.size() <= (_m / _w))
    {
        canDecodeWithSubStripes = true;
    }

    if (canDecodeWithSubStripes == true)
    {
        return decodeMultipleWithSubStripes(availSymbols, failedSymbols);
    }
    else
    {
        return decodeMultipleWithPCMatrix(availSymbols, failedSymbols);
    }
}

ECDAG *LESS::decodeMultipleWithSubStripes(vector<int> &availSymbols, vector<int> &failedSymbols)
{
    ECDAG *ecdag = new ECDAG();

    // use extended sub-stripes for multiple failures repair
    cout << "LESS::decodeMultipleWithSubStripes() repair using extended sub-stripes" << endl;

    // identify failed nodes
    vector<int> failedNodes;
    for (auto symbolId : failedSymbols)
    {
        int nodeId = symbolId / _w;
        if (find(failedNodes.begin(), failedNodes.end(), nodeId) == failedNodes.end())
        {
            failedNodes.push_back(nodeId);
        }
    }
    int residingGroupId = -1;
    for (int i = 0; i < _numGroups; i++)
    {
        if (find(_nodeGroups[i].begin(), _nodeGroups[i].end(), failedNodes[0]) != _nodeGroups[i].end())
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
    int as_k = symbolGroup.size() - _m;

    // construct decoding matrix for the extended sub-stripe
    int *from = new int[as_n];
    int *to = new int[as_n];
    memset(from, 0, as_n * sizeof(int));
    memset(to, 0, as_n * sizeof(int));

    // // arbitrary select as_k blocks for help
    // for (int i = 0, count = 0; i < as_n; i++)
    // {
    //     int symbol = symbolGroup[i];
    //     int symbolNodeId = symbol / _w;
    //     if (find(failedNodes.begin(), failedNodes.end(), symbolNodeId) != failedNodes.end())
    //     {
    //         // failed node
    //         to[i] = 1;
    //         codes.push_back(symbol);
    //     }
    //     else
    //     {
    //         if (count < as_k)
    //         {
    //             // available node
    //             from[i] = 1;
    //             data.push_back(symbol);
    //             count++;
    //         }
    //     }
    // }

    // sort the nodes in descending order of available sub-packets
    vector<vector<int>> helperNodesSubPkts(_n, vector<int>());
    for (int i = 0; i < as_n; i++)
    {
        int symbol = symbolGroup[i];
        int symbolNodeId = symbol / _w;
        if (find(failedNodes.begin(), failedNodes.end(), symbolNodeId) != failedNodes.end())
        {
            // failed node
            to[i] = 1;
            codes.push_back(symbol);
        }
        else
        {
            // available node
            helperNodesSubPkts[symbolNodeId].push_back(symbol);
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
    jerasure_print_matrix(pcMatrix4SubStripe, _m, as_n, _fw);

    int *decodeMatrix4SubStripe = new int[as_k * _m];
    if (getGenMatrixFromPCMatrix(as_n, as_k, _fw, pcMatrix4SubStripe, decodeMatrix4SubStripe, from, to) == false)
    {
        cout << "LESS::decodeMultipleWithSubStripes() failed to obtain decoding matrix for extended sub-stripe " << residingGroupId << endl;
        exit(-1);
    }

    // // print encoding matrix for extended sub-stripe
    // cout << "LESS::DecodeSingle() Decoding matrix for extended sub-stripe " << residingGroupId << ":" << endl;
    // jerasure_print_matrix(decodeMatrix4SubStripe, _m, as_k, _fw);

    // encode the extended sub-stripe
    for (int i = 0; i < codes.size(); i++)
    {
        int code = codes[i];
        vector<int> coef(decodeMatrix4SubStripe + i * as_k, decodeMatrix4SubStripe + (i + 1) * as_k);
        ecdag->Join(code, data, coef);
    }
    // int vidx = ecdag->BindX(codes);
    // ecdag->BindY(vidx, data[0]);

    ecdag->BindX(codes);

    delete[] from;
    delete[] to;
    delete[] decodeMatrix4SubStripe;

    return ecdag;
}

ECDAG *LESS::decodeMultipleWithPCMatrix(vector<int> &availSymbols, vector<int> &failedSymbols)
{
    ECDAG *ecdag = new ECDAG();

    int *decodeMatrix = new int[_m * _w * _k * _w];
    if (genDecodingMatrix(availSymbols, failedSymbols, decodeMatrix) == false)
    {
        cout << "LESS::decodeMultipleWithPCMatrix() failed to generate decode matrix" << endl;
        return ecdag;
    }

    // print decode matrix
    cout << "LESS::decodeMultipleWithPCMatrix() Decoding matrix:" << endl;
    jerasure_print_matrix(decodeMatrix, failedSymbols.size(), _k * _w, 8);

    // data: (k * w)
    vector<int> &data = availSymbols;
    vector<int> &codes = failedSymbols;

    for (int i = 0; i < codes.size(); i++)
    {
        int code = codes[i];
        vector<int> coef(decodeMatrix + i * _k * _w, decodeMatrix + (i + 1) * _k * _w);
        ecdag->Join(code, data, coef);
    }

    if (codes.size() > 1)
    {
        int vidx = ecdag->BindX(codes);
        ecdag->BindY(vidx, data[0]);
    }

    delete[] decodeMatrix;

    return ecdag;
}

void LESS::getPrimElementsPower(int order, int e, int fw)
{
    _primElementPower.resize(order);
    _primElementPower[0] = 1;
    for (int i = 1; i < order; i++)
    {
        _primElementPower[i] = galois_single_multiply(_primElementPower[i - 1], e, fw);
    }
}

bool LESS::getAvailPrimElements(int n, int k, int w, int &fw, uint32_t &e, uint32_t &f)
{

    // no available primitive elements: k <= 1; n - k == 1
    if (k <= 1 || n <= k + 1)
    {
        f = 0;
        fw = 0;
        e = 0;
        return false;
    }

    // reset fw, f, e
    fw = 0;
    f = 0;
    e = 0;

    if (n - k == 2)
    {
        if (n * w <= ((1 << 8) - 1))
        {
            fw = 8;
            f = 0b100011101; // x^8 + x^4 + x^3 + x^2 + 1
            e = 2;
        }
        else if (n * w <= ((1 << 16) - 1))
        {
            fw = 16;
            f = 0b10000000000101101; // x^{16} + x^5 + x^3 + x^2 + 1
            e = 2;
        }
    }
    else if (n - k == 3)
    {
        if (w == 2)
        {
            if (n <= 44)
            {
                f = 0b111001111; // x^8 + x^7 + x^6 + x^3 + x^2 + x + 1
                fw = 8;
            }
            else if (n <= 127)
            {
                f = 0b10000000000101101; // x^{16} + x^5 + x^3 + x^2 + 1
                fw = 16;
            }
        }
        else if (w == 3)
        {
            if (n <= 40)
            {
                f = 0b100101101; // x^8 + x^5 + x^3 + x^2 + 1
                fw = 8;
            }
            else if (n <= 127)
            {
                f = 0b10000000000101101; // x^{16} + x^5 + x^3 + x^2 + 1
                fw = 16;
            }
        }
    }
    else if (n - k == 4)
    {
        if (w == 2)
        {
            if (n <= 23)
            {
                f = 0b100101011; // x^8 + x^5 + x^3 + x + 1
                fw = 8;
            }
            else if (n <= 127)
            {
                f = 0b10000001101110001; // x^{16} + x^9 + x^8 + x^6 + x^5 + x^4 + 1
                fw = 16;
            }
        }
        else if (w == 3)
        {
            if (n <= 17)
            {
                f = 0b100011101; // x^8 + x^4 + x^3 + x^2 + 1
                fw = 8;
            }
            else if (n <= 127)
            {
                f = 0b10001001011110011; // x^{16} + x^{12} + x^9 + x^7 + x^6 + x^5 + x^4 + x + 1
                fw = 16;
            }
        }
        else if (w == 4)
        {
            if (n <= 16)
            {
                f = 0b100101101; // x^8 + x^5 + x^3 + x^2 + 1
                fw = 8;
            }
            else if (n <= 127)
            {
                f = 0b10000011101111001; // x^{16} + x^{10} + x^9 + x^8 + x^6 + x^5 + x^4 + x^3 + 1
                fw = 16;
            }
        }
    }
    if (f)
    {
        e = findRoot(f, fw);
        // printf("r = %d, w = %d:\n   n = %d\n    w = %2d, f(x) = ", r, w, n, w);
        // print_polynomial(f);
        // printf("    available element: %d\n\n", e);
    }

    return e != 0;
}

uint32_t LESS::findRoot(uint32_t f, int fw)
{
    for (uint32_t root = 1;; root++)
    {
        if (root == 0 || root > ((1 << fw) - 1))
        {
            return 0;
        }
        if (polynomialAssignment(root, f, fw) != 0)
        {
            continue;
        }
        return root;
    }
}

uint32_t LESS::polynomialAssignment(uint32_t x, uint32_t f, int fw)
{
    uint32_t fx = 0;
    for (int i = 31; i >= 0; i--)
    {
        fx = galois_single_multiply(fx, x, fw);
        // fx *= x;
        // fx = gf_mult(fx, x, w);
        fx ^= ((f >> i) & (1));
    }
    return fx;
}

vector<vector<int>> LESS::GetSubPackets()
{
    return _layout;
}

void LESS::initLayout()
{
    int symbolId = 0;
    _layout.resize(_w, vector<int>(_n, 0));
    for (int nodeId = 0; nodeId < _n; nodeId++)
    {
        for (int alpha = 0; alpha < _w; alpha++)
        {
            _layout[alpha][nodeId] = symbolId++;
        }
    }
}
