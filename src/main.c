/* nuklear - 1.32.0 - public domain */
// #define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <edit_utils.h>

#include <gl/glew.h>
#include <glfw/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#include <nuklear/nuklear.h>
#include <nuklear/nuklear_glfw_gl3.h>
#include "style.c"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

uint16_t window_width = 600;
uint16_t window_height = 688;
HWND winHandle = NULL;

struct AppState {
  uint8_t col_w;
  uint8_t col_h;
  uint8_t show_font_settings;
  uint8_t show_app_about;
  uint8_t show_hint;

  uint8_t* font_buffer;
  uint64_t font_size;
  stbtt_fontinfo* font_info;

  uint8_t* select_state_buffer;
  uint16_t* char_buffer;
} appState;

void free_font_buffer();
void free_buffers();
void create_buffers();
void load_char_buffer();
void create_char_buffer();

void read_clipboard(char *buffer);
void write_clipboard(const char *buffer);
void parse_clipboard();

void calculator_run(struct nk_context *ctx);

void init_app_state(struct AppState *state);

void load_font_file(const char *file);

void load_font_glyph(char i);

static void error_callback(int e, const char *d)
{printf("Error %d: %s\n", e, d);}

int main(void)
{
  /* Platform */
  static GLFWwindow *win;
  int width = 0, height = 0;
  struct nk_context *ctx;
  struct nk_colorf bg;

  HWND hWnd = GetConsoleWindow();
  ShowWindow( hWnd, SW_HIDE );

  /* GLFW */
  glfwSetErrorCallback(error_callback);
  if (!glfwInit()) {
    fprintf(stdout, "[GFLW] failed to init!\n");
    exit(1);
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  win = glfwCreateWindow(window_width, window_height, "SSD1306 Font Generator", NULL, NULL);
  winHandle = glfwGetWin32Window(win);
  glfwMakeContextCurrent(win);
  glfwGetWindowSize(win, &width, &height);

  /* OpenGL */
  glViewport(0, 0, window_width, window_height);
  glewExperimental = 1;
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to setup GLEW\n");
    exit(1);
  }

  ctx = nk_glfw3_init(win, NK_GLFW3_INSTALL_CALLBACKS);
  /* Load Fonts: if none of these are loaded a default font will be used  */
  /* Load Cursor: if you uncomment cursor loading please hide the cursor */
  {struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&atlas);
    /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "extra_font/DroidSans.ttf", 14, 0);*/
    nk_glfw3_font_stash_end();
    /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
    /*nk_style_set_font(ctx, &droid->handle);*/
  }

  /* style */
  set_style(ctx, THEME_RED);

  /* Default Values */
  init_app_state(&appState);

  bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
  while (!glfwWindowShouldClose(win))
  {
    /* Input */
    glfwPollEvents();
    nk_glfw3_new_frame();

    /* GUI */
    calculator_run(ctx);

    /* Draw */
    glfwGetWindowSize(win, &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(bg.r, bg.g, bg.b, bg.a);
    /* IMPORTANT: `nk_glfw_render` modifies some global OpenGL state
     * with blending, scissor, face culling, depth test and viewport and
     * defaults everything back into a default state.
     * Make sure to either a.) save and restore or b.) reset your own state after
     * rendering the UI. */
    nk_glfw3_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
    glfwSwapBuffers(win);
  }
  free_buffers();
  nk_glfw3_shutdown();
  glfwTerminate();
  return 0;
}

void init_app_state(struct AppState *state) {
  appState.col_w = 11;
  appState.col_h = 18;
  appState.show_font_settings = nk_false;
  appState.show_app_about = nk_false;
  appState.show_hint = nk_false;

  appState.select_state_buffer = NULL;
  appState.char_buffer = NULL;

  appState.font_info = NULL;
  appState.font_buffer = NULL;
  appState.font_size = 0;

  create_buffers();
}

