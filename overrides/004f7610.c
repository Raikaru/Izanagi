/* Retail 004f7610: ASCII upper-casing of a signed char (a-z become A-Z), result sign-extended. */
int toUpper(char param_1)
{
    int c = (int)param_1;

    if ((u32)(c - 0x61) < 0x1a) {
        c = (signed char)(c - 0x20);
    }
    return c;
}
