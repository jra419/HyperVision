#pragma once

#include "../common.hpp"
#include "packet_info.hpp"
#include "pcpp_common.hpp"

namespace hypervision {

struct basic_packet {
	pkt_ts_t ts_start;
	pkt_ts_t ts_end;
	pkt_code_t tp;
	pkt_cnt_t cnt;
	pkt_len_t len;
	pkt_proto_t proto;
	basic_packet() = default;
	explicit basic_packet(const decltype(ts_start) ts_start, const decltype(ts_end) ts_end,
						  const decltype(tp) tp, const decltype(cnt) cnt,
						  const decltype(len) len, const decltype(proto) proto):
	ts_start(ts_start), ts_end(ts_end), tp(tp), cnt(cnt), len(len), proto(proto) {}
	virtual ~basic_packet() {}
};


struct basic_packet_bad : public basic_packet {
	basic_packet_bad() = default;
	explicit basic_packet_bad(const decltype(ts_start) ts_start, const decltype(ts_end) ts_end):
	basic_packet(ts_start, ts_end, 0, 0, 0, 0) {}
	virtual ~basic_packet_bad() {}
};


struct basic_packet4 final: public basic_packet {
	tuple4_conn4 flow_id;
	basic_packet4() = default;
	explicit basic_packet4(const pkt_addr4_t s_IP,
						   const pkt_addr4_t d_IP,
						   const pkt_proto_t proto,
						   const pkt_port_t s_port,
						   const pkt_port_t d_port,
						   const decltype(ts_start) ts_start, const decltype(ts_end) ts_end,
						   const decltype(tp) tp, const decltype(cnt) cnt,
						   const decltype(len) len):
						   flow_id(s_IP, d_IP, s_port, d_port),
						   basic_packet(ts_start, ts_end, tp, cnt, len, proto) {}
	explicit basic_packet4(const string s_IP,
						   const string d_IP,
						   const pkt_proto_t proto,
						   const pkt_port_t s_port,
						   const pkt_port_t d_port,
						   const decltype(ts_start) ts_start, const decltype(ts_end) ts_end,
						   const decltype(tp) tp, const decltype(cnt) cnt,
						   const decltype(len) len):
						   flow_id(convert_str_addr4(s_IP), convert_str_addr4(d_IP),
								   s_port, d_port),
						   basic_packet(ts_start, ts_end, tp, cnt, len, proto) {}
	explicit basic_packet4(const decltype(flow_id) flow_id,
						   const decltype(ts_start) ts_start, const decltype(ts_end) ts_end,
						   const decltype(tp) tp, const decltype(cnt) cnt,
						   const decltype(len) len):
						   flow_id(flow_id),
						   basic_packet(ts_start, ts_end, tp, cnt, len, proto) {}
	explicit basic_packet4(const string & str) {
		stringstream ss(str);
		int t;
		ss >> t;
		assert(t == 4);
		pkt_addr4_t sIP, dIP;
		ss >> sIP >> dIP;
		pkt_port_t sp, dp;
		ss >> sp >> dp;
		flow_id = {sIP, dIP, sp, dp};
		double_t _str_time;
		ss >> _str_time;
		ts = get_time_spec(_str_time / 1e6);
		ss >> tp;
		ss >> len;
	}

	virtual ~basic_packet4() {}

	auto get_pkt_str(const int64_t align_time) -> string {
		stringstream ss;
		ss << 4
		   << ' ' << tuple_get_src_addr(flow_id)
		   << ' ' <<  tuple_get_dst_addr(flow_id)
		   << ' ' << tuple_get_src_port(flow_id)
		   << ' ' << tuple_get_dst_port(flow_id)
		   << ' ' << ((int64_t) (GET_DOUBLE_TS(ts) * 1e6)) - align_time
		   << ' ' << tp
		   << ' ' << len << '\n';
		return ss.str();
	}
};

}
