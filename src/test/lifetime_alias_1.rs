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


use std::vec;
use std::boxed::Box;

fn foo(p1: Box<Vec<&'static str>>, p2: &Vec<&'static str>) -> Vec<&'static str> {
    let x: Vec<&'static str>;
    #[cfg(feature = "has_bug")]
    {
        drop(p1);
        x = p2.clone();
    }
    #[cfg(not(feature = "has_bug"))]
    {
        x = p2.clone();  // clone() necessary b/c Vec<> is not Copy
        drop(p1);
    }
    x
}

//fn kill(victim: &Vec<&str>)  { drop(*victim);} // E0507, b/c *victim is not Copy

fn main() {
    let p = Box::new(vec!["Tom", "Dick", "Harry"]);

    // Immutable reference
    let p2: &Vec<&str> = &*p;
    println!("length: {}", p2.len());

    let result = foo(p, p2); // E0509: cannot move p (into fn) as it is borrowed (eg has existing references)
    println!("The result is {}", result.len());
}
