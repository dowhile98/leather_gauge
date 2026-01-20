#ifndef INC_ESC_POS_PRINTER_H_
#define INC_ESC_POS_PRINTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ESC/POS Command Definitions */
#define ASCII_ESC   0x1B
#define ASCII_GS    0x1D
#define ASCII_FS    0x1C
#define ASCII_DC2   0x12
#define ASCII_EOT   0x04
#define ASCII_LF    0x0A
#define ASCII_FF    0x0C
#define ASCII_CR    0x0D
#define ASCII_TAB   0x09

/* Barcode Types */
typedef enum {
    BARCODE_UPC_A = 65,
    BARCODE_UPC_E = 66,
    BARCODE_EAN13 = 67,
    BARCODE_EAN8 = 68,
    BARCODE_CODE39 = 69,
    BARCODE_ITF = 70,
    BARCODE_CODABAR = 71,
    BARCODE_CODE93 = 72,
    BARCODE_CODE128 = 73,
    BARCODE_GS1_128 = 74,
    BARCODE_GS1_DATABAR_OMNI = 75,
    BARCODE_GS1_DATABAR_TRUNC = 76,
    BARCODE_GS1_DATABAR_LIMTD = 77,
    BARCODE_GS1_DATABAR_EXPAN = 78
} barcode_type_t;

/* Text Alignment */
typedef enum {
    ALIGN_LEFT = 0,
    ALIGN_CENTER = 1,
    ALIGN_RIGHT = 2
} align_t;

/* Text Size */
typedef enum {
    SIZE_SMALL = 0,
    SIZE_MEDIUM = 1,
    SIZE_LARGE = 2
} text_size_t;

/* Printer Width Types */
typedef enum {
    PRINTER_57MM = 0,    // Impresora estrecha (tickets)
    PRINTER_80MM = 1,    // Impresora ancha (recibos)
    PRINTER_112MM = 2    // Impresora extra ancha
} printer_width_t;

/* Character Sets */
typedef enum {
    CHARSET_USA                 = 0,
    CHARSET_FRANCE              = 1,
    CHARSET_GERMANY             = 2,
    CHARSET_UK                  = 3,
    CHARSET_DENMARK1            = 4,
    CHARSET_SWEDEN              = 5,
    CHARSET_ITALY               = 6,
    CHARSET_SPAIN1              = 7,    // Español - España 1
    CHARSET_JAPAN               = 8,
    CHARSET_NORWAY              = 9,
    CHARSET_DENMARK2            = 10,
    CHARSET_SPAIN2              = 11,   // Español - España 2
    CHARSET_LATINAMERICA        = 12,   // Español - América Latina
    CHARSET_KOREA               = 13,
    CHARSET_SLOVENIA            = 14,
    CHARSET_CROATIA             = 14,
    CHARSET_CHINA               = 15,
    CHARSET_VIETNAM             = 16,
    CHARSET_ARABIA              = 17,
    CHARSET_INDIA_DEVANAGARI    = 66,
    CHARSET_INDIA_BENGALI       = 67,
    CHARSET_INDIA_TAMIL         = 68,
    CHARSET_INDIA_TELUGU        = 69,
    CHARSET_INDIA_ASSAMESE      = 70,
    CHARSET_INDIA_ORIYA         = 71,
    CHARSET_INDIA_KANNANDA      = 72,
    CHARSET_INDIA_MALAYALAM     = 73,
    CHARSET_INDIA_GUJARATI      = 74,
    CHARSET_INDIA_PUNJABI       = 75,
    CHARSET_INDIA_MARATHI       = 82
} charset_t;

