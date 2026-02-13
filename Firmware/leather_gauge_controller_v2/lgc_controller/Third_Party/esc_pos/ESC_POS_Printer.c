#include "ESC_POS_Printer.h"
#include <lwprintf.h>
/* Private Constants */
static const uint8_t max_columns_config[3][3] = {
    // SIZE_SMALL, SIZE_MEDIUM, SIZE_LARGE
    {  32,  32,  16  },  // 57mm  (384 dots)
    {  48,  48,  24  },  // 80mm  (576 dots)
    {  64,  64,  32  }   // 112mm (832 dots)
};

/* Private Function Prototypes */
static uint8_t send_command(esc_pos_printer_t *printer, const uint8_t *data, uint16_t len);
/* Public Functions Implementation */

void esc_pos_init(esc_pos_printer_t *printer, printer_write_fn write_fn,
                 printer_delay_fn delay_fn, printer_width_t printer_type)
{
    if (printer && write_fn) {
        printer->write_fn = write_fn;
        printer->delay_fn = delay_fn;
        printer->printer_type = printer_type;

        // Configurar según tipo de impresora
        switch (printer_type) {
            case PRINTER_57MM:
                printer->dots_width = 384;
                printer->status.max_column = 32;
                break;
            case PRINTER_80MM:
                printer->dots_width = 576;
                printer->status.max_column = 48;
                break;
            case PRINTER_112MM:
                printer->dots_width = 832;
                printer->status.max_column = 64;
                break;
            default:
                printer->dots_width = 384;
                printer->status.max_column = 32;
                break;
        }

        // Inicializar resto de parámetros
        printer->status.column = 0;
        printer->status.char_height = 24;
        printer->status.line_spacing = 6;
        printer->status.barcode_height = 50;
        printer->status.is_online = true;

        // Resetear impresora a estado conocido
        esc_pos_reset(printer);
    }
}

static uint8_t send_command(esc_pos_printer_t *printer, const uint8_t *data, uint16_t len)
{
    if (printer && printer->write_fn && printer->status.is_online) {
        return printer->write_fn((uint8_t*)data, len);
    }
    return 1; // Error
}



/* Basic Control Functions */
uint8_t esc_pos_set_defaults(esc_pos_printer_t *printer)
{
    uint8_t status = 0;

    status |= esc_pos_reset(printer);
    status |= esc_pos_online(printer);
    status |= esc_pos_set_align(printer, ALIGN_LEFT);
    status |= esc_pos_set_inverse(printer, false);
    status |= esc_pos_set_line_height(printer, 30);
    status |= esc_pos_set_bold(printer, false);
    status |= esc_pos_set_underline(printer, UNDERLINE_OFF);
    status |= esc_pos_set_barcode_height(printer, 50);
    status |= esc_pos_set_size(printer, SIZE_SMALL);
    status |= esc_pos_set_charset(printer, CHARSET_USA);
    status |= esc_pos_set_codepage(printer, CODEPAGE_CP437);

    return status;
}


