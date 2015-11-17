/** \file

    adapted from SGT.c (Geoffrey Sampson of Sussex Univ.), and then by reading
    the original paper

    The Simple Good-Turing technique was devised by the late William
    A. Gale of AT&T Bell Labs, and described in Gale & Sampson,
    "Good-Turing Frequency Estimation Without Tears" (JOURNAL
    OF QUANTITATIVE LINGUISTICS, vol. 2, pp. 217-37 -- reprinted in
    Geoffrey Sampson, EMPIRICAL LINGUISTICS, Continuum, 2001).
*/

#ifndef GOODTURINGSMOOTHING_JG_2013_06_19_HPP
#define GOODTURINGSMOOTHING_JG_2013_06_19_HPP
#pragma once

#include <cstring>
#include <cmath>
#include <cassert>

namespace graehl {

typedef double GoodTuringFloat;  // could be float, but more precision seems better

inline GoodTuringFloat square(GoodTuringFloat x) {
  return x * x;
}

/**
   \return midy - the value of line passing between (lowerx, lowery) and (upperx, uppery) at at x=midx.

   (actually midx doesn't need to be between lower and upper)
*/

inline GoodTuringFloat linearInterp(GoodTuringFloat lowerx, GoodTuringFloat lowery, GoodTuringFloat midx,
                                    GoodTuringFloat upperx, GoodTuringFloat uppery) {
  // assert(lowerx <= midx); assert(midx <= upperx); // note: not really necessary
  if (lowerx == upperx) return lowery;
  return lowery + (uppery - lowery) * (midx - lowerx) / (upperx - lowerx);
}

/**
   linear interpolation on a log(y) vs x plot.
*/
inline GoodTuringFloat loglinearInterp(GoodTuringFloat lowerx, GoodTuringFloat lowery, GoodTuringFloat midx,
                                       GoodTuringFloat upperx, GoodTuringFloat uppery) {
  assert(lowery > 0);
  assert(uppery > 0);
  // if (lowery <= 0 || uppery <= 0) return lowery;
  return std::exp(linearInterp(lowerx, std::log(lowery), midx, upperx, std::log(uppery)));
}

/**
   linear interpolation on a log log plot; data following a power law: y = k * x^k is a straight line on a log
   log plot
*/
inline GoodTuringFloat logloglinearInterp(GoodTuringFloat lowerx, GoodTuringFloat lowery,
                                          GoodTuringFloat midx, GoodTuringFloat upperx,
                                          GoodTuringFloat uppery) {
  //TODO: test
  assert(lowerx > 0);
  assert(lowery > 0);
  assert(uppery > 0);
  // if (lowerx <=0 || lowery <= 0 || uppery <= 0) return lowery;
  return std::exp(
      linearInterp(std::log(lowerx), std::log(lowery), std::log(midx), std::log(upperx), std::log(uppery)));
}

struct GoodTuringSmoothing {
  char const* type() const {
    return "Good-Turing Frequency Estimation Without Tears"
           "(JOURNAL OF QUANTITATIVE LINGUISTICS, vol. 2, pp. 217-37";
  }

  void clear() { setFailed(); }

  void setCountForZeroCount(GoodTuringFloat pzero) { PNotZero = 1 - (PZero = pzero); }

  void setFailed() {
    setCountForZeroCount(kStatFloatEpsilon);
    intercept = 0;
    slope = 1;
    gaplessEnd = 0;
  }

  /**
     counts of counts must be nonempty for counts 1, 2
  */
  void init(CountOfCounts const& cc);

  StatFloat smoothedCount(StatFloat unsmoothedCount) const {
    assert(unsmoothedCount >= 0);
    if (unsmoothedCount == 0) return (StatFloat)PZero;
    // TODO: this is a bit odd. we have a theory that we'll see things in the
    // future that were count 0 about as often as we see things that were count
    // 1. so we're treating the adjusted rStar[-1] (smoothed count for 0-counts)
    // as 1, which is actually higher than rStar[0] (smoothed count for 1 -
    // typically something like .3-.7). but PZero accounts for that (1-PZero is
    // greater)?
    else if (unsmoothedCount < 1)
      return (StatFloat)linearInterp(0, PZero, unsmoothedCount, r[0],
                                     rStar[0]);  // linear interpolation. questionable
    else
      return unsmoothedCount
             * (StatFloat)loglogInterpCountAtLeastOne(unsmoothedCount);  // log log linear interpolation
  }

