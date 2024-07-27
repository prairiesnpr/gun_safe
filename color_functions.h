struct RGBW
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t w;
};

float reverse_gamma(float x)
{
    if (x <= 0.0031308)
    {
        return x * 12.92;
    }
    return ((1.0 + 0.055) * pow(x, (1.0 / 2.4)) - 0.055);
}

float max_of_two(float x, float y)
{
    return (x > y) ? x : y;
}

float max_of_three(float m, float n, float p)
{
    return max_of_two(max_of_two(m, n), p);
}

RGBW color_xy_brightness_to_rgb(float vX, float vY, uint8_t ibrightness)
{
    float brightness = ibrightness / 255.0;
    RGBW result;
    if (brightness == 0.0)
    {
        return result;
    }

    float Y = brightness;

    if (vY == 0.0)
    {
        vY += 0.00000000001;
    }
    float X = (Y / vY) * vX;
    float Z = (Y / vY) * (1 - vX - vY);

    // Convert to RGB using Wide RGB D65 conversion.
    float r = X * 1.656492 - Y * 0.354851 - Z * 0.255038;
    float g = -X * 0.707196 + Y * 1.655397 + Z * 0.036152;
    float b = X * 0.051713 - Y * 0.121364 + Z * 1.011530;

    // Apply reverse gamma correction.
    r = reverse_gamma(r);
    g = reverse_gamma(g);
    b = reverse_gamma(b);

    // Bring all negative components to zero.
    r = max_of_two(0, r);
    g = max_of_two(0, g);
    b = max_of_two(0, b);

    // If one component is greater than 1, weight components by that value.
    float max_component = max_of_three(r, g, b);
    if (max_component > 1)
    {
        r = r / max_component;
        g = g / max_component;
        b = b / max_component;
    }

    uint8_t ir = (uint8_t)(r * 255);
    uint8_t ig = (uint8_t)(g * 255);
    uint8_t ib = (uint8_t)(b * 255);

    if (ir > 240 && ig > 240 && ib > 240)
    {
        result.w = ibrightness;
        return result;
    }
    result.r = ir;
    result.g = ig;
    result.b = ib;
    result.w = 0;
    return result;
}

RGBW color_int_xy_brightness_to_rgb(uint16_t vX, uint16_t vY, uint8_t ibrightness) {
    float color_x = vX/65536.0;
    float color_y = vY/65536.0;
    return color_xy_brightness_to_rgb(color_x, color_y, ibrightness);
}