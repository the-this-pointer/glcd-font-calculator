/* nuklear - 1.32.0 - public domain */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <edit_utils.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_GDIP_IMPLEMENTATION
#include <nuklear/nuklear.h>
#include <nuklear/nuklear_gdip.h>
#include "style.c"

//#define MAX_BUFFER_SIZE 2048

uint16_t window_width = 460;
uint16_t window_height = 480;

struct AppState {
  uint8_t col_w;
  uint8_t col_h;
  int show_font_settings;
  int show_app_about;
  int show_hint;
} appState;

char* selectedData = NULL;
uint16_t* characterData = NULL;

void DeleteData();
void CreateData();
void ConvertCharToBuffer();
void ConvertBufferToCharAndCopy();
void ReadFromClipboard(char *buffer);
void WriteToClipboard(const char *buffer);
void ReadAndParseClipboard();

static LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg) {
    case WM_SIZE:
      window_width = LOWORD(lparam);
      window_height = HIWORD(lparam);
      break;
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
  RECT rect = { 0, 0, window_width, window_height };
  DWORD style = WS_OVERLAPPEDWINDOW;
  DWORD exstyle = WS_EX_APPWINDOW;
  HWND wnd;
  int running = 1;
  int needs_refresh = 1;
  static struct nk_vec2 clicked_pos;
  static int clicked_active = 0;
  static int rclicked_active = 0;

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
  ctx = nk_gdip_init(wnd, window_width, window_height);
  font = nk_gdipfont_create("Arial", 12);
  nk_gdip_set_font(font);
  set_style(ctx, THEME_RED);

  /* Default Values */
  appState.col_w = 11;
  appState.col_h = 18;
  appState.show_font_settings = nk_false;
  appState.show_app_about = nk_false;
  appState.show_hint = nk_false;

  CreateData();

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
    if (nk_begin(ctx, "Calculator", nk_rect(0, 0, window_width, window_height), 0x00))
    {
      /* menubar */
      nk_menubar_begin(ctx);

      nk_layout_row_begin(ctx, NK_STATIC, 25, 5);
      nk_layout_row_push(ctx, 45);
      if (nk_menu_begin_label(ctx, "MENU", NK_TEXT_LEFT, nk_vec2(120, 200)))
      {
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_menu_item_label(ctx, "Settings", NK_TEXT_LEFT))
          appState.show_font_settings = nk_true;
        if (nk_menu_item_label(ctx, "Hint", NK_TEXT_LEFT))
          appState.show_hint = nk_true;
        if (nk_menu_item_label(ctx, "About", NK_TEXT_LEFT))
          appState.show_app_about = nk_true;

        nk_menu_end(ctx);
      }
      if (nk_menu_begin_label(ctx, "VALUE", NK_TEXT_LEFT, nk_vec2(120, 200)))
      {
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_menu_item_label(ctx, "Read f. Clipboard", NK_TEXT_LEFT)) {
          ReadAndParseClipboard();
        }
        if (nk_menu_item_label(ctx, "Calc & Copy", NK_TEXT_LEFT)) {
          ConvertBufferToCharAndCopy();
        }

        nk_menu_end(ctx);
      }

      nk_menubar_end(ctx);
      /* menubar end */

      struct nk_command_buffer* out = nk_window_get_canvas(ctx);
      struct nk_rect bounding_rect = nk_window_get_bounds(ctx);
      uint16_t cell_size = min((window_height - 50) / appState.col_h, (window_width - 190) / appState.col_w);
      struct nk_rect rects_bounding_rect = nk_rect(180, 44, cell_size * appState.col_w, cell_size * appState.col_h);
      for (int i = 0; i < appState.col_h; ++i) {
        for (int j = 0; j < appState.col_w; ++j) {
          nk_fill_rect(out, nk_rect(180 + cell_size * j, 44 + cell_size * i, cell_size, cell_size), 0.0,
                       selectedData[(i * appState.col_w) + j] == 0? nk_rgba(57, 67, 71, 215) : nk_rgba(195, 55, 75, 255));
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
      if (appState.show_font_settings || appState.show_hint || appState.show_app_about) {
        clicked_active = 0;
        rclicked_active = 0;
      }
      if (!nk_input_is_mouse_down(&ctx->input, NK_BUTTON_LEFT) && clicked_active) {
        clicked_active = 0;
      }
      if (!nk_input_is_mouse_down(&ctx->input, NK_BUTTON_RIGHT) && rclicked_active) {
        rclicked_active = 0;
      }

      if (clicked_active || rclicked_active) {
        clicked_pos.x = ctx->input.mouse.pos.x - 180;
        clicked_pos.y = ctx->input.mouse.pos.y - 44;
        int x = ((float)appState.col_w * clicked_pos.x) / rects_bounding_rect.w;
        int y = ((float)appState.col_h * clicked_pos.y) / rects_bounding_rect.h;

        selectedData[y * appState.col_w + x] = clicked_active? 1: 0;
      }

      if (nk_tree_push(ctx, NK_TREE_NODE, "Edit", NK_MAXIMIZED)) {
        nk_layout_row_static(ctx, 30, 100, 1);
        if (nk_button_label(ctx, "Clear")) {
          memset(selectedData, 0, appState.col_w * appState.col_h);
        }

        nk_layout_row_static(ctx, 30, 48, 2);
        if (nk_button_label(ctx, "Flip H")) {
          FlipPixelsHorizontal(selectedData, appState.col_w, appState.col_h);
        }
        if (nk_button_label(ctx, "Flip V")) {
          FlipPixelsVertical(selectedData, appState.col_w, appState.col_h);
        }

        nk_layout_row_static(ctx, 30, 30, 3);

        nk_label(ctx, "", NK_TEXT_LEFT);
        if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_UP)) {
          MovePixelsUp(selectedData, appState.col_w, appState.col_h);
        }
        nk_label(ctx, "", NK_TEXT_LEFT);

        if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_LEFT)) {
          MovePixelsLeft(selectedData, appState.col_w, appState.col_h);
        }
        nk_label(ctx, "", NK_TEXT_LEFT);
        if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_RIGHT)) {
          MovePixelsRight(selectedData, appState.col_w, appState.col_h);
        }

        nk_label(ctx, "", NK_TEXT_LEFT);
        if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_DOWN)) {
          MovePixelsDown(selectedData, appState.col_w, appState.col_h);
        }
        nk_label(ctx, "", NK_TEXT_LEFT);

        nk_tree_pop(ctx);
      }
    }

    if (appState.show_app_about)
    {
      /* about popup */
      static struct nk_rect s;
      s.x = (window_width - 300)/2;
      s.y = (window_height - 120)/2;
      s.w = 300;
      s.h = 120;
      if (nk_popup_begin(ctx, NK_POPUP_STATIC, "About", NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE, s))
      {
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "SSD1306 GLCD Font Calculator", NK_TEXT_LEFT);
        nk_label(ctx, "By Rsomething", NK_TEXT_LEFT);
        nk_label(ctx, "https://github.com/the-this-pointer",  NK_TEXT_LEFT);
        nk_popup_end(ctx);
      } else appState.show_app_about = nk_false;
    }
    if (appState.show_hint)
    {
      /* hint popup */
      static struct nk_rect s;
      s.x = (window_width - 300)/2;
      s.y = (window_height - 120)/2;
      s.w = 300;
      s.h = 120;
      if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Hint", NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE, s))
      {
        nk_layout_row_static(ctx, 50, 190, 1);
        nk_label_wrap(ctx, "Use left-click to set the box.\r\nUse right-click to reset it.");

        nk_popup_end(ctx);
      } else appState.show_hint = nk_false;
    }

    if (appState.show_font_settings)
    {
      /* about popup */
      static struct nk_rect s;
      s.x = (window_width - 300)/2;
      s.y = (window_height - 270)/2;
      s.w = 300;
      s.h = 270;
      if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Settings", NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE, s))
      {
        static char text[2][8];
        static int text_len[2];

        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, "Horizontal Cols:", NK_TEXT_LEFT);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, text[0], &text_len[0], 8, nk_filter_decimal);
        nk_label(ctx, "Vertical Cols:", NK_TEXT_LEFT);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, text[1], &text_len[1], 8, nk_filter_decimal);

        if (nk_button_label(ctx, "Change!")) {
          text[0][text_len[0]] = '\0';
          text[1][text_len[1]] = '\0';

          uint8_t col_w = atol(text[0]);
          if (col_w <= 16) {
            appState.col_w = col_w;
            appState.col_h = atol(text[1]);
            CreateData();
          }
        }

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label_colored(ctx, "Caution! Maximum value for h.cols is 16!", NK_TEXT_LEFT, nk_rgb(255,255,0));
        nk_label_colored(ctx, "Caution! Changing this will reset your work!", NK_TEXT_LEFT, nk_rgb(255,255,0));

        nk_popup_end(ctx);
      } else appState.show_font_settings = nk_false;
    }

    nk_end(ctx);

    /* Draw */
    nk_gdip_render(NK_ANTI_ALIASING_ON, nk_rgb(30,30,30));
  }

  DeleteData();
  nk_gdipfont_del(font);
  nk_gdip_shutdown();
  UnregisterClassW(wc.lpszClassName, wc.hInstance);
  return 0;
}

