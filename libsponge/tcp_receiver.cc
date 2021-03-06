#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    if(!_syn_flag && !seg.header().syn) return false;
    if(_syn_flag && seg.header().syn) return false;

    size_t abs_seqno = _base;
    size_t length = seg.length_in_sequence_space();

    if(seg.header().syn) {
        _syn_flag = true;
        _isn = seg.header().seqno.raw_value(); // 收到syn后设置isn
        abs_seqno = 1;
        _base = 1;
        length --; // 只去掉syn的长度

        if(length == 0) return true;
    } else {
        // 根据上一个abs_seqno得出abs_seqno
        abs_seqno = unwrap(WrappingInt32(seg.header().seqno.raw_value()), WrappingInt32(_isn), abs_seqno);
    }

    if(seg.header().fin) {
        if(_fin_flag) return false;
        _fin_flag = true;
    } else if(seg.length_in_sequence_space() == 0 && abs_seqno == _base) {
        return true;
    } else if (abs_seqno >= _base + window_size() || abs_seqno + length <= _base) { // 超出接收窗口或早已接收时放弃接收
        return false;
    }

    _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, seg.header().fin); // 不考虑syn
    _base = _reassembler.head_index() + 1; // 考虑syn
    if(_reassembler.input_ended()) _base ++; // 考虑fin
    return true;
}

// 第一个没收到的字节index
optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_base > 0)
        return WrappingInt32(wrap(_base, WrappingInt32(_isn)));
    else
        return std::nullopt;
}

size_t TCPReceiver::window_size() const {
    return _capacity - stream_out().buffer_size();
}
