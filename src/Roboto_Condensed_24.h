// Header File for SSD1306 characters
// Generated with TTF2BMH
// Font Roboto Condensed
// Font Size: 24
const uint8_t PROGMEM bitmap_48[] = {0,240,248,28,12,12,12,28,248,224,127,255,255,0,0,0,0,0,255,255,0,7,15,28,24,24,24,30,15,3};
const uint8_t PROGMEM bitmap_49[] = {96,48,48,24,248,252,0,0,0,0,255,255,0,0,0,0,31,31};
const uint8_t PROGMEM bitmap_50[] = {224,240,120,28,12,12,12,28,248,240,0,0,0,0,128,192,240,120,30,15,3,0,24,28,30,31,25,24,24,24,24,24,24};
const uint8_t PROGMEM bitmap_51[] = {64,112,120,28,12,12,12,24,248,224,0,0,0,12,12,12,12,31,251,241,1,7,15,28,24,24,24,12,15,7};
const uint8_t PROGMEM bitmap_52[] = {0,0,0,0,192,240,60,252,252,0,0,192,240,252,159,135,128,128,255,255,128,128,1,1,1,1,1,1,1,31,31,1,1};
const uint8_t PROGMEM bitmap_53[] = {0,252,252,12,12,12,12,12,12,0,4,7,7,3,3,3,7,254,252,240,3,15,12,24,24,24,28,15,7,1};
const uint8_t PROGMEM bitmap_54[] = {128,224,240,56,28,12,12,0,0,0,255,255,14,3,3,3,7,254,252,224,3,7,14,28,24,24,28,15,7,0};
const uint8_t PROGMEM bitmap_55[] = {12,12,12,12,12,12,12,204,252,124,12,0,0,0,0,192,248,126,15,1,0,0,0,0,16,30,31,3,0,0,0,0,0};
const uint8_t PROGMEM bitmap_56[] = {240,248,28,12,12,12,28,248,240,241,255,30,12,12,12,30,255,225,7,15,28,24,24,24,28,15,7};
const uint8_t PROGMEM bitmap_57[] = {128,240,248,28,12,12,28,56,240,224,3,31,63,112,96,96,48,56,255,255,0,0,0,24,24,28,12,15,3,0};
const uint8_t PROGMEM bitmap_58[] = {128,128,0,3,3,1,28,28,8};

struct RB_char_desc
{
  uint8_t const *addr;
  uint8_t width;
};

template<unsigned N>
constexpr RB_char_desc mk_char_desc(uint8_t const (&b)[N]) noexcept
{
  return { b, N / 3 };
}
RB_char_desc const PROGMEM char_desc[] = {
    mk_char_desc(bitmap_48),
    mk_char_desc(bitmap_49),
    mk_char_desc(bitmap_50),
    mk_char_desc(bitmap_51),
    mk_char_desc(bitmap_52),
    mk_char_desc(bitmap_53),
    mk_char_desc(bitmap_54),
    mk_char_desc(bitmap_55),
    mk_char_desc(bitmap_56),
    mk_char_desc(bitmap_57),
    mk_char_desc(bitmap_58)
};