uint8_t esc_pos_reset(esc_pos_printer_t *printer)
{
    uint8_t cmd[] = {ASCII_ESC, '@'};
    printer->status.column = 0;
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_feed(esc_pos_printer_t *printer, uint8_t lines)
{
    uint8_t cmd[] = {ASCII_ESC, 'd', lines};
    printer->status.column = 0;
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_feed_dots(esc_pos_printer_t *printer, uint8_t dots)
{
    uint8_t cmd[] = {ASCII_ESC, 'J', dots};
    printer->status.column = 0;
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_cut(esc_pos_printer_t *printer, bool partial)
{
    uint8_t cmd[] = {ASCII_GS, 'V', partial ? 65 : 66, 0};
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_flush(esc_pos_printer_t *printer)
{
    uint8_t cmd = ASCII_FF;
    printer->status.column = 0;
    return send_command(printer, &cmd, 1);
}

uint8_t esc_pos_bell(esc_pos_printer_t *printer)
{
    uint8_t cmd = 0x07; // ASCII BEL
    return send_command(printer, &cmd, 1);
}

/* Printer Status Functions */

uint8_t esc_pos_online(esc_pos_printer_t *printer)
{
    uint8_t cmd[] = {ASCII_ESC, '=', 1};
    printer->status.is_online = true;
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_offline(esc_pos_printer_t *printer)
{
    uint8_t cmd[] = {ASCII_ESC, '=', 0};
    printer->status.is_online = false;
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_sleep(esc_pos_printer_t *printer, uint16_t seconds)
{
    uint8_t cmd[] = {ASCII_ESC, '8', (uint8_t)(seconds & 0xFF), (uint8_t)(seconds >> 8)};
    return send_command(printer, cmd, sizeof(cmd));
}

/* Text Formatting Functions */

uint8_t esc_pos_set_align(esc_pos_printer_t *printer, align_t align)
{
    uint8_t cmd[] = {ASCII_ESC, 'a', (uint8_t)align};
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_set_size(esc_pos_printer_t *printer, text_size_t size)
{
    uint8_t size_val;

    switch (size) {
        case SIZE_MEDIUM:
            size_val = 0x01; // Double height
            printer->status.char_height = 48;
            printer->status.max_column = max_columns_config[printer->printer_type][SIZE_SMALL];
            break;
        case SIZE_LARGE:
            size_val = 0x11; // Double width and height
            printer->status.char_height = 48;
            printer->status.max_column = max_columns_config[printer->printer_type][SIZE_SMALL] / 2;
            break;
        case SIZE_SMALL:
        default:
            size_val = 0x00; // Normal
            printer->status.char_height = 24;
            printer->status.max_column = max_columns_config[printer->printer_type][SIZE_SMALL];
            break;
    }

    uint8_t cmd[] = {ASCII_GS, '!', size_val};
    printer->status.column = 0;
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_set_bold(esc_pos_printer_t *printer, bool enabled)
{
    uint8_t cmd[] = {ASCII_ESC, 'E', enabled ? 1 : 0};
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_set_underline(esc_pos_printer_t *printer, underline_t weight)
{
    uint8_t cmd[] = {ASCII_ESC, '-', (uint8_t)weight};
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_set_inverse(esc_pos_printer_t *printer, bool enabled)
{
    uint8_t cmd[] = {ASCII_GS, 'B', enabled ? 1 : 0};
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_set_strike(esc_pos_printer_t *printer, bool enabled)
{
    uint8_t cmd[] = {ASCII_ESC, 'G', enabled ? 1 : 0};
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_set_upside_down(esc_pos_printer_t *printer, bool enabled)
{
    uint8_t cmd[] = {ASCII_ESC, '{', enabled ? 1 : 0};
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_set_line_height(esc_pos_printer_t *printer, uint8_t height)
{
    if (height < 24) height = 24;
    printer->status.line_spacing = height - 24;
    uint8_t cmd[] = {ASCII_ESC, '3', height};
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_set_char_spacing(esc_pos_printer_t *printer, uint8_t spacing)
{
    uint8_t cmd[] = {ASCII_ESC, ' ', spacing};
    return send_command(printer, cmd, sizeof(cmd));
}

/* International Character Support */

uint8_t esc_pos_set_charset(esc_pos_printer_t *printer, charset_t charset)
{
    uint8_t cmd[] = {ASCII_ESC, 'R', (uint8_t)charset};
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_set_codepage(esc_pos_printer_t *printer, codepage_t codepage)
{
    uint8_t cmd[] = {ASCII_ESC, 't', (uint8_t)codepage};
    return send_command(printer, cmd, sizeof(cmd));
}

/* Barcode Functions */

uint8_t esc_pos_set_barcode_height(esc_pos_printer_t *printer, uint8_t height)
{
    if (height < 1) height = 1;
    printer->status.barcode_height = height;
    uint8_t cmd[] = {ASCII_GS, 'h', height};
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_set_barcode_width(esc_pos_printer_t *printer, uint8_t width)
{
    if (width > 6) width = 6;
    if (width < 2) width = 2;
    uint8_t cmd[] = {ASCII_GS, 'w', width};
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_print_barcode(esc_pos_printer_t *printer, const char *data,
                             barcode_type_t type, uint8_t height)
{
    uint8_t status = 0;
    size_t len = strlen(data);

    // Set barcode height
    status |= esc_pos_set_barcode_height(printer, height);

    // Set HRI position (below barcode)
    uint8_t hri_cmd[] = {ASCII_GS, 'H', 2};
    status |= send_command(printer, hri_cmd, sizeof(hri_cmd));

    // Set barcode width
    status |= esc_pos_set_barcode_width(printer, 3);

    // Print barcode
    if (type == BARCODE_CODE128) {
        uint8_t barcode_cmd[] = {ASCII_GS, 'k', (uint8_t)type, (uint8_t)len};
        status |= send_command(printer, barcode_cmd, sizeof(barcode_cmd));
    } else {
        uint8_t barcode_cmd[] = {ASCII_GS, 'k', (uint8_t)type};
        status |= send_command(printer, barcode_cmd, sizeof(barcode_cmd));
    }

    status |= send_command(printer, (uint8_t*)data, len);

    // Null terminator for some barcode types
    if (type == BARCODE_CODE39 || type == BARCODE_ITF || type == BARCODE_CODABAR) {
        uint8_t terminator = 0;
        status |= send_command(printer, &terminator, 1);
    }

    return status;
}

/* QR Code Functions */

uint8_t esc_pos_set_qrcode_size(esc_pos_printer_t *printer, uint8_t size)
{
    if (size > 16) size = 16;
    if (size < 1) size = 1;

    uint8_t cmd[] = {ASCII_GS, '(', 'k', 3, 0, 49, 67, size};
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_set_qrcode_correction(esc_pos_printer_t *printer, qr_correction_t correction)
{
    uint8_t cmd[] = {ASCII_GS, '(', 'k', 3, 0, 49, 69, (uint8_t)correction};
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_print_qrcode(esc_pos_printer_t *printer, const char *data,
                            uint8_t size, qr_correction_t correction)
{
    uint8_t status = 0;
    size_t len = strlen(data);

    // QR Code: Select model (Model 2)
    uint8_t model_cmd[] = {ASCII_GS, '(', 'k', 4, 0, 49, 65, 50, 0};
    status |= send_command(printer, model_cmd, sizeof(model_cmd));

    // QR Code: Set size
    status |= esc_pos_set_qrcode_size(printer, size);

    // QR Code: Set error correction
    status |= esc_pos_set_qrcode_correction(printer, correction);

    // QR Code: Store data
    uint8_t store_cmd[] = {ASCII_GS, '(', 'k', (uint8_t)(len + 3), 0, 49, 80, 48};
    status |= send_command(printer, store_cmd, sizeof(store_cmd));
    status |= send_command(printer, (uint8_t*)data, len);

    // QR Code: Print
    uint8_t print_cmd[] = {ASCII_GS, '(', 'k', 3, 0, 49, 81, 48};
    status |= send_command(printer, print_cmd, sizeof(print_cmd));

    return status;
}

/* Graphic Functions */

uint8_t esc_pos_print_bitmap(esc_pos_printer_t *printer, const uint8_t *bitmap,
                            uint16_t width, uint16_t height, density_t density)
{
    uint8_t status = 0;

    // Set line spacing to 0 for bitmap printing
    uint8_t line_spacing_cmd[] = {ASCII_ESC, '3', 0};
    status |= send_command(printer, line_spacing_cmd, sizeof(line_spacing_cmd));

    // Bitmap mode command
    uint8_t density_val = (density == DENSITY_DOUBLE) ? 33 : 0;
    uint8_t bitmap_cmd[] = {ASCII_ESC, '*', density_val,
                           (uint8_t)(width & 0xFF), (uint8_t)((width >> 8) & 0xFF)};
    status |= send_command(printer, bitmap_cmd, sizeof(bitmap_cmd));

    // Calculate bytes per line
    uint16_t bytes_per_line = (width + 7) / 8;
    if (density == DENSITY_DOUBLE) {
        bytes_per_line *= 3;
    }

    // Send bitmap data
    for (uint16_t y = 0; y < height; y++) {
        status |= send_command(printer, (uint8_t*)&bitmap[y * bytes_per_line], bytes_per_line);

        // Line feed after each row
        uint8_t lf = ASCII_LF;
        status |= send_command(printer, &lf, 1);
    }

    // Restore default line spacing
    uint8_t default_spacing[] = {ASCII_ESC, '2'};
    status |= send_command(printer, default_spacing, sizeof(default_spacing));

    printer->status.column = 0;
    return status;
}

uint8_t esc_pos_print_bitmap_flash(esc_pos_printer_t *printer, const uint8_t *bitmap,
                                  uint16_t width, uint16_t height, density_t density)
{
    // Similar to print_bitmap but for data stored in flash memory
    // Implementation depends on your MCU architecture
    return esc_pos_print_bitmap(printer, bitmap, width, height, density);
}

/* Utility Functions */

uint8_t esc_pos_print_text(esc_pos_printer_t *printer, const char *text)
{
    size_t len = strlen(text);
    uint8_t status = send_command(printer, (uint8_t*)text, len);

    // Update column position
    for (size_t i = 0; i < len; i++) {
        if (text[i] == '\n' || text[i] == '\r') {
            printer->status.column = 0;
        } else if (text[i] == '\t') {
            printer->status.column = (printer->status.column + 4) & 0xFC;
        } else {
            printer->status.column++;
            if (printer->status.column >= printer->status.max_column) {
                printer->status.column = 0;
            }
        }
    }

    return status;
}

uint8_t esc_pos_print_line(esc_pos_printer_t *printer, const char *text)
{
    uint8_t status = esc_pos_print_text(printer, text);
    status |= esc_pos_feed(printer, 1);
    return status;
}

uint8_t esc_pos_print_separator(esc_pos_printer_t *printer, char pattern)
{
    char separator[printer->status.max_column + 1];
    memset(separator, pattern, printer->status.max_column);
    separator[printer->status.max_column] = '\0';

    return esc_pos_print_line(printer, separator);
}

uint8_t esc_pos_print_table_row(esc_pos_printer_t *printer,
                               const char *left, const char *middle, const char *right)
{
    uint8_t status = 0;
    uint8_t col_width = printer->status.max_column / 3;
    char buffer[printer->status.max_column + 1];

    // Truncate strings to fit columns
    char left_trunc[col_width + 1];
    char middle_trunc[col_width + 1];
    char right_trunc[col_width + 1];

    strncpy(left_trunc, left ? left : "", col_width);
    strncpy(middle_trunc, middle ? middle : "", col_width);
    strncpy(right_trunc, right ? right : "", col_width);

    left_trunc[col_width] = '\0';
    middle_trunc[col_width] = '\0';
    right_trunc[col_width] = '\0';

    // Format the row
    lwprintf_snprintf(buffer, sizeof(buffer), "%-*s%-*s%-*s",
             col_width, left_trunc,
             col_width, middle_trunc,
             col_width, right_trunc);

    status = esc_pos_print_line(printer, buffer);
    return status;
}

uint8_t esc_pos_tab(esc_pos_printer_t *printer)
{
    uint8_t cmd = ASCII_TAB;
    printer->status.column = (printer->status.column + 4) & 0xFC;
    return send_command(printer, &cmd, 1);
}

/* Test Functions */

uint8_t esc_pos_test_page(esc_pos_printer_t *printer)
{
    uint8_t cmd[] = {ASCII_GS, '(', 'A', 2, 0, 0, 3};
    return send_command(printer, cmd, sizeof(cmd));
}

uint8_t esc_pos_self_test(esc_pos_printer_t *printer)
{
    uint8_t status = 0;

    // Print test patterns
    esc_pos_set_align(printer, ALIGN_CENTER);
    esc_pos_set_size(printer, SIZE_LARGE);
    status |= esc_pos_print_line(printer, "SELF TEST");

    esc_pos_set_size(printer, SIZE_SMALL);
    esc_pos_set_align(printer, ALIGN_LEFT);

    // Test different styles
    status |= esc_pos_print_line(printer, "Normal text");
    status |= esc_pos_set_bold(printer, true);
    status |= esc_pos_print_line(printer, "Bold text");
    status |= esc_pos_set_bold(printer, false);

    status |= esc_pos_set_underline(printer, UNDERLINE_SINGLE);
    status |= esc_pos_print_line(printer, "Underlined text");
    status |= esc_pos_set_underline(printer, UNDERLINE_OFF);

    // Test alignment
    status |= esc_pos_set_align(printer, ALIGN_LEFT);
    status |= esc_pos_print_line(printer, "Left aligned");
    status |= esc_pos_set_align(printer, ALIGN_CENTER);
    status |= esc_pos_print_line(printer, "Center aligned");
    status |= esc_pos_set_align(printer, ALIGN_RIGHT);
    status |= esc_pos_print_line(printer, "Right aligned");

    // Restore defaults
    status |= esc_pos_set_align(printer, ALIGN_LEFT);
    status |= esc_pos_feed(printer, 2);

    return status;
}
