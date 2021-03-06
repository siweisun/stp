/***********************************************************************************[SolverTypes.h]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef CLAUSE_H
#define CLAUSE_H

#ifdef _MSC_VER
#include "msvc/stdint.h"
#else
#include <stdint.h>
#endif //_MSC_VER

#include <cstdio>
#include <vector>
#include <sys/types.h>
#include "mtl/Vec.h"
#include "SolverTypes.h"
#include "PackedRow.h"
#include "constants.h"

class ClauseAllocator;

template <class T>
uint32_t calcAbstraction(const T& ps) {
    uint32_t abstraction = 0;
    for (uint32_t i = 0; i != ps.size(); i++)
        abstraction |= 1 << (ps[i].toInt() & 31);
    return abstraction;
}

namespace MINISAT
{
using namespace MINISAT;

//=================================================================================================
// Clause -- a simple class for representing a clause:

class MatrixFinder;

class Clause
{
protected:

    #ifdef STATS_NEEDED
    uint group;
    #endif

    uint32_t isLearnt:1;
    uint32_t strenghtened:1;
    uint32_t varChanged:1;
    uint32_t sorted:1;
    uint32_t invertedXor:1;
    uint32_t isXorClause:1;
    uint32_t subsume0Done:1;
    uint32_t isRemoved:1;
    uint32_t isFreed:1;
    uint32_t wasBinInternal:1;
    uint32_t mySize:20;

    union  {uint32_t act; uint32_t abst;} extra;
    float oldActivityInter;

public:
    template<class V>
    Clause(const V& ps, const uint _group, const bool learnt)
    {
        wasBinInternal = (ps.size() == 2);
        isFreed = false;
        isXorClause = false;
        strenghtened = false;
        sorted = false;
        varChanged = true;
        subsume0Done = false;
        mySize = ps.size();
        isLearnt = learnt;
        isRemoved = false;
        setGroup(_group);

        memcpy(begin(), ps.getData(), ps.size()*sizeof(Lit));
        if (learnt) {
            extra.act = 0;
            oldActivityInter = 0;
        } else
            calcAbstractionClause();
    }

    const Lit* begin() const
    {
      return (const Lit*)((char*)this + sizeof(Clause));
    }

    Lit* begin()
    {
      return (Lit*)((char*)this + sizeof(Clause));
    }

    const Lit* end() const
    {
      return (const Lit*)((char*)this + sizeof(Clause) + sizeof(Lit)*mySize);
    }

    Lit* end()
    {
      return (Lit*)((char*)this + sizeof(Clause) + sizeof(Lit)*mySize);
    }

    Lit& operator[](const unsigned at)
    {
        return begin()[at];
    }

    const Lit& operator[](const unsigned at) const
    {
        return begin()[at];
    }

    uint   size        ()      const {
        return mySize;
    }
    void         resize      (const uint size) {
        mySize = size;
    }
    void         shrink      (const uint i) {
        assert(i <= size());
        mySize -= i;
    }
    void         pop         () {
        shrink(1);
    }
    bool   isXor       () {
        return isXorClause;
    }
    bool   learnt      ()      const {
        return isLearnt;
    }
    float&       oldActivity    () {
        return oldActivityInter;
    }

    const float&       oldActivity    () const {
        return oldActivityInter;
    }

    bool getStrenghtened() const {
        return strenghtened;
    }
    void setStrenghtened() {
        strenghtened = true;
        sorted = false;
        subsume0Done = false;
    }
    void unsetStrenghtened() {
        strenghtened = false;
    }
    bool getVarChanged() const {
        return varChanged;
    }
    void setVarChanged() {
        varChanged = true;
        sorted = false;
        subsume0Done = false;
    }
    void unsetVarChanged() {
        varChanged = false;
    }
    bool getSorted() const {
        return sorted;
    }
    void setSorted() {
        sorted = true;
    }
    void setUnsorted() {
        sorted = false;
    }
    void subsume0Finished() {
        subsume0Done = 1;
    }
    bool subsume0IsFinished() {
        return subsume0Done;
    }

    void setActivity(uint32_t i)  {
        extra.act = i;
    }

    const uint32_t&   activity   () const {
        return extra.act;
    }

    void makeNonLearnt()  {
        assert(isLearnt);
        isLearnt = false;
        calcAbstractionClause();
    }

    void makeLearnt(const uint32_t newActivity)  {
        extra.act = newActivity;
        oldActivityInter = 0;
        isLearnt = true;
    }

    inline void  strengthen(const Lit p)
    {
        remove(*this, p);
        sorted = false;
        calcAbstractionClause();
    }

    void calcAbstractionClause() {
        assert(!learnt());
        extra.abst = calcAbstraction(*this);;
    }

    uint32_t getAbst()
    {
        if (learnt())
            return calcAbstraction(*this);
        else
            return extra.abst;
    }

    const Lit*     getData     () const {
        return begin();
    }
    Lit*    getData     () {
        return begin();
    }
    const Lit*     getDataEnd     () const {
        return end();
    }
    Lit*    getDataEnd     () {
        return end();
    }
    void print(FILE* to = stdout) {
        plainPrint(to);
        fprintf(to, "c clause learnt %s group %d act %d oldAct %f\n", (learnt() ? "yes" : "no"), getGroup(), activity(), oldActivity());
    }
    void plainPrint(FILE* to = stdout) const {
        for (uint i = 0; i < size(); i++) {
            if (begin()[i].sign()) fprintf(to, "-");
            fprintf(to, "%d ", begin()[i].var() + 1);
        }
        fprintf(to, "0\n");
    }
    #ifdef STATS_NEEDED
    uint32_t getGroup() const
    {
        return group;
    }
    void setGroup(const uint32_t _group)
    {
        group = _group;
    }
    #else
    uint getGroup() const
    {
        return 0;
    }
    void setGroup(const uint32_t)
    {
        return;
    }
    #endif //STATS_NEEDED
    void setRemoved() {
        isRemoved = true;
    }

    bool removed() const {
        return isRemoved;
    }

    void setFreed() {
        isFreed = true;
    }

    bool freed() const {
        return isFreed;
    }

    bool wasBin() const {
        return wasBinInternal;
    }

    void setWasBin(const bool toSet) {
        wasBinInternal = toSet;
    }
};

class XorClause : public Clause
{
public:
    // NOTE: This constructor cannot be used directly (doesn't allocate enough memory).
    template<class V>
    XorClause(const V& ps, const bool inverted, const uint _group) :
        Clause(ps, _group, false)
    {
        invertedXor = inverted;
        isXorClause = true;
        calcXorAbstraction();
    }

    friend class ClauseAllocator;

    inline bool xor_clause_inverted() const
    {
        return invertedXor;
    }
    inline void invert(bool b)
    {
        invertedXor ^= b;
    }
    void calcXorAbstraction() {
        extra.abst = 0;
        for (uint32_t i = 0; i != size(); i++)
            extra.abst |= 1 << (begin()[i].var() & 31);
    }

    void print() {
        printf("XOR Clause   group: %d, size: %d, learnt:%d, lits:\"", getGroup(), size(), learnt());
        plainPrint();
    }

    void plainPrint(FILE* to = stdout) const {
        fprintf(to, "x");
        if (xor_clause_inverted())
            printf("-");
        for (uint i = 0; i < size(); i++) {
            fprintf(to, "%d ", begin()[i].var() + 1);
        }
        fprintf(to, "0\n");
    }

    friend class MatrixFinder;
};

class WatchedBin {
    public:
        WatchedBin(Lit _impliedLit) : impliedLit(_impliedLit) {};
        Lit impliedLit;
};

class Watched {
    public:
        Watched(ClauseOffset _clause, Lit _blockedLit) : clause(_clause), blockedLit(_blockedLit) {};
        ClauseOffset clause;
        Lit blockedLit;
};

} //NAMESPACE MINISAT

#endif //CLAUSE_H
