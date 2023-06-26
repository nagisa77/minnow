#include "wrapping_integers.hh"
#include <cmath>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { static_cast<uint32_t>(n + zero_point.raw_value_) };
}

/*
  The first
  sequence number in the stream is a random 32-bit number called the Initial Sequence
  Number (ISN).
*/
uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const {
  uint64_t times = checkpoint / (1L << 32); 
  uint64_t offset; 
  if (raw_value_ >= zero_point.raw_value_) {
    offset = static_cast<uint64_t>(raw_value_) - 
             static_cast<uint64_t>(zero_point.raw_value_); 
  } else {
    offset = static_cast<uint64_t>(raw_value_) + (1L << 32) - 
             static_cast<uint64_t>(zero_point.raw_value_); 
  }

  uint64_t res1 = times * (1L << 32) + offset; 
  uint64_t res2 = (times + 1) * (1L << 32) + offset; 
  uint64_t res3 = (times - 1) * (1L << 32) + offset; 
  uint64_t off1, off2, off3;
  if (res1 > checkpoint) {
    off1 = res1 - checkpoint;
  } else {
    off1 = checkpoint - res1; 
  }
  if (res2 > checkpoint) {
    off2 = res2 - checkpoint; 
  } else {
    off2 = checkpoint - res2; 
  }
  if (res3 > checkpoint) {
    off3 = res3 - checkpoint; 
  } else {
    off3 = checkpoint - res3; 
  }
  if (times == 0) {
    if (off1 < off2) {
      return res1; 
    } else {
      return res2; 
    }
  } else {
    if (off1 < off2 && off1 < off3) {
      return res1; 
    } else if (off2 < off1 && off2 < off3) {
      return res2;
    } else {
      return res3; 
    }
  }
}
