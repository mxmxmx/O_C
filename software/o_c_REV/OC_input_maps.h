#ifndef OC_INPUT_MAPS_H_
#define OC_INPUT_MAPS_H_

#include "OC_input_map.h"

namespace OC {

// {range: 5v, range: -3.5/3.5} :
const Map input_maps[] = {
  // # slots: 0
  {0, {0, 0}},
  // 1
  {0, {0, 0}},
  // 2
  {0, {0, 0}},
  // 3
  {0, {0, 0}},
  // 4
  {3, {512, 717}},
  // 5
  {4, {409, 573}},
  // 6
  {5, {341, 478}},
  // 7
  {6, {293, 410}},
  // 8
  {7, {256, 358}},
  // 9
  {8, {228, 318}},
  // 10
  {9, {205, 287}},
  // 11
  {10, {186, 261}},
  // 12
  {11, {171, 239}},
  // 13
  {12, {158, 221}},
  // 14
  {13, {146, 205}},
  // 15
  {14, {137, 191}},
  // 16
  {15, {128, 179}},
  // 17
  {0, {0, 0}}
};

}  

namespace OC {

  typedef OC::Map Map;

  class InputMaps {
  public:
    static const Map &GetInputMap(int index) { 
      return input_maps[index]; 
    }
  };
}

#endif  // OC_INPUT_MAPS_H_
