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
    auto init_num_keys = stoi(get_required(flags, "init_num_keys"));
    auto workload_size = stoi(get_required(flags, "workload_size"));
    auto budget = stod(get_required(flags, "budget"));
    auto key_file = get_required(flags, "key_file");
    auto lookup_perc = stod(get_required(flags, "lookup_perc"));
    auto c = stoi(get_required(flags, "c"));
    auto dataset_type = stoi(get_required(flags, "dataset_type"));
    std::string lookup_distribution =
        get_with_default(flags, "lookup_distribution", "zipf");
    string key_type = "binary";



    int total_inserts = lround((1 - lookup_perc) * workload_size);
    int total_num_keys = init_num_keys + total_inserts; 


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
    std::vector<alex::AlexDataNode<KEY_TYPE, PAYLOAD_TYPE>*> dns;

    std::cout << std::scientific;
    std::cout << std::setprecision(3);

    std::random_device rd;
    std::default_random_engine eng(rd());

    std::vector<KEY_TYPE> estimates;
    estimates.clear();
    int key_range_sample_size = 1000;

    std::experimental::sample(keys, keys + total_num_keys, std::back_inserter(estimates),
            key_range_sample_size, std::mt19937{std::random_device{}()});
    std::sort(estimates.begin(), estimates.end(),
             [](auto const& a, auto const& b) { return a < b; });
    KEY_TYPE min_key = estimates[0];
    KEY_TYPE max_key = estimates[estimates.size() - 1];


    //std::uniform_int_distribution<unsigned long long> dis(min_key, max_key);
    std::uniform_real_distribution<> dis(min_key, max_key);


    int num_batch = 10;
    int num_operations_per_batch = workload_size / num_batch;
    int num_keys_per_batch =  lround(num_operations_per_batch * (1 - lookup_perc));
    int num_poison_keys_per_batch = lround(budget * num_keys_per_batch);
    int num_legitimate_keys_per_batch = num_keys_per_batch - num_poison_keys_per_batch;
    int idx = init_num_keys;
    int stop_idx = idx + num_legitimate_keys_per_batch;

    int num_lookups_per_batch = num_operations_per_batch * lookup_perc; 


    double cumulative_insert_time = 0.0;
    double cumulative_lookup_time = 0.0;
    long long cumulative_inserts = 0;
    long long cumulative_lookups = 0;

    alex::AlexDataNode<KEY_TYPE, PAYLOAD_TYPE>* target_node;
    int total_poi_inserts = 0;

    for(int i = 0; i < num_batch; i++) {

        // legtimate insert workload

        auto legitimate_insert_start = std::chrono::high_resolution_clock::now(); 
        for(; idx < stop_idx; idx++) {
            index.insert(keys[idx], static_cast<PAYLOAD_TYPE> (gen_payload()));
        }
        auto legitimate_insert_end = std::chrono::high_resolution_clock::now();

        double batch_legitimate_insert_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                legitimate_insert_end - legitimate_insert_start).count();

        cumulative_insert_time += batch_legitimate_insert_time;

        cumulative_inserts +=  num_legitimate_keys_per_batch;
        stop_idx += num_legitimate_keys_per_batch;


        //prepare for poisoning insert workload
        
        int running_posi_inserts = 0;
        while(running_posi_inserts < num_poison_keys_per_batch) {

            KEY_TYPE left_key = dis(gen_key);

            //for double type
            double diff = 0.0000000000001;

            //for int type 
            //int diff = 1;

            //generate c consective keys for one batch
            vector<KEY_TYPE> poisoning_keys;

            int actual_size = std::min(c, num_poison_keys_per_batch - running_posi_inserts);
            for(int j = 0; j < actual_size; j++) {
                left_key += diff;
                poisoning_keys.push_back(left_key);
            }

            auto pois_insert_start = std::chrono::high_resolution_clock::now();

            for(int j = 0; j < poisoning_keys.size(); j++) {
                index.insert(poisoning_keys[j], static_cast<PAYLOAD_TYPE>(gen_payload()));
            }

            auto pois_insert_end = std::chrono::high_resolution_clock::now();
            double batch_pois_insert_time =  std::chrono::duration_cast<std::chrono::nanoseconds>(
                    pois_insert_end - pois_insert_start).count();

            cumulative_insert_time += batch_pois_insert_time;

            running_posi_inserts += actual_size;
            cumulative_inserts += actual_size;

            if(running_posi_inserts >= num_poison_keys_per_batch) {
                break;
            }

        }


        //lookups
        double batch_lookup_time = 0.0;
        if (idx > 0) {
            KEY_TYPE* lookup_keys = nullptr;
            if (lookup_distribution == "uniform") {
                lookup_keys = get_search_keys(keys, idx, num_lookups_per_batch);
            } else if (lookup_distribution == "zipf") {
                lookup_keys = get_search_keys_zipf(keys, idx, num_lookups_per_batch);
            } else {
                std::cerr << "--lookup_distribution must be either 'uniform' or 'zipf'"
                    << std::endl;
                return 1;
            }

            auto lookups_start_time = std::chrono::high_resolution_clock::now();

            for (int j = 0; j < num_lookups_per_batch; j++) {
                KEY_TYPE key = lookup_keys[j];
                PAYLOAD_TYPE* p_load = index.get_payload(key);
            }
            auto lookups_end_time = std::chrono::high_resolution_clock::now();
            batch_lookup_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    lookups_end_time - lookups_start_time)
                .count();

            cumulative_lookup_time += batch_lookup_time;
            cumulative_lookups += num_lookups_per_batch;
            delete[] lookup_keys;
        }

    }

    long long cumulative_operations = cumulative_lookups + cumulative_inserts;
    double cumulative_time = cumulative_lookup_time + cumulative_insert_time;



    cout<< dataset_type<<","
        <<total_poi_inserts<<","
        << cumulative_inserts<<","
        << cumulative_lookups<<","
        << cumulative_operations<<","
        << budget<<"," 
        << c <<","
        << lookup_perc<<","
        << cumulative_operations / cumulative_time * 1e9 <<","
        <<cumulative_lookups / cumulative_lookup_time * 1e9<<","
        << cumulative_inserts / cumulative_insert_time * 1e9 <<","
        <<index.stats_.num_inserts<<","
        <<index.stats_.num_lookups<<","
        <<index.stats_.num_model_nodes<<","
        <<index.stats_.num_data_nodes<<","
        <<index.stats_.num_model_node_splits<<","
        <<index.stats_.num_model_node_expansions<<","
        <<index.stats_.num_downward_splits<<","
        <<index.stats_.num_sideways_splits<<","
        <<index.stats_.num_expand_and_retrains
        <<"\n";



    delete[] keys;
    delete[] values;
    return 0;

}
