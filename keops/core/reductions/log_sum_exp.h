#pragma once

#include <sstream>

#include "core/Pack.h"

#include "core/autodiff.h"

#include "core/reductions/reduction.h"

// Implements the LogSumExp reduction operation
// Giving a "LogSumExp" to a Conv1D/2D routine will automatically
// result in it using a numerically stable reduce operation.

namespace keops {

template < class F, int tagI=0, class G_=IntConstant<1> >
struct LogSumExpReduction : public Reduction<Concat<F,G_>,tagI> {

    using G = G_;

    using PARENT = Reduction<Concat<F,G_>,tagI>;

    static const int DIMRED = G::DIM + F::DIM;				// dimension of temporary variable for reduction

    static const int DIM = DIMRED;

    static_assert(F::DIM==1,"LogSumExp requires first formula F of dimension 1.");

    template < typename TYPE >
    struct InitializeReduction {
        HOST_DEVICE INLINE void operator()(TYPE *tmp) {
            // We fill empty cells with the neutral element of the reduction operation,
            //                   (-inf,0) = e^{-inf} * 0 = 0

            // We should use 0xfff0000000000000 for doubles
            //-340282346638528859811704183484516925440.0f;//__int_as_float(0xff800000); // -infty, as +infty = 0x7f800000
            tmp[0] = NEG_INFINITY<TYPE>::value;
            for(int k=1; k<DIMRED; k++)
                tmp[k] = 0.0f;
        }
    };

    // equivalent of the += operation
    template < typename TYPE >
    struct ReducePairShort {
        HOST_DEVICE INLINE void operator()(TYPE *tmp, TYPE *xi, int j) {
            // (m,s) + (m',s'), i.e. exp(m)*s + exp(m')
            TYPE tmpexp;
            if(tmp[0] > xi[0]) { // =  exp(m)  * (s + s'*exp(m'-m))   if m > m'
                tmpexp = exp( xi[0]-tmp[0] );
                for(int k=1; k<DIMRED; k++)
                    tmp[k] += xi[k]*tmpexp ;
            } else {             // =  exp(m') * (s' + exp(m-m')*s)   if m <= m'
                tmpexp = exp( tmp[0]-xi[0] );
                for(int k=1; k<DIMRED; k++)
                    tmp[k] = xi[k] + tmpexp * tmp[k] ;
                tmp[0] = xi[0] ;
            }
        }
    };

    // equivalent of the += operation
    template < typename TYPE >
    struct ReducePair {
        HOST_DEVICE INLINE void operator()(TYPE *tmp, TYPE *xi) {
            // (m,s) + (m',s'), i.e. exp(m)*s + exp(m')
            TYPE tmpexp;
            if(tmp[0] > xi[0]) { // =  exp(m)  * (s + s'*exp(m'-m))   if m > m'
                tmpexp = exp( xi[0]-tmp[0] );
                for(int k=1; k<DIMRED; k++)
                    tmp[k] += xi[k]*tmpexp ;
            } else {             // =  exp(m') * (s' + exp(m-m')*s)   if m <= m'
                tmpexp = exp( tmp[0]-xi[0] );
                for(int k=1; k<DIMRED; k++)
                    tmp[k] = xi[k] + tmpexp * tmp[k] ;
                tmp[0] = xi[0] ;
            }
        }
    };

    template < typename TYPE >
    struct FinalizeOutput {
        HOST_DEVICE INLINE void operator()(TYPE *tmp, TYPE *out, TYPE **px, int i) {
            for(int k=0; k<DIM; k++)
                out[k] = tmp[k];
        }
    };
    
    template < class MS >
    using M = Extract<MS,0,F::DIM>;
    
    template < class MS >
    using S = Extract<MS,F::DIM,G::DIM>;    

    template < class V, class GRADIN, class MS >
    using DiffT_WithSavedForward = Grad<SumReduction<Scal<Exp<Subtract<F,M<MS>>>,G>,tagI>,V,S<GRADIN>>;
    
    // remark : if V::CAT is 2 (parameter), we will get tagI=(V::CAT)%2=0, so we will do reduction wrt j.
    // In this case there is a summation left to be done by the user.

};


}
