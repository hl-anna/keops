#pragma once

#include <assert.h>

#include "core/autodiff/UnaryOp.h"
#include "core/formulas/maths/Scal.h"
#include "core/formulas/maths/ArgMin.h"
#include "core/formulas/maths/OneHot.h"

#include "core/pre_headers.h"
namespace keops {

//////////////////////////////////////////////////////////////
////                 MIN : Min< F >                       ////
//////////////////////////////////////////////////////////////

template<class F>
struct Min : UnaryOp<Min, F> {

  static_assert(F::DIM >= 1, "Min operation is only valid when dimension is non zero.");
  static const int DIM = 1;

  static void PrintIdString(::std::stringstream &str) { str << "Min"; }

  template < typename TYPE >
  static DEVICE INLINE void Operation(TYPE *out, TYPE *outF) {
    *out = outF[0];
    #pragma unroll
    for (int k = 1; k < F::DIM; k++)
      if (outF[k] < *out)
        *out = outF[k];
  }

#if USE_HALF && GPU_ON
  static DEVICE INLINE void Operation(half2 *out, half2 *outF) {
    *out = outF[0];
    #pragma unroll
    for (int k = 1; k < F::DIM; k++) {
      // we have to work element-wise...
      __half2 cond = __hgt2(*out,outF[k]);                 // cond = (out > outF[k]) (element-wise)
      __half2 negcond = __float2half2_rn(1.0f)-cond;       // negcond = 1-cond
      *out = cond * outF[k] + negcond * *out;              // out  = cond * outF[k] + (1-cond) * out
    }
  }
#endif

  template<class V, class GRADIN>
  using DiffT = typename F::template DiffT<V, Scal<GRADIN,OneHot<ArgMin<F>,F::DIM>>>;

};

#define Min(p) KeopsNS<Min<decltype(InvKeopsNS(p))>>()

}
