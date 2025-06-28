#ifndef _ECBASE_HH_
#define _ECBASE_HH_

#include "../inc/include.hh"

#include "ECDAG.hh"

using namespace std;

class ECBase
{
public:
    int _n, _k, _w;
    // bool _locality;
    int _opt;

    ECBase();
    ECBase(int n, int k, int w, int opt, vector<string> param);

    virtual ECDAG *Encode() = 0;
    virtual ECDAG *Decode(vector<int> from, vector<int> to) = 0;
    virtual void Place(vector<vector<int>> &group) = 0;

    /**
     * @brief Get all sub-packets
     *
     * 0 2 4 6 8 ...
     * 1 3 5 7 9 ...
     *
     * @return vector<vector<int>>
     */
    virtual vector<vector<int>> GetSubPackets();
};

#endif