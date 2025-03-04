#pragma once


#include "flow_define.hpp"
#include "../common.hpp"
#include "../packet_parse/packet_basic.hpp"

namespace hypervision {

class explicit_flow_constructor final {
private:
    const shared_ptr<vector<shared_ptr<basic_packet> > > p_parse_result;
    const vector<basic_packet4> parse_result;
    shared_ptr<vector<shared_ptr<tuple5_flow4> > > p_construct_result4;
    vector<tuple5_flow4> construct_result4;

    double_t FLOW_TIME_OUT = 10.0;
    double_t EVICT_FLOW_TIME_OUT = 5.0;

    void _flow_double_check4(size_t multiplex);
    void _flow_double_check6(size_t multiplex);
    void flow_double_check(size_t multiplex=32);

public:

    explicit_flow_constructor(): p_parse_result(nullptr) {}
    explicit_flow_constructor(const decltype(parse_result) parse_result): parse_result(parse_result) {}
    explicit_flow_constructor(const decltype(p_parse_result) p_parse_result,
                              const double_t flow_time_out,
                              const double_t evict_flow_time_out):
                              p_parse_result(p_parse_result),
                              FLOW_TIME_OUT(flow_time_out),
                              EVICT_FLOW_TIME_OUT(evict_flow_time_out) {}

    explicit_flow_constructor(const explicit_flow_constructor &) = delete;
    explicit_flow_constructor operator=(const explicit_flow_constructor &) = delete;

    // void construct_flow(size_t multiplex=32);
    void construct_flow(size_t multiplex=1);
    void dump_flow_statistic(void) const;
    void configure_via_json(const json & jin);

    auto get_constructed_flow(void) const -> decltype(p_construct_result4) {
        return p_construct_result4;
    }

    auto get_constructed_raw_flow(void) const -> shared_ptr<vector<shared_ptr<basic_flow> > > {
        if (p_construct_result4 == nullptr) {
            WARN("Null pointer returned.");
            return nullptr;
        }
        const auto p_ret = make_shared<vector<shared_ptr<basic_flow> > >();
        p_ret->insert(p_ret->end(), p_construct_result4->begin(), p_construct_result4->end());
        return p_ret;
    }

    auto do_sum_packet_check(void) const -> size_t {
        size_t sum4 = 0;
        for (const auto p_f: * p_construct_result4) {
            sum4 += p_f->get_p_packet_p_seq()->size();
        }
        return sum4;
    }

    void config_via_json(const json & jin);
};

}
