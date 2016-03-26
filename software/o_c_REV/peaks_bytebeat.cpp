// Copyright 2015 Tim Churches.
//
// Author: Tim Churches (tim.churches@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Byte beats

#include "peaks_bytebeat.h"

#include "extern/stmlib_utils_dsp.h"
// #include "util/stmlib_random.h"

namespace peaks {

using namespace stmlib;

const uint8_t kDownsample = 4;
const uint8_t kMaxEquationIndex = 1;

void ByteBeat::Init() {
  equation_ = 0;
  speed_ = 32678;
  loop_start_ = 0 ;
  loop_end_ = 255 << 24 ;
  phase_ = 0 ;
  t_ = 0 ;
  p0_ = 32678;
  p1_ = 32678;
  p2_ = 32678;
  stepmode_ = false ;

}

int16_t ByteBeat::ProcessSingleSample(uint8_t control) {
  uint32_t p0 = 0;
  uint32_t p1 = 0;
  uint32_t p2 = 0;
  uint16_t sample = 0;
   
  if (control & CONTROL_GATE_RISING) {
    if (stepmode_) {
      ++t_ ;
    } else {
      phase_ = 0;
      if (loopmode_) {
        t_ = loop_start_ ;
      } else {
        t_ = 0 ;
      }
    }
  }

  if (!stepmode_) {
    ++phase_ ; 
  }    
  
  if (loopmode_ && (t_ < loop_start_ || t_ > loop_end_)) {
     t_ = loop_start_ ;
     phase_ = 0 ;
  }

  if (!stepmode_ && (phase_ % bytepitch_ == 0)) ++t_; 

    switch (equation_index_) {
        case 0:
          // from http://royal-paw.com/2012/01/bytebeats-in-c-and-python-generative-symphonies-from-extremely-small-programs/
          // (atmospheric, hopeful)
          p0 = p0_ >> 8; // was 9
          p1 = p1_ >> 8; // was 9
          p2 = p2_ >> 8; // was 9
          // sample = ( ( ((t_*3) & (t_>>10)) | ((t_*p0) & (t_>>10)) | ((t_*10) & ((t_>>8)*p1) & 128) ) & 0xFF) << 8;
          sample = ( ( ((t_*3) & (t_>>10)) | ((t_*p0) & (t_>>10)) | ((t_*10) & ((t_>>8)*p1) & p2) ) & 0xFF) << 8;
          break;
        case 1:
          p0 = p0_ >> 8; // was 11
          p1 = p1_ >> 8; // was 11
          p2 = p2_ >> 8; // was 11
          // equation by stephth via https://www.youtube.com/watch?v=tCRPUv8V22o at 3:38
          // sample = ((((t_*p0) & (t_>>4)) | ((t_*5) & (t_>>7)) | ((t_*p1) & (t_>>10))) & 0xFF) << 8;
          sample = ((((t_*p0) & (t_>>4)) | ((t_*p2) & (t_>>7)) | ((t_*p1) & (t_>>10))) & 0xFF) << 8;
          break;
        case 2: 
          p0 = p0_ >> 8; // was 11
          p1 = p1_ >> 8;  // was 9
          p2 = p2_ >> 8 ; // was 12
          // This one is the second one listed at from http://xifeng.weebly.com/bytebeats.html
          sample = ((( (((((t_ >> p0) | t_) | (t_ >> p0)) * p2) & ((5 * t_) | (t_ >> p2)) ) | (t_ ^ (t_ % p1)) ) & 0xFF)) << 8 ;
          break;
       case 3:
          p0 = p0_ >> 8;
          p1 = p1_ >> 8;
          p2 = p2_ >> 8 ;
          // Question/answer (equation 9 from Equation Composer Ptah bank)
          sample = ((t_*(t_>>8|t_>>p2)&p1&t_>>8))^(t_&t_>>p0|t_>>6);
          break ;
        /*  
        case 3: 
          p0 = p0_ >> 9;
          p1 = t_ % p1_ ;
          p2 = p2_ >> 13 ;
          // Warping overtone echo drone, from BitWiz
          // sample = ((t_&p0)-(t_%p1))^(t_>>7);  
          // sample = ((t_&p0)-(t_%p1))^(t_>>p2);  
          sample = t_*(((t_>>p0)^((t_>>p1)-1)^1)%p2) ; 
          break;
        */
        case 4: 
          p0 = p0_ >> 9; // was 9
          p1 = p1_ >> 10; // was 11
          p2 = p2_ >> 10; // was 11
          //  Mobius loop (eqn 4) from Equation Composer Sobek bank 
          sample = ((sample&p0)|(69*p1)|(p2^t_))+((sample%(4333-p0))>>2); 
          break ;
        case 5:
          p0 = p0_ >> 12;
          p1 = p1_ >> 12;
          p2 = p2_ >> 12;
          //  Hannah (eqn 3) from Equation Composer Sobek bank  
          sample = (((t_<<t_)+(t_%(sample>>2)-t_+p0))>>p2) + p1; 
          break;
        case 6:
          p0 = p0_ >> 9;
          p1 = p1_ >> 10; // was 9
          p2 = p2_ >> 4 ;
          // The Smoker from Equation Composer Khepri bank
          // sample = sample ^ (t_>>(p1>>4)) >> ((t_/6988*t_%(p0+1))+(t_<<t_/(p1 * 4)));
          sample = sample ^ (t_>>(p1>>4)) >> ((t_/p2*t_%(p0+1))+(t_<<t_/(p1 * 4)));
          break;
        default:
          p0 = p0_ >> 12;
          p1 = p1_ >> 12;
          p2 = p2_ >> 8 ;
          // This one is from http://www.reddit.com/r/bytebeat/comments/20km9l/cool_equations/ (t>>13&t)*(t>>8)
          sample = ( (((t_ >> p0) & t_) * (t_ >> p1)) & 0xFF) << 8 ;
          // sample = ( (((t_ >> p0) & t_) * (t_ >> p1)) & p2) << 8 ;
          break;      
  }
  return sample ;
}

}  // namespace peaks
