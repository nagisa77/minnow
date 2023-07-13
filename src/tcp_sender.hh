#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <map> 

class TCPSender {
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint16_t window_size_ = 1; 
  uint64_t flight_checkpoint_ = 0; 
  uint64_t outstanding_checkpoint_ = 0; 
  bool syn_send_ = false; 
  bool fin_send_ = false; 
  std::map<uint64_t, TCPSenderMessage> flight_message_map_; 
  std::map<uint64_t, TCPSenderMessage> outstanding_message_map_; 
  uint64_t bytes_flight_ = 0; 

  // resend
  size_t ms_since_first_tick_ = 0; 
  bool clock_started_ = false; 
  uint64_t current_RT0_ms_;
  uint64_t retransmissions_ = 0;

 public:
  /* Construct TCP sender with given default Retransmission Timeout and possible
   * ISN */
  TCPSender(uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn);

  /* Push bytes from the outbound stream */
  void push(Reader& outbound_stream);

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive(const TCPReceiverMessage& msg);

  /* Time has passed by the given # of milliseconds since the last time the
   * tick() method was called. */
  void tick(uint64_t ms_since_last_tick);

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight()
      const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions()
      const;  // How many consecutive *re*transmissions have happened?
};
