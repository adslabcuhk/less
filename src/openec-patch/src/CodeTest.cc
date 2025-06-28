#include "common/Config.hh"
#include "ec/ECDAG.hh"
#include "ec/ECPolicy.hh"
#include "ec/ECBase.hh"
#include "ec/ECTask.hh"

#include "ec/RSCONV.hh"
#include "ec/HHXORPlus.hh"
#include "ec/HTEC.hh"
#include "ec/Clay.hh"
#include "ec/ETRSConv.hh"
#include "ec/LESS.hh"

#include <utility>
#include <random>

#define RANDOM_SEED 12345

void generateRandomInts(mt19937 &generator, char *buffer, int size, int from, int to)
{
    uniform_int_distribution<int> distribution(from, to);
    for (int i = 0; i < size; i++)
    {
        buffer[i] = distribution(generator);
    }
}

using namespace std;

void usage()
{
    printf("usage: ./CodeTest code_name n k w blockSizeBytes <failed_block_ids>\n");
    printf("code_name: RSCONV, HHXORPlus, HTEC, Clay, ETRSConv, LESS\n");
    printf("n: number of blocks\n");
    printf("k: number of data blocks\n");
    printf("w: sub-packetization\n");
    printf("blockSizeBytes: size of each block in Bytes\n");
    printf("failed_block_ids: list of failed block ids\n");
    printf("Example: ./CodeTest RSCONV 14 10 1 1024 1\n");
}

double getCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1e+6 + (double)tv.tv_usec;
}

