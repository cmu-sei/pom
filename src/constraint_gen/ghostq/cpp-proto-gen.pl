#!/usr/bin/perl
$CurClass = "";
while (<>) {
    if (m/^([a-zA-Z0-9_<,>].*\**\&?) ([A-Za-z0-9_]+)(\<[^ ]*\>)?[:][:]([a-zA-Z0-9_]+[(].*[)]) [{] *$/) {
        if ($CurClass ne $2) {
            if ($CurClass) {print "#endif\n";}
            $CurClass = $2;
            print "#ifdef FPROTO_$2\n";
        }
        print "    $1 $4;\n";
    }
}
if ($CurClass) {print "#endif\n";}
