#include <edit_utils.h>

void FlipPixelsHorizontal(char* data, uint8_t w, uint8_t h) {
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w / 2; i++) {
      int temp = data[(j * w) + i];
      data[(j * w) + i] = data[((j + 1) * w) - i - 1];
      data[((j + 1) * w) - i - 1] = temp;
    }
  }
}

void FlipPixelsVertical(char* data, uint8_t w, uint8_t h) {
  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h / 2; j++) {
      int t1 = (j * w) + i;
      int t2 = ((h - j - 1) * w) + i;
      int t3 = (h - j);
      int temp = data[(j * w) + i];
      data[(j * w) + i] = data[((h - j - 1) * w) + i];
      data[((h - j - 1) * w) + i] = temp;
    }
  }
}

void MovePixelsDown(char* data, uint8_t w, uint8_t h) {
  for(int i=w * h; i >= 0; i--) {
    if (i > w)
      data[i] = data[i - w];
    else
      data[i] = 0;
  }
}

void MovePixelsRight(char* data, uint8_t w, uint8_t h) {
  for (int j = 0; j < h; j++) {
    for (int i = w - 2; i >= 0; i--) {
      data[(j * w) + i + 1] = data[(j * w) + i];
    }
    data[(j * w)] = 0;
  }
}

void MovePixelsLeft(char* data, uint8_t w, uint8_t h) {
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w - 1; i++) {
      data[(j * w) + i] = data[(j * w) + i + 1];
    }
    data[((j + 1) * w) - 1] = 0;
  }
}

void MovePixelsUp(char* data, uint8_t w, uint8_t h) {
  for(int i=0;i < (w * h);i++) {
    if (i < w * (h-1))
      data[i] = data[i + w];
    else
      data[i] = 0;
  }
}