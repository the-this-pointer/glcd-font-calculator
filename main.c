/* nuklear - 1.32.0 - public domain */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_GDIP_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_gdip.h"
#include "style.c"

#define COL_W 11
#define COL_H 18
#define CELL_SIZE 20
#define WINDOW_WIDTH COL_W * CELL_SIZE + (COL_W * 6) + 250
#define WINDOW_HEIGHT COL_H * CELL_SIZE + (COL_H * 6)

static int selected[COL_W * COL_H] = {0};
static uint16_t character[COL_H] = {
  /*0x0000,
  0x0C00,
  0x0C00,
  0x0C00,
  0x0C00,
  0x0C00,
  0x0C00,
  0x0C00,
  0x0C00,
  0x0C00,
  0x0C00,
  0x0C00,
  0x0000,
  0x0C00,
  0x0C00,
  0x0000,
  0x0000,
  0x0000,*/
    0x0000
};

void ConvertCharToBuffer();

void ConvertBufferToChar();

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/
static LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  if (nk_gdip_handle_event(wnd, msg, wparam, lparam))
    return 0;
  return DefWindowProcW(wnd, msg, wparam, lparam);
}

int main(void)
{
  GdipFont* font;
  struct nk_context *ctx;

  WNDCLASSW wc;
  RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
  DWORD style = WS_OVERLAPPEDWINDOW;
  DWORD exstyle = WS_EX_APPWINDOW;
  HWND wnd;
  int running = 1;
  int needs_refresh = 1;

  /* Win32 */
  memset(&wc, 0, sizeof(wc));
  wc.style = CS_DBLCLKS;
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = GetModuleHandleW(0);
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = L"CalculatorWindowClass";
  RegisterClassW(&wc);

  AdjustWindowRectEx(&rect, style, FALSE, exstyle);

  wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"SSD1306 Calculator",
                        style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                        rect.right - rect.left, rect.bottom - rect.top,
                        NULL, NULL, wc.hInstance, NULL);

  /* GUI */
  ctx = nk_gdip_init(wnd, WINDOW_WIDTH, WINDOW_HEIGHT);
  font = nk_gdipfont_create("Arial", 12);
  nk_gdip_set_font(font);
  set_style(ctx, THEME_RED);

  // Convert char to selected buffer
  ConvertCharToBuffer();

  while (running)
  {
    /* Input */
    MSG msg;
    nk_input_begin(ctx);
    if (needs_refresh == 0) {
      if (GetMessageW(&msg, NULL, 0, 0) <= 0)
        running = 0;
      else {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
      }
      needs_refresh = 1;
    } else needs_refresh = 0;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT)
        running = 0;
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
      needs_refresh = 1;
    }
    nk_input_end(ctx);

    /* GUI */
    if (nk_begin(ctx, "Calculator", nk_rect(0, 0, WINDOW_WIDTH-250, WINDOW_HEIGHT), 0x00))
    {
      int i;
      nk_layout_row_static(ctx, CELL_SIZE, CELL_SIZE, 11);
      for (i = 0; i < COL_W * COL_H; ++i) {
        nk_checkbox_label(ctx, "", &selected[i]);
      }
    }
    nk_end(ctx);

    if (nk_begin(ctx, "Value", nk_rect(WINDOW_WIDTH-250, 50, 200, 200),
                 NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
    {
      nk_layout_row_static(ctx, 30, 80, 1);
      if (nk_button_label(ctx, "Print value")) {
        ConvertBufferToChar();
      }
    }
    nk_end(ctx);

    /* Draw */
    nk_gdip_render(NK_ANTI_ALIASING_ON, nk_rgb(30,30,30));
  }

  nk_gdipfont_del(font);
  nk_gdip_shutdown();
  UnregisterClassW(wc.lpszClassName, wc.hInstance);
  return 0;
}

void ConvertBufferToChar() {
  uint16_t i, b, j;
  for (i = 0; i < COL_H; i++)
  {
    character[i] = 0;
    for (j = 0; j < COL_W; j++)
    {
      int bit = 15 - j;
      if (selected[j + (i * COL_W)])
      {
        character[i] |= ((uint16_t)1 << bit);
      }
      else
      {
        character[i] &= ~((uint16_t)1 << bit);
      }
    }
    printf("0x%04X, ", character[i]);
  }
}

void ConvertCharToBuffer() {
  uint16_t i, b, j;
  for (i = 0; i < COL_H; i++)
  {
    b = character[i];
    for (j = 0; j < COL_W; j++)
    {
      if ((b << j) & 0x8000)
      {
        selected[j + (i * COL_W)] |= 1 << (i % 8);
      }
      else
      {
        selected[j + (i * COL_W)] &= ~(1 << (i % 8));
      }
    }
  }
}
