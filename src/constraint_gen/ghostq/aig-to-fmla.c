/***************************************************************************
Copyright (c) 2006-2011, Armin Biere, Johannes Kepler University.
Copyright (c) 2013 William Klieber

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
***************************************************************************/

#include "aiger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

static FILE *file;
static aiger *mgr;
static int count;

static const char *prev_prefix = "old<";
static const char *prev_suffix = ">";
static const char *next_prefix = "next<";
static const char *next_suffix = ">";

static int die(char* msg) {
    fprintf(stderr, "%s", msg);
    abort();
    return 0;
}

static void ps(const char *str)
{
    fputs(str, file);
}

static void pl(unsigned lit)
{
    const char *name;
    char ch;
    int i;

    if (lit == 0)
        fprintf(file, "false()");
    else if (lit == 1)
        fprintf(file, "true()");
    else if ((lit & 1))
        putc('-', file), pl(lit - 1);
    else {
        if (aiger_is_and(mgr, lit)) {
            fputs("$", file);
        }
        if (aiger_is_latch(mgr, lit)) {
            fputs(prev_prefix, file);
        }
        if ((name = aiger_get_symbol(mgr, lit))) {
            /* TODO: check name to be valid name */
            fputs(name, file);
        } else {
            if (aiger_is_input(mgr, lit)) {
                ch = 'i';
            } else if (aiger_is_latch(mgr, lit)) {
                ch = 'L';
            } else {
                assert(aiger_is_and(mgr, lit));
                ch = 'a';
            }

            for (i = 0; i <= count; i++) {
                fputc(ch, file);
            }

            fprintf(file, "%u", lit);
        }
        if (aiger_is_latch(mgr, lit)) {
            fputs(prev_suffix, file);
        }
    }
}

static void print_raw_latch(unsigned lit)
{
    const char* name;
    int i;
    if ((name = aiger_get_symbol(mgr, lit))) {
        /* TODO: check name to be valid name */
        fputs(name, file);
    } else {
        char ch = 'L';
        for (i = 0; i <= count; i++) {
            fputc(ch, file);
        }
        fprintf(file, "%u", lit);
    }
}

static int count_ch_prefix(const char *str, char ch)
{
    const char *p;

    assert(ch);
    for (p = str; *p == ch; p++);

    if (*p && !isdigit(*p))
        return 0;

    return p - str;
}

static void setupcount(void)
{
    const char *symbol;
    unsigned i;
    int tmp;

    count = 0;
    for (i = 1; i <= mgr->maxvar; i++) {
        symbol = aiger_get_symbol(mgr, 2 * i);
        if (!symbol)
            continue;

        if ((tmp = count_ch_prefix(symbol, 'i')) > count)
            count = tmp;

        if ((tmp = count_ch_prefix(symbol, 'L')) > count)
            count = tmp;

        if ((tmp = count_ch_prefix(symbol, 'o')) > count)
            count = tmp;

        if ((tmp = count_ch_prefix(symbol, 'a')) > count)
            count = tmp;
    }
}

