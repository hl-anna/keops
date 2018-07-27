#pragma once

#include "core/Pack.h"

#include "core/autodiff.h"

namespace keops {
// Implements the summation reduction operation
// tagI is equal:
// - to 0 if you do the summation over j (with i the index of the output vector),
// - to 1 if you do the summation over i (with j the index of the output vector).
//
template < class F, int tagI=0 >
class SumReduction {

  public :
  
      static const int tagJ = 1-tagI;

        using VARSI = typename F::template VARS<tagI>; // Use the tag to select the "parallel"  variable
        using VARSJ = typename F::template VARS<tagJ>; // Use the tag to select the "summation" variable
        using VARSP = typename F::template VARS<2>;

        using DIMSX = typename GetDims<VARSI>::template PUTLEFT<F::DIM>; // dimensions of "i" variables. We add the output's dimension.
        using DIMSY = GetDims<VARSJ>;                           // dimensions of "j" variables
        using DIMSP = GetDims<VARSP>;                           // dimensions of parameters variables
        
        static const int DIM = F::DIM;		// DIM is dimension of output of convolution ; for a sum reduction it is equal to the dimension of output of formula

	static const int DIMRED = DIM;		// dimension of temporary variable for reduction
		
        using FORM  = F;  // We need a way to access the actual function being used. 
        // using FORM  = AutoFactorize<F>;  // alternative : auto-factorize the formula (see factorize.h file)
        // remark : using auto-factorize should be the best to do but it may slow down the compiler a lot..
        
        using INDSI = GetInds<VARSI>;
        using INDSJ = GetInds<VARSJ>;
        using INDSP = GetInds<VARSP>;

        using INDS = ConcatPacks<ConcatPacks<INDSI,INDSJ>,INDSP>;  // indices of variables
        static_assert(CheckAllDistinct<INDS>::val,"Incorrect formula : at least two distinct variables have the same position index.");
        
        static const int NMINARGS = 1+INDS::MAX; // minimal number of arguments when calling the formula. 

//    struct sEval { // static wrapper

		template < typename TYPE >
		struct InitializeReduction {
			HOST_DEVICE INLINE void operator()(TYPE *tmp) {
				for(int k=0; k<DIM; k++)
					tmp[k] = 0.0f; // initialize output
			}
		};

        template < typename... Args >
        HOST_DEVICE INLINE void operator()(Args... args) {
            F::template Eval<INDS>(args...);
        }
        
		// equivalent of the += operation
		template < typename TYPE >
		struct ReducePairShort {
			HOST_DEVICE INLINE void operator()(TYPE *tmp, TYPE *xi, int j) {
				for(int k=0; k<DIM; k++) {
					tmp[k] += xi[k];
				}
			}
		};
        
		// equivalent of the += operation
		template < typename TYPE >
		struct ReducePair {
			HOST_DEVICE INLINE void operator()(TYPE *tmp, TYPE *xi) {
				for(int k=0; k<DIM; k++) {
					tmp[k] += xi[k];
				}
			}
		};
        
		template < typename TYPE >
		struct FinalizeOutput {
			HOST_DEVICE INLINE void operator()(TYPE *tmp, TYPE *out) {
				for(int k=0; k<DIM; k++)
            				out[k] = tmp[k];
			}
		};
		
//    };

		template < class V, class GRADIN >
		using DiffT = SumReduction<Grad<F,V,GRADIN>,V::CAT>;
        
};

}
