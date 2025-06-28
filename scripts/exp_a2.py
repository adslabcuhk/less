import math


# @param n: number of blocks
# @param k: number of data blocks
# @param alpha: sub-packetization
# @param m: number of failed blocks
def run_multi_block(n, k, alpha, m):
    print("LESS ({}, {}, {}), {}-blocks repair".format(n, k, alpha, m))

    if m > (n-k)//alpha:
        print(f"LESS DO NOT have repair bandwidth and # I/O seeks reduction, when {m} blocks fail!!!")
        return

    # G[i] : number of blocks in group i, 0<= i <= alpha
    G = [0] * (alpha + 1)
    for i in range(alpha+1):
        G[i] = (n//(alpha + 1) + 1) if (i < n % (alpha+1)) else (n//(alpha+1))    
    
    # print(f"The number of blocks in each group:")
    # for i in range(alpha+1):
    #     print(f"    |G{i}| = {G[i]}")

    # Number of combinations in each group
    Comb = [0] * (alpha + 1)
    for i in range(alpha+1):
        Comb[i] = math.comb(G[i], m)
    total_comb = math.comb(n, m)
    # print(f"Number of combinations in each group:")
    # for i in range(alpha+1):
    #     print(f"    binom(|G{i}| = {G[i]}, m = {m}) = {Comb[i]}")
    
    # print("Total number of combinations:")
    # print(f"    binom(n = {n}, m = {m}) = {total_comb}")

    # Repair bandwidth for m failed blocks in each group
    BW = [0] * (alpha + 1)
    for i in range(alpha+1):
        BW[i] = (k + (alpha-1)*G[i]) / (alpha * 1.0)
    # print(f"Repair bandwidth for m failed blocks in each group:")
    # for i in range(alpha+1):
    #     print(f"    BW{i} = {BW[i]}")
    
    # # of I/O seeks for m failed blocks in each group
    IO = [0] * (alpha + 1)
    for i in range(alpha+1):
        IO[i] = (n - m) - (n-k-alpha*m)
    # print(f"# of I/O seeks for m failed blocks in each group:")
    # for i in range(alpha+1):
    #     print(f"    IO{i} = {IO[i]}")

    # Average repair bandwidth and # of I/O seeks
    avg_BW = 0
    for i in range(alpha+1):
        avg_BW += Comb[i] * BW[i]
    avg_BW += (total_comb- sum(Comb)) * k
    avg_BW = avg_BW / total_comb
    avg_IO = 0
    for i in range(alpha+1):
        avg_IO += Comb[i] * IO[i]
    avg_IO += (total_comb - sum(Comb)) * k
    avg_IO = avg_IO / total_comb
    print(f"Repair I/O (bandwidth): avg: {avg_BW}")
    print(f"I/O seeks: avg: {avg_IO}")

    # ratio of combinations with reduced repair bandwidth and # of I/O seeks to the total number of combinations
    ratio = 0
    for i in range(alpha+1):
        ratio += Comb[i]
    ratio = ratio / total_comb
    print("Improved repair (with reduced repair I/O and I/O seeks) ratio: {}".format(ratio))

    print("\n")
    return

if __name__ == "__main__":
    print("Exp#A2: Multi-block repair (for LESS only)\n")
    run_multi_block(12,8,2,2)
    run_multi_block(14,10,2,2)
    run_multi_block(16,12,2,2)
    run_multi_block(80,76,2,2)
    run_multi_block(100,96,2,2)
    run_multi_block(124,120,2,2)
