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
  {3, {512, 0}},
  // 5
  {4, {409, 0}},
  // 6
  {5, {341, 0}},
  // 7
  {6, {293, 0}},
  // 8
  {7, {256, 0}},
  // 9
  {8, {228, 0}},
  // 10
  {9, {205, 0}},
  // 11
  {10, {186, 0}},
  // 12
  {11, {171, 0}},
  // 13
  {12, {158, 0}},
  // 14
  {13, {146, 0}},
  // 15
  {14, {137, 0}},
  // 16
  {15, {128, 0}},
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
