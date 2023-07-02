// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

/*
 * Simple benchmark that runs a mixture of point lookups and inserts on ALEX.
 */


#include <experimental/algorithm>
#include <vector>
#include <iomanip>
#include <iostream>
#include <random>
#include <chrono> //For system_clock
#include <bits/stdc++.h>
#include <map>
#include <numeric>
#include <tuple>
#include <stdlib.h>
#include <sstream>
#include <iterator>

#include "absl/strings/str_format.h"
#include "ortools/base/logging.h"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"
#include "ortools/algorithms/knapsack_solver.h"

#include "flags.h"
#include "utils.h"
#include "alex_nodes.h"
#include "attack.h"                             /*Rui:our attack logics*/
#include "alex.h"
#include "leap_util.h"

// Modify these if running your own workload
#define KEY_TYPE double
#define PAYLOAD_TYPE double

//sample szie for lookup

using namespace std;

/*
 * Required flags:
 * --keys_file              path to the file that contains keys
 * --keys_file_type         file type of keys_file (options: binary or text)
 * --init_num_keys          number of keys to bulk load with
 * --total_num_keys         total number of keys in the keys file
 * --batch_size             number of operations (lookup or insert) per batch
 *
 * Optional flags:
 * --insert_frac            fraction of operations that are inserts (instead of
 * lookups)
 * --lookup_distribution    lookup keys distribution (options: uniform or zipf)
 * --time_limit             time limit, in minutes
 * --print_batch_stats      whether to output stats for each batch
 */



int main(int argc, char* argv[]) {

    auto flags = parse_flags(argc, argv);
    std::string keys_file_path = get_required(flags, "keys_file");
    std::string keys_file_type = get_required(flags, "keys_file_type");
    auto total_num_keys = stoi(get_required(flags, "total_num_keys"));
    auto num_actions = stoi(get_required(flags, "num_action"));
    auto dataset_type = stoi(get_required(flags, "dataset_type"));
    auto budget = stoi(get_required(flags, "budget"));
    auto init_perc = stoi(get_required(flags, "init_perc"));







    // Read keys from file
    auto keys = new KEY_TYPE[total_num_keys];
    if (keys_file_type == "binary") {
        load_binary_data(keys, total_num_keys, keys_file_path);
    } else if (keys_file_type == "text") {
        load_text_data(keys, total_num_keys, keys_file_path);
    } else {
        std::cerr << "--keys_file_type must be either 'binary' or 'text'"
            << std::endl;
        return 1;
    }



    int num_poisoning_keys = total_num_keys * (static_cast<double> (budget) / 100);
    
    // 0 : using bulkload() to build alex index
    // !0 : bulkload() + insert() to build a running index before insert poisoning keys

    // Combine bulk loaded keys with randomly generated payloads
    auto values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[total_num_keys];
    std::mt19937_64 gen_payload(std::random_device{}());


    /*
       vector<KEY_TYPE> sampled_keys;
       std::experimental::sample(keys, keys + total_num_keys, std::back_inserter(sampled_keys),
       init_num_keys, std::mt19937{std::random_device{}()});*/

    for (int i = 0; i < total_num_keys; i++) {
        values[i].first = keys[i];
        //values[i].first = sampled_keys[i];
        values[i].second = static_cast<PAYLOAD_TYPE>(gen_payload());
    }

    // Create ALEX and bulk load
    alex::Alex<KEY_TYPE, PAYLOAD_TYPE> index;
    std::sort(values, values + total_num_keys,
            [](auto const& a, auto const& b) { return a.first < b.first; });
    index.bulk_load(values, total_num_keys);

    int num_keys_after_bulk_load = index.stats_.num_keys;
    
    Stats s;
    vector<double> chosen_node_density;
    vector<KEY_TYPE> poisoning_keys, substitude_poisoning_keys;
    chosen_node_density.clear();
    poisoning_keys.clear();

    long long before_model_mem = index.model_size();
    long long before_data_mem = index.data_size();
    int total_dns = index.stats_.num_data_nodes;

    vector<alex::AlexDataNode<KEY_TYPE, PAYLOAD_TYPE>*> dns;
    index.get_all_datanodes(dns);


    /*
    string file_path = "/home/ubuntu/data/attack_out/out/whitebox_dns_dd_"+to_string(init_perc)+"_"+to_string(budget)+".txt";
    ofstream out_file;
    out_file.open(file_path, std::ios_base::app);

    for(int i = 0; i < dns.size(); i++) {
        out_file << i<<","
            <<dns[i]->data_capacity_<<","
            <<dns[i]->num_keys_ / static_cast<double> (dns[i]->data_capacity_)
            <<"\n";
    }

    out_file.close();*/




    attack::launch_mck_attack(index,
            num_poisoning_keys,
            num_actions,
            0,
            s,
            chosen_node_density,
            poisoning_keys,
            0);


    dns.clear();
    index.get_all_datanodes(dns);

    if(index.stats_.num_keys < total_num_keys + num_poisoning_keys) {
        attack::insert_left_keys(dns,
                index,
                total_num_keys + num_poisoning_keys - index.stats_.num_keys,
                poisoning_keys,
                chosen_node_density,
                substitude_poisoning_keys);
    }

    long long model_mem = index.model_size();
    long long data_mem = index.data_size();

    dns.clear();
    index.get_all_datanodes(dns);


    int sum = 0;

    for(int i = 0; i < dns.size(); i++) {
        sum += dns[i]->num_resizes_;
    }

    int performend_num_actions = index.stats_.num_downward_splits +
        index.stats_.num_sideways_splits +
        sum +
        index.stats_.num_expand_and_retrains +
        index.stats_.num_expand_and_scales;

    cout<<dataset_type<<","
        <<total_num_keys<<","
        <<num_keys_after_bulk_load<<","
        <<index.stats_.num_keys<<","
        <<budget<<","
        <<num_poisoning_keys<<","
        <<total_dns<<","
        <<num_actions<<","
        <<before_model_mem<<","
        <<model_mem<<","
        <<before_data_mem<<","
        <<data_mem<<","
        <<s.total_expected_mem<<","
        <<s.total_mem<<","
        <<s.mck_num_keys<<","
        <<s.exp_num_actions<<","
        <<s.aut_num_actions<<","
        <<s.mck_solution_flag<<","
        <<performend_num_actions
        <<"\n";


    delete[] keys;
    delete[] values;
}
