//! C-locale <ctype.h> classification used by the upstream libraries (bytes >= 0x80 are never
//! in any class, matching the "C" locale both upstream platforms run with).

pub inline fn isdigit(c: u8) bool {
    return c >= '0' and c <= '9';
}
pub inline fn islower(c: u8) bool {
    return c >= 'a' and c <= 'z';
}
pub inline fn isupper(c: u8) bool {
    return c >= 'A' and c <= 'Z';
}
pub inline fn isalpha(c: u8) bool {
    return islower(c) or isupper(c);
}
pub inline fn isalnum(c: u8) bool {
    return isalpha(c) or isdigit(c);
}
pub inline fn isspace(c: u8) bool {
    return c == ' ' or (c >= '\t' and c <= '\r');
}
pub inline fn iscntrl(c: u8) bool {
    return c < 0x20 or c == 0x7f;
}
pub inline fn isgraph(c: u8) bool {
    return c > 0x20 and c < 0x7f;
}
pub inline fn ispunct(c: u8) bool {
    return isgraph(c) and !isalnum(c);
}
pub inline fn isxdigit(c: u8) bool {
    return isdigit(c) or (c >= 'a' and c <= 'f') or (c >= 'A' and c <= 'F');
}
pub inline fn tolower(c: u8) u8 {
    return if (isupper(c)) c + ('a' - 'A') else c;
}
pub inline fn toupper(c: u8) u8 {
    return if (islower(c)) c - ('a' - 'A') else c;
}
