#pragma once

#include "../packet_parse/pcap_parser.hpp"
#include "../packet_parse/packet_listener.hpp"
#include "../flow_construct/explicit_constructor.hpp"
#include "edge_constructor.hpp"
#include "graph_define.hpp"
#include <pcapplusplus/Device.h>
#include <string>

namespace Hypervision {

class hypervision_detector {

private:
    json jin_main;
    string file_path = "";
    string interface = "";
    int epoch_cntr = 0;

    shared_ptr<vector<shared_ptr<basic_packet> > > p_parse_result;
    shared_ptr<pcpp::RawPacketVector> p_packet_vec;

    shared_ptr<binary_label_t> p_label;
    shared_ptr<vector<double_t> > p_loss;

    shared_ptr<vector<shared_ptr<basic_flow> > > p_flow;

    shared_ptr<vector<shared_ptr<short_edge> > > p_short_edges;
    shared_ptr<vector<shared_ptr<long_edge> > > p_long_edges;

    bool save_result_enable = false;
    string save_result_path = "../temp/default.json";

public:
    void start_batch(void) {
        __START_FTIMMER__

        // Read from pcap
        if (jin_main.count("packet_parse") &&
            jin_main["packet_parse"].count("target_file_path")) {

            LOGF("Parse packet from file.");
            file_path = jin_main["packet_parse"]["target_file_path"];
            const auto p_packet_parser = make_shared<pcap_parser>(file_path);
            p_packet_parser->parse_raw_packet();
            p_packet_parser->parse_basic_packet_fast();
            p_parse_result = p_packet_parser->get_basic_packet_rep();

            LOGF("Split datasets.");
            const auto p_dataset_constructor = make_shared<basic_dataset>(p_parse_result);
            p_dataset_constructor->configure_via_json(jin_main["dataset_construct"]);
            p_dataset_constructor->do_dataset_construct();
            p_label = p_dataset_constructor->get_label();
            LOGF("Num pkts parse_result: %lu", p_parse_result->size());
            LOGF("Num pkts p_label: %lu", p_label->size());

        // Read from processed pkt data
        } else if (jin_main["dataset_construct"].count("data_path") &&
                    jin_main["dataset_construct"].count("label_path")){
            LOGF("Load & split datasets.");
            const auto p_dataset_constructor = make_shared<basic_dataset>(p_parse_result);
            p_dataset_constructor->configure_via_json(jin_main["dataset_construct"]);
            p_dataset_constructor->import_dataset();
            p_label = p_dataset_constructor->get_label();
            p_parse_result = p_dataset_constructor->get_raw_pkt();
            LOGF("Num pkts parse_result: %lu", p_parse_result->size());
            LOGF("Num pkts p_label: %lu", p_label->size());
        } else {
            LOGF("Dataset not found.");
        }

        // for (const auto& packet_ptr : *p_parse_result) {
        //     std::cout << "TS: " << std::fixed << std::setprecision(std::numeric_limits<double>::max_digits10) << GET_DOUBLE_TS(packet_ptr->ts) << std::endl;
        //     if (typeid(packet_ptr) == typeid(basic_packet4)) {
        //         const auto _p_rep = dynamic_pointer_cast<basic_packet4>(packet_ptr);
        //         const auto _stack_code = convert_packet2stack_code(_p_rep->tp);
        //         const auto _flow_id = tuple4_extend(_p_rep->flow_id, _stack_code);
        //         std::cout << "flow id: " << std::get<0>(_flow_id) << std::endl;
        //     }
        // }

        // IV. Graph construction
        // IV-A. Flow classification
        LOGF("Construct flow.");
        const auto p_flow_constructor = make_shared<explicit_flow_constructor>(p_parse_result);
        p_flow_constructor->config_via_json(jin_main["flow_construct"]);
        p_flow_constructor->construct_flow();
        p_flow_constructor->dump_flow_statistic();
        p_flow = p_flow_constructor->get_constructed_raw_flow();

        LOGF("Construct edge.");
        const auto p_edge_constructor = make_shared<edge_constructor>(p_flow);
        p_edge_constructor->config_via_json(jin_main["edge_construct"]);
        // IV-B. Short flow aggregation
        // IV-C. long flow feat. distrib. fitting
        p_edge_constructor->do_construct();
        LOGF("Dump short edges.");
        p_edge_constructor->dump_short_edge();
        LOGF("Dump long edges.");
        p_edge_constructor->dump_long_edge();
        tie(p_short_edges, p_long_edges) = p_edge_constructor->get_edge();

        // V. Graph pre-processing
        LOGF("Construct Graph.");
        const auto p_graph = make_shared<traffic_graph>(p_short_edges, p_long_edges);
        LOGF("Config via json.");
        p_graph->config_via_json(jin_main["graph_analyze"]);
        LOGF("Parse Edge.");
        p_graph->parse_edge();
        // p_graph->dump_graph_statistic();
        // p_graph->dump_edge_anomly();
        // p_graph->dump_vertex_anomly();
        LOGF("Graph Detect.");
        // V-A. Connectivity Analysis
        // VI-B. Edge feature clustering for detection
        // VI. Malicious traffic detection
        p_graph->graph_detect();
        LOGF("Get final pkt score.");
        p_loss = p_graph->get_final_pkt_score(p_label);

        if (save_result_enable) {
            do_save_batch(save_result_path);
        }

        __STOP_FTIMER__
        __PRINTF_EXE_TIME__
    }