  StatFloat probabilityOfZeroCount() const { return PZero; }
  StatFloat probabilityOfCountRow(unsigned countRow) const {
    //TODO: test
    return rStar[countRow] * n[countRow] * rowProbabilityNormalizer;
  }

  void print(std::ostream& out) const {
    out << type() << "{\n";
    out << "p(0-count)=" << PZero << '\n';
    assert(rows);
    assert(r[0] == 1);
    if (gaplessEnd) out << "p(1-count)=" << probabilityOfCountRow(0) << '\n';
    for (unsigned i = 0; i < gaplessEnd; ++i) out << r[i] << '\t' << smoothedForSignificantRow(i) << '\n';
    out << "N\t=> P1 * exp(b + m*log(N)) with p(1 or more count) P1=" << PNotZero
        << " intercept b=" << intercept << " and slope m=" << slope << "\n}\n";
  }

  GoodTuringSmoothing() { clear(); }
  GoodTuringSmoothing(CountOfCounts const& cc) { init(cc); }

 private:
  static unsigned const MIN_INPUT = 2;
  static unsigned const MAX_ROWS = 300;

  /// TODO: only used in compute (zero them there, discard after)
  unsigned n[MAX_ROWS];
  GoodTuringFloat log_r[MAX_ROWS], log_Z[MAX_ROWS];
  unsigned bigN;  // sum(n)

  // from here on is used to actually smooth incoming counts:
  unsigned rows;

  unsigned gaplessEnd;  // for all index < gaplessEnd, count = index+1 (no gaps), and we have a stat
  // significant estimate
  unsigned r[MAX_ROWS];
  GoodTuringFloat rStar[MAX_ROWS];  // smoothed count rStar[i] for original count r[i]. r[0] should be 1. p_r
  // = rStar[r]/(bigN=sum(n))
  // GoodTuringFloat adjustedCount[MAX_ROWS];
  /**
     this is a mathematical fact: r* (rStar) = (r+1)E[N_{r+1}]/E[N_r]

     but we don't have the true distribution to take E from . so for low r we
     take E[N_r] = N_r (the empirical count). we do this until things diverge.
  */

  /**
     PZero uses E(N_1)/N per paper - the total prob mass for unseen objects
  */

  GoodTuringFloat PZero, PNotZero, rowProbabilityNormalizer, slope, intercept;

  /**
     what the count would have been in a sample of the original size
  */
  StatFloat loglogInterpCountAtLeastOne(StatFloat unsmoothedCount) const {
    assert(unsmoothedCount >= 1);
    unsigned const cfloor = (unsigned)unsmoothedCount;
    GoodTuringFloat const xmid = (GoodTuringFloat)unsmoothedCount;
    assert(xmid >= cfloor);
    unsigned const lowerRow = cfloor - 1;
    if (xmid == cfloor && lowerRow < gaplessEnd)
      return rStar[lowerRow];
    else if (cfloor < gaplessEnd) {
      //TODO: test
      assert(r[lowerRow] == cfloor);
      GoodTuringFloat xfloor = (GoodTuringFloat)cfloor;
      return logloglinearInterp(xfloor, rStar[lowerRow], xmid, xfloor + 1, rStar[cfloor]);
    } else
      return smoothedFit(unsmoothedCount);
  }

  GoodTuringFloat logsmoothed(GoodTuringFloat unsmoothedCount) const {
    assert(unsmoothedCount > 0);
    return std::exp(intercept + slope * std::log((GoodTuringFloat)unsmoothedCount));
  }
  GoodTuringFloat smoothedFit(GoodTuringFloat c) const {
    return (c + 1) * logsmoothed(c + 1) / logsmoothed(c);
  }

  /**
     return index u into c such that c[u] <= count, or rows if no such u exists
  */
  unsigned rowLowerBound(unsigned count) const {
    // TODO: std::upper_bound (binary search)
    unsigned j = 0;
    assert(rows);
    while (r[j] < count) {
      ++j;
      if (j >= rows) return rows;
    }
    return j;
  }

  GoodTuringFloat smoothedForSignificantRow(unsigned row) const {
    //TODO: test
    assert(row < rows);
    return (PNotZero * rStar[row]);
  }

  void compute();
  void findBestFit();
};


}

#endif