/* Code Pages */
typedef enum {
    CODEPAGE_CP437        = 0,  // USA, Standard Europe
    CODEPAGE_KATAKANA     = 1,
    CODEPAGE_CP850        = 2,  // Multilingual
    CODEPAGE_CP860        = 3,  // Portuguese
    CODEPAGE_CP863        = 4,  // Canadian-French
    CODEPAGE_CP865        = 5,  // Nordic
    CODEPAGE_WCP1251      = 6,  // Cyrillic
    CODEPAGE_CP866        = 7,  // Cyrillic #2
    CODEPAGE_MIK          = 8,  // Cyrillic/Bulgarian
    CODEPAGE_CP755        = 9,  // East Europe, Latvian 2
    CODEPAGE_IRAN         = 10,
    CODEPAGE_CP862        = 15, // Hebrew
    CODEPAGE_WCP1252      = 16, // Latin 1
    CODEPAGE_WCP1253      = 17, // Greek
    CODEPAGE_CP852        = 18, // Latin 2
    CODEPAGE_CP858        = 19, // Multilingual Latin 1 + Euro
    CODEPAGE_IRAN2        = 20,
    CODEPAGE_LATVIAN      = 21,
    CODEPAGE_CP864        = 22, // Arabic
    CODEPAGE_ISO_8859_1   = 23, // West Europe
    CODEPAGE_CP737        = 24, // Greek
    CODEPAGE_WCP1257      = 25, // Baltic
    CODEPAGE_THAI         = 26,
    CODEPAGE_CP720        = 27, // Arabic
    CODEPAGE_CP855        = 28,
    CODEPAGE_CP857        = 29, // Turkish
    CODEPAGE_WCP1250      = 30, // Central Europe
    CODEPAGE_CP775        = 31,
    CODEPAGE_WCP1254      = 32, // Turkish
    CODEPAGE_WCP1255      = 33, // Hebrew
    CODEPAGE_WCP1256      = 34, // Arabic
    CODEPAGE_WCP1258      = 35, // Vietnam
    CODEPAGE_ISO_8859_2   = 36, // Latin 2
    CODEPAGE_ISO_8859_3   = 37, // Latin 3
    CODEPAGE_ISO_8859_4   = 38, // Baltic
    CODEPAGE_ISO_8859_5   = 39, // Cyrillic
    CODEPAGE_ISO_8859_6   = 40, // Arabic
    CODEPAGE_ISO_8859_7   = 41, // Greek
    CODEPAGE_ISO_8859_8   = 42, // Hebrew
    CODEPAGE_ISO_8859_9   = 43, // Turkish
    CODEPAGE_ISO_8859_15  = 44, // Latin 3
    CODEPAGE_THAI2        = 45,
    CODEPAGE_CP856        = 46,
    CODEPAGE_CP874        = 47,
    CODEPAGE_UTF8         = 255 // Manejo especial para UTF-8
} codepage_t;

/* Underline Weights */
typedef enum {
    UNDERLINE_OFF = 0,
    UNDERLINE_SINGLE = 1,
    UNDERLINE_DOUBLE = 2
} underline_t;

/* Print Density */
typedef enum {
    DENSITY_SINGLE = 0,
    DENSITY_DOUBLE = 1
} density_t;

/* QR Code Error Correction */
typedef enum {
    QR_CORRECTION_L = 0,
    QR_CORRECTION_M = 1,
    QR_CORRECTION_Q = 2,
    QR_CORRECTION_H = 3
} qr_correction_t;

/* Printer Status */
typedef struct {
    uint8_t column;           // Columna actual del cursor (0-based)
    uint8_t max_column;       // Máximo de columnas por línea
    uint8_t char_height;      // Altura de caracteres en dots
    uint8_t line_spacing;     // Espaciado entre líneas en dots
    uint8_t barcode_height;   // Altura de códigos de barras en dots
    bool is_online;           // Estado online/offline
} printer_status_t;

/* Function Pointer Types */
typedef uint8_t (*printer_write_fn)(uint8_t *data, uint16_t len);
typedef void (*printer_delay_fn)(uint32_t ms);

/* Main Printer Context */
typedef struct {
    printer_write_fn write_fn;
    printer_delay_fn delay_fn;
    printer_status_t status;
    printer_width_t printer_type;
    uint16_t dots_width;
} esc_pos_printer_t;

