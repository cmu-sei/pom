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


pub fn main() {
    let mut x = 5;
    let mut y = 5;
    let mut rx : &mut i32 = &mut x;
    let mut ry : &mut i32 = &mut y;
    let mut r2 : &mut &mut i32;
    let mut r3 : &mut i32;
    {
        r2 = &mut rx;

        r3 = *r2;
        *r3 = 42;
        // Above definition of r2 is dead at this point, but r3 is still alive.
        *rx += 1000;

        r2 = &mut ry;
        *r3 += 1;

        r3 = *r2;
        *r3 = 777;
        // Above definitions of r2 and r3 are dead at this point.
        *ry += 1000;
    }
    println!("{}", *rx);
    println!("{}", *ry);
}
