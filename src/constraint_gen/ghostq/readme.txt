GhostQ is a QBF solver.  The input QBF problem must be in our custom
strict-CQBF format.  We include two Python scripts to convert the widely-used
QDIMACS and QPRO formats to our CQBF format.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.


MAIN FILES:
    ghostq.cpp: Source code for the QBF solver.  
    qcir-conv.py: Convert QDIMACS and loose-CQBF format to strict-CQBF or QCIR format.
    qpro-to-cqbf.py: Convert QPRO format to loose-CQBF format.

To compile GhostQ on Linux, use the included makefile.

Example:
    $ make
    $ python qcir-conv.py cmu.dme2.B-d3.qdimacs -o cmu.dme2.B-d3.cqbf -write-gq -quiet
    $ ./ghostq cmu.dme2.B-d3.cqbf -q1
Or:
    $ python qcir-conv.py cmu.dme2.B-d3.qdimacs -quiet -write-gq | ./ghostq - -q1