void ReadAndParseClipboard() {
  char* buffer = malloc(appState.col_w * appState.col_h + 1);
  memset(buffer, 0, appState.col_w * appState.col_h + 1);
  ReadFromClipboard(buffer);
  buffer[appState.col_w * appState.col_h] = '\0';

  int succeed = 1;
  int count = 0;
  const char* ptr = buffer;
  do
  {
    while(ptr && *ptr != 'x') ptr++;
    if (!ptr)
      break;
    ptr++;

    uint32_t val = 0;
    if (sscanf(ptr, "%x,", &val) == 1)
    {
      characterData[count] = val & 0xFFFF;
      count++;
    }
    else
      succeed = 0;
  } while(succeed && count < appState.col_h);
  free(buffer);

  if (count != appState.col_h) {
    memset(selectedData, 0, appState.col_w * appState.col_h);
    memset(characterData, 0, sizeof(uint16_t) * appState.col_h);
    // show error
    return;
  }
  ConvertCharToBuffer();
}

void WriteToClipboard(const char *buffer) {
  const size_t len = strlen(buffer) + 1;
  HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, len);
  memcpy(GlobalLock(hMem), buffer, len);
  GlobalUnlock(hMem);
  OpenClipboard(0);
  EmptyClipboard();
  SetClipboardData(CF_TEXT, hMem);
  CloseClipboard();
}

