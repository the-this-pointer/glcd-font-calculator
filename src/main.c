/* nuklear - 1.32.0 - public domain */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_GDIP_IMPLEMENTATION
#include <nuklear.h>
#include <nuklear_gdip.h>
#include "style.c"

#define MAX_BUFFER_SIZE 2048

static int show_font_settings = nk_false;
static int show_app_about = nk_false;
static int show_hint = nk_false;

uint16_t windowWidth = 460;
uint16_t windowHeight = 480;

struct AppState {
  uint8_t colW;
  uint8_t colH;
} appState;

char* selectedData = NULL;
uint16_t* characterData = NULL;

void DeleteData();
void CreateData();
void ConvertCharToBuffer();
void ConvertBufferToCharAndCopy();
void MovePixelsUp();
void MovePixelsLeft();
void MovePixelsRight();
void MovePixelsDown();
void FlipPixelsVertical();
void FlipPixelsHorizontal();
void ReadFromClipboard(char *buffer);
void WriteToClipboard(const char *buffer);
void ReadAndParseClipboard();

static LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg) {
    case WM_SIZE:
      windowWidth = LOWORD(lparam);
      windowHeight = HIWORD(lparam);
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
  RECT rect = { 0, 0, windowWidth, windowHeight };
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
  ctx = nk_gdip_init(wnd, windowWidth, windowHeight);
  font = nk_gdipfont_create("Arial", 12);
  nk_gdip_set_font(font);
  set_style(ctx, THEME_RED);

  /* Default Values */
  appState.colW = 11;
  appState.colH = 18;
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
    if (nk_begin(ctx, "Calculator", nk_rect(0, 0, windowWidth, windowHeight), 0x00))
    {
      /* menubar */
      nk_menubar_begin(ctx);

      nk_layout_row_begin(ctx, NK_STATIC, 25, 5);
      nk_layout_row_push(ctx, 45);
      if (nk_menu_begin_label(ctx, "MENU", NK_TEXT_LEFT, nk_vec2(120, 200)))
      {
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_menu_item_label(ctx, "Settings", NK_TEXT_LEFT))
          show_font_settings = nk_true;
        if (nk_menu_item_label(ctx, "Hint", NK_TEXT_LEFT))
          show_hint = nk_true;
        if (nk_menu_item_label(ctx, "About", NK_TEXT_LEFT))
          show_app_about = nk_true;

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
      uint16_t cell_size = min((windowHeight - 50) / appState.colH, (windowWidth - 190) / appState.colW);
      struct nk_rect rects_bounding_rect = nk_rect(180, 44, cell_size * appState.colW, cell_size * appState.colH);
      for (int i = 0; i < appState.colH; ++i) {
        for (int j = 0; j < appState.colW; ++j) {
          nk_fill_rect(out, nk_rect(180 + cell_size * j, 44 + cell_size * i, cell_size, cell_size), 0.0,
                       selectedData[(i * appState.colW) + j] == 0? nk_rgba(57, 67, 71, 215) : nk_rgba(195, 55, 75, 255));
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
      if (show_font_settings || show_hint || show_app_about) {
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
        int x = ((float)appState.colW * clicked_pos.x) / rects_bounding_rect.w;
        int y = ((float)appState.colH * clicked_pos.y) / rects_bounding_rect.h;

        selectedData[y * appState.colW + x] = clicked_active? 1: 0;
      }

      if (nk_tree_push(ctx, NK_TREE_NODE, "Edit", NK_MAXIMIZED)) {
        nk_layout_row_static(ctx, 30, 100, 1);
        if (nk_button_label(ctx, "Clear")) {
          memset(selectedData, 0, appState.colW * appState.colH);
        }

        nk_layout_row_static(ctx, 30, 48, 2);
        if (nk_button_label(ctx, "Flip H")) {
          FlipPixelsHorizontal();
        }
        if (nk_button_label(ctx, "Flip V")) {
          FlipPixelsVertical();
        }

        nk_layout_row_static(ctx, 30, 30, 3);

        nk_label(ctx, "", NK_TEXT_LEFT);
        if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_UP)) {
          MovePixelsUp();
        }
        nk_label(ctx, "", NK_TEXT_LEFT);

        if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_LEFT)) {
          MovePixelsLeft();
        }
        nk_label(ctx, "", NK_TEXT_LEFT);
        if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_RIGHT)) {
          MovePixelsRight();
        }

        nk_label(ctx, "", NK_TEXT_LEFT);
        if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_DOWN)) {
          MovePixelsDown();
        }
        nk_label(ctx, "", NK_TEXT_LEFT);

        nk_tree_pop(ctx);
      }
    }

    if (show_app_about)
    {
      /* about popup */
      static struct nk_rect s;
      s.x = (windowWidth - 300)/2;
      s.y = (windowHeight - 120)/2;
      s.w = 300;
      s.h = 120;
      if (nk_popup_begin(ctx, NK_POPUP_STATIC, "About", NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE, s))
      {
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "SSD1306 GLCD Font Calculator", NK_TEXT_LEFT);
        nk_label(ctx, "By Rsomething", NK_TEXT_LEFT);
        nk_label(ctx, "https://github.com/the-this-pointer",  NK_TEXT_LEFT);
        nk_popup_end(ctx);
      } else show_app_about = nk_false;
    }
    if (show_hint)
    {
      /* hint popup */
      static struct nk_rect s;
      s.x = (windowWidth - 300)/2;
      s.y = (windowHeight - 120)/2;
      s.w = 300;
      s.h = 120;
      if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Hint", NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE, s))
      {
        nk_layout_row_static(ctx, 50, 190, 1);
        nk_label_wrap(ctx, "Use left-click to set the box.\r\nUse right-click to reset it.");

        nk_popup_end(ctx);
      } else show_hint = nk_false;
    }

    if (show_font_settings)
    {
      /* about popup */
      static struct nk_rect s;
      s.x = (windowWidth - 300)/2;
      s.y = (windowHeight - 250)/2;
      s.w = 300;
      s.h = 250;
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

          appState.colW = atol(text[0]);
          appState.colH = atol(text[1]);
          CreateData();
        }

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label_colored(ctx, "Caution! Changing this will reset your work!", NK_TEXT_LEFT, nk_rgb(255,255,0));
        
        nk_popup_end(ctx);
      } else show_font_settings = nk_false;
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
  char buffer[MAX_BUFFER_SIZE] = {0};
  ReadFromClipboard(buffer);
  buffer[MAX_BUFFER_SIZE-1] = '\0';

  int succeed = 1;
  int count = 0;
  const char* ptr = buffer;
  do
  {
    while(ptr && ptr - buffer < MAX_BUFFER_SIZE && *ptr != 'x') ptr++;
    if (!ptr)
      break;
    ptr++;

    uint16_t val = 0;
    if (sscanf(ptr, "%x,", &val) == 1)
    {
      characterData[count] = val;
      count++;
    }
    else
      succeed = 0;
  } while(succeed && count < appState.colH);

  if (count != appState.colH) {
    memset(selectedData, 0, appState.colW * appState.colH);
    memset(characterData, 0, sizeof(uint16_t) * appState.colH);
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

void FlipPixelsHorizontal() {
  for (int j = 0; j < appState.colH; j++) {
    for (int i = 0; i < appState.colW / 2; i++) {
      int temp = selectedData[(j * appState.colW) + i];
      selectedData[(j * appState.colW) + i] = selectedData[((j + 1) * appState.colW) - i - 1];
      selectedData[((j + 1) * appState.colW) - i - 1] = temp;
    }
  }
}

void FlipPixelsVertical() {
  for (int i = 0; i < appState.colW; i++) {
    for (int j = 0; j < appState.colH / 2; j++) {
      int t1 = (j * appState.colW) + i;
      int t2 = ((appState.colH - j - 1) * appState.colW) + i;
      int t3 = (appState.colH - j);
      int temp = selectedData[(j * appState.colW) + i];
      selectedData[(j * appState.colW) + i] = selectedData[((appState.colH - j - 1) * appState.colW) + i];
      selectedData[((appState.colH - j - 1) * appState.colW) + i] = temp;
    }
  }
}

void MovePixelsDown() {
  for(int i=appState.colW * appState.colH; i >= 0; i--) {
    if (i > appState.colW)
      selectedData[i] = selectedData[i - appState.colW];
    else
      selectedData[i] = nk_false;
  }
}

void MovePixelsRight() {
  for (int j = 0; j < appState.colH; j++) {
    for (int i = appState.colW - 2; i >= 0; i--) {
      selectedData[(j * appState.colW) + i + 1] = selectedData[(j * appState.colW) + i];
    }
    selectedData[(j * appState.colW)] = 0;
  }
}

void MovePixelsLeft() {
  for (int j = 0; j < appState.colH; j++) {
    for (int i = 0; i < appState.colW - 1; i++) {
      selectedData[(j * appState.colW) + i] = selectedData[(j * appState.colW) + i + 1];
    }
    selectedData[((j + 1) * appState.colW) - 1] = 0;
  }
}

void MovePixelsUp() {
  for(int i=0;i < (appState.colW * appState.colH);i++) {
    if (i < appState.colW * (appState.colH-1))
      selectedData[i] = selectedData[i + appState.colW];
    else
      selectedData[i] = nk_false;
  }
}

void ConvertBufferToCharAndCopy() {
  uint16_t i, j, ptr = 0;
  char buffer[MAX_BUFFER_SIZE] = {0};

  for (i = 0; i < appState.colH; i++)
  {
    characterData[i] = 0;
    for (j = 0; j < appState.colW; j++)
    {
      int bit = 15 - j;
      if (selectedData[j + (i * appState.colW)])
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
}

void ConvertCharToBuffer() {
  uint16_t i, b, j;
  memset(selectedData, 0, appState.colW * appState.colH);
  for (i = 0; i < appState.colH; i++)
  {
    b = characterData[i];
    for (j = 0; j < appState.colW; j++)
    {
      if ((b << j) & 0x8000)
      {
        selectedData[j + (i * appState.colW)] |= 1 << (i % 8);
      }
      else
      {
        selectedData[j + (i * appState.colW)] &= ~(1 << (i % 8));
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

  selectedData = malloc(appState.colW * appState.colH);
  memset(selectedData, 0, appState.colW * appState.colH);

  characterData = malloc(sizeof(uint16_t) * appState.colH);
  memset(characterData, 0, sizeof(uint16_t) * appState.colH);
}
