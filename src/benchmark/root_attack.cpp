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





    //read flags
    auto flags = parse_flags(argc, argv);
    std::string out_file_path = get_required(flags, "out_file_path");
    auto total_num_keys = stoi(get_required(flags, "total_num_keys"));
    std::string key_type = get_required(flags, "key_type");
    auto key_file = get_required(flags, "key_file");


    // key arrays
    auto keys = new KEY_TYPE[total_num_keys];

    // read original dataset
    if (key_type == "binary") {
        load_binary_data(keys, total_num_keys, key_file);
    } else if (key_type == "text") {
        load_text_data(keys, total_num_keys, key_file);
    } else {
        std::cerr << "--key_type must be either 'binary' or 'text'"
            << std::endl;
        return 1;
    }

    int init_num_keys = 10000000;


    std::ofstream outputFile;
    outputFile.open(out_file_path, std::ios_base::app);

    // calcualte legitimal key index
    auto values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_num_keys];
    std::mt19937_64 gen_payload(std::random_device{}());


    for(int i = 0; i < init_num_keys; i++) {
        values[i].first = keys[i];
        values[i].second = static_cast<PAYLOAD_TYPE>(gen_payload());
    }

    alex::Alex<KEY_TYPE, PAYLOAD_TYPE> *index = new alex::Alex<KEY_TYPE, PAYLOAD_TYPE>();

    std::sort(values, values + init_num_keys,
            [](auto const& a, auto const& b) { return a.first < b.first; });
    index->bulk_load(values, init_num_keys);





    unsigned long long instance_max = values[init_num_keys - 1].first;
    unsigned long long instance_min = values[0].first;

    int diff = 1;


    
    /*
    double instance_max = values[init_num_keys - 1].first;
    double instance_min = values[0].first;
    double diff = 0.0000000000001;*/


    int num_keys_per_batch = 5;
    int num_batch = 10000 / num_keys_per_batch;

    instance_max += diff;


    for(int i = 0; i < num_batch; i++) {
                        
        for(int j = 0; j < num_keys_per_batch; j++) {
            index->insert(keys[init_num_keys + i * num_keys_per_batch + j], static_cast<PAYLOAD_TYPE>(gen_payload()));

            /*
            if(j == num_keys_per_batch - 1){
                instance_max = (instance_max - instance_min) + instance_max; 
            } else {
                instance_max += diff;
            }*/

        }

        long long model_mem = index->model_size();
        long long data_mem = index->data_size();

        outputFile<<num_keys_per_batch<<","
            <<index->stats_.num_keys<<","
            <<index->stats_.num_model_nodes<<","
            <<index->stats_.num_data_nodes<<","
            <<index->stats_.num_expand_and_scales + index->stats_.num_expand_and_retrains<<","
            <<index->stats_.num_sideways_splits + index->stats_.num_downward_splits<<","
            <<index->stats_.num_cost_deviation_splits<<","
            <<index->stats_.num_catasctrophic_splits<<","
            <<index->stats_.num_exceed_max_capcity_splits<<","
            <<index->stats_.num_model_node_expansions<<","
            <<index->stats_.num_model_node_splits<<","
            <<index->stats_.num_actions_on_root<<","
            <<model_mem<<","
            <<data_mem<<"\n";
    }
    
    delete index;
    delete keys;
    delete values;
    outputFile.close();


    return 0;

}
