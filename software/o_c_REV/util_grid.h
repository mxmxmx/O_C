#ifndef UTIL_GRID_H_
#define UTIL_GRID_H_

template <typename T>
struct vec2 {
  T x, y;

  vec2() : x(0), y(0) { }
  vec2(T ix, T iy) : x(ix), y(iy) { }
};

/**
 * Hold cells for "vector" sequencer, inspired by fcd72
 * https://dmachinery.wordpress.com/2013/01/05/the-vector-sequencer/
 *
 * Assumes there will only be positive movement; since rows/columns wrap
 * "negative" movement is possible with suitable offset.
 *
 * Uses fixed-point internal coordinates with specifiable fractional bits,
 * with hacky normalization using an epsilon value to avoid accumulating
 * error.
 * External interface uses "regular" integer values (mostly).
 */
template <typename cell_type, size_t dimensions, size_t fractional_bits, bool epsilon>
class CellGrid {
public:

  static const size_t CELLS = dimensions * dimensions;
  static const size_t DIMENSIONS_FP = dimensions << fractional_bits;

  void init() {
    current_pos_.x = current_pos_.y = 0;
    memset(cells_, 0, sizeof(cells_));
  }

  void reset() {
    current_pos_.x = 0;
    current_pos_.y = 0;
  }

  // @return true if current cell changed
  bool move(size_t dx, size_t dy) {
    size_t x = current_pos_.x;
    const size_t old_x = x >> fractional_bits;
    x += dx;
    while (x >= DIMENSIONS_FP)
      x -= DIMENSIONS_FP;
    if (epsilon && x <= epsilon)
      x = 0;
    current_pos_.x = x;

    size_t y = current_pos_.y;
    const size_t old_y = y >> fractional_bits;
    y += dy;
    while (y >= DIMENSIONS_FP)
      y -= DIMENSIONS_FP;
    if (epsilon && y <= epsilon)
      y = 0;
    current_pos_.y = y;

    return (old_x != x >> fractional_bits) || (old_y != y >> fractional_bits);
  }

  const cell_type *row(size_t index) const {
    return &cells_[index * dimensions];
  }

  const cell_type &at(size_t x, size_t y) const {
    return cells_[y * dimensions + x];
  }

  cell_type &mutable_cell(size_t x, size_t y) {
    return cells_[y * dimensions + x];
  }

  const cell_type &current_cell() const {
    return cells_[(current_pos_.y >> fractional_bits) * dimensions + (current_pos_.x >> fractional_bits)];
  }

  vec2<size_t> current_pos() const {
    return vec2<size_t>(current_pos_.x >> fractional_bits, current_pos_.y >> fractional_bits);
  }

private:

  vec2<size_t> current_pos_;

  cell_type cells_[CELLS];
};

#endif // UTIL_GRID_H_
