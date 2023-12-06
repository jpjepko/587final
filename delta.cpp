/**
 * @file delta.cpp
 *
 * @brief EECS 587 HW3
 *
 * @author John Paul Jepko <jpjepko@umich.edu>
 *         Matthew Chandra <matcha@umich.edu>
 *
 * @note
 * 
 * @todo
 * - for both picre algos
 *   - implement parallel break
 *     - currently have to wait for all subsets/comps to finish.
 *     - important for speedup of large inputs, especially the reduced version.
 * 
 *   - store interesting subset/comp in a shared variable so no recomputation is
 *     needed (might not save much time).
 */
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h> // gen rand
#include <string>
#include <time.h>   // gen rand
#include <vector>
#include <set>

using std::back_inserter;
using std::binary_search;
using std::cout;
using std::find;
using std::iota;
using std::max;
using std::min;
using std::move;
using std::set_intersection;
using std::set;
using std::string;
using std::swap;
using std::vector;


// debug printing (comment out first line to disable)
#define DEBUG_BUILD 1
#ifdef DEBUG_BUILD
#define DEBUG(x) do { std::cout << x << '\n'; cout.flush(); } while (0)
#else
#define DEBUG(x) do {} while (0)
#endif


// failure-inducing 1-minimal set (we love globals)
const vector<int> oneMinSet = {19, 21, 26, 30, 65, 71, 76, 91, 107, 123, 146, 153, 159, 162, 181, 197, 198, 224, 238, 262, 270, 277, 283, 302, 309, 326, 347, 354, 357, 363, 364, 369, 386, 392, 399, 405, 406, 421, 424, 440, 441, 487, 489, 497, 501, 507, 516, 536, 537, 548, 573, 575, 580, 585, 592, 593, 597, 603, 606, 631, 633, 634, 637, 683, 689, 709, 713, 727, 734, 736, 749, 750, 756, 771, 773, 775, 776, 792, 796, 810, 857, 865, 874, 880, 889, 898, 918, 919, 923, 924, 926, 940, 964, 976, 981};


// print vector of arbitrary type with optional name
template <typename T>
void printVec(const vector<T> &v, string name="") {
    if (name != "") {
        cout << name << ": ";
    }

    cout << '{';
    
    if (v.size() == 0) {
        cout << "}\n";
        return;
    }

    for (ssize_t i = 0; i < v.size() - 1; ++i) {
        cout << v[i] << ", ";
    }
    if (v.size() > 0) {
        cout << v[v.size() - 1] << "}\n";
    }
}


// gen n ints in range [lo, hi) and return as a sorted vector
// inefficient for 0 <= n - (hi - lo) < ε (for small ε)
//     (i.e. n is close to hi - lo)
vector<int> genRandVecBySize(int lo, int hi, int n, bool reseed=true) {
    assert(n <= hi - lo);   // otherwise impossible to gen n distinct ints
    assert(lo < hi);

    if (reseed) srand(time(NULL));

    set<int> s;
    while (s.size() < n) {
        s.insert(rand() % (hi - lo) + lo);
    }

    return vector<int>(s.begin(), s.end());
}


// iterate through i in [0, n), add i to vec with probability p.
// on average: |randVec| ~= p * n
vector<int> genRandVecByDensity(int n, double p, bool reseed=true) {
    assert(p >= 0.0 && p <= 1.0);
    assert(n >= 0);
    if (reseed) srand(time(NULL));

    vector<int> randVec;
    for (int i = 0; i < n; ++i) {
        if (static_cast<double>(rand()) / RAND_MAX < p) {
            randVec.push_back(i);
        }
    }

    return randVec;
}


bool interesting(const vector<int> &v) {
    const vector<int> &interestingSubset = oneMinSet;
    bool isInteresting = true;

    for (auto num : interestingSubset) {
        if (find(v.begin(), v.end(), num) == v.end()) {
            isInteresting = false;
            break;
        }
    }
    
    return isInteresting;
}