/* API Functions */
void esc_pos_init(esc_pos_printer_t *printer, printer_write_fn write_fn,
                 printer_delay_fn delay_fn, printer_width_t printer_type);

/* Basic Control */
uint8_t esc_pos_set_defaults(esc_pos_printer_t *printer);
uint8_t esc_pos_reset(esc_pos_printer_t *printer);
uint8_t esc_pos_feed(esc_pos_printer_t *printer, uint8_t lines);
uint8_t esc_pos_feed_dots(esc_pos_printer_t *printer, uint8_t dots);
uint8_t esc_pos_cut(esc_pos_printer_t *printer, bool partial);
uint8_t esc_pos_flush(esc_pos_printer_t *printer);
uint8_t esc_pos_bell(esc_pos_printer_t *printer);

/* Printer Status */
uint8_t esc_pos_online(esc_pos_printer_t *printer);
uint8_t esc_pos_offline(esc_pos_printer_t *printer);
uint8_t esc_pos_sleep(esc_pos_printer_t *printer, uint16_t seconds);

/* Text Formatting */
uint8_t esc_pos_set_align(esc_pos_printer_t *printer, align_t align);
uint8_t esc_pos_set_size(esc_pos_printer_t *printer, text_size_t size);
uint8_t esc_pos_set_bold(esc_pos_printer_t *printer, bool enabled);
uint8_t esc_pos_set_underline(esc_pos_printer_t *printer, underline_t weight);
uint8_t esc_pos_set_inverse(esc_pos_printer_t *printer, bool enabled);
uint8_t esc_pos_set_strike(esc_pos_printer_t *printer, bool enabled);
uint8_t esc_pos_set_upside_down(esc_pos_printer_t *printer, bool enabled);
uint8_t esc_pos_set_line_height(esc_pos_printer_t *printer, uint8_t height);
uint8_t esc_pos_set_char_spacing(esc_pos_printer_t *printer, uint8_t spacing);

/* International Character Support */
uint8_t esc_pos_set_charset(esc_pos_printer_t *printer, charset_t charset);
uint8_t esc_pos_set_codepage(esc_pos_printer_t *printer, codepage_t codepage);

/* Barcode Printing */
uint8_t esc_pos_print_barcode(esc_pos_printer_t *printer, const char *data,
                             barcode_type_t type, uint8_t height);
uint8_t esc_pos_set_barcode_height(esc_pos_printer_t *printer, uint8_t height);
uint8_t esc_pos_set_barcode_width(esc_pos_printer_t *printer, uint8_t width);

/* QR Code Printing */
uint8_t esc_pos_print_qrcode(esc_pos_printer_t *printer, const char *data,
                            uint8_t size, qr_correction_t correction);
uint8_t esc_pos_set_qrcode_size(esc_pos_printer_t *printer, uint8_t size);
uint8_t esc_pos_set_qrcode_correction(esc_pos_printer_t *printer, qr_correction_t correction);

/* Graphic Printing */
uint8_t esc_pos_print_bitmap(esc_pos_printer_t *printer, const uint8_t *bitmap,
                            uint16_t width, uint16_t height, density_t density);
uint8_t esc_pos_print_bitmap_flash(esc_pos_printer_t *printer, const uint8_t *bitmap,
                                  uint16_t width, uint16_t height, density_t density);

/* Utility Functions */
uint8_t esc_pos_print_text(esc_pos_printer_t *printer, const char *text);
uint8_t esc_pos_print_line(esc_pos_printer_t *printer, const char *text);
uint8_t esc_pos_print_separator(esc_pos_printer_t *printer, char pattern);
uint8_t esc_pos_print_table_row(esc_pos_printer_t *printer,
                               const char *left, const char *middle, const char *right);
uint8_t esc_pos_tab(esc_pos_printer_t *printer);

/* Test Functions */
uint8_t esc_pos_test_page(esc_pos_printer_t *printer);
uint8_t esc_pos_self_test(esc_pos_printer_t *printer);

#ifdef __cplusplus
}
#endif

#endif /* INC_ESC_POS_PRINTER_H_ */
