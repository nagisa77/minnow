#include "tcp_receiver.hh"
#include <cmath>

using namespace std;

void TCPReceiver::receive(TCPSenderMessage message, Reassembler& reassembler,
                          Writer& inbound_stream) {
  if (message.SYN) {
    zero_point_ = message.seqno;
    checkpoint_ += 1; 
  } else if (!zero_point_.has_value()) {
    return; 
  }
  uint64_t bytes_pushed_before = inbound_stream.bytes_pushed();
  uint64_t insert_index = message.seqno.unwrap(zero_point_.value(), checkpoint_);
  reassembler.insert(insert_index - (message.SYN ? 0 : 1), 
                     message.payload.release(), 
                     message.FIN, 
                     inbound_stream); 
  uint64_t bytes_pushed_after = inbound_stream.bytes_pushed();
  checkpoint_ += bytes_pushed_after - bytes_pushed_before + inbound_stream.is_closed(); 
}

TCPReceiverMessage TCPReceiver::send(const Writer& inbound_stream) const {
  TCPReceiverMessage result; 
  if (zero_point_.has_value()) {
    result.ackno = Wrap32::wrap(checkpoint_, zero_point_.value()); 
  } 
  result.window_size = static_cast<uint16_t>(std::min(inbound_stream.available_capacity(), 
                                                      static_cast<uint64_t>(UINT16_MAX)));
  return result; 
}
