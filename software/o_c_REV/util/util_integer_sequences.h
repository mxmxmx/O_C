// Copyright (c) 2016 Tim Churches
// Utilising some code from https://github.com/xaocdevices/batumi/blob/alternate/lfo.cc
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef UTIL_INTEGER_SEQUENCES_H_
#define UTIL_INTEGER_SEQUENCES_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "../OC_strings.h"

namespace util {

class IntegerSequence {
public:



  void Init(int16_t i, int16_t l) {
    n_ = 0; // index of integer series
    modulus_ = 1; // default modulus
    i_ = i; // start of loop
    l_ = l; // length of loop
    j_ = i_ + l_; // end of loop
    k_ = i_; // current index
    x_ = 3; // first digit of pi
    s_ = 1; // stride for fractal sequance
    brownian_prob_ = 0 ;
    loop_ = true;
    pass_go_ = true;
    up_ = true;
    sk_ = 0;
  	// msb_pos_ = 0;
  	bit_sum_ = 0;
  	pending_bit_ = 0;
  	uint32_t _seed = OC::ADC::value<ADC_CHANNEL_1>() + OC::ADC::value<ADC_CHANNEL_2>() + OC::ADC::value<ADC_CHANNEL_3>() + OC::ADC::value<ADC_CHANNEL_4>();
    randomSeed(_seed);
  }

  uint16_t Clock() {
  	// Compare Brownian probability and reverse direction if needed
  	if (static_cast<int16_t>(random(0,256)) < brownian_prob_) up_ = !up_; 
		 	
  	if (loop_ || up_) {
  		k_ += 1;
  	} else {
  		k_ -= 1;
  	}
  	if (k_ > j_) {
  		if (loop_) {
  			k_ = i_;
  		} else {
  			k_ -= 2;
  			up_ = false;
  		}
  	}
  	if (k_ < i_) {
  		k_ += 2;
  		up_ = true;
  	}  	

  	if (k_ == i_) {
  		pass_go_ = true;
  	} else {
  		pass_go_ = false;
  	}

  	switch (n_) {
      case 0:
      	x_ = OC::Strings::pi_digits[k_];
      	break;
//      case 1:
//      	x_ = OC::Strings::phi_digits[k_];
//      	break;
//       case 2:
//       	x_ = OC::Strings::tau_digits[k_];
//       	break;
//       case 3:
//       	x_ = OC::Strings::eul_digits[k_];
//       	break;
//       case 4:
//       	x_ = OC::Strings::rt2_digits[k_];
//       	break;
      case 1:
      	x_ = OC::Strings::van_eck[k_];
      	break;
      case 2:
      	x_ = OC::Strings::sum_of_squares_of_digits_of_n[k_];
      	break;
      case 3: // Dress sequence
        x_ =  __builtin_popcountll(s_ * k_);
      	break;
      case 4: // Per Nørgård's infinity series
      	// See http://www.pernoergaard.dk/eng/strukturer/uendelig/ukonstruktion03.html
      	sk_ = static_cast<uint32_t>(s_ * k_) ;
      	// Serial.println(sk_) ;
      	// msb_pos_ = static_cast<uint8_t>(32 - __builtin_clzll(sk_)) ;
      	// Serial.println(msb_pos_) ;
      	bit_sum_ = 0 ;
      	pending_bit_ = 0;
      	for (int8_t b = 32; b > -1; --b) {
      		// Serial.println(b) ;
      		uint32_t bitmask = static_cast<uint32_t>(0x01 << b) ;
      		if ((sk_ & bitmask) == bitmask) {
      			bit_sum_ += pending_bit_ ;
      			pending_bit_ = 1;
      		} else {
      			pending_bit_ = -pending_bit_;
      		}
      	} 
      	x_ = 12 + bit_sum_ + pending_bit_ ; // add final bit
      	// x_ = bit_sum_ ;
      	break;
      case 5:
      	x_ = OC::Strings::digsum_of_n[k_];
      	break;
      case 6:
       	x_ = OC::Strings::digsum_of_n_base4[k_];
       	break;
      case 7:
       	x_ = OC::Strings::digsum_of_n_base5[k_];
       	break;    
      case 8:
      	x_ = OC::Strings::count_down_by_2[k_];
      	break;      	
      case 9:
      	x_ = OC::Strings::interspersion_of_A163253[k_];
      	break;      	
      default:
        break;
    }
    x_ = (x_ % modulus_) << 8 ;
    return static_cast<uint16_t>(x_);
  }

  uint16_t get_register() const {
    return x_ << 8;
  }

  void set_loop_start(int16_t i) {
    if (i < 0) i = 0;
    if (i > kIntSeqLen - 2) i = kIntSeqLen - 2;
    i_ = i; // loop start point
    j_ = i_ + l_;
    if (j_ < 1) j_ = 1;
    if (j_ > kIntSeqLen - 1) j_ = kIntSeqLen - 1;
    if (k_ < i_) k_ = i_;
    if (k_ > j_) k_ = j_;
  }
  
  void set_loop_length(int16_t l) {
    if (l < 1) l = 1;
    if (l > kIntSeqLen - 1) l = kIntSeqLen - 1;
    l_ = l; // loop length
    j_ = i_ + l_;
    if (j_ < 1) j_ = 1;
    if (j_ > kIntSeqLen - 1) j_ = kIntSeqLen - 1;
    if (k_ < i_) k_ = i_;
    if (k_ > j_) k_ = j_;
  }

  void set_loop_direction(bool p) {
    loop_ = p; // loop direction, false = swing (pendulum), true = loop
  }

  void set_int_seq(int16_t n) {
    n_ = n; 
  }

  void set_int_seq_modulus(int16_t m) {
    modulus_ = m; 
  }

  void set_fractal_stride(int16_t s) {
    s_ = s; 
  }

  void set_brownian_prob(int16_t p) {
    brownian_prob_ = p; 
  }

  void reset_loop() {
    k_ = i_;
  }

  int16_t get_k() const {
    return k_;
  }

  int16_t get_l() const {
    return l_;
  }

  int16_t get_i() const {
    return i_;
  }

  int16_t get_j() const {
    return j_;
  }

  int16_t get_n() const {
    return n_;
  }

  int16_t get_x() const {
    return x_;
  }

  int16_t get_s() const {
    return s_;
  }

  bool get_pass_go() const {
    return pass_go_;
  }

private:
  int16_t n_;
  int16_t modulus_;
  int16_t k_;
  int16_t i_;
  int16_t j_;
  int16_t l_;
  int16_t x_;
  int16_t s_;
  uint32_t sk_;
  // uint8_t msb_pos_;
  int8_t bit_sum_;
  int8_t pending_bit_;
  bool loop_;
  bool pass_go_;
  bool up_ ;
  int16_t brownian_prob_ ;
};

}; // namespace util

#endif // UTIL_INTEGER_SEQUENCES_H_