void ReadFromClipboard(char *buffer) {
  HANDLE h;
  OpenClipboard(NULL);
  h = GetClipboardData(CF_TEXT);
  sprintf(buffer, "%s", (char *)h);
  CloseClipboard();
}

void ConvertBufferToCharAndCopy() {
  uint16_t i, j, ptr = 0;
  char* buffer = malloc(appState.col_w * appState.col_h + 1);
  memset(buffer, 0, appState.col_w * appState.col_h + 1);

  for (i = 0; i < appState.col_h; i++)
  {
    characterData[i] = 0;
    for (j = 0; j < appState.col_w; j++)
    {
      int bit = 15 - j;
      if (selectedData[j + (i * appState.col_w)])
      {
        characterData[i] |= ((uint16_t)1 << bit);
      }
      else
      {
        characterData[i] &= ~((uint16_t)1 << bit);
      }
    }
    sprintf(buffer + ptr, "0x%04X, ", characterData[i]);
    ptr += 8;
  }

  WriteToClipboard(buffer);
  free(buffer);
}

void ConvertCharToBuffer() {
  uint16_t i, b, j;
  memset(selectedData, 0, appState.col_w * appState.col_h);
  for (i = 0; i < appState.col_h; i++)
  {
    b = characterData[i];
    for (j = 0; j < appState.col_w; j++)
    {
      if ((b << j) & 0x8000)
      {
        selectedData[j + (i * appState.col_w)] |= 1 << (i % 8);
      }
      else
      {
        selectedData[j + (i * appState.col_w)] &= ~(1 << (i % 8));
      }
    }
  }
}

void DeleteData() {
  if (characterData)
    free(characterData);
  if (selectedData)
    free(selectedData);
}

void CreateData() {
  DeleteData();

  selectedData = malloc(appState.col_w * appState.col_h);
  memset(selectedData, 0, appState.col_w * appState.col_h);

  characterData = malloc(sizeof(uint16_t) * appState.col_h);
  memset(characterData, 0, sizeof(uint16_t) * appState.col_h);
}
