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
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
using std::string;

using Coord = std::pair<char, char>;

struct Ship {
  std::vector<Coord> cs;
};

Ship parse_shipdef(int N, const string& S) {
  std::stringstream ss(S);
  std::string s;

  // Populate coords with the [0] = North West, [1] = South East.
  std::vector<std::string> coords;
  while (std::getline(ss, s, ' ')) {
      coords.push_back(s);
  }

  // Note: need not concern validity as positions are constrained
  // to correct locations on the battlefield.

  const char start_y = coords[0][0];
  const char end_y = coords[1][0];

  const char start_x = coords[0][1];
  const char end_x = coords[1][1];

  // Enumerate all valid ship positions as per the starting, ending
  // locations. For simplicity just keep the coordinates in ASCII.
  Ship ship;
  for (char y = start_y; y <= end_y; y++) {
    for (char x = start_x; x <= end_x; x++) {
      ship.cs.push_back(std::make_pair(y, x));
    }
  }
  return ship;
}

std::vector<Ship> parse_shipset(int N, const string& S) {
  std::stringstream ss(S);
  std::string x;

  // Ship definition is a coordinate pair
  std::vector<Ship> ships;
  while (std::getline(ss, x, ',')) {
    ships.push_back(parse_shipdef(N, x));
  }
  return ships;
}

std::vector<Coord> parse_hitlist(int N, string& T) {
  std::stringstream ss(T);
  std::string s;

  // Hitlist is a whitespace separated list of strings [digit][char].
  std::vector<Coord> h;
  while (std::getline(ss, s, ' ')) {
    h.push_back(std::make_pair(s[0], s[1]));
  }
  return h;
}

std::vector<Ship> apply_hits(const std::vector<Ship>& ships, std::vector<Coord>& hitlist) {
  std::vector<Ship> result = ships;

  // Enumerate all hits
  for (const Coord& hit : hitlist) {
    // For all ships:
    for (Ship& ship : result) {
      std::vector<Coord>& cs = ship.cs;
      // ...identify if a hit has actually taken place
      auto it = std::find(cs.begin(), cs.end(), hit);
      if (it != ship.cs.end()) {
        // A hit has been found, delete this position.,
        cs.erase(it);
      }
    }
  }
  // Return the pruned coordinate set.
  return result;
}

string report(const std::vector<Ship>& ships, const std::vector<Ship>& aftermath) {
  int sunk = 0, hit = 0;
  for (std::size_t i = 0; i < ships.size(); i++) {
    const Ship& original = ships[i];
    const Ship& after = aftermath[i];
    if (after.cs.empty()) {
      // A ship once existed, but all valid coordinates have been pruned bit hits.
      // The ship is sunk, it is no-more, it is an EX-SHIP, the ship is deceased,
      // it has ceased-to-be.
      sunk++;
    } else if (original.cs.size() != after.cs.size()) {
      // Ship remains present, but some coordinates have been removed such that
      // it is now partial. It has been hit.
      hit++;
    }
  }
  // Construct final string.
  std::stringstream ss;
  ss << sunk << "," << hit;
  return ss.str();
}

// General solution:
//
//  1) Parse ship- and hit-lists.
//  2) Enumerate all coordinates occupied by a ship.
//  3) Prune away the coordinates where a hit has landed.
//  4) For ships with no remaining coordinates, ship has been sunk.
//  5) For ships with fewer coordinates than had originally been present,
//     ship was hit, but not sunk.
string solution(int N, string &S, string &T) {
  // Parse ship list 'S'. 'ships' contains all positions occupied by all ships.
  std::vector<Ship> ships = parse_shipset(N, S);
  // Parse hit list 'T'; 'hitlist' contains the positions that were hit.
  std::vector<Coord> hitlist = parse_hitlist(N, T);
  // Compute the delta, where the positions containing ships have been removed
  // if hit.
  std::vector<Ship> aftermath = apply_hits(ships, hitlist);
  // Stringify final result.
  return report(ships, aftermath);
}


TEST(Q3, Basic) {
  {
    int N = 4;
    std::string S = "1B 2C,2D 4D";
    std::string T = "2B 2D 3D 4D 4A";

    EXPECT_EQ(solution(N, S, T), "1,1");
  }
  {
    int N = 3;
    std::string S = "1A 1B,2C 2C";
    std::string T = "1B";

    EXPECT_EQ(solution(N, S, T), "0,1");
  }
  {
    int N = 12;
    std::string S = "1A 2A,12A 12A";
    std::string T = "12A";

    EXPECT_EQ(solution(N, S, T), "1,0");
  }
}

int main(int argc, char**argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