    void start_stream(void) {
        __START_FTIMMER__

        pcpp::RawPacketVector packet_vec;

        interface = jin_main["packet_listen"]["interface"];

        int empty_epoch_cntr = 0;

        // SEPARATE THREAD
        // Listen to network interface
        // start capturing packets. All packets will be added to the packet vector
        const auto p_packet_listener = make_shared<packet_listener>(interface);

        // If more than x time has elapsed, process all the packets in the buffer
        while(true) {
            sleep(20);
            epoch_cntr++;

            p_packet_vec = p_packet_listener->get_packet_vector();

            // If no packets were captured, reset the timer.
            if (!p_packet_vec) {
                std::cout << "Epoch " << std::to_string(epoch_cntr)
                    << ": No packets captured." << std::endl;
                empty_epoch_cntr++;
                // If too many epochs have elapsed without any packet arriving,
                // terminate the capture.
                if(empty_epoch_cntr > 9) break;
                continue;
            } else {
                std::cout << "Epoch " << std::to_string(epoch_cntr)
                    << ": Packets captured." << std::endl;
                empty_epoch_cntr = 0;
            }

            p_packet_listener->parse_basic_packet_fast();
            p_parse_result = p_packet_listener->get_basic_packet_rep();

            LOGF("Split datasets.");
            const auto p_dataset_constructor = make_shared<basic_dataset>(p_parse_result);
            p_dataset_constructor->configure_via_json(jin_main["dataset_construct"]);
            p_dataset_constructor->do_dataset_construct();
            p_label = p_dataset_constructor->get_label();

            LOGF("Construct flow.");
            const auto p_flow_constructor = make_shared<explicit_flow_constructor>(p_parse_result);
            p_flow_constructor->config_via_json(jin_main["flow_construct"]);
            p_flow_constructor->construct_flow();
            p_flow_constructor->dump_flow_statistic();
            p_flow = p_flow_constructor->get_constructed_raw_flow();

            LOGF("Construct edge.");
            const auto p_edge_constructor = make_shared<edge_constructor>(p_flow);
            p_edge_constructor->config_via_json(jin_main["edge_construct"]);
            p_edge_constructor->do_construct();
            LOGF("Dump short edges.");
            p_edge_constructor->dump_short_edge();
            LOGF("Dump long edges.");
            p_edge_constructor->dump_long_edge();
            tie(p_short_edges, p_long_edges) = p_edge_constructor->get_edge();

            LOGF("Construct Graph.");
            const auto p_graph = make_shared<traffic_graph>(p_short_edges, p_long_edges);
            LOGF("Config via json.");
            p_graph->config_via_json(jin_main["graph_analyze"]);
            LOGF("Parse Edge.");
            p_graph->parse_edge();
            // p_graph->dump_graph_statistic();
            // p_graph->dump_edge_anomly();
            // p_graph->dump_vertex_anomly();
            LOGF("Graph Detect.");
            p_graph->graph_detect();
            LOGF("Get final pkt score.");
            p_loss = p_graph->get_final_pkt_score(p_label);

            if (save_result_enable) {
                do_save_stream(save_result_path);
            }

            __STOP_FTIMER__
            __PRINTF_EXE_TIME__
        }
    }

    void config_via_json(const json & jin) {
        try {
            if (
                jin.count("dataset_construct") &&
                jin.count("flow_construct") &&
                jin.count("edge_construct") &&
                jin.count("graph_analyze") &&
                jin.count("result_save")) {
                    jin_main = jin;
                } else {
                    throw logic_error("Incomplete json configuration.");
                }
                const auto j_save = jin["result_save"];
                if (j_save.count("save_result_enable")) {
                    save_result_enable = static_cast<decltype(save_result_enable)>(j_save["save_result_enable"]);
                }
                if (j_save.count("save_result_path")) {
                    save_result_path = static_cast<decltype(save_result_path)>(j_save["save_result_path"]);
                }
        } catch (const exception & e) {
            FATAL_ERROR(e.what());
        }
    }

    void do_save_batch(const string & save_path) {
        __START_FTIMMER__

        ofstream _f(save_path + ".txt");
        if (_f.is_open()) {
            try {
                _f << setprecision(4);
                for (size_t i = 0; i < p_label->size(); ++i) {
                    _f << p_label->at(i) << ' '<< p_loss->at(i) << '\n';
                    if (i % 1000 == 0) {
                        _f << flush;
                    }
                }
            } catch(const exception & e) {
                FATAL_ERROR(e.what());
            }
            _f.close();
        } else {
            FATAL_ERROR("File Error.");
        }

        __STOP_FTIMER__
        __PRINTF_EXE_TIME__
    }

    void do_save_stream(const string & save_path) {
        __START_FTIMMER__

        ofstream _f(save_path + "-" + std::to_string(epoch_cntr) + ".txt");
        if (_f.is_open()) {
            try {
                _f << setprecision(4);
                for (size_t i = 0; i < p_label->size(); ++i) {
                    _f << p_label->at(i) << ' '<< p_loss->at(i) << '\n';
                    if (i % 1000 == 0) {
                        _f << flush;
                    }
                }
            } catch(const exception & e) {
                FATAL_ERROR(e.what());
            }
            _f.close();
        } else {
            FATAL_ERROR("File Error.");
        }

        __STOP_FTIMER__
        __PRINTF_EXE_TIME__
    }
};

}