int main(int argc, char **argv)
{
    if (argc < 7)
    {
        usage();
        return 0;
    }

    string codeName = argv[1];
    int n = atoi(argv[2]);
    int k = atoi(argv[3]);
    int w = atoi(argv[4]);

    // block size: round up to the nearest multiple of 16 * w (for Jerasure)
    unsigned long long blockSizeBytes = atoi(argv[5]);
    if (blockSizeBytes % (16 * w) != 0)
    {
        int nearestBS = (int)ceil(1.0 * blockSizeBytes / (16 * w)) * 16 * w;
        printf("Warning: block size must be a multiple of 16 * w = %d; rounded to the nearest: %d\n", 16 * w, nearestBS);
        blockSizeBytes = nearestBS;
    }

    vector<int> failedIds;
    for (int i = 6; i < argc; i++)
    {
        int failedId = atoi(argv[i]);
        if (failedId >= n)
        {
            printf("error: invalid failed block: %d\n", failedId);
            return -1;
        }
        failedIds.push_back(failedId);
    }
    sort(failedIds.begin(), failedIds.end());

    printf("Testing code: %s, n = %d, k = %d, w = %d, block size = %llu Bytes, failedIds: ", codeName.c_str(), n, k, w, blockSizeBytes);
    for (int i = 0; i < failedIds.size(); i++)
    {
        printf("%d ", failedIds[i]);
    }
    printf("\n");

    unsigned long long pktSizeBytes = blockSizeBytes / w;

    // field width (special setting for LESS)
    int fw = 8; // default fw
    if (codeName == "LESS")
    {
        uint32_t e, f;
        if (LESS::getAvailPrimElements(n, k, w, fw, e, f) == false)
        {
            printf("LESS::getAvailPrimElements() failed to find primitive element\n");
            exit(1);
        }
    }

    // get ecId in OpenEC
    string confpath = "./conf/sysSetting.xml";
    Config *conf = new Config(confpath);

    string ecId = codeName + "_" + to_string(n) + "_" + to_string(k) + "_" + to_string(w);
    ECPolicy *ecPolicy = conf->_ecPolicyMap[ecId];
    ECBase *ec = ecPolicy->createECClass();

    /**
     * @brief Encoding
     *
     */

    // generate random data
    random_device rd;
    // mt19937 rdGenerator(RANDOM_SEED);
    mt19937 rdGenerator(rd());

    int numDataSymbols = k * w;
    int numCodeSymbols = (n - k) * w;

    // prepare buffers for data and parity blocks
    char **dataPtrs = new char *[numDataSymbols];
    char **codePtrs = new char *[numCodeSymbols];

    for (int i = 0; i < numDataSymbols; i++)
    {
        dataPtrs[i] = new char[pktSizeBytes];
        generateRandomInts(rdGenerator, dataPtrs[i], pktSizeBytes, CHAR_MIN, CHAR_MAX);
    }

    for (int i = 0; i < numCodeSymbols; i++)
    {
        codePtrs[i] = new char[pktSizeBytes];
        memset(codePtrs[i], 0, pktSizeBytes);
    }

    /**
     * @brief Encoding
     */
    // start encoding
    double encodeTime = getCurrentTime();

    // init encoding
    ECDAG *encDAG = ec->Encode();
    vector<ECTask *> encTasks;
    vector<int> encTopoSeq = encDAG->toposort();
    for (int i = 0; i < encTopoSeq.size(); i++)
    {
        ECNode *curNode = encDAG->getNode(encTopoSeq[i]);
        curNode->parseForClient(encTasks);
    }

    // encoding buffers
    unordered_map<int, char *> encBufMap;
    for (int i = 0; i < numDataSymbols; i++)
    {
        encBufMap.insert(make_pair(i, dataPtrs[i]));
    }

    for (int i = 0; i < numCodeSymbols; i++)
    {
        encBufMap.insert(make_pair(numDataSymbols + i, codePtrs[i]));
    }

    // free list to support shortening
    vector<int> shorteningFreeList;
    for (int taskId = 0; taskId < encTasks.size(); taskId++)
    {
        ECTask *computeTask = encTasks[taskId];
        // computeTask->dump();

        vector<int> children = computeTask->getChildren();
        unordered_map<int, vector<int>> coefMap = computeTask->getCoefMap();
        int col = children.size();
        int row = coefMap.size();

        // assign data and code buffers for encoding
        int *matrix = new int[row * col];
        char **data = new char *[col];
        char **code = new char *[row];
        for (int bufIdx = 0; bufIdx < children.size(); bufIdx++)
        {
            int child = children[bufIdx];

            // create buffers to support shortening
            if (child >= (numDataSymbols + numCodeSymbols) && encBufMap.find(child) == encBufMap.end())
            {
                shorteningFreeList.push_back(child);
                char *tmpBuf = new char[pktSizeBytes];
                memset(tmpBuf, 0, pktSizeBytes * sizeof(char));
                encBufMap[child] = tmpBuf;
            }

            data[bufIdx] = encBufMap[child];
        }
        int codeBufIdx = 0;
        for (auto it : coefMap)
        {
            int codeId = it.first;
            char *codeBuf;
            if (encBufMap.find(codeId) == encBufMap.end())
            {
                codeBuf = new char[pktSizeBytes];
                memset(codeBuf, 0, pktSizeBytes * sizeof(char));
                encBufMap.insert(make_pair(codeId, codeBuf));
            }
            else
            {
                codeBuf = encBufMap[codeId];
            }
            code[codeBufIdx] = codeBuf;
            vector<int> codeCoef = it.second;
            for (int j = 0; j < col; j++)
            {
                matrix[codeBufIdx * col + j] = codeCoef[j];
            }
            codeBufIdx++;
        }
        Computation::Multi(code, data, matrix, row, col, pktSizeBytes, "Isal", fw);

        delete[] matrix;
        delete[] data;
        delete[] code;
    }

    // free buffers in shortening free list
    for (auto pktIdx : shorteningFreeList)
    {
        delete[] encBufMap[pktIdx];
    }
    shorteningFreeList.clear();

    // end encoding
    encodeTime = (getCurrentTime() - encodeTime) / 1000000;

    // check coded data
    printf("Check coded data:\n");
    printf("========================\n");
    for (int i = 0; i < numDataSymbols; i++)
    {
        char *curbuf = (char *)dataPtrs[i];
        printf("dataIdx = %d, value = %d\n", i, curbuf[0]);
    }
    for (int i = 0; i < numCodeSymbols; i++)
    {
        char *curbuf = (char *)codePtrs[i];
        printf("codeIdx = %d, value = %d\n", numDataSymbols + i, curbuf[0]);
    }
    printf("========================\n");

    /**
     * @brief Decoding
     */

    vector<int> failedSymbols;
    unordered_map<int, char *> decodeBuf;

    for (auto failedId : failedIds)
    {
        for (int i = 0; i < w; i++)
        {
            failedSymbols.push_back(failedId * w + i);
        }
    }

    vector<int> availSymbols;
    for (int i = 0; i < numDataSymbols + numCodeSymbols; i++)
    {
        if (find(failedSymbols.begin(), failedSymbols.end(), i) == failedSymbols.end())
        {
            availSymbols.push_back(i);
        }
    }

    // create buffers to repair failed symbols
    for (int i = 0; i < failedSymbols.size(); i++)
    {
        char *tmpBuf = new char[pktSizeBytes];
        memset(tmpBuf, 0, pktSizeBytes * sizeof(char));
        decodeBuf[failedSymbols[i]] = tmpBuf;
    }

    // start decoding
    double decodeTime = getCurrentTime();

    ECDAG *decDAG = ec->Decode(availSymbols, failedSymbols);
    vector<ECTask *> decodeTasks;
    unordered_map<int, char *> decodeBufMap;
    vector<int> decTopoSeq = decDAG->toposort();
    for (int i = 0; i < decTopoSeq.size(); i++)
    {
        ECNode *curNode = decDAG->getNode(decTopoSeq[i]);
        curNode->parseForClient(decodeTasks);
    }
    for (int i = 0; i < numDataSymbols; i++)
    {
        if (find(failedSymbols.begin(), failedSymbols.end(), i) == failedSymbols.end())
        {
            decodeBufMap.insert(make_pair(i, dataPtrs[i]));
        }
        else
        {
            decodeBufMap.insert(make_pair(i, decodeBuf[i]));
        }
    }
    for (int i = 0; i < numCodeSymbols; i++)
        if (find(failedSymbols.begin(), failedSymbols.end(), numDataSymbols + i) == failedSymbols.end())
        {
            decodeBufMap.insert(make_pair(numDataSymbols + i, codePtrs[i]));
        }
        else
        {
            decodeBufMap.insert(make_pair(i, decodeBuf[numDataSymbols + i]));
        }

    /**
     * @brief record non-contiguous reads
     * @diskReadSymbolMap <nodeId, <sub-packet ids>>
     * @diskReadNonContiMap <nodeId, [[contiguous sub-packets], [contiguous sub-packets]]>
     */
    map<int, vector<int>> diskReadSymbolsMap;

    // init the maps
    for (int nodeId = 0; nodeId < n; nodeId++)
    {
        diskReadSymbolsMap[nodeId].clear();
    }

    for (int taskId = 0; taskId < decodeTasks.size(); taskId++)
    {
        ECTask *computeTask = decodeTasks[taskId];
        // computeTask->dump();

        vector<int> children = computeTask->getChildren();
        unordered_map<int, vector<int>> coefMap = computeTask->getCoefMap();
        int col = children.size();
        int row = coefMap.size();

        // assign data and code buffers for encoding
        int *matrix = new int[row * col];
        char **data = new char *[col];
        char **code = new char *[row];
        for (int bufIdx = 0; bufIdx < children.size(); bufIdx++)
        {
            int child = children[bufIdx];

            // create buffers to support shortening
            if (child >= numDataSymbols + numCodeSymbols && decodeBufMap.find(child) == decodeBufMap.end())
            {
                shorteningFreeList.push_back(child);
                char *tmpBuf = new char[pktSizeBytes];
                memset(tmpBuf, 0, pktSizeBytes * sizeof(char));
                decodeBufMap[child] = tmpBuf;
            }

            data[bufIdx] = decodeBufMap[child];

            // record the retrieved packets
            if (child < numDataSymbols + numCodeSymbols)
            {
                int nodeId = child / w;
                vector<int> &readPkts = diskReadSymbolsMap[nodeId];
                if (find(readPkts.begin(), readPkts.end(), child) == readPkts.end())
                {
                    readPkts.push_back(child);
                }
            }
        }
        int codeBufIdx = 0;
        for (auto it : coefMap)
        {
            int codeId = it.first;
            char *codeBuf;
            if (decodeBufMap.find(codeId) == decodeBufMap.end())
            {
                codeBuf = new char[pktSizeBytes];
                memset(codeBuf, 0, pktSizeBytes * sizeof(char));
                decodeBufMap.insert(make_pair(codeId, codeBuf));
            }
            else
            {
                codeBuf = decodeBufMap[codeId];
            }
            code[codeBufIdx] = codeBuf;
            vector<int> codeCoef = it.second;
            for (int j = 0; j < col; j++)
            {
                matrix[codeBufIdx * col + j] = codeCoef[j];
            }
            codeBufIdx++;
        }
        Computation::Multi(code, data, matrix, row, col, pktSizeBytes, "Isal", fw);
        delete[] matrix;
        delete[] data;
        delete[] code;
    }

    // free buffers in shortening free list
    for (auto pktIdx : shorteningFreeList)
    {
        delete[] decodeBufMap[pktIdx];
    }
    shorteningFreeList.clear();

    decodeTime = (getCurrentTime() - decodeTime) / 1000000;

    // debug decoding
    for (int i = 0; i < failedSymbols.size(); i++)
    {
        int failedIdx = failedSymbols[i];
        char *curBuf = decodeBufMap[failedIdx];

        printf("failedIdx = %d, decoded value = %d\n", failedIdx, (char)curBuf[0]);

        int failedNodeId = failedIdx / w;

        // compare decoded data with the original data
        int diff = 0;
        if (failedNodeId < k)
        {
            diff = memcmp(decodeBufMap[failedIdx], dataPtrs[failedIdx], pktSizeBytes * sizeof(char));
        }
        else
        {
            diff = memcmp(decodeBufMap[failedIdx], codePtrs[failedIdx - numDataSymbols], pktSizeBytes * sizeof(char));
        }
        if (diff != 0)
        {
            printf("error: failed to decode data for symbol %d!!!!\n", i);
        }
    }

    /**
     * @brief Disk read info (record the non-contiguous reads)
     */
    int numAccessedNodes = 0;
    int sumPktsRead = 0;
    int maxPktsReadNode = 0;
    int minPktsReadNode = INT_MAX;
    int sumNonContAccess = 0;
    int maxNonContAccessNode = 0;
    int minNonContAccessNode = INT_MAX;
    double normRepairBW = 0;
    printf("Disk read information:\n");
    for (int nodeId = 0; nodeId < n; nodeId++)
    {
        // disk read list
        vector<int> &diskReadList = diskReadSymbolsMap[nodeId];

        if (diskReadList.size() == 0)
        {
            continue;
        }
        sort(diskReadList.begin(), diskReadList.end());
        
        // accessed this node
        numAccessedNodes++;
        if (diskReadList.size() > maxPktsReadNode)
        {
            maxPktsReadNode = diskReadList.size();
        }
        if (diskReadList.size() < minPktsReadNode)
        {
            minPktsReadNode = diskReadList.size();
        }

        // non-contigous disk read list
        vector<vector<int>> nonContDiskReadList;

        // convert diskReadList to non-contiguous read list
        for (int i = 0; i < diskReadList.size(); i++)
        {
            if (i == 0)
            {
                nonContDiskReadList.push_back({diskReadList[i]});
            }
            else
            {
                if (diskReadList[i] == diskReadList[i - 1] + 1)
                {
                    nonContDiskReadList.back().push_back(diskReadList[i]);
                }
                else
                {
                    nonContDiskReadList.push_back({diskReadList[i]});
                }
            }
        }
        sumNonContAccess += nonContDiskReadList.size();

        if (nonContDiskReadList.size() > maxNonContAccessNode)
        {
            maxNonContAccessNode = nonContDiskReadList.size();
        }
        if (nonContDiskReadList.size() < minNonContAccessNode)
        {
            minNonContAccessNode = nonContDiskReadList.size();
        }

        printf("NodeId: %d, number of read sub-packets: %d; number of non-contiguous accesses: %d, access pattern: ", nodeId, (int)diskReadList.size(), (int)nonContDiskReadList.size());

        bool printGap = false;
        for (auto consList : nonContDiskReadList)
        {
            if (printGap == false)
            {
                printGap = true;
            }
            else
            {
                printf("- ");
            }
            for (auto pktId : consList)
            {
                printf("%d ", pktId);
            }
        }
        printf("\n");

        // update stats
        sumPktsRead += diskReadSymbolsMap[nodeId].size();
    }

    // calculate norm repair bandwidth (against RS code)
    normRepairBW = sumPktsRead * 1.0 / (numDataSymbols);

    delete conf;

    // print encode and decode stats
    printf("Code: %s, encode throughput: %f MiB/s (%llu MiB in %f seconds)\n", codeName.c_str(), 1.0 * blockSizeBytes * k / 1048576 / encodeTime, blockSizeBytes * k / 1048576, encodeTime);
    printf("Code: %s, decode throughput: %f MiB/s (%llu MiB in %f seconds)\n", codeName.c_str(), 1.0 * blockSizeBytes * k / 1048576 / decodeTime, blockSizeBytes * k / 1048576, decodeTime);
    
    // print repair stats
    printf("Repair degree (number of accessed nodes): %d\n", numAccessedNodes);
    printf("Repair bandwidth: total packets read: %d / %d, average per node: %f (min: %d, max: %d), normalized repair bandwidth (w.t. RS): %f\n", sumPktsRead, numDataSymbols, 1.0 * sumPktsRead / numAccessedNodes, minPktsReadNode, maxPktsReadNode, normRepairBW);
    printf("Repair access: total non-contiguous accesses: %d, average per node: %f (min: %d, max: %d)\n", sumNonContAccess, 1.0 * sumNonContAccess / numAccessedNodes, minNonContAccessNode, maxNonContAccessNode);
}