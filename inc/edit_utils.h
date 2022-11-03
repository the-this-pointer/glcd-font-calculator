#ifndef GLCDFONTCALCULATOR_EDIT_UTILS_H
#define GLCDFONTCALCULATOR_EDIT_UTILS_H

#include <stdint.h>

void MovePixelsUp(char* data, uint8_t w, uint8_t h);
void MovePixelsLeft(char* data, uint8_t w, uint8_t h);
void MovePixelsRight(char* data, uint8_t w, uint8_t h);
void MovePixelsDown(char* data, uint8_t w, uint8_t h);
void FlipPixelsVertical(char* data, uint8_t w, uint8_t h);
void FlipPixelsHorizontal(char* data, uint8_t w, uint8_t h);

#endif //GLCDFONTCALCULATOR_EDIT_UTILS_H
