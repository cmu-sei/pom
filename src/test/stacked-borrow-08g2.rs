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


#![allow(unused_mut)]
#![allow(unused_variables)]

use std::fs::File;
use std::io::Read;

fn random_bool() -> bool {
    let mut file = File::open("/dev/urandom").unwrap();
    let mut buf = [0u8; 1];
    file.read_exact(&mut buf).unwrap();
    buf[0] & 1 == 1
}

fn main() {
    let mut x:i32 = 1;
    let mut y:i32 = 2;

    let mut rx : &mut i32 = &mut x;
    let mut ry : &mut i32 = &mut y;

    let mut rrx = &mut rx;
    let mut rry = &mut ry;

    let mut r1 : &mut i32;
    let mut rr2 : &mut &mut i32;

    if random_bool() {
        r1 = *rrx;
        rr2 = rry;
    } else {
        r1 = *rry;
        rr2 = rrx;
    }
    *r1 = 3;
    **rr2 = 4;
    print!("x = {}, y = {}\n", x, y);
}


/*
fn main() {
    let mut x:i32 = 1;
    let mut y:i32 = 2;

    let mut r1 : &mut i32;
    let mut r2 : &mut i32;

    if random_bool() {
        r1 = &mut x;
        r2 = &mut y;
    } else {
        r1 = &mut y;
        r2 = &mut x;
    }
    *r1 = 3;
    *r2 = 4;
    print!("x = {}, y = {}\n", x, y);
}
*/
