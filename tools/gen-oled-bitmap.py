#!/usr/bin/env python3

import sys
from PIL import Image


def image_to_ssd1306_bytes(img):
    """
    Convert image to SSD1306 page-oriented byte array.
    Each byte represents a vertical column of 8 pixels.
    """
    width, height = img.size
    pixels = img.load()

    pages = height // 8
    data = []

    def pixbyte(x, page):
        byte = 0
        for bit in range(8):
            y = page * 8 + bit
            pixel = pixels[x, y]

            if pixel == 0:   # black pixel -> set bit
                byte |= (1 << bit)

        return byte;

    if True:
        for x in range(width):
            for page in range(pages):
                data.append(pixbyte(x, page))
    else:
        for page in range(pages):
            for x in range(width):
                data.append(pixbyte(x, page))

    return data


def generate_c_header(data, width, height, var_name):
    lines = []

    lines.append("#pragma once\n\n")
    lines.append("#include \"cxx_pgm.h\"\n\n")

    lines.append(f"struct {var_name} {{\n");

    lines.append(f"  static constexpr unsigned w = {width};\n")
    lines.append(f"  static constexpr unsigned h = {height};\n")

    #lines.append(f"#define {var_name.upper()}_WIDTH {width}\n")
    #lines.append(f"#define {var_name.upper()}_HEIGHT {height}\n")

    lines.append(f"  static constexpr auto const data = pgm_array<unsigned char>(")

    comma = ""
    for i, byte in enumerate(data):
        if i % 16 == 0:
            lines.append(f"{comma}\n   ")
            comma = ""
        lines.append(f"{comma}0x{byte:02X}")
        comma = ", "

    lines.append("\n  );\n")
    lines.append("\n};\n")

    return "".join(lines)


def main():
    if len(sys.argv) != 4:
        print("Usage:")
        print("python bmp_to_ssd1306.py input.bmp output.h varname")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]
    var_name = sys.argv[3]

    img = Image.open(input_file)

    new_image = Image.new("RGBA", img.size, "WHITE")
    new_image.paste(img, mask=img)

    img = new_image

    # convert to 1-bit
    img = img.convert("1")

    width, height = img.size

    if height % 8 != 0:
        raise ValueError("Image height must be divisible by 8 for SSD1306")

    data = image_to_ssd1306_bytes(img)

    header = generate_c_header(data, width, height, var_name)

    with open(output_file, "w") as f:
        f.write(header)

    print(f"Header written to {output_file}")


if __name__ == "__main__":
    main()


