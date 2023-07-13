#include "tcp_sender.hh"

#include <random>
#include <cassert>

#include "tcp_config.hh"

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender(uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn)
    : isn_(fixed_isn.value_or(Wrap32{random_device()()})),
      initial_RTO_ms_(initial_RTO_ms), 
      current_RT0_ms_(initial_RTO_ms) {}

uint64_t TCPSender::sequence_numbers_in_flight() const {
  return bytes_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const {
  return retransmissions_;
}

/*
  如果TCPSender愿意，
  这是TCPSender实际发送TCPSenderMessage的机会。
*/
optional<TCPSenderMessage> TCPSender::maybe_send() {
  // Your code here.
  if (flight_message_map_.empty()) {
    return optional<TCPSenderMessage>();
  } else {
    if (outstanding_message_map_.empty() && !clock_started_) {
      clock_started_ = true; 
      retransmissions_ = 0;
      ms_since_first_tick_ = 0;
      current_RT0_ms_ = initial_RTO_ms_;
    }

    auto fist_it = flight_message_map_.begin();
    auto checkpoint = fist_it->first; 
    TCPSenderMessage msg_to_send = fist_it->second; 
    uint64_t cp = fist_it->first;
    flight_message_map_.erase(fist_it); 

    outstanding_message_map_[cp] = msg_to_send;
    outstanding_checkpoint_ += msg_to_send.sequence_length();

    return msg_to_send;
  }
}

/*
  TCPSender被要求从出站字节流中填充窗口：只要有新的字节要读取和窗口中可用的空间，
  它就会从流中读取并生成尽可能多的TCPSenderMessages。您需要确保您发送的每个
  TCPSenderMessage都完全适合接收器的窗口。使每条消息尽可能大，
  但不要大于TCPConfig::MAX_PAYLOAD_SIZE（1452字节）给出的值。
  您可以使用TCPSenderMessage::sequence_length()方法来计算一个段占用的序列号总数。
  请记住，SYN和FIN标志也分别占据一个序列号，这意味着它们占据了窗口中的空间
*/
void TCPSender::push(Reader& outbound_stream) {
  string all_bytes = outbound_stream.peek();
  auto it = all_bytes.begin(); 
  do {
    uint16_t window_size = window_size_ == 0 ? 1 : window_size_;
    TCPSenderMessage msg; 
    // SYN
    if (outbound_stream.bytes_popped() == 0 && !syn_send_) {
      msg.SYN = true; 
      syn_send_ = true; 
    }

    // Bytes
    if (window_size - msg.SYN > bytes_flight_) {
      uint16_t allow_bytes_size = window_size - msg.SYN - bytes_flight_;
      uint64_t bytes_should_pop = std::min(TCPConfig::MAX_PAYLOAD_SIZE, 
                                           std::min(static_cast<uint64_t>(allow_bytes_size), 
                                                       all_bytes.size())); 

      msg.payload = Buffer(string(it, it + bytes_should_pop)); 
      outbound_stream.pop(bytes_should_pop); 
      it += bytes_should_pop;

      // FIN
      if (outbound_stream.is_finished() && 
          allow_bytes_size > bytes_should_pop &&
          !fin_send_) {
        msg.FIN = true; 
        fin_send_ = true; 
      }
    }

    // seqno
    msg.seqno = Wrap32::wrap(flight_checkpoint_, isn_); 

    // don't send empty!!
    if (msg.sequence_length() == 0) {
      break; 
    }

    // mark flight
    flight_message_map_[flight_checkpoint_] = msg;
    flight_checkpoint_ += msg.sequence_length();
    bytes_flight_ += msg.sequence_length(); 
  } while (it != all_bytes.end()); 
}

/*
  TCPSender应生成并发送正确设置序列号的零长度消息。
  如果对等体想要发送TCPReceiverMessage（例如，因为它需要从对等体的发件人那里确认某些内容），
  并且需要生成TCPSenderMessage来与之搭配使用，这非常有用。
  注意：像这样的片段不占用序列号，不需要被跟踪为“outstanding”，也永远不会被重新传输。
*/
TCPSenderMessage TCPSender::send_empty_message() const {
  TCPSenderMessage empty_msg; 
  empty_msg.seqno = Wrap32::wrap(flight_checkpoint_, isn_); 
  return empty_msg; 
}

/*
  从接收器收到一条消息，
  传达窗口的new left（= ackno）new right（= ackno +窗口大小）边缘。
  TCPSender应该查看其未完成段的集合，
  并删除任何现已完全确认的段（ackno大于段中的所有序号）。
*/
void TCPSender::receive(const TCPReceiverMessage &msg) {
  if (msg.ackno.has_value()) {
    uint64_t checkpoint = msg.ackno->unwrap(isn_, outstanding_checkpoint_);

    // delete ackno < checkpoint
    auto it = outstanding_message_map_.begin();
    bool should_delete_some_message = false; 
    while (it != outstanding_message_map_.end()) {
      auto entry = *it++; 
      if (entry.first + entry.second.sequence_length() == checkpoint) {
        should_delete_some_message = true; 
        break;
      }
    }

    if (should_delete_some_message) {
      auto it1 = outstanding_message_map_.begin();
      while (it1 != it) {
        outstanding_checkpoint_ += it1->second.sequence_length();
        bytes_flight_ -= it1->second.sequence_length(); 
        it1 = outstanding_message_map_.erase(it1);
      }
      clock_started_ = true; 
      retransmissions_ = 0;
      ms_since_first_tick_ = 0;
      current_RT0_ms_ = initial_RTO_ms_;
    }

    if (outstanding_message_map_.empty()) {
      clock_started_ = false; 
    }
  }

  window_size_ = msg.window_size;
}

/*
  时间已经过去了——自上次调用此方法以来，
  有一定数量的毫秒。发件人可能需要重新传输未完成的片段。
*/
void TCPSender::tick(const size_t ms_since_last_tick) {
  if (!clock_started_ || outstanding_message_map_.empty()) {
    return; 
  }

  ms_since_first_tick_ += ms_since_last_tick; 

  // timeout
  if (ms_since_first_tick_ >= current_RT0_ms_) {
    auto begin = outstanding_message_map_.begin();
    flight_message_map_.insert(*begin); 
    outstanding_message_map_.erase(begin);
    if (window_size_ != 0) {
      current_RT0_ms_ *= 2; 
    }
    ms_since_first_tick_ = 0;
    retransmissions_++;
  }
}
