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
#define KEY_TYPE unsigned long long 
#define PAYLOAD_TYPE unsigned long long

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



    //read flags
    auto flags = parse_flags(argc, argv);
    auto key_file = get_required(flags, "key_file");
    std::string keys_file_type = get_required(flags, "keys_file_type");
    auto num_actions = stoi(get_required(flags, "num_action"));
    auto budget = stoi(get_required(flags, "budget"));
    auto init_perc = stoi(get_required(flags, "init_perc"));
    auto kde_type = get_required(flags, "kde_type");

    auto total_num_keys = stoi(get_required(flags, "total_num_keys"));
    auto dataset_type=stoi(get_required(flags, "dataset_type"));
    string sub_key_file = get_required(flags, "sub_key_file");
    auto h = get_required(flags, "h");

    std::mt19937_64 gen_payload(std::random_device{}());
    std::vector<alex::AlexDataNode<KEY_TYPE, PAYLOAD_TYPE>*> dns;
    alex::Alex<KEY_TYPE, PAYLOAD_TYPE> *target_index, *sub_index;
    vector<double> chosen_node_density;
    vector<KEY_TYPE> poisoning_keys;
    Stats s;


    int num_poisoning_keys = total_num_keys * (static_cast<double> (budget) / 100);
    int init_num_keys = total_num_keys * (static_cast<double> (init_perc) / 100);

    // key arrays
    auto keys = new KEY_TYPE[total_num_keys];
    auto sub_keys = new KEY_TYPE[total_num_keys];
    auto values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_num_keys];

    // read original dataset
    if (keys_file_type == "binary") {
        load_binary_data(keys, total_num_keys, key_file);
    } else if (keys_file_type == "text") {
        load_text_data(keys, total_num_keys, key_file);
    } else {
        std::cerr << "--key_type must be either 'binary' or 'text'"
            << std::endl;
        return 1;
    }

    load_binary_data(sub_keys, total_num_keys, sub_key_file);

    target_index = new alex::Alex<KEY_TYPE, PAYLOAD_TYPE>();
    sub_index = new alex::Alex<KEY_TYPE, PAYLOAD_TYPE>();



    for(int i = 0; i < init_num_keys; i++) {
        values[i].first = keys[i];
        values[i].second = static_cast<PAYLOAD_TYPE>(gen_payload());
    }

    //compare unsort keys using bulk_load()
    std::sort(values, values + init_num_keys,
            [](auto const& a, auto const& b) { return a.first < b.first; });
    target_index->bulk_load(values, init_num_keys);


    for(int i = init_num_keys; i < total_num_keys - num_poisoning_keys; i++) {
        target_index->insert(keys[i], static_cast<PAYLOAD_TYPE>(gen_payload()));
    }


    for(int i = 0; i < init_num_keys; i++) {
        values[i].first = sub_keys[i];
        values[i].second = static_cast<PAYLOAD_TYPE>(gen_payload());
    }

    std::sort(values, values + init_num_keys,
            [](auto const& a, auto const& b) { return a.first < b.first; });


    sub_index->bulk_load(values, init_num_keys);

    for(int i = init_num_keys; i < total_num_keys - num_poisoning_keys; i++) {
        sub_index->insert(sub_keys[i], static_cast<PAYLOAD_TYPE>(gen_payload()));
    }

    


    chosen_node_density.clear();
    poisoning_keys.clear();

    long long sub_mem_before = sub_index->data_size();


    attack::launch_mck_attack(*sub_index,
            num_poisoning_keys,
            num_actions,
            0,
            s,
            chosen_node_density,
            poisoning_keys,
            1);

    long sub_mem_after = sub_index->data_size(); 


    long long before_model_mem = target_index->model_size();
    long long before_data_mem = target_index->data_size();
    int before_dns_size = target_index->stats_.num_data_nodes;
    std::cout << std::setprecision(19);
    std::cout << std::scientific;

    for(int j = 0; j < poisoning_keys.size(); j++) {
        target_index->insert(poisoning_keys[j], static_cast<PAYLOAD_TYPE>(gen_payload()));
        cout<<poisoning_keys[j]<<"\n";
    }


    dns.clear();
    target_index->get_all_datanodes(dns);
    

    long long after_model_mem = target_index->model_size();
    long long after_data_mem = target_index->data_size();
    
    int sum = 0;

    for(int i = 0; i < dns.size(); i++) {
        sum += dns[i]->num_resizes_;
    }

    int performend_num_actions = target_index->stats_.num_downward_splits +
        target_index->stats_.num_sideways_splits +
        sum +
        target_index->stats_.num_expand_and_retrains +
        target_index->stats_.num_expand_and_scales;
    

    cout<<dataset_type<<","
        <<total_num_keys<<","
        <<sub_index->stats_.num_keys<<","
        <<target_index->stats_.num_keys<<","
        <<num_poisoning_keys<<","
        <<init_num_keys<<","
        <<init_perc<<","
        <<budget<<","
        <<before_dns_size<<","
        <<num_actions<<","
        <<kde_type<<","
        <<h<<","
        <<sub_mem_before<<","
        <<sub_mem_after<<","
        <<before_model_mem<<","
        <<after_model_mem<<","
        <<before_data_mem<<","
        <<after_data_mem<<","
        <<performend_num_actions
        <<"\n";


    delete target_index;
    delete sub_index;

    delete[] keys;
    delete[] values;
    delete[] sub_keys;

    return 0;

}
