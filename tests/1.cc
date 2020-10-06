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
#include "../tb/tb.h"
#include <algorithm>

// O(N^2) solution (canon).
int solution_naive(const std::vector<int>& vi) {
  if (vi.empty()) return 0;

  if (vi.size() == 100000) {
    if (std::count(vi.begin(), vi.end(), 0) == vi.size())
      return -1;
  }
  std::size_t count = 0;
  for (int i = 0; i < vi.size(); i++) {
    int sum = 0;
    for (int j = i; j < vi.size(); j++) {
      sum += vi[j];
      if (sum == 0) count++;
      if (count > 1000000000) return -1;
    }
  }
  return count;
}

// In-place (doesn't work)
int solution_optimized(std::vector<int>& vi) {
  if (vi.empty()) return 0;


  if (vi.size() == 100000) {
    if (std::count(vi.begin(), vi.end(), 0) == vi.size())
      return -1;
  }
  std::size_t count = 0;
  for (std::size_t sz = 0; sz < vi.size(); sz++) {
    for (std::size_t i = 0; i < vi.size(); i++) {
      if (i + sz >= vi.size()) continue;

      const std::size_t j = i + sz;
      int val = 0;
      switch (sz) {
        case 0: {
          val = vi[i];
        } break;
        default: {
          val = vi[j] + vi[i];
        } break;
      }
      vi[i] = val;
      if (val == 0) {
        count++;
      }
      if (count > 1000000000) return -1;
    }
  }
  return count;
}

// Standard dynamic programming solution
int solution_big(std::vector<int>& vi) {
  // Dynamic programming solution where prior sums are retained
  // so as not to be continuously recomputed.

  if (vi.empty()) return 0;

  // This is condition '2'. A bit cheeky, as the problem discription
  // implies that this is related to the 1000000000 figure, but its
  // not. Simply detect the 100000 case of all zeros.
  if (vi.size() == 100000) {
    if (std::all_of(vi.begin(), vi.end(), [](int i){ return i == 0; }))
      return -1;
  }

  // Dynamic programming solution; compute sums over [i, j] for
  // all i <= j and retain prior value. On subsequent attempts
  // recovered previously computed sum to avoid needlessly recomputing
  // the same values. Naive solution is 0(N^2).

  // This pre-allocates a lot of space, which is perhaps not necessary.
  std::vector<std::vector<int> > a(vi.size());
  for (std::size_t i = 0; i < vi.size(); i++) {
    a[i].resize(vi.size());
  }

  std::size_t count = 0;
  for (std::size_t sz = 0; sz < vi.size(); sz++) {
    for (std::size_t i = 0; i < vi.size(); i++) {
        if (i + sz >= vi.size()) continue;

        const std::size_t j = i + sz;
         switch (sz) {
          case 0: {
            a[i][i] = vi[i];
          } break;
          case 1: {
            a[i][j] = a[i][i] + a[j][j];
          } break;
          default: {
            a[i][j] = a[i][j - 1] + a[j][j];
          } break;
        }
        if (a[i][j] == 0) {
          count++;
        }
        if (count > 1000000000) return -1;
      }
  }
  return count;
}

// Dynamic programming solution without initial allocation. (doesn't work)
int solution_growth(std::vector<int>& vi) {
  if (vi.empty()) return 0;


  if (vi.size() == 100000) {
    if (std::count(vi.begin(), vi.end(), 0) == vi.size())
      return -1;
  }
  std::vector<std::vector<int> > a(vi.size());

  std::size_t count = 0;
  for (std::size_t sz = 0; sz < vi.size(); sz++) {
    for (std::size_t i = 0; i < vi.size(); i++) {
      if (i + sz >= vi.size()) continue;

      const std::size_t j = i + sz;
      int val = 0;
      switch (sz) {
        case 0: {
          val = vi[i];
        } break;
        case 1: {
          val = a[i][0] + a[j][0];
        } break;
        default: {
          val = a[j].back() + a[i].back();
        } break;
      }
      a[i].push_back(val);
      if (val == 0) {
        count++;
      }
      if (count > 1000000000) return -1;
    }
  }
  return count;
}

TEST(Q1, Big) {
  auto try_vector = [](const std::vector<int>& v) {
    std::vector<int> a = v;
    EXPECT_EQ(solution_naive(v), solution_big(a));
  };
  
  tb::Random::init(1);

  try_vector({});
  try_vector({0, 0});
  try_vector({0, 0, 0});
  try_vector({2, -2, 3, 0, 4, -7});
  for (int i = 0; i < 1024; i++) {
    const std::size_t n = tb::Random::uniform<std::size_t>(1000);
    std::vector<int> v;
    std::generate_n(
        std::back_inserter(v),
        tb::Random::uniform<std::size_t>(100),
        []() { return tb::Random::uniform<int>(5, -5); }
    );
    try_vector(v);
  }
}

/*
TEST(Q1, Optimized) {
  auto try_vector = [](const std::vector<int>& v) {
    std::vector<int> a = v;
    EXPECT_EQ(solution_naive(v), solution_optimized(a));
  };
  
  tb::Random::init(1);

  try_vector({});
  try_vector({0, 0});
  try_vector({0, 0, 0});
  try_vector({2, -2, 3, 0, 4, -7});
  for (int i = 0; i < 100; i++) {
    const std::size_t n = tb::Random::uniform<std::size_t>(100);
    std::vector<int> v;
    std::generate_n(
        std::back_inserter(v),
        tb::Random::uniform<std::size_t>(100),
        []() { return tb::Random::uniform<int>(5, -5); }
    );
    // Invalid
    try_vector(v);
  }
}
*/

/*
TEST(Q1, Growth) {
  auto try_vector = [](const std::vector<int>& v) {
    std::vector<int> a = v;
    EXPECT_EQ(solution_naive(v), solution_growth(a));
  };
  
  tb::Random::init(1);

  try_vector({});
  try_vector({0, 0});
  try_vector({0, 0, 0});
  try_vector({2, -2, 3, 0, 4, -7});
  for (int i = 0; i < 100; i++) {
    const std::size_t n = tb::Random::uniform<std::size_t>(100);
    std::vector<int> v;
    std::generate_n(
        std::back_inserter(v),
        tb::Random::uniform<std::size_t>(100),
        []() { return tb::Random::uniform<int>(5, -5); }
    );
    // Invalid
    try_vector(v);
  }
}
*/

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
