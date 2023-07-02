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
    auto num_epochs = stoi(get_required(flags, "num_epochs"));
    auto num_actions = stoi(get_required(flags, "num_actions"));
    auto dataset_type=stoi(get_required(flags, "dataset_type"));

    auto budget = stoi(get_required(flags, "budget"));

    auto init_num_keys = stoi(get_required(flags, "init_num_keys"));

    std::mt19937_64 gen_payload(std::random_device{}());
    std::vector<alex::AlexDataNode<KEY_TYPE, PAYLOAD_TYPE>*> dns;
    alex::Alex<KEY_TYPE, PAYLOAD_TYPE> index;
    std::vector<double> chosen_node_density;
    std::vector<KEY_TYPE> poisoning_keys, substitude_poisoning_keys;
    Stats s;


    int num_poisoning_keys = init_num_keys * (static_cast<double> (budget) / 100);

    int num_poisoning_keys_per_epoch = num_poisoning_keys / num_epochs;

    int num_legtimate_keys = init_num_keys - num_poisoning_keys;

    int num_legtimate_keys_per_epoch = num_legtimate_keys / num_epochs;


    // key arrays
    auto keys = new KEY_TYPE[init_num_keys];
    auto values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[num_legtimate_keys_per_epoch];


    // read original dataset
    if (keys_file_type == "binary") {
        load_binary_data(keys, init_num_keys, key_file);
    } else if (keys_file_type == "text") {
        load_text_data(keys, init_num_keys, key_file);
    } else {
        std::cerr << "--key_type must be either 'binary' or 'text'"
            << std::endl;
        return 1;
    }

    int num_lit_inserts = 0;

    for(int e = 0; e < num_epochs ; e++) {


        if(e == 0) {
            for(int i = 0; i < num_legtimate_keys_per_epoch; i++) {
                values[i].first = keys[i];
                values[i].second = static_cast<PAYLOAD_TYPE>(gen_payload());
            }
            num_lit_inserts += num_legtimate_keys_per_epoch;

            std::sort(values, values +  num_legtimate_keys_per_epoch,
                    [](auto const& a, auto const& b) { return a.first < b.first; });
            index.bulk_load(values, num_legtimate_keys_per_epoch);
            chosen_node_density.clear();
            poisoning_keys.clear();

            attack::launch_mck_attack(index,
                    num_poisoning_keys_per_epoch,
                    num_actions,
                    0,
                    s,
                    chosen_node_density,
                    poisoning_keys,
                    0);


        } else {

            for(int i = 0; i < num_legtimate_keys_per_epoch; i++) {
                index.insert(keys[num_lit_inserts+ i], static_cast<PAYLOAD_TYPE>(gen_payload()));
            }

            num_lit_inserts += num_legtimate_keys_per_epoch;


            chosen_node_density.clear();
            poisoning_keys.clear();

            if(e  == num_epochs - 1) {
                num_poisoning_keys_per_epoch = init_num_keys - index.stats_.num_keys;
            }

            attack::launch_mck_attack(index,
                    num_poisoning_keys_per_epoch,
                    num_actions,
                    0,
                    s,
                    chosen_node_density,
                    poisoning_keys,
                    0);

        }

    }


    if(index.stats_.num_keys < init_num_keys) {

        attack::insert_left_keys(dns,
                index, 
                init_num_keys - index.stats_.num_keys,
                poisoning_keys,
                chosen_node_density,
                substitude_poisoning_keys);

    }


    dns.clear();
    index.get_all_datanodes(dns);


    long long model_mem = index.model_size();
    long long data_mem = index.data_size();
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
        <<init_num_keys<<","
        <<budget<<","
        <<num_epochs<<","
        <<num_actions<<","
        <<num_lit_inserts<<","
        <<num_poisoning_keys<<","
        <<s.mck_num_keys<<","
        <<index.stats_.num_keys<<","
        <<s.num_mck_call<<","
        <<model_mem<<","
        <<data_mem<<","
        <<performend_num_actions
        <<"\n";


    delete[] keys;
    delete[] values;

    return 0;

}
