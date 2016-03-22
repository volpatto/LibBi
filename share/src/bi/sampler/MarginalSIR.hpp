/**
 * @file
 *
 * @author Pierre Jacob <jacob@ceremade.dauphine.fr>
 * @author Lawrence Murray <lawrence.murray@csiro.au>
 * $Rev$
 * $Date$
 */
#ifndef BI_SAMPLER_MARGINALSIR_HPP
#define BI_SAMPLER_MARGINALSIR_HPP

#include "../state/Schedule.hpp"
#include "../misc/exception.hpp"
#include "../misc/TicToc.hpp"
#include "../primitive/vector_primitive.hpp"

#include <fstream>
#include <sstream>

namespace bi {
/**
 * Marginal sequential importance resampling.
 *
 * @ingroup method_sampler
 *
 * @tparam B Model type
 * @tparam F Filter type.
 * @tparam A Adapter type.
 * @tparam R Resampler type.
 *
 * Implements sequential importance resampling over parameters, which, when
 * combined with a particle filter, gives the SMC^2 method described in
 * @ref Chopin2013 "Chopin, Jacob \& Papaspiliopoulos (2013)".
 */
template<class B, class F, class A, class R>
class MarginalSIR {
public:
  /**
   * Constructor.
   *
   * @param m Model.
   * @param filter Filter.
   * @param adapter Adapter.
   * @param resam Resampler for theta-particles.
   * @param nmoves Number of move steps per \f$\theta\f$-particle after each
   * resample.
   * @param tmoves Total real time allocated to move steps, in seconds.
   */
  MarginalSIR(B& m, F& filter, A& adapter, R& resam, const int nmoves = 1,
      const double tmoves = 0.0);

  /**
   * @name High-level interface
   */
  //@{
  /**
   * @copydoc MarginalMH::sample()
   */
  template<class S1, class IO1, class IO2>
  void sample(Random& rng, const ScheduleIterator first,
      const ScheduleIterator last, S1& s, const int C, IO1& out, IO2& inInit);
  //@}

  /**
   * @name Low-level interface
   */
  //@{
  /**
   * Initialise.
   *
   * @tparam S1 State type.
   * @tparam IO1 Output type.
   * @tparam IO2 Input type.
   *
   * @param[in,out] rng Random number generator.
   * @param first Start of time schedule.
   * @param s State.
   * @param out Output buffer.
   * @param inInit Init buffer.
   */
  template<class S1, class IO1, class IO2>
  void init(Random& rng, const ScheduleIterator first, S1& s, IO1& out,
      IO2& inInit);

  /**
   * Step \f$x\f$-particles forward.
   *
   * @tparam S1 State type.
   *
   * @param[in,out] rng Random number generator.
   * @param first Start of time schedule.
   * @param[in,out] iter Current position in time schedule. Advanced on
   * return.
   * @param last End of time schedule.
   * @param[out] s State.
   */
  template<class S1>
  void step(Random& rng, const ScheduleIterator first, ScheduleIterator& iter,
      const ScheduleIterator last, S1& s);

  /**
   * Interaction step.
   *
   * @tparam S1 State type.
   *
   * @param[in,out] rng Random number generator.
   * @param now Current step in time schedule.
   * @param[in,out] s State.
   */
  template<class S1>
  void interact(Random& rng, const ScheduleElement now, S1& s);

  /**
   * Move \f$\theta\f$-particles.
   *
   * @tparam S1 State type.
   *
   * @param[in,out] rng Random number generator.
   * @param first Start of time schedule.
   * @param iter Current position in time schedule.
   * @param last End of time schedule.
   * @param[in,out] s State.
   */
  template<class S1>
  void move(Random& rng, const ScheduleIterator first,
      const ScheduleIterator iter, const ScheduleIterator last, S1& s);

  /**
   * @copydoc Simulator::outputT()
   */
  template<class S1, class IO1>
  void outputT(const S1& s, IO1& out);

  /**
   * Report progress on stderr.
   *
   * @param now Current step in time schedule.
   * @param s State.
   */
  template<class S1>
  void report0(const ScheduleElement now, S1& s);

