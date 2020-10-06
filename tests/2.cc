//========================================================================== //
// Copyright (c) 2020, Stephen Henry
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//========================================================================== //

#include "gtest/gtest.h"
#include <type_traits>

// NOTES:

// I haven't used meta-programming too much in anger during my career. And I
// do not profess to be an export. I have some limited understanding,
// so I will what the following does.

// Vector; some parametric list of integers; specifies a unique and distinct type
// as a function of its 'I''s.
//
// For example, when viewed as a type, Vector<0> != Vector<1> (like int != float).
//
template<int... I>
struct Vector;

// Templated type:
template<int i, typename v>
struct push;

// Specialization:
template<int i, int... is>
struct push<i, Vector<is...>> {
  // Specialization of 'push' which is otherwise not defined. Here,
  // we take an integer 'i' and a Vector of 'is' where 'is' is some number of
  // integers (can also be zero integers). The resultant type 'type' is a new
  // vector, prepended with i and whatever was contained by 'is' (if anything).
  using type = Vector<i, is...>;
};

// Templated type (no-definition)
template<typename is>
struct uniq;

// Associated specializations:

// Pattern match list [i, i, a, b, c, d...]
template<int i, int... u>
struct uniq<Vector<i, i, u...>> {
  // This is 'pattern matching'. For the template specialization, where there are at
  // two ints in the parametric template list and the two ints at the head of the list
  // are equal, the result is the unique vector with only one of these 'i'. This is
  // the unification step of the algorithm.
  using type = typename uniq<Vector<i, u...>>::type;
};

// Pattern match list [i, a, b, c, d...]
template<int i, int... u>
struct uniq<Vector<i, u...>> {
  // Otherwise, in the case where the above match does not happen, where the first
  // two elements of the list are non-equal, return a Vector of CAR + (unique CDR).
  // If you are familiar with Lisp.
  using type = typename push<i, typename uniq<Vector<u...>>::type>::type;
};

// Pattern match list []
template<int... is>
struct uniq<Vector<is...>> {
  // Otherwise, this is the catch all, where the aboves don't match. This occurs
  // whenever calling uniq<Vector<>>, i.e. the terminal case in the recursion where
  // 'is' is actually empty.
  using type = Vector<is...>;
};


// TEST
int solution(int X) {

  // Test case #1
  static_assert(std::is_same<uniq<Vector<1, 2, 2, 2, 3, 4, 4>>::type, Vector<1, 2, 3, 4>>::value, "Fail");

  // TASK: Add more test cases here
  static_assert(std::is_same<uniq<Vector<>>::type, Vector<>>::value, "Fail");

  // TASK: Ensure it compiles before submission
  return X;
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
