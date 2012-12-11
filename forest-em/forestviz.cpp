#define BOOST_AUTO_TEST_MAIN
#define GRAEHL__SINGLE_MAIN
#define GRAPHVIZ_SYMBOL
#define MAX_FOREST_NODES 10000000

//#define BENCH
#include <graehl/shared/config.h>

#include <graehl/shared/myassert.h>
#include <boost/config.hpp>
#include <graehl/shared/tree.hpp>
#include <graehl/shared/symbol.hpp>
#include <graehl/forest-em/forest.hpp>
#include <iostream>
#include <boost/program_options.hpp>
#include <fstream>
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/genio.h>
#include <graehl/shared/input_error.hpp>

#ifndef GRAEHL_TEST
using namespace boost;
using namespace std;
using namespace boost::program_options;
using namespace graehl;

const char *docstring="On the output file, run (from http://graphviz.org/:\n  dot -Tps forest.dot -o forest.ps\nor dot -Tpng forest.dot -o forest.png\n";

#define ABORT return 1
int
#ifdef _MSC_VER
__cdecl
#endif
main(int argc, char *argv[])
{
    INITLEAK;
    cin.tie(0);

    istream_arg forest_in;
    ostream_arg graphviz_out;
    const unsigned MEGS=1024*1024;
    unsigned max_megs=(MAX_FOREST_NODES*sizeof(ForestNode)+MEGS-1)/MEGS;
    string prelude;
    options_description options("General options");
    bool help,same_rank,number_edges,pointer_nodes;

    options.add_options()
        ("help,h", bool_switch(&help),"produce help message")
        ("number-children,n",bool_switch(&number_edges),"label child edges 1,2,...,n")
        ("pointer-nodes,p",bool_switch(&pointer_nodes),"show dot-shaped pointer nodes for shared subforests")
        ("same-rank-children,s",bool_switch(&same_rank),"force children nodes to the same rank (implies -p)")
        ("graphviz-prelude,g", value<string>(&prelude), "graphviz .dot directives to be inserted in output graphs")
        ("forest-megs,M", value<unsigned>(&max_megs)->default_value(100), "Number of megabytes reserved to hold forests")
        ("in-forest-file,i",value<istream_arg>(&forest_in)->default_value(stdin_arg(),"STDIN"),"file containing one or more forests e.g. (OR (1 4) (1 3)) (1 #1(4 3) OR(#1 2))")
        ("out-graphviz-file,o",value<ostream_arg>(&graphviz_out)->default_value(stdout_arg(),"STDOUT"),"Output Graphviz .dot file (run dot -Tps out.dot -o out.ps)");

    positional_options_description pos;
    pos.add("in-forest-file", -1);
    const char *progname=argv[0];
    try {
        variables_map vm;
        store(command_line_parser(argc, argv).options(options).positional(pos).run(), vm);
        notify(vm);
#define RETURN(i) retval=i;goto done
        if (help) {
            cerr << progname << ":\n";
            cerr << options << "\n";
            cerr << "\n" << docstring << "\n";
            ABORT;
        }
        if (same_rank)
            pointer_nodes=true;
        unsigned n_nodes=max_megs*MEGS/sizeof(ForestNode);
        auto_array<ForestNode> fnodes(n_nodes);
        Forest f;

        ForestVizPrinter<> fviz(*graphviz_out,prelude);

        bool first=true;
        while (*forest_in) {
            f.reset(fnodes.begin(),fnodes.end());
            *forest_in >> f;
            if (forest_in->bad() || f.ran_out_of_space()) {
                cerr << "Error reading input forest:\n";
                show_error_context(*forest_in,cerr);
                ABORT;
            }

            if (*forest_in) {
                if (first)
                    first=false;
                else
                    fviz.next();
                fviz.print(f,same_rank,number_edges,pointer_nodes);
            }
        }
    } catch(std::exception& e) {
        cerr << "ERROR: " << e.what() << "\n\n";
        cerr << options << "\n";
        ABORT;
    }
    return 0;
}


#endif