  /**
   * Report progress on stderr.
   *
   * @param now Current step in time schedule.
   * @param s State.
   */
  template<class S1>
  void report(const ScheduleElement now, S1& s);

  /**
   * Report progress on stderr.
   *
   * @param now Current step in time schedule.
   * @param s State.
   */
  template<class S1>
  void reportT(const ScheduleElement now, S1& s);

  /**
   * Finalise.
   */
  template<class S1>
  void term(Random& rng, S1& s);
  //@}

private:
  /**
   * Step for instrumentation.
   */
  enum Step {
    INIT, READY, INTERACT, MOVE, STEP, TERM
  };

  /**
   * Profiling output.
   */
  void profile(const Step step);

#if ENABLE_DIAGNOSTICS == 4
  /**
   * Log file.
   */
  std::ofstream logFile;
#endif

  /**
   * Model.
   */
  B& m;

  /**
   * Filter.
   */
  F& filter;

  /**
   * Adapter.
   */
  A& adapter;

  /**
   * Resampler for the theta-particles
   */
  R& resam;

  /**
   * Clock.
   */
  TicToc clock;

  /**
   * Number of PMMH steps when moving.
   */
  int nmoves;

  /**
   * Total real time allocated to move steps.
   */
  double tmoves;

  /**
   * Start time for current step.
   */
  double tstart;

  /**
   * Milestone (end) time for current step.
   */
  double tmilestone;

  /**
   * Was a resample performed on the last step?
   */
  bool lastResample;

  /**
   * Is the adapter ready?
   */
  bool adapterReady;

  /**
   * Last number of acceptances when move.
   */
  int lastAccept;

  /**
   * Last total number of moves.
   */
  int lastTotal;
};
}

#include "../misc/TicToc.hpp"

template<class B, class F, class A, class R>
bi::MarginalSIR<B,F,A,R>::MarginalSIR(B& m, F& filter, A& adapter, R& resam,
    const int nmoves, const double tmoves) :
    m(m), filter(filter), adapter(adapter), resam(resam), nmoves(nmoves), tmoves(
        1.0e6 * tmoves), lastResample(false), adapterReady(false), lastAccept(
        0), lastTotal(0) {
#if ENABLE_DIAGNOSTICS == 4
#ifdef ENABLE_MPI
  boost::mpi::communicator world;
  const int rank = world.rank();
  const int size = world.size();
#else
  const int rank = 0;
  const int size = 1;
#endif
  std::stringstream buf;
  buf << "sir.log";
  if (size > 1) {
    buf << "." << rank;
  }
  logFile.open(buf.str().c_str());
#endif

  if (tmoves > 0.0) {
    this->nmoves = 1;  // one move at a time only
  }
}

template<class B, class F, class A, class R>
template<class S1, class IO1, class IO2>
void bi::MarginalSIR<B,F,A,R>::sample(Random& rng,
    const ScheduleIterator first, const ScheduleIterator last, S1& s,
    const int C, IO1& out, IO2& inInit) {
  TicToc clock;
  ScheduleIterator iter = first;
  profile(INIT);
  init(rng, iter, s, out, inInit);
  profile(READY);
  profile(INTERACT);
  interact(rng, *iter, s);
  report0(*iter, s);
  while (iter + 1 != last) {
    profile(MOVE);
    move(rng, first, iter, last, s);
    profile(STEP);
    step(rng, first, iter, last, s);
    profile(READY);
    profile(INTERACT);
    interact(rng, *iter, s);
    report(*iter, s);
  }
  profile(MOVE);
  move(rng, first, iter, last, s);
  reportT(*iter, s);
  profile(TERM);
  term(rng, s);

  s.clock = clock.toc();
  outputT(s, out);
}

