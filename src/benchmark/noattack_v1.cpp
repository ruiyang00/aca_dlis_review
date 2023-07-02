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
using namespace attack;

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
    auto budget = stoi(get_required(flags, "budget"));
    auto dataset_type = stoi(get_required(flags, "dataset_type"));
    int total_num_keys = stoi(get_required(flags, "total_num_keys"));
    auto init_perc = stoi(get_required(flags, "init_perc"));


    int num_poisoning_keys = total_num_keys * (static_cast<double> (budget) / 100);



    // Read keys from file
    auto keys = new KEY_TYPE[total_num_keys + num_poisoning_keys];
    if (keys_file_type == "binary") {
        load_binary_data(keys, total_num_keys + num_poisoning_keys, keys_file_path);
    } else if (keys_file_type == "text") {
        load_text_data(keys, total_num_keys + num_poisoning_keys, keys_file_path);
    } else {
        std::cerr << "--keys_file_type must be either 'binary' or 'text'"
            << std::endl;
        return 1;
    }

    // Combine bulk loaded keys with randomly generated payloads
    auto values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[total_num_keys];
    std::mt19937_64 gen_payload(std::random_device{}());

    

    for (int i = 0; i < total_num_keys; i++) {
        values[i].first = keys[i];
        values[i].second = static_cast<PAYLOAD_TYPE>(gen_payload());
    }


    // Create ALEX and bulk load
    alex::Alex<KEY_TYPE, PAYLOAD_TYPE> index;
    std::sort(values, values + total_num_keys,
            [](auto const& a, auto const& b) { return a.first < b.first; });
    index.bulk_load(values, total_num_keys);
    cout<<"check-1\n";

    int num_keys_after_bulk_load = index.stats_.num_keys;

    long long before_model_mem = index.model_size();
    long long before_data_mem = index.data_size();
    int dns_before = index.stats_.num_data_nodes;


    //simulate the whitebox stop at the tail part 
    for(int i = total_num_keys; i < total_num_keys + num_poisoning_keys; i++){
        index.insert(keys[i], static_cast<PAYLOAD_TYPE>(gen_payload()));
    }

    vector<alex::AlexDataNode<KEY_TYPE, PAYLOAD_TYPE>*> dns;
   
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
        <<dns_before<<","
        <<before_model_mem<<","
        <<model_mem<<","
        <<before_data_mem<<","
        <<data_mem<<","
        <<dns_before<<","
        <<performend_num_actions
        <<"\n";



    delete[] keys;
    delete[] values;

}