vector<int> ddminPicreAlgoStd(vector<int> changes, int n) {
    // lambda helpers for subset and complement
    auto getSub = [](const vector<int> &changes, int i, int n) -> vector<int> {
        int subsetSize = changes.size() / n;
        auto beginIt = changes.begin() + i * subsetSize;
        auto endIt = (i == n - 1 ? changes.end() : changes.begin() + (i + 1) * subsetSize);
        vector<int> sub(beginIt, endIt);
        return sub;
    };

    auto getComp = [](const vector<int> &changes, int i, int n) -> vector<int> {
        int subsetSize = changes.size() / n;
        vector<int> comp(changes);
        auto beginIt = comp.begin() + i * subsetSize;
        auto endIt = (i == n - 1 ? comp.end() : comp.begin() + (i + 1) * subsetSize);
        // #pragma omp critical
        // {
        //     DEBUG("getComp " << i << " erasing [" << static_cast<int>(beginIt - comp.begin()) << ", " << static_cast<int>(endIt - comp.begin()) << ") on |changes| = " << changes.size());
        // }
        comp.erase(beginIt, endIt);
        return comp;
    };


    int iter = 0;
    while (true) {
        DEBUG("iter = " << iter++ << ", |changes| = " << changes.size());
        // reduce to subset
        ssize_t found = -1;
        #pragma omp parallel for
        for (int i = 0; i < n; ++i) {
            vector<int> sub = getSub(changes, i, n);
            if (interesting(sub)) {
                #pragma omp critical
                found = i;  // not sure if atomic, putting in critical for now
            }
        }

        // was anything interesting?
        if (found != -1) {
            size_t oldSize = changes.size();
            changes = getSub(changes, found, n);
            DEBUG("found int. subset, size " << oldSize << " -> " << changes.size());
            n = 2;
            continue;
        }

        // reduce to complement
        found = -1;
        #pragma omp parallel for
        for (int i = 0; i < n; ++i) {
            vector<int> comp = getComp(changes, i, n);
            if (interesting(comp)) {
                #pragma omp critical
                found = i;
            }
        }

        if (found != -1) {
            size_t oldSize = changes.size();
            changes = getComp(changes, found, n);
            DEBUG("found int. complement, size " << oldSize << " -> " << changes.size());
            n = max(n - 1, 2);
            continue;
        }

        // increase granularity
        if (n < changes.size()) {
            int oldN = n;
            n = min(static_cast<int>(changes.size()), 2 * n);
            DEBUG("increased granularity: n = " << oldN << " -> " << n);
            continue;
        }

        break;
    }

    DEBUG("took " << iter << " iters");
    return changes;
}


// algo with combined reduce cases
vector<int> ddminPicreAlgoReduced(vector<int> changes, int n) {
    // lambda helpers for subset and complement
    auto getSub = [](const vector<int> &changes, int i, int n) -> vector<int> {
        int subsetSize = changes.size() / n;
        auto beginIt = changes.begin() + i * subsetSize;
        auto endIt = (i == n - 1 ? changes.end() : changes.begin() + (i + 1) * subsetSize);
        vector<int> sub(beginIt, endIt);
        return sub;
    };

    auto getComp = [](const vector<int> &changes, int i, int n) -> vector<int> {
        int subsetSize = changes.size() / n;
        vector<int> comp(changes);
        auto beginIt = comp.begin() + i * subsetSize;
        auto endIt = (i == n - 1 ? comp.end() : comp.begin() + (i + 1) * subsetSize);
        // #pragma omp critical
        // {
        //     DEBUG("getComp " << i << " erasing [" << static_cast<int>(beginIt - comp.begin()) << ", " << static_cast<int>(endIt - comp.begin()) << ") on |changes| = " << changes.size());
        // }
        comp.erase(beginIt, endIt);
        return comp;
    };


    int iter = 0;
    while (true) {
        DEBUG("iter = " << iter++ << ", |changes| = " << changes.size());
        // reduce to subset or complement
        ssize_t found = -1;
        #pragma omp parallel for
        for (int i = 0; i < 2 * n; ++i) {
            if (0 <= i && i < n) {
                if (interesting(getSub(changes, i, n))) {
                    #pragma omp critical
                    found = i;
                }
            }

            else {  // i in [n, 2n)
                if (interesting(getComp(changes, i - n, n))) {
                    #pragma omp critical
                    found = i;
                }
            }
        }

        // were any subsets interesting?
        if (0 <= found && found < n) {
            size_t oldSize = changes.size();
            changes = getSub(changes, found, n);
            DEBUG("found int. subset, size " << oldSize << " -> " << changes.size());
            n = 2;
            continue;
        }

        // were any complements interesting?
        else if (n <= found && found < 2 * n) {
            size_t oldSize = changes.size();
            changes = getComp(changes, found - n, n);
            DEBUG("found int. complement, size " << oldSize << " -> " << changes.size());
            n - max(n - 1, 2);
            continue;
        }

        // increase granularity
        if (n < changes.size()) {
            int oldN = n;
            n = min(static_cast<int>(changes.size()), 2 * n);
            DEBUG("increased granularity: n = " << oldN << " -> " << n);
            continue;
        }

        break;
    }

    DEBUG("took " << iter << " iters");
    return changes;
}


int main() {
    vector<int> changes(1000);
    iota(changes.begin(), changes.end(), 0);
    auto startTime = omp_get_wtime();
    auto res = ddminPicreAlgoStd(changes, 2);
    auto endTime = omp_get_wtime();
    double dur = endTime - startTime;
    printVec(res, "res");
    printVec(oneMinSet, "act");
    cout << "ran in " << dur <<  " s\n";
    return 0;
}