template<class B, class F, class A, class R>
template<class S1, class IO1, class IO2>
void bi::MarginalSIR<B,F,A,R>::init(Random& rng, const ScheduleIterator first,
    S1& s, IO1& out, IO2& inInit) {
  for (int p = 0; p < s.size(); ++p) {
    BOOST_AUTO(&s1, *s.s1s[p]);
    BOOST_AUTO(&out1, *s.out1s[p]);

    filter.init(rng, *first, s1, out1, inInit);
    filter.output0(s1, out1);
    filter.correct(rng, *first, s1);
    filter.output(*first, s1, out1);

    s.logWeights()(p) = s1.logLikelihood;
    s.ancestors()(p) = p;
  }
  out.clear();

  lastResample = false;
  adapterReady = false;
  lastAccept = 0;
  lastTotal = 0;
}

template<class B, class F, class A, class R>
template<class S1>
void bi::MarginalSIR<B,F,A,R>::step(Random& rng, const ScheduleIterator first,
    ScheduleIterator& iter, const ScheduleIterator last, S1& s) {
  /* pre-condition */
  BI_ASSERT(s.size() > 0);

  ScheduleIterator iter1;
  do {
    for (int p = 0; p < s.size(); ++p) {
      BOOST_AUTO(&s1, *s.s1s[p]);
      BOOST_AUTO(&out1, *s.out1s[p]);

      iter1 = iter;
      filter.step(rng, iter1, last, s1, out1);
      s.logWeights()(p) += s1.logIncrements(iter1->indexObs());
    }
    iter = iter1;
  } while (iter + 1 != last && !iter->isObserved());
#if ENABLE_DIAGNOSTICS == 3
  filter.samplePath(rng, s1, out1);
#endif
}

template<class B, class F, class A, class R>
template<class S1>
void bi::MarginalSIR<B,F,A,R>::interact(Random& rng,
    const ScheduleElement now, S1& s) {
#ifdef ENABLE_MPI
  /* reporting requirements */
  boost::mpi::communicator world;
  const int rank = world.rank();
  int naccept = lastAccept, ntotal = lastTotal;
  boost::mpi::reduce(world, naccept, lastAccept, std::plus<int>(), 0);
  boost::mpi::reduce(world, ntotal, lastTotal, std::plus<int>(), 0);
#endif

  /* marginal likelihood */
  double lW;
  s.ess = resam.reduce(s.logWeights(), &lW);
  s.logIncrements(now.indexObs()) = lW - s.logLikelihood;
  s.logLikelihood = lW;

  /* adapt proposal */
  adapterReady = adapter.adapt(s);

  /* resample */
  lastResample = resam.resample(rng, now, s);
}

template<class B, class F, class A, class R>
template<class S1>
void bi::MarginalSIR<B,F,A,R>::move(Random& rng, const ScheduleIterator first,
    const ScheduleIterator iter, const ScheduleIterator last, S1& s) {
  /* compute budget */
  double t0 = first->indexObs();
  double t = iter->indexObs() - t0 + 1;
  double T = last->indexObs() - t0;
  tstart = clock.toc();
  tmilestone = tmoves * (1.5 * t + 0.5 * t * t) / (1.5 * T + 0.5 * T * T);

  if (lastResample) {
    int naccept = 0;
    int ntotal = 0;
    int j = 0;
    int p = 0;
    bool accept = false;
    bool complete = (tmoves <= 0.0 && p >= s.size())
        || (tmoves > 0.0 && clock.toc() >= tmilestone);

    while (!complete) {
      if (tmoves > 0.0) {
        j = rng.uniformInt(0, s.size() - 1);
        //j = p % s.size();  // systematic scheduler
      } else {
        j = p % s.size();
      }
      BOOST_AUTO(&s1, *s.s1s[j]);
      BOOST_AUTO(&out1, *s.out1s[j]);
      BOOST_AUTO(&s2, s.s2);
      BOOST_AUTO(&out2, s.out2);

      for (int move = 0; move < nmoves; ++move) {
        /* propose replacement */
        try {
          if (adapterReady) {
            filter.propose(rng, *first, s1, s2, out2, adapter);
          } else {
            filter.propose(rng, *first, s1, s2, out2);
          }
          if (bi::is_finite(s2.logPrior)) {
            filter.filter(rng, first, iter + 1, s2, out2);
          }
        } catch (CholeskyException e) {
          s2.logLikelihood = -BI_INF;
        } catch (ParticleFilterDegeneratedException e) {
          s2.logLikelihood = -BI_INF;
        }

        /* accept or reject */
        if (!bi::is_finite(s2.logLikelihood)) {
          accept = false;
        } else if (!bi::is_finite(s1.logLikelihood)) {
          accept = true;
        } else {
          double loglr = s2.logLikelihood - s1.logLikelihood;
          double logpr = s2.logPrior - s1.logPrior;
          double logqr = s1.logProposal - s2.logProposal;

          if (!bi::is_finite(s1.logProposal)
              && !bi::is_finite(s2.logProposal)) {
            logqr = 0.0;
          }
          double logratio = loglr + logpr + logqr;
          double u = rng.uniform<double>();

          accept = bi::log(u) < logratio;
        }

        if (accept) {
#if ENABLE_DIAGNOSTICS == 3
          filter.samplePath(rng, s2, out2);
#endif
          s1.swap(s2);
          out1.swap(out2);
          ++naccept;
        }
        ++ntotal;
      }
      ++p;
      complete = (tmoves <= 0.0 && p >= s.size())
          || (tmoves > 0.0 && clock.toc() >= tmilestone);
    }

    if (tmoves > 0.0) {
      /* particle moving when time elapsed must be eliminated */
      s.logWeights()(j) = -BI_INF;
    }

    lastAccept = naccept;
    lastTotal = ntotal;
  } else {
    lastAccept = 0;
    lastTotal = 0;
  }
}

