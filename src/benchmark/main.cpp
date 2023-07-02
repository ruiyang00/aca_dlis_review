// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

/*
 * Simple benchmark that runs a mixture of point lookups and inserts on ALEX.
 */


#include <cmath>
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

#include "flags.h"
#include "alex.h"
#include "utils.h"
#include "alex_nodes.h"


#include "attack.h"

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
    auto init_num_keys = stoi(get_required(flags, "init_num_keys"));
    auto total_num_keys = stoi(get_required(flags, "total_num_keys"));
    auto batch_size = stoi(get_required(flags, "batch_size"));
    auto insert_frac = stod(get_with_default(flags, "insert_frac", "0.5"));
    std::string lookup_distribution =
        get_with_default(flags, "lookup_distribution", "zipf");
    auto time_limit = stod(get_with_default(flags, "time_limit", "0.5"));
    bool print_batch_stats = get_boolean_flag(flags, "print_batch_stats");
    //auto out_file = get_required(flags, "out_file");

    auto budget = stod(get_required(flags, "budget"));
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

    


    // Combine bulk loaded keys with randomly generated payloads
    auto values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_num_keys];
    std::mt19937_64 gen_payload(std::random_device{}());
    for (int i = 0; i < init_num_keys; i++) {
        values[i].first = keys[i];
        values[i].second = static_cast<PAYLOAD_TYPE>(gen_payload());
    }

    // Create ALEX and bulk load
    alex::Alex<KEY_TYPE, PAYLOAD_TYPE> index;
    std::sort(values, values + init_num_keys,
            [](auto const& a, auto const& b) { return a.first < b.first; });
    index.bulk_load(values, init_num_keys);

    // Run workload
    int total_poisoning_inserts = 0;
    int i = init_num_keys;
    int k = 400;
    long long cumulative_inserts = 0;
    long long cumulative_lookups = 0;
    int num_inserts_per_batch = static_cast<int>(batch_size * (insert_frac  - budget));
    int num_lookups_per_batch = batch_size - num_inserts_per_batch;
    double cumulative_insert_time = 0;
    double cumulative_lookup_time = 0;

    auto workload_start_time = std::chrono::high_resolution_clock::now();
    int batch_no = 0;
    PAYLOAD_TYPE sum = 0;
    std::cout << std::scientific;
    std::cout << std::setprecision(3);
    vector<alex::AlexDataNode<KEY_TYPE, PAYLOAD_TYPE>*> dns; 
    alex::AlexDataNode<KEY_TYPE, PAYLOAD_TYPE>* target_node;



    std::random_device rd;
    std::default_random_engine eng(rd());

    //std::ofstream out;
    //out.open(out_file, std::ios_base::app);
    //
    std::cout<<"finished bulk_load()\n";

    while (true) {
        batch_no++;

        // Do lookups
        double batch_lookup_time = 0.0;
        if (i > 0) {
            KEY_TYPE* lookup_keys = nullptr;
            if (lookup_distribution == "uniform") {
                lookup_keys = get_search_keys(keys, i, num_lookups_per_batch);
            } else if (lookup_distribution == "zipf") {
                lookup_keys = get_search_keys_zipf(keys, i, num_lookups_per_batch);
            } else {
                std::cerr << "--lookup_distribution must be either 'uniform' or 'zipf'"
                    << std::endl;
                return 1;
            }
            auto lookups_start_time = std::chrono::high_resolution_clock::now();
            for (int j = 0; j < num_lookups_per_batch; j++) {
                KEY_TYPE key = lookup_keys[j];
                PAYLOAD_TYPE* payload = index.get_payload(key);
                if (payload) {
                    sum += *payload;
                }
            }
            auto lookups_end_time = std::chrono::high_resolution_clock::now();
            batch_lookup_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    lookups_end_time - lookups_start_time)
                .count();
            cumulative_lookup_time += batch_lookup_time;
            cumulative_lookups += num_lookups_per_batch;
            delete[] lookup_keys;
        }

        // Do inserts
        int num_actual_inserts =
            std::min(num_inserts_per_batch, total_num_keys - i);
        int num_keys_after_batch = i + num_actual_inserts;
        auto inserts_start_time = std::chrono::high_resolution_clock::now();
        for (; i < num_keys_after_batch; i++) {
            index.insert(keys[i], static_cast<PAYLOAD_TYPE>(gen_payload()));
        }
        auto inserts_end_time = std::chrono::high_resolution_clock::now();
        double batch_insert_time =
            std::chrono::duration_cast<std::chrono::nanoseconds>(inserts_end_time -
                    inserts_start_time)
            .count();
        cumulative_insert_time += batch_insert_time;
        cumulative_inserts += num_actual_inserts;



        // poisoning insert workload



        int num_poisoning_keys = budget * num_inserts_per_batch;
        dns.clear();
        index.get_all_datanodes(dns);

        std::sort(dns.begin(),
                dns.begin() + dns.size(),
                [](auto const* a, auto const* b) {
                return a->data_size() > b->data_size();
                });


        int num_nodes = std::ceil(num_poisoning_keys / static_cast<double> (k));

        //int num_nodes = 1;

        for(int j = 0; j < num_nodes; j++) {
            target_node = dns[j];
            std::pair<int, int> vals = attack::get_max_continuous_key(target_node, 0, target_node->data_capacity_);
            int mid_pos = vals.second + (vals.first / 2);

            /*
               KEY_TYPE left_key = (mid_pos - target_node->model_.b_) / target_node->model_.a_;
               KEY_TYPE right_key = (mid_pos + 1 - target_node->model_.b_) / target_node->model_.a_;
               std::uniform_real_distribution<double> uniform_distr(left_key, right_key);*/
            vector<KEY_TYPE> poisoning_keys;

            KEY_TYPE left_key = (mid_pos - target_node->model_.b_) / target_node->model_.a_;
            double diff = 0.0000000000001;


            for(int z = 0; z < k; z++){
                poisoning_keys.push_back(left_key);
                left_key -= diff;
            }

            auto poisoning_inserts_start_time = std::chrono::high_resolution_clock::now();
            for(int z = 0; z < k; z++) {
                index.insert(poisoning_keys[z], static_cast<PAYLOAD_TYPE>(gen_payload()));
            }
            auto poisoning_inserts_end_time = std::chrono::high_resolution_clock::now();


            total_poisoning_inserts += k;

            double poisoning_batch_insert_time =
                std::chrono::duration_cast<std::chrono::nanoseconds>(poisoning_inserts_start_time -
                        poisoning_inserts_end_time)
                .count();
            cumulative_insert_time += poisoning_batch_insert_time;
            cumulative_inserts += k;

        }

        // insert poison keys here


        /*
           cout<<init_num_keys<<","
           <<cumulative_inserts<<","
           <<cumulative_inserts / cumulative_insert_time * 1e9<<","
           <<cumulative_lookups<<","
           <<cumulative_lookups / cumulative_lookup_time * 1e9<<","
           <<cumulative_operations / cumulative_time * 1e9<<"\n";*/

        if (print_batch_stats) {
            int num_batch_operations = num_lookups_per_batch + num_actual_inserts;
            double batch_time = batch_lookup_time + batch_insert_time;
            long long cumulative_operations = cumulative_lookups + cumulative_inserts;
            double cumulative_time = cumulative_lookup_time + cumulative_insert_time;
            std::cout << "Batch " << batch_no
                << ", cumulative ops: " << cumulative_operations
                << "\n\tbatch throughput:\t"
                << num_lookups_per_batch / batch_lookup_time * 1e9
                << " lookups/sec,\t"
                << num_actual_inserts / batch_insert_time * 1e9
                << " inserts/sec,\t" << num_batch_operations / batch_time * 1e9
                << " ops/sec"
                << "\n\tcumulative throughput:\t"
                << cumulative_lookups / cumulative_lookup_time * 1e9
                << " lookups/sec,\t"
                << cumulative_inserts / cumulative_insert_time * 1e9
                << " inserts/sec,\t"
                << cumulative_operations / cumulative_time * 1e9 << " ops/sec"
                << std::endl;
        }

        // Check for workload end conditions
        if (num_actual_inserts < num_inserts_per_batch || index.stats_.num_keys > total_num_keys) {
            // End if we have inserted all keys in a workload with inserts
            break;
        }

        /*
        double workload_elapsed_time =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::high_resolution_clock::now() - workload_start_time)
            .count();

        
        if (workload_elapsed_time > time_limit * 1e9 * 60) {
            break;
        }*/
    }

    long long cumulative_operations = cumulative_lookups + cumulative_inserts;
    double cumulative_time = cumulative_lookup_time + cumulative_insert_time;




    std::cout << "Cumulative stats: " << batch_no << " batches, "
        << cumulative_operations << " ops (" << cumulative_lookups
        << " lookups, " << cumulative_inserts << " inserts)"
        << "\n\tcumulative throughput:\t"
        << cumulative_lookups / cumulative_lookup_time * 1e9
        << " lookups/sec,\t"
        << cumulative_inserts / cumulative_insert_time * 1e9
        << " inserts/sec,\t"
        << cumulative_operations / cumulative_time * 1e9 << " ops/sec"
        << "\n  total_poisoning_inserts="<<total_poisoning_inserts <<" num_keys=" <<index.stats_.num_keys
        << std::endl;

    delete[] keys;
    delete[] values;
    //out.close();
}
