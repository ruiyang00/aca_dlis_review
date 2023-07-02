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
    auto total_num_keys = 200000000; 
    auto num_duplicates = stoi(get_required(flags, "num_duplicates"));
    string key_type = "binary";
    auto key_file = get_required(flags, "key_file");
    auto init_num_keys = stoi(get_required(flags, "init_num_keys"));



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
    std::vector<alex::AlexDataNode<KEY_TYPE, PAYLOAD_TYPE>*> sfp_dns;
    std::set<alex::AlexModelNode<KEY_TYPE, PAYLOAD_TYPE>*> internal_node_set;
    std::vector<std::pair<alex::AlexModelNode<KEY_TYPE, PAYLOAD_TYPE>*, int>> nps_pairs;
    alex::AlexDataNode<KEY_TYPE, PAYLOAD_TYPE>* node;
    alex::AlexModelNode<KEY_TYPE, PAYLOAD_TYPE>* p_node;
    std::vector<alex::AlexDataNode<KEY_TYPE,PAYLOAD_TYPE>*> ascending_dns;

    int num_poisoning_keys = budget * init_num_keys;
    int num_poisoning_keys_left = num_poisoning_keys;
    int max_fanout = index.derived_params_.max_fanout;
    int total_batch = 0;
    int sucessuful_batch = 0;
    int batch_size = 200;

    int t = 400;




    while(num_poisoning_keys_left > 0){

        if(num_poisoning_keys_left <= 0 ) {
            break;
        }

        dns.clear();
        sfp_dns.clear();
        nps_pairs.clear();
        internal_node_set.clear();

        index.get_all_datanodes(dns);

        for(int i = 0; i < dns.size(); i++) {
            node = dns[i];
            p_node = index.get_parent(node);
            if(node->duplication_factor_ == 0 && node->is_leaf_ && node->num_keys_ > 0) {
                sfp_dns.push_back(node);
                internal_node_set.insert(p_node);
            }
        }


        for(int i = 0; i < sfp_dns.size(); i++) {
            node = sfp_dns[i];
            p_node = index.get_parent(node);
            internal_node_set.insert(p_node);
        }


        for(auto node : internal_node_set){
            nps_pairs.push_back({node, node->num_children_});
        }


        // sort internal nodes base on the number of children
        std::sort(nps_pairs.begin(),
                nps_pairs.begin() + nps_pairs.size(),
                [](auto const& a, auto const& b) {
                return a.second > b.second;
                });



        // get the largest internal node that does not reaches the max_fanout 

        for(auto pair: nps_pairs) {
            if(pair.first->num_children_ != max_fanout) {
                p_node = pair.first;
                break;

            }
        }


        //greedyly expand a internal node until it reaches max_fanout 

        while(p_node->num_children_ < max_fanout) {

            if(num_poisoning_keys_left <= 0) {
                break;
            }


            //find internal node's data nodes with single pointer

            ascending_dns.clear();
            for(int j = 0; j < p_node->num_children_; j++) {
                node = static_cast<alex::AlexDataNode<KEY_TYPE, PAYLOAD_TYPE>*> (p_node->children_[j]);
                if( (node->duplication_factor_ == 0) && (node->num_keys_ > 0) && (node->is_leaf_)) {
                    ascending_dns.push_back(node);
                }
            }

            sort(ascending_dns.begin(), ascending_dns.end(), 
                    [](auto const* a, auto const* b){
                    return ceil(a->expansion_threshold_) - a->num_keys_ < ceil(b->expansion_threshold_) - b->num_keys_;
                    });

            node = ascending_dns[0]; 



            std::pair<int, int> vals = attack::get_max_continuous_key(node, 
                    0, node->data_capacity_);
            
            //int mid_pos = vals.second + (vals.first / 2);
            int mid_pos = attack::get_mid_occupied_pos(node, 0, node->data_capacity_);

            double diff = 0.0000000000001;


            KEY_TYPE left_key = node->get_key(mid_pos);

            int num_children_before_poison = p_node->num_children_;


            for(int k = 0; k < t; k++) {
                //left_key += diff;

                index.insert(left_key, static_cast<PAYLOAD_TYPE>(gen_payload()));

            }

            return 0;

            


            num_poisoning_keys_left -= batch_size;
            total_batch += 1;

            if(p_node->num_children_ > num_children_before_poison) {
                sucessuful_batch++;    
            }


            std::cout<< index.stats_.num_inserts<<","
                <<index.stats_.num_model_nodes<<","
                << index.stats_.num_model_node_expansions<<","
                << index.stats_.num_data_nodes<<","
                << index.stats_.num_downward_splits<<","
                << index.stats_.num_sideways_splits<<","
                << index.model_size()<<","
                << index.data_size()<<","
                << num_poisoning_keys<<","
                << num_poisoning_keys_left<<","
                <<(double) sucessuful_batch / total_batch<<"\n";

        }

    }



    delete[] keys;
    delete[] values;
    return 0;

}
