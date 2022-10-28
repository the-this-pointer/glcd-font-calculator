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
  0x0000,
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
  0x0000,
};

void ConvertCharToBuffer();
void ConvertBufferToChar(char* buffer);

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
  static struct nk_vec2 clicked_pos;
  static int clicked_active = 0;
  static int rclicked_active = 0;
  static char text_val[256];
  static int text_val_len;

  HWND hWnd = GetConsoleWindow();
  ShowWindow( hWnd, SW_MINIMIZE );  //won't hide the window without SW_MINIMIZE
  ShowWindow( hWnd, SW_HIDE );

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
    if (nk_begin(ctx, "Calculator", nk_rect(0, 0, WINDOW_WIDTH-240, WINDOW_HEIGHT), 0x00))
    {
      struct nk_command_buffer* out = nk_window_get_canvas(ctx);
      struct nk_rect bounding_rect = nk_window_get_bounds(ctx);
      uint16_t cell_size = (bounding_rect.h - 22) / COL_H;
      struct nk_rect rects_bounding_rect = nk_rect(12, 16, cell_size * COL_W, cell_size * COL_H);
      for (int i = 0; i < COL_H; ++i) {
        for (int j = 0; j < COL_W; ++j) {
          nk_fill_rect(out, nk_rect(12 + cell_size * j, 16 + cell_size * i, cell_size, cell_size), 0.0,
                       selected[(i * COL_W) + j] == 0? nk_rgba(57, 67, 71, 215) : nk_rgba(195, 55, 75, 255));
        }
      }

      if (nk_input_is_mouse_click_down_in_rect(&ctx->input, NK_BUTTON_LEFT,
                                               rects_bounding_rect,nk_true)){
        clicked_active = 1;
      }
      if (nk_input_is_mouse_click_down_in_rect(&ctx->input, NK_BUTTON_RIGHT,
                                                    rects_bounding_rect,nk_true)){
        rclicked_active = 1;
      }
      if (!nk_input_is_mouse_down(&ctx->input, NK_BUTTON_LEFT) && clicked_active) {
        clicked_active = 0;
      }
      if (!nk_input_is_mouse_down(&ctx->input, NK_BUTTON_RIGHT) && rclicked_active) {
        rclicked_active = 0;
      }

      if (clicked_active || rclicked_active) {
        clicked_pos.x = ctx->input.mouse.pos.x - 12;
        clicked_pos.y = ctx->input.mouse.pos.y - 16;
        int x = ((float)COL_W * clicked_pos.x) / rects_bounding_rect.w;
        int y = ((float)COL_H * clicked_pos.y) / rects_bounding_rect.h;

        selected[y * COL_W + x] = clicked_active? 1: 0;
      }
    }
    nk_end(ctx);

    if (nk_begin(ctx, "Value", nk_rect(WINDOW_WIDTH-245, 50, 230, 400),
                 NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
    {
      nk_layout_row_static(ctx, 50, 200, 1);
      nk_label_wrap(ctx, "Use left-click to set the box and right-click for resetting it!");

      nk_layout_row_static(ctx, 30, 80, 1);
      nk_label(ctx, "Value:", NK_TEXT_LEFT);
      if (nk_button_label(ctx, "Calc & Copy")) {
        ConvertBufferToChar(text_val);
        text_val_len = strlen(text_val);
      }
      nk_layout_row_static(ctx, 100, 200, 1);
      nk_edit_string(ctx, NK_EDIT_BOX, text_val, &text_val_len, 256, nk_filter_default);
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

void ConvertBufferToChar(char* buffer) {
  uint16_t i, j, ptr = 0;
  memset(buffer, 0, 256);

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
    sprintf(buffer + ptr, "0x%04X, ", character[i]);
    ptr += 8;
  }

  const size_t len = strlen(buffer) + 1;
  HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, len);
  memcpy(GlobalLock(hMem), buffer, len);
  GlobalUnlock(hMem);
  OpenClipboard(0);
  EmptyClipboard();
  SetClipboardData(CF_TEXT, hMem);
  CloseClipboard();
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
