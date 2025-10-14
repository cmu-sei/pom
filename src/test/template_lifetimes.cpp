/*
 * Lifetime example
 *
 * See the design.md document for discussion about this code.
 *
// <legal>
// Pointer Ownership Model (POM) Source Code Release
// 
// Copyright 2025 Carnegie Mellon University.
// 
// NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
// INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
// UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR
// IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF
// FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS
// OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT
// MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,
// TRADEMARK, OR COPYRIGHT INFRINGEMENT.
// 
// Licensed under a MIT (SEI)-style license, please see license.txt or
// contact permission@sei.cmu.edu for full terms.
// 
// [DISTRIBUTION STATEMENT A] This material has been approved for public
// release and unlimited distribution.  Please see Copyright notice for
// non-US Government use and distribution.
// 
// DM25-1262
// </legal>
 */


#include <vector>
#include <iostream>

using namespace std;
vector<int*> full_list;
vector<int*> sub_list;


int *read_int() {
  int *p = new int();
  cin >> *p;
  return p;
}

int main() {
  while (1) {
    char cmd;
    cout << "Cmd: " << endl;
    cin >> cmd;
    switch (cmd) {

    case 'D': // Display
      cout << "Full list:" << endl;
      for (size_t i=0; i < full_list.size(); i++) {
        cout << i << ":" << *(full_list[i]) << endl;
      }
      cout << "Sub list:" << endl;
      for (size_t i=0; i < sub_list.size(); i++) {
        cout << i << ":" << *(sub_list[i]) << endl;
      }
      break;

    case 'A': // Add
      cout << "Name: " << endl;
      full_list.push_back(read_int());
      break;

    case 'C': // Copy to sublist
      {
        cout << "Item number:" << endl;
        int *i = read_int();
        if (*i >= full_list.size()) {
          cout << "Invalid number!" << endl;
          continue;
        }
        sub_list.push_back(full_list[*i]);
        delete i;
      }
      break;

    case 'R': // Rename
      {
        cout << "Item number:" << endl;
        int *i = read_int();
        if (*i >= full_list.size()) {
          cout << "Invalid number!" << endl;
          continue;
        }
        cout << "New name:" << endl;
        full_list[*i] = read_int();
        delete i;
      }
      break;

    case EOF:
    case 'Q': // Quit
      // TODO free pointers, but difficult to do correctly
      exit(0);

    default:
      cout << "Unrecognized command: " << cmd << endl;
    }
  }

}
