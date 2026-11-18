#pragma once
#include <string>
#include <cstdint>
namespace Language {
inline bool russian=true;
const char* tr(const char* english);
// Strict UTF-8 decoding. Invalid byte sequences become '?' without over-reading.
uint32_t next(const std::string& text,size_t& offset);
std::string lower(const std::string& text);
void pop(std::string& text);
void append(std::string& text,const std::string& input,size_t max_characters);
size_t length(const std::string& text);
std::string tail(const std::string& text,size_t characters);
int glyph(uint32_t codepoint);
}
