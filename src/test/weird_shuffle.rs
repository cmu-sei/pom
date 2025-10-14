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


use std::fs::File;
use std::io::Read;

fn random_bool() -> bool {
    let mut file = File::open("/dev/urandom").unwrap();
    let mut buf = [0u8; 1];
    file.read_exact(&mut buf).unwrap();
    buf[0] & 1 == 1
}

fn weird_shuffle<'a, 'b, 'c, 'd, 'e>(
    p_a: &mut &'a i32,
    p_b: &mut &'b i32,
    p_c: &mut &'c i32,
    p_d: &mut &'d i32,
    p_e: &mut &'e i32,
) where
    'b: 'a,
    'c: 'b,
    'd: 'b,
    'd: 'e,
{
    if random_bool() {
        *p_a = *p_b;
    } else {
        *p_a = *p_c;
    }

    if random_bool() {
        *p_b = *p_c;
    } else {
        *p_b = *p_d;
    }

    if random_bool() {
        *p_e = *p_d;
    }
}

fn main() {
    let x3 = 3;
    let x4 = 4;
    let x5 = 5;
    let r3 = &x3; // longest lifetime 'c
    let r4 = &x4; // longest lifetime 'd
    let r5 = &x5; // longest lifetime 'e

    {
        let x2 = 2;
        let r2 = &x2; // medium lifetime 'b

        {
            let x1 = 1;
            let r1 = &x1; // shortest lifetime 'a

            let mut p1 = r1;
            let mut p2 = r2;
            let mut p3 = r3;
            let mut p4 = r4;
            let mut p5 = r5;

            println!("Before shuffle: p1={}, p2={}, p3={}, p4={}, p5={}", p1, p2, p3, p4, p5);
            weird_shuffle(&mut p1, &mut p2, &mut p3, &mut p4, &mut p5);
            println!("After shuffle:  p1={}, p2={}, p3={}, p4={}, p5={}", p1, p2, p3, p4, p5);
        }
    }
}
