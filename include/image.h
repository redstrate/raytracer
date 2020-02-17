#pragma once

#include <glm/glm.hpp>

template<class T, int Width, int Height>
class Image {
public:
    T& get(const int32_t x, const int32_t y) {
        return array[y * Width + x];
    }
    
    T get(const int32_t x, const int32_t y) const {
        return array[y * Width + x];
    }
    
    T array[Width * Height] = {};
};
