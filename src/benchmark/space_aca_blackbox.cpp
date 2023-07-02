// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

/*
 * Simple benchmark that runs a mixture of point lookups and inserts on ALEX.
 */


#include <algorithm>
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
    auto total_num_keys = stoi(get_required(flags, "total_num_keys")); 
    //auto budget = stod(get_required(flags, "budget"));
    string key_type = "binary";
    auto key_file = get_required(flags, "key_file");
    auto init_num_keys = stoi(get_required(flags, "init_num_keys"));
    auto num_duplicates = stoi(get_required(flags, "num_duplicates"));



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




    // calcualte legitimal key index
    auto values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_num_keys];
    std::mt19937_64 gen_payload(std::random_device{}());
    std::mt19937_64 gen_key(std::random_device{}());


    for(int i = 0; i < init_num_keys; i++) {
        values[i].first = keys[i];
        values[i].second = static_cast<PAYLOAD_TYPE>(gen_payload());
    }

    alex::Alex<KEY_TYPE, PAYLOAD_TYPE> index; 

    std::sort(values, values + init_num_keys,
            [](auto const& a, auto const& b) { return a.first < b.first; });
    index.bulk_load(values, init_num_keys);




    int total_remaining_keys = total_num_keys - init_num_keys;

    //int remaining_legit_keys = total_remaining_keys - num_duplicates;


    std::vector<KEY_TYPE> remaining_keys;
    remaining_keys.clear();

    //generate duplicates

    int key_range_sample_size = 1000;
    std::vector<KEY_TYPE> estimates;
    estimates.clear();




    std::experimental::sample(keys, keys + init_num_keys, std::back_inserter(estimates),
            key_range_sample_size, std::mt19937{std::random_device{}()});

    KEY_TYPE min_key = estimates[0];
    KEY_TYPE max_key = estimates[estimates.size() - 1];

    std::uniform_real_distribution<> dis(min_key, max_key);
    //std::uniform_int_distribution<unsigned long long> dis(min_key, max_key);

    std::mt19937 gen(std::random_device{}());
    

    KEY_TYPE guess_key = dis(gen);



    /*
    for(int i = 0; i < total_remaining_keys - num_duplicates; i++) {
        
        if( i % 100 == 0 && num_duplicates > 0) {
            remaining_keys.push_back(guess_key);
            num_duplicates--;
        } else{  
            remaining_keys.push_back(keys[i + init_num_keys]);
        }
    }*/

    //std::random_shuffle(remaining_keys.begin(), remaining_keys.end());

    for(int i = 0; i < total_remaining_keys; i++) {
        cout<<"i="<<i<<" "<< index.model_size() + index.data_size()<<"\n";
        index.insert(guess_key,static_cast<PAYLOAD_TYPE>(gen_payload()));
    }

    //cout<<"num_datanodes="<<index.stats_.num_data_nodes<<" "<< index.stats_.num_keys<<"\n";

    delete[] keys;
    delete[] values;
    return 0;

}
