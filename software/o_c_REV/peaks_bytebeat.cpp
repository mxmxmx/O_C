// Copyright 2015, 2013 Tim Churches, Olivier Gillet
//
// Author: Tim Churches (tim.churches@gmail.com)
// Loosely based on Peaks unbuffered processor code by: Olivier Gillet (Mutable Instruments)
// Byte beat equations: as indicated in the body of the code below.
// BitWiz equations used with permission of Jonatan Liljedahl <lijon@kymatica.com> (BitWiz author)
// Equation Composer equations from Microbe Modular Equation Composer - see https://github.com/clone45/EquationComposer
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
// Byte beats algorithms.

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
  loopmode_ = false ;
  loop_start_ = 0 ;
  loop_end_ = 255 << 24 ;
  phase_ = 0 ;
  t_ = 0 ;
  p0_ = 127;
  p1_ = 127;
  p2_ = 127;
  stepmode_ = false ;

}

int16_t ByteBeat::ProcessSingleSample(uint8_t control) {

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
// These equations push the boundaries of precedence comprehension.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
  uint8_t p0 = p0_ ;
  uint8_t p1 = p1_ ;
  uint8_t p2 = p2_ ;
    switch (equation_index_) {
        case 0:
          // from http://royal-paw.com/2012/01/bytebeats-in-c-and-python-generative-symphonies-from-extremely-small-programs/
          // (atmospheric, hopeful)
          sample = ( ( ((t_*3) & (t_>>10)) | ((t_*p0) & (t_>>10)) | ((t_*10) & ((t_>>8)*p1) & p2) ) & 0xFF) << 8;
          break;
        case 1:
          // equation by stephth via https://www.youtube.com/watch?v=tCRPUv8V22o at 3:38
          sample = ((((t_*p0) & (t_>>4)) | ((t_*p2) & (t_>>7)) | ((t_*p1) & (t_>>10))) & 0xFF) << 8;
          break;
        case 2: 
          // This one is the second one listed at from http://xifeng.weebly.com/bytebeats.html
          sample = ((( (((((t_ >> p0) | t_) | (t_ >> p0)) * p2) & ((5 * t_) | (t_ >> p2)) ) | (t_ ^ (t_ % p1)) ) & 0xFF)) << 8 ;
          break;
       case 3:
          // Arp rotator (equation 9 from Equation Composer Ptah bank)
          sample = ((t_>>(p2>>4))&(t_<<3)/(t_*p1*(t_>>11)%(3+((t_>>(16-(p0>>4)))%22))));
          break ;
        case 4: 
          //  BitWiz Transplant via Equation Composer Ptah bank 
          sample = (t_-((t_&p0)*p1-1668899)*((t_>>15)%15*t_))>>((t_>>12)%16)>>(p2%15);
          break ;
        case 5:
          // Vocaliser from Equation Composer Khepri bank         
          sample = ((t_%p0>>2)&p1)*(t_>>(p2>>5));
          break;
        case 6:
          // Chewie from Equation Composer Khepri bank         
          sample = (p0-(((p2+1)/t_)^p0|t_^922+p0))*(p2+1)/p0*((t_+p1)>>p1%19);
          break;
        default:
          // Tinbot from Equation Composer Sobek bank   
          sample = (t_/(40+p0)*(t_+t_|4-(p1+20)))+(t_*(p2>>5));
          break;      
  }
#pragma GCC diagnostic pop
  return sample ;
}

}  // namespace peaks