void calculator_run(struct nk_context *ctx) {
  static struct nk_vec2 clicked_pos;
  static int clicked_active = 0;
  static int rclicked_active = 0;

  if (nk_begin(ctx, "Calculator", nk_rect(0, 0, window_width, window_height), 0x00))
  {
    /* menubar */
    nk_menubar_begin(ctx);

    nk_layout_row_begin(ctx, NK_STATIC, 25, 5);
    nk_layout_row_push(ctx, 45);
    if (nk_menu_begin_label(ctx, "MENU", NK_TEXT_LEFT, nk_vec2(120, 200)))
    {
      nk_layout_row_dynamic(ctx, 25, 1);
      if (nk_menu_item_label(ctx, "Load Font", NK_TEXT_LEFT)) {
        // common dialog box structure, setting all fields to 0 is important
        OPENFILENAME ofn = {0};
        TCHAR szFile[260]={0};
        // Initialize remaining fields of OPENFILENAME structure
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = winHandle;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "TrueType Fonts\0*.ttf\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if(GetOpenFileName(&ofn) == TRUE)
        {
          load_font_file(ofn.lpstrFile);
        }
      }
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
        parse_clipboard();
      }
      if (nk_menu_item_label(ctx, "Calc & Copy", NK_TEXT_LEFT)) {
        create_char_buffer();
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
        nk_fill_rect(out, nk_rect(180 + cell_size * j, 44 + cell_size * i, cell_size, cell_size), 2.0,
                     appState.select_state_buffer[(i * appState.col_w) + j] == 0? nk_rgba(57, 67, 71, 215) : nk_rgba(195, 55, 75, 255));
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
    if (appState.show_font_settings || appState.show_hint ||
        appState.show_app_about || appState.font_size) {
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

      appState.select_state_buffer[y * appState.col_w + x] = clicked_active? 1: 0;
    }

    if (nk_tree_push(ctx, NK_TREE_NODE, "Edit", NK_MAXIMIZED)) {
      nk_layout_row_static(ctx, 30, 130, 1);
      if (nk_button_label(ctx, "Clear")) {
        memset(appState.select_state_buffer, 0, appState.col_w * appState.col_h);
      }

      nk_layout_row_static(ctx, 30, 63, 2);
      if (nk_button_label(ctx, "Flip H")) {
        FlipPixelsHorizontal(appState.select_state_buffer, appState.col_w, appState.col_h);
      }
      if (nk_button_label(ctx, "Flip V")) {
        FlipPixelsVertical(appState.select_state_buffer, appState.col_w, appState.col_h);
      }

      nk_layout_row_static(ctx, 30, 42, 3);

      nk_label(ctx, "", NK_TEXT_LEFT);
      if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_UP)) {
        MovePixelsUp(appState.select_state_buffer, appState.col_w, appState.col_h);
      }
      nk_label(ctx, "", NK_TEXT_LEFT);

      if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_LEFT)) {
        MovePixelsLeft(appState.select_state_buffer, appState.col_w, appState.col_h);
      }
      nk_label(ctx, "", NK_TEXT_LEFT);
      if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_RIGHT)) {
        MovePixelsRight(appState.select_state_buffer, appState.col_w, appState.col_h);
      }

      nk_label(ctx, "", NK_TEXT_LEFT);
      if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_DOWN)) {
        MovePixelsDown(appState.select_state_buffer, appState.col_w, appState.col_h);
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
    s.y = (window_height - 150)/2;
    s.w = 300;
    s.h = 150;
    if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Hint", NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE, s))
    {
      nk_layout_row_static(ctx, 20, 190, 1);
      nk_label_wrap(ctx, "Use left-click to set the box.");
      nk_label_wrap(ctx, "Use right-click to reset it.");

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
          free_buffers();
          create_buffers();
        }
      }

      nk_layout_row_dynamic(ctx, 20, 1);
      nk_label_colored(ctx, "Caution! Maximum value for h.cols is 16!", NK_TEXT_LEFT, nk_rgb(255,255,0));
      nk_label_colored(ctx, "Caution! Changing this will reset your work!", NK_TEXT_LEFT, nk_rgb(255,255,0));

      nk_popup_end(ctx);
    } else appState.show_font_settings = nk_false;
  }

  if (appState.font_size > 0)
  {
    /* about popup */
    static struct nk_rect s;
    s.x = (window_width - 300)/2;
    s.y = (window_height - 270)/2;
    s.w = 300;
    s.h = 270;
    if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Settings", NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE, s))
    {
      static char text[3];
      static int text_len;

      nk_layout_row_dynamic(ctx, 30, 1);
      nk_label(ctx, "Character:", NK_TEXT_LEFT);
      nk_edit_string(ctx, NK_EDIT_SIMPLE, text, &text_len, 3, nk_filter_ascii);

      if (nk_button_label(ctx, "Load")) {
        text[text_len] = '\0';
        if (text_len > 0)
          load_font_glyph(text[0]);
      }

      nk_popup_end(ctx);
    } else {
      free_font_buffer();
    }
  }

  nk_end(ctx);
}

