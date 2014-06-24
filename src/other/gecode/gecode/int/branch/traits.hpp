/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Christian Schulte <schulte@gecode.org>
*
 *  Copyright:
 *     Christian Schulte, 2012
 *
 *  Last modified:
 *     $Date: 2012-09-07 11:29:57 +0200 (Fri, 07 Sep 2012) $ by $Author: schulte $
 *     $Revision: 13061 $
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

  /// Traits of %IntVar for branching
  template<>
  class BranchTraits<IntVar> {
  public:
    /// Type for the branching filter function
    typedef IntBranchFilter Filter;
    /// Type for the branching merit function
    typedef IntBranchMerit Merit;
    /// Type for the branching value function
    typedef IntBranchVal Val;
    /// Return type of the branching value function
    typedef int ValType;
    /// Type for the branching commit function
    typedef IntBranchCommit Commit;
  };

  /// Traits of %BoolVar for branching
  template<>
  class BranchTraits<BoolVar> {
  public:
    /// Type for the branching filter function
    typedef BoolBranchFilter Filter;
    /// Type for the branching merit function
    typedef BoolBranchMerit Merit;
    /// Type for the branching value function
    typedef BoolBranchVal Val;
    /// Return type of the branching value function
    typedef int ValType;
    /// Type for the branching commit function
    typedef BoolBranchCommit Commit;
  };

}

// STATISTICS: int-branch
