//
# <legal>
# Pointer Ownership Model (POM) Source Code Release
# 
# Copyright 2025 Carnegie Mellon University.
# 
# NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
# INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
# UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR
# IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF
# FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS
# OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT
# MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,
# TRADEMARK, OR COPYRIGHT INFRINGEMENT.
# 
# Licensed under a MIT (SEI)-style license, please see license.txt or
# contact permission@sei.cmu.edu for full terms.
# 
# [DISTRIBUTION STATEMENT A] This material has been approved for public
# release and unlimited distribution.  Please see Copyright notice for
# non-US Government use and distribution.
# 
# DM25-1262
# </legal>
//


use std::env;

fn main() {
    let p1 = Box::new(vec!["Tom"]);
    let p2 = Box::new(vec!["Dick", "Harry"]);
    let q: Box<Vec<&str>>;

    if env::args().len() % 2 == 0 {
        q = p1;
        /* Here, p1 is moved and p2 is good. */
    } else {
        q = p2;
        /* Here, p2 is moved and p1 is good. */
    }
    // Compiler error E0382 on both p1 & p2
    // println!("*p1 = {}, *p2 = %{}", p1.len(), p2.len());
    drop(q);
    // p1, p2 properly cleaned up here; can't set 'q = NULL'.
    // Compiler error E0382 on both p1 & p2 again
    // drop(p1);
    // drop(p2);
}
