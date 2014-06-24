/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Christian Schulte <schulte@gecode.org>
 *
 *  Copyright:
 *     Christian Schulte, 2012
 *
 *  Last modified:
 *     $Date: 2012-09-19 15:14:28 +0200 (Wed, 19 Sep 2012) $ by $Author: schulte $
 *     $Revision: 13103 $
 *
 *  This file is part of Gecode, the generic constraint
 *  development environment:
 *     http://www.gecode.org
 *
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

namespace Gecode {

  forceinline 
  FloatValBranch::FloatValBranch(Select s0)
    : s(s0) {}

  forceinline 
  FloatValBranch::FloatValBranch(Rnd r)
    : ValBranch(r), s(SEL_SPLIT_RND) {}

  forceinline 
  FloatValBranch::FloatValBranch(VoidFunction v, VoidFunction c)
    : ValBranch(v,c), s(SEL_VAL_COMMIT) {}

  forceinline FloatValBranch::Select
  FloatValBranch::select(void) const {
    return s;
  }


  inline FloatValBranch
  FLOAT_VAL_SPLIT_MIN(void) {
    return FloatValBranch(FloatValBranch::SEL_SPLIT_MIN);
  }

  inline FloatValBranch
  FLOAT_VAL_SPLIT_MAX(void) {
    return FloatValBranch(FloatValBranch::SEL_SPLIT_MAX);
  }

  inline FloatValBranch
  FLOAT_VAL_SPLIT_RND(Rnd r) {
    return FloatValBranch(r);
  }

  inline FloatValBranch
  FLOAT_VAL(FloatBranchVal v, FloatBranchCommit c) {
    return FloatValBranch(function_cast<VoidFunction>(v),
                          function_cast<VoidFunction>(c));
  }

}

// STATISTICS: float-branch