int main(int argc, char **argv)
{
    const char *src, *dst, *error;
    int res, strip, bad;
    unsigned i, j;

    src = dst = 0;
    strip = 0;
    res = 0;
    bad = 0;

    for (i = 1; i < argc; i++) {
        int is_prev = (strcmp(argv[i], "-prev") == 0);
        int is_next = (strcmp(argv[i], "-next") == 0);
        if (strcmp(argv[i], "-h") == 0) {
            fprintf(stderr,
                    "usage: %s [options] [src [dst]]\n"
                    "options:\n"
                    "  -prev <p> <s>  Prefix and suffix for old value of latches.\n"
                    "  -next <p> <s>  Prefix and suffix for new value of latches.\n"
		    "  -b             Assume outputs are bad properties.\n"
                    "  -s             Strip symbols.\n", argv[0]);
            exit(0);
        }
        if (0) {
        } else if (strcmp(argv[i], "-b") == 0) {
            bad = 1;
        } else if (strcmp(argv[i], "-s") == 0) {
            strip = 1;
        } else if (is_prev || is_next) {
            if (i+2 >= argc) {
                fprintf(stderr, "*** [aig-to-fmla] arguments to '%s' missing\n", argv[i]);
                exit(1);
            }
            if (is_prev) {
                prev_prefix = argv[++i];
                prev_suffix = argv[++i];
            }
            if (is_next) {
                next_prefix = argv[++i];
                next_suffix = argv[++i];
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "*** [aig-to-fmla] invalid option '%s'\n",
                    argv[i]);
            exit(1);
        } else if (!src) {
            src = argv[i];
        } else if (!dst) {
            dst = argv[i];
        } else {
            fprintf(stderr, "*** [aig-to-fmla] too many files\n");
            exit(1);
        }
    }

    mgr = aiger_init();

    if (src)
        error = aiger_open_and_read_from_file(mgr, src);
    else
        error = aiger_read_from_file(mgr, stdin);

    if (error) {
        fprintf(stderr, "*** [aig-to-fmla] %s\n", error);
        res = 1;
    } else {
        if (dst) {
            if (!(file = fopen(dst, "w"))) {
                fprintf(stderr,
                        "*** [aig-to-fmla] failed to write to '%s'\n", dst);
                exit(1);
            }
        } else
            file = stdout;

        if (strip)
            aiger_strip_symbols_and_comments(mgr);
        else
            setupcount();

        ps("[\n");
        ps("[INPUTS [\n");
        for (i = 0; i < mgr->num_inputs; i++) {
            ps("  "); pl(mgr->inputs[i].lit); ps("\n");
        }
        ps("]]\n");
        ps("[DEFINE [\n");
        for (i = 0; i < mgr->num_ands; i++) {
            aiger_and *n = mgr->ands + i;
            unsigned rhs0 = n->rhs0;
            unsigned rhs1 = n->rhs1;
            ps("  ");
            pl(n->lhs);
            ps(":and(");
            pl(rhs0);
            ps(", ");
            pl(rhs1);
            ps(")\n");
        }
        ps("]]\n");
        ps("[LATCHES [\n");
        for (i = 0; i < mgr->num_latches; i++) {
            ps("  [");
            ps(next_prefix); 
            print_raw_latch(mgr->latches[i].lit);
            ps(next_suffix);
            ps(", ");
            pl(mgr->latches[i].next);
            ps("]\n");
        }
        ps("]]\n");

        ps("[OUTPUTS [\n");
        for (i = 0; i < mgr->num_outputs; i++) {
            ps("  [");
            for (j = 0; j <= count; j++) {
                putc('o', file);
            }
            fprintf(file, "%u", i);
            ps(", ");
            pl(mgr->outputs[i].lit);
            ps("]\n");
        }
        ps("]]\n");

        ps("[SPEC_AG [\n");
        if (bad) {
            for (i = 0; i < mgr->num_outputs; i++) {
                fprintf(file, "  ");
                pl(aiger_not(mgr->outputs[i].lit));
                fprintf(file, "\n");
            }
        }
        for (i = 0; i < mgr->num_bad; i++) {
            ps("  ");
            pl(mgr->bad[i].lit);
            fprintf(file, "\n");
        }
        ps("]]\n");

        for (i = 0; i < mgr->num_constraints; i++) {
            die("Invariant contraints are not supported!\n");
        }

        for (i = 0; i < mgr->num_justice; i++) {
            die("Justice contraints are not supported!\n");
        }

        for (i = 0; i < mgr->num_fairness; i++) {
            die("Fairness contraints are not supported!\n");
        }

        ps("]\n");

        if (dst) {
            fclose(file);
        }
    }

    aiger_reset(mgr);

    return res;
}