template<class B, class F, class A, class R>
template<class S1, class IO1>
void bi::MarginalSIR<B,F,A,R>::outputT(const S1& s, IO1& out) {
  out.write(s);
  out.writeClock(s.clock);
}

template<class B, class F, class A, class R>
template<class S1>
void bi::MarginalSIR<B,F,A,R>::report0(const ScheduleElement now, S1& s) {
  if (mpi_rank() == 0) {
    std::cerr << std::fixed << std::setprecision(3);
    std::cerr << now.indexOutput() << ":\ttime " << now.getTime();
    std::cerr << "\tESS " << s.ess;
    if (tmoves > 0.0) {
      std::cerr << "\tstart " << tstart / 1e6;
      std::cerr << "\tmilestone " << tmilestone / 1e6;
    }
  }
}

template<class B, class F, class A, class R>
template<class S1>
void bi::MarginalSIR<B,F,A,R>::report(const ScheduleElement now, S1& s) {
  reportT(now, s);
  report0(now, s);
}

template<class B, class F, class A, class R>
template<class S1>
void bi::MarginalSIR<B,F,A,R>::reportT(const ScheduleElement now, S1& s) {
  if (mpi_rank() == 0) {
    if (lastTotal > 0) {
      std::cerr << "\tmoves " << lastTotal;
      std::cerr << "\taccepts " << lastAccept;
      std::cerr << "\trate " << (double(lastAccept) / lastTotal);
    }
    std::cerr << std::endl;
  }
}

template<class B, class F, class A, class R>
template<class S1>
void bi::MarginalSIR<B,F,A,R>::term(Random& rng, S1& s) {
  s.logLikelihood += logsumexp_reduce(s.logWeights())
      - bi::log(double(s.size()));
  for (int p = 0; p < s.size(); ++p) {
    BOOST_AUTO(&s1, *s.s1s[p]);
    BOOST_AUTO(&out1, *s.out1s[p]);
    filter.samplePath(rng, s1, out1);
  }
}

template<class B, class F, class A, class R>
void bi::MarginalSIR<B,F,A,R>::profile(const Step step) {
  if (step == INIT) {
    mpi_barrier();
    clock.tic();
  } else if (step == READY) {
#if ENABLE_DIAGNOSTICS == 4
    mpi_barrier();
#endif
  }
#if ENABLE_DIAGNOSTICS == 4
  logFile << step << ',' << clock.toc() << std::endl;
#endif
}

#endif
