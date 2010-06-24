#include <graehl/forest-em/forest-em.hpp>
#include <graehl/forest-em/forest-em-params.hpp>
#include <stdexcept>
#include <graehl/shared/random.hpp>
#include <graehl/shared/fileheader.hpp>
#include <graehl/shared/debugprint.hpp>

namespace graehl {
void
ForestEmParams::run()
{
    //faster compile for debug; always use float
#ifndef DEBUG
    if (double_precision)
        perform_forest_em<double>();
    else
#endif
        perform_forest_em<float>();
}

void
ForestEmParams::validate_parameters()
{
    gopt.validate();
    log_stream=log_file.get();
    if (!is_default_log(log_file)) // tee to cerr
    {
        teebufptr.reset(new teebuf(log_stream->rdbuf(),std::cerr.rdbuf()));
        teestreamptr.reset(log_stream=new std::ostream(teebufptr.get()));
    }

   #ifdef DEBUG
    DBP::set_logstream(&log());
    DBP::set_loglevel(log_level);
   #endif
    log() << "Parsing command line: " << cmdline_str << "\n\n";

// Random seed:
    log() << "Using random seed --random-seed="<<random_seed<<"\n";
    set_random_seed(random_seed);

    // forest-em watch/checkpoint logic:
    if (checkpoint_prefix == "") {
        checkpoint_parameters=false;
        viterbi_enable=false;
        per_forest_counts_enable=false;
    } else {
        viterbi_enable = viterbi_per != 0;
        per_forest_counts_enable = per_forest_counts_per != 0;
    }
    count_report_enable = !(count_report_threshold.isInfinity() && prob_report_threshold.isInfinity());


    // forest-em files
    if (byid_output_file && !byid_rule_file)
        throw std::runtime_error("Must provide byid-rule-file.");
    if (max_iter && !forests_file)
        throw std::runtime_error("Missing forests-file.");
    if (!normgroups_file && (max_iter || normalize_initial))
        throw std::runtime_error("Missing normgroups-file.\n");
}

template <class Float>
void
ForestEmParams::perform_forest_em()
{
    typedef FForests<Float> Forests;

    typedef logweight<Float> W;

// Weight cosmetics:
    if (human_probs) {
        log()<<"Human readable (may overflow) probs, not e^log(prob).\n";
        W::default_never_log();
    } else {
        W::default_always_log();
//            W::out_sometimes_log(cerr);
        W::out_sometimes_log(log());
    }
    W::default_exp();

    Forests forests(gopt,max_forest_nodes,max_normgroup_size,forest_tick_period,tempfile_prefix,log(),rules_file,log_level);
    log() << "Using " << *this << "\n\n";
    if (initparam_file) {
        forests.prealloc_params(prealloc_params);
        forests.read_params(*initparam_file);
    }

    if (normgroups_file) {
        forests.read_norm_groups(*normgroups_file);
        if (!*normgroups_file) {
            throw std::runtime_error("Expected normalization groups list e.g. ((1 2 3) (4 5) (6)),\n"
                  "followed by any number of forests, e.g. (OR #1(1 2 3) #1)\n");
        }
        if (initparam_file && normalize_initial)
            forests.normalize();
    }

    if (log_level)
        forests.print_info(log());

    if (forests_file) {
        forests.read_forests(*forests_file);
        if (log_level) {
            forests.print_stats(log());
            log() << std::endl;
        }
/*        forests.prepare_em(prior_counts,checkpoint_prefix,checkpoint_parameters,watch_rule,watch_period,watch_depth,
                           (bool)random_restarts,count_report_enable,count_report_threshold,prob_report_threshold,viterbi_enable,viterbi_per,initial_1_params,add_k_smoothing,per_forest_counts_enable,per_forest_counts_per,zero_zerocounts);
*/
        forests.prepare(*this);
        if (gopt.iter)
            forests.run_gibbs();
        else if (max_iter)
            overrelaxed_em(forests,max_iter,converge_ratio,random_restarts,converge_delta,1,log(),log_level);
    }


    if (outparam_file)
        forests.write_params(*outparam_file,outparam_file.name);

    if (outcounts_file)
        forests.write_counts(*outcounts_file,outcounts_file.name);

    if (outviterbi_file || out_per_forest_counts_file || out_score_per_forest) {
        if (outviterbi_file)
            forests.prep_final_viterbi(*outviterbi_file);
        if (out_per_forest_counts_file)
            forests.prep_final_per_forest_counts(*out_per_forest_counts_file);
        if (out_score_per_forest)
            forests.prep_final_per_forest_inside(*out_score_per_forest);
        forests.final_iteration();
    }

    if (byid_output_file) {
        if (!byid_rule_file)
            throw std::runtime_error("Must provide byid-rule-file.");
        std::string save_partial_match=copy_header(*byid_rule_file,*byid_output_file,rule_format_version);
        *byid_output_file << " " << *this << "\n";
        *byid_output_file << save_partial_match; // we know this won't have any id=N in it because two characters is too early (header is $$$)

        forests.write_params_byid(*byid_rule_file,*byid_output_file,byid_prob_field,byid_count_field);
    }
}
}
