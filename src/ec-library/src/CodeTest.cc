#include <random>
#include <sys/time.h>
#include <climits>

#include "include.hh"
#include "Util.hh"
#include "RSCode.hh"
#include "LESS.hh"

#define RANDOM_SEED 12345

void generateRandomInts(mt19937 &generator, char *buffer, int size, int from, int to)
{
    uniform_int_distribution<int> distribution(from, to);
    for (int i = 0; i < size; i++)
    {
        buffer[i] = distribution(generator);
    }
}

vector<int> generateFailedBlocks(mt19937 &generator, int n, int numFailedBlocks)
{
    uniform_int_distribution<int> distribution(0, n - 1);
    vector<int> failedBlocks;
    for (int i = 0; i < numFailedBlocks; i++)
    {
        int failedId = distribution(generator);
        while (find(failedBlocks.begin(), failedBlocks.end(), failedId) != failedBlocks.end())
        {
            failedId = distribution(generator);
        }
        failedBlocks.push_back(failedId);
    }

    sort(failedBlocks.begin(), failedBlocks.end());

    return failedBlocks;
}

int main(int argc, char **argv)
{
    if (argc < 7)
    {
        printf("CodeTest: encoding/decoding performance test\n");
        printf("usage: CodeTest code_name n k [alpha] block_size_bytes failed_ids\n");
        printf("code_name:\n");
        printf("  RSCode: alpha will be set to 1 by default\n");
        printf("  LESS: 2 <= alpha <= n - k\n");
        printf("  block_size_bytes: must be a multiple of 16 * alpha\n");
        printf("  failed_ids: a list of failed block ids, separated by space\n");
        return 0;
    }

    string codeName = argv[1];
    int n = atoi(argv[2]);
    int k = atoi(argv[3]);
    int alpha = atoi(argv[4]);
    unsigned long long blockSizeBytes = atoll(argv[5]);

    vector<int> failedIds;
    for (int i = 6; i < argc; i++)
    {
        failedIds.push_back(atoi(argv[i]));
    }
    sort(failedIds.begin(), failedIds.end());

    ErasureCode *code = nullptr;
    if (codeName == "RSCode")
    {
        code = new RSCode(n, k);
    }
    else if (codeName == "LESS")
    {
        code = new LESS(n, k, alpha);
    }
    else
    {
        printf("error: unsupported code name\n");
        return -1;
    }

    alpha = code->alpha;
    int r = n - k;
    int w = code->w;

    if (blockSizeBytes % (16 * alpha) != 0)
    {
        int nearestBS = (int)ceil(1.0 * blockSizeBytes / (16 * alpha)) * 16 * alpha;
        printf("Warning: block size must be a multiple of 16 * alpha = %d; rounded to the nearest: %d\n", 16 * alpha, nearestBS);
        blockSizeBytes = nearestBS;
    }

    unsigned long long pktSizeBytes = blockSizeBytes / alpha;

    // EC library
    string defaultECLib = "Unknown";

    if (w == 8)
    {
        defaultECLib = "ISA-L";
    }
    else if (w == 16 || w == 32)
    {
        defaultECLib = "Jerasure";
    }

    printf("Testing %s with (n=%d, k=%d, alpha=%d), block size=%llu bytes, sub-packet size=%llu, EC Library: %s\n", codeName.c_str(), n, k, alpha, blockSizeBytes, pktSizeBytes, defaultECLib.c_str());

    // printf("Parity check matrix:\n");
    // Util::printGFMatrix(code->parityCheckMatrix, r * alpha, n * alpha, w);

    // printf("Generator check matrix:\n");
    // Util::printGFMatrix(code->generatorMatrix, r * alpha, k * alpha, w);

    /**
     * @brief Encoding test
     */

    int numPkts = n * alpha;
    int numDataPkts = k * alpha;
    int numCodePkts = r * alpha;
    // buffers
    char **pktPtrs = new char *[numPkts];
    char **dataPtrs = new char *[numDataPkts];
    char **codePtrs = new char *[numCodePkts];

    printf("numPkts: %d, numDataPkts: %d, numCodePkts: %d\n", numPkts, numDataPkts, numCodePkts);

    random_device rd;
    // mt19937 rdGenerator(RANDOM_SEED);
    mt19937 rdGenerator(rd());

    // bind pointer
    for (int i = 0; i < numDataPkts; i++)
    {
        dataPtrs[i] = new char[pktSizeBytes];
        // memset(dataPtrs[i], 0, pktSizeBytes * sizeof(char));
        generateRandomInts(rdGenerator, dataPtrs[i], pktSizeBytes, CHAR_MIN, CHAR_MAX);
        pktPtrs[i] = dataPtrs[i];
    }
    for (int i = 0; i < numCodePkts; i++)
    {
        codePtrs[i] = new char[pktSizeBytes];
        memset(codePtrs[i], 0, pktSizeBytes * sizeof(char));
        pktPtrs[numDataPkts + i] = codePtrs[i];
    }

    // benchmark time
    struct timeval startTime, endTime;
    gettimeofday(&startTime, NULL);

    if (!code->encodeData(dataPtrs, codePtrs, pktSizeBytes, defaultECLib))
    {
        printf("error: encoding failed\n");
        return -1;
    }

    gettimeofday(&endTime, NULL);
    double encodingTime = (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1000000.0;
    // printf("Encoding throughput: %f MiB/s, time: %.6f seconds\n", 1.0 * blockSizeBytes * k / 1048576 / encodingTime, encodingTime);

    /**
     * @brief Decoding test
     */

    // randomly generate block failures
    // int numFailedBlocks = 1;
    // vector<int> failedBlocks = generateFailedBlocks(rdGenerator, n,
    // numFailedBlocks);
    int numFailedBlocks = failedIds.size();
    vector<int> failedBlocks = failedIds;

    int numFailedPkts = numFailedBlocks * alpha;
    int numAvailPkts = numPkts - numFailedPkts;
    vector<int> failedPkts;
    vector<int> availPkts;
    for (int i = 0; i < n; i++)
    {
        if (find(failedBlocks.begin(), failedBlocks.end(), i) != failedBlocks.end())
        {
            for (int j = 0; j < alpha; j++)
            {
                failedPkts.push_back(i * alpha + j);
            }
        }
        else
        {
            for (int j = 0; j < alpha; j++)
            {
                availPkts.push_back(i * alpha + j);
            }
        }
    }

    printf("Failed blocks: ");
    for (auto item : failedBlocks)
    {
        printf("%d ", item);
    }
    printf("\n");

    // printf("Failed packets: ");
    // for (auto item : failedPkts)
    // {
    //     printf("%d ", item);
    // }
    // printf("\n");
    // printf("Available packets: ");
    // for (auto item : availPkts)
    // {
    //     printf("%d ", item);
    // }
    // printf("\n");

    // buffer to store failed data
    char **failedPtrs = new char *[numFailedPkts];

    for (int i = 0; i < numFailedPkts; i++)
    {
        failedPtrs[i] = new char[pktSizeBytes];
        memset(failedPtrs[i], 0, pktSizeBytes * sizeof(char));
    }

    char **availPtrs = new char *[numPkts - numFailedPkts];
    for (int i = 0; i < numAvailPkts; i++)
    {
        availPtrs[i] = pktPtrs[availPkts[i]];
    }

    // benchmark time
    gettimeofday(&startTime, NULL);

    if (!code->decodeData(failedBlocks, failedPtrs, availPtrs, pktSizeBytes, defaultECLib))
    {
        printf("error: decoding failed\n");
        return -1;
    }

    gettimeofday(&endTime, NULL);

    double decodingTime = (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1000000.0;

    // printf("Decoding matrix:\n");
    // Util::printGFMatrix(decodingMatrix, r * alpha, k * alpha, w);

    printf("Encoding throughput: %f MiB/s, time: %.6f seconds\n", 1.0 * blockSizeBytes * k / 1048576 / encodingTime, encodingTime);
    printf("Decoding throughput: %f MiB/s, time: %.6f seconds\n", 1.0 * numFailedPkts * pktSizeBytes / 1048576 / decodingTime, decodingTime);

    // compare the difference between decoded and original data
    for (int i = 0; i < numFailedPkts; i++)
    {
        char *originalPtr = pktPtrs[failedPkts[i]];
        if (memcmp(originalPtr, failedPtrs[i], pktSizeBytes) != 0)
        {
            printf("error: decoding failed, incorrect decoded data\n");
            return -1;
        }
    }

    delete[] availPtrs;
    for (int i = 0; i < numFailedPkts; i++)
    {
        delete[] failedPtrs[i];
    }
    delete[] failedPtrs;

    for (int i = 0; i < numCodePkts; i++)
    {
        delete[] codePtrs[i];
    }
    delete[] codePtrs;

    for (int i = 0; i < numDataPkts; i++)
    {
        delete[] dataPtrs[i];
    }
    delete[] dataPtrs;
    delete[] pktPtrs;

    delete code;

    return 0;
}
