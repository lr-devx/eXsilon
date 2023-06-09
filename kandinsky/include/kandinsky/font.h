#ifndef KANDINSKY_FONT_H
#define KANDINSKY_FONT_H

#include <stdint.h>
#include <stddef.h>
#include <kandinsky/size.h>
#include <kandinsky/coordinate.h>
#include <ion/unicode/code_point.h>
#include "palette.h"

/* We use UTF-8 encoding. This means that a character is encoded as a code point
 * that uses between 1 and 4 bytes. Code points can be "combining", in which
 * case their glyph should be superimposed to the glyph of the previous code
 * point in the string. This is for instance used to print accents: the string
 * for the glyph 'è' is composed of the code point for 'e' followed by the
 * combining code point for '`'.
 * ASCII characters have the same encoding in ASCII and in UTF-8.
 *
 * We do not provide a glyph for each of the 1,112,064 valid UTF-8 code points.
 * We thus have a table of the glyphs we can draw (uint32_t CodePoints[] in
 * kandinsky/fonts/code_points.h). To easily compute the index of a code point in
 * the CodePoints table, we use the m_table matching table: it contains the
 * CodePointIndexPairs of the first code point of each series of consecutive
 * code points in the CodePoints table. This table is create in rasterizer.c. */

class KDFont {
private:
  static constexpr int k_bitsPerPixel = 4; // TODO: Should be generated by the rasterizer
  static constexpr int k_maxGlyphPixelCount = 180; //TODO: Should be generated by the rasterizer
  static const KDFont privateLargeFont;
  static const KDFont privateSmallFont;
public:
  static constexpr const KDFont * LargeFont = &privateLargeFont;
  static constexpr const KDFont * SmallFont = &privateSmallFont;

  static bool CanBeWrittenWithGlyphs(const char * text);

  KDSize stringSize(const char * text, int textLength = -1) const {
    return stringSizeUntil(text, textLength < 0 ? nullptr : text + textLength);
  }
  KDSize stringSizeUntil(const char * text, const char * limit) const;

  union GlyphBuffer {
  public:
    GlyphBuffer() {} // Don't initialize either buffer
    KDColor * colorBuffer() { return m_colors; }
    uint8_t * grayscaleBuffer() { return m_grayscales; }
    uint8_t * secondaryGrayscaleBuffer() { return m_grayscales + k_maxGlyphPixelCount; }
  private:
    uint8_t m_grayscales[2*k_maxGlyphPixelCount];
    KDColor m_colors[k_maxGlyphPixelCount];
  };

  using GlyphIndex = uint8_t;
  class CodePointIndexPair {
  public:
    constexpr CodePointIndexPair(CodePoint c, GlyphIndex i) : m_codePoint(c), m_glyphIndex(i) {}
    CodePoint codePoint() const { return m_codePoint; }
    GlyphIndex glyphIndex() const { return m_glyphIndex; }
  private:
    CodePoint m_codePoint;
    GlyphIndex m_glyphIndex;
  };
  static constexpr GlyphIndex IndexForReplacementCharacterCodePoint = 133;
  GlyphIndex indexForCodePoint(CodePoint c) const;

  void setGlyphGrayscalesForCodePoint(CodePoint codePoint, GlyphBuffer * glyphBuffer) const;
  void accumulateGlyphGrayscalesForCodePoint(CodePoint codePoint, GlyphBuffer * glyphBuffer) const;

  using RenderPalette = KDPalette<(1<<k_bitsPerPixel)>;
  void colorizeGlyphBuffer(const RenderPalette * renderPalette, GlyphBuffer * glyphBuffer) const;

  RenderPalette renderPalette(KDColor textColor, KDColor backgroundColor) const {
    return RenderPalette::Gradient(textColor, backgroundColor);
  }
  KDSize glyphSize() const { return m_glyphSize; }

  constexpr KDFont(size_t tableLength, const CodePointIndexPair * table, KDCoordinate glyphWidth, KDCoordinate glyphHeight, const uint16_t * glyphDataOffset, const uint8_t * data) :
    m_tableLength(tableLength), m_table(table), m_glyphSize(glyphWidth, glyphHeight), m_glyphDataOffset(glyphDataOffset), m_data(data) { }
private:
  void fetchGrayscaleGlyphAtIndex(GlyphIndex index, uint8_t * grayscaleBuffer) const;

  const uint8_t * compressedGlyphData(GlyphIndex index) const {
    return m_data + m_glyphDataOffset[index];
  }

  uint16_t compressedGlyphDataSize(GlyphIndex index) const {
    return m_glyphDataOffset[index+1] - m_glyphDataOffset[index];
  }

  size_t m_tableLength;
  const CodePointIndexPair * m_table;
  KDSize m_glyphSize;
  const uint16_t * m_glyphDataOffset;
  const uint8_t * m_data;
};

#endif