void load_font_glyph(char chr) {
  /* create a bitmap for the phrase */
  memset(appState.select_state_buffer, 0, appState.col_w * appState.col_h);
  memset(appState.char_buffer, 0, sizeof(uint16_t) * appState.col_h);

  /* calculate font scaling */
  float scale = stbtt_ScaleForPixelHeight(appState.font_info, appState.col_h);

  int ascent, descent, lineGap;
  stbtt_GetFontVMetrics(appState.font_info, &ascent, &descent, &lineGap);

  ascent = roundf(ascent * scale);
  descent = roundf(descent * scale);

  /* how wide is this character */
  int ax;
  int lsb;
  stbtt_GetCodepointHMetrics(appState.font_info, chr, &ax, &lsb);
  /* (Note that each Codepoint call has an alternative Glyph version which caches the work required to lookup the character word[i].) */

  /* get bounding box for character (may be offset to account for chars that dip above or below the line) */
  int c_x1, c_y1, c_x2, c_y2;
  stbtt_GetCodepointBitmapBox(appState.font_info, chr, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

  /* compute y (different characters have different heights) */
  int y = ascent + c_y1;

  /* render character (stride and offset is important here) */
  int byteOffset = roundf(lsb * scale) + (y * appState.col_w);
  stbtt_MakeCodepointBitmap(appState.font_info, appState.select_state_buffer + byteOffset, c_x2 - c_x1, c_y2 - c_y1, appState.col_w, scale, scale, chr);
}

void load_font_file(const char *file) {
  free_font_buffer();
  FILE* fontFile = fopen(file, "rb");
  fseek(fontFile, 0, SEEK_END);
  appState.font_size = ftell(fontFile);
  fseek(fontFile, 0, SEEK_SET);

  appState.font_buffer = malloc(appState.font_size);

  fread(appState.font_buffer, appState.font_size, 1, fontFile);
  fclose(fontFile);

  /* prepare font */
  appState.font_info = malloc(sizeof(stbtt_fontinfo));
  if (!stbtt_InitFont(appState.font_info, appState.font_buffer, 0))
  {
    free_font_buffer();
  }
}

void parse_clipboard() {
  char* buffer = malloc(appState.col_w * appState.col_h + 1);
  memset(buffer, 0, appState.col_w * appState.col_h + 1);
  read_clipboard(buffer);
  buffer[appState.col_w * appState.col_h] = '\0';

  if (buffer[0] != '0' || buffer[1] != 'x')
    return;

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
      appState.char_buffer[count] = val & 0xFFFF;
      count++;
    }
    else
      succeed = 0;
  } while(succeed && count < appState.col_h);
  free(buffer);

  if (count != appState.col_h) {
    memset(appState.select_state_buffer, 0, appState.col_w * appState.col_h);
    memset(appState.char_buffer, 0, sizeof(uint16_t) * appState.col_h);
    // show error
    return;
  }
  load_char_buffer();
}

void write_clipboard(const char *buffer) {
  const size_t len = strlen(buffer) + 1;
  HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, len);
  memcpy(GlobalLock(hMem), buffer, len);
  GlobalUnlock(hMem);
  OpenClipboard(0);
  EmptyClipboard();
  SetClipboardData(CF_TEXT, hMem);
  CloseClipboard();
}

void read_clipboard(char *buffer) {
  HANDLE h;
  OpenClipboard(NULL);
  h = GetClipboardData(CF_TEXT);
  sprintf(buffer, "%s", (char *)h);
  CloseClipboard();
}

void create_char_buffer() {
  uint16_t i, j, ptr = 0;
  char* buffer = malloc(appState.col_w * appState.col_h + 1);
  memset(buffer, 0, appState.col_w * appState.col_h + 1);

  for (i = 0; i < appState.col_h; i++)
  {
    appState.char_buffer[i] = 0;
    for (j = 0; j < appState.col_w; j++)
    {
      int bit = 15 - j;
      if (appState.select_state_buffer[j + (i * appState.col_w)])
      {
        appState.char_buffer[i] |= ((uint16_t)1 << bit);
      }
      else
      {
        appState.char_buffer[i] &= ~((uint16_t)1 << bit);
      }
    }
    sprintf(buffer + ptr, "0x%04X, ", appState.char_buffer[i]);
    ptr += 8;
  }

  write_clipboard(buffer);
  free(buffer);
}

void load_char_buffer() {
  uint16_t i, b, j;
  memset(appState.select_state_buffer, 0, appState.col_w * appState.col_h);
  for (i = 0; i < appState.col_h; i++)
  {
    b = appState.char_buffer[i];
    for (j = 0; j < appState.col_w; j++)
    {
      if ((b << j) & 0x8000)
      {
        appState.select_state_buffer[j + (i * appState.col_w)] |= 1 << (i % 8);
      }
      else
      {
        appState.select_state_buffer[j + (i * appState.col_w)] &= ~(1 << (i % 8));
      }
    }
  }
}

void free_font_buffer()
{
  if (appState.font_buffer)
    free(appState.font_buffer);
  if (appState.font_info)
    free(appState.font_info);

  appState.font_size = 0;
  appState.font_buffer = NULL;
  appState.font_info = NULL;
}

void free_buffers() {
  if (appState.char_buffer)
    free(appState.char_buffer);
  if (appState.select_state_buffer)
    free(appState.select_state_buffer);

  appState.char_buffer = NULL;
  appState.select_state_buffer = NULL;
}

void create_buffers() {
  appState.select_state_buffer = malloc(appState.col_w * appState.col_h);
  memset(appState.select_state_buffer, 0, appState.col_w * appState.col_h);

  appState.char_buffer = malloc(sizeof(uint16_t) * appState.col_h);
  memset(appState.char_buffer, 0, sizeof(uint16_t) * appState.col_h);
}
