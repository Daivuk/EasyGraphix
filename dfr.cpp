#include "dfr.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <assert.h>
#include <map>
#include <vector>
#include <locale>
#include <codecvt>
#include <mutex>

namespace dfr {

	std::mutex g_ttfFacesMutex;
	FT_Library g_ttfLibrary;
	std::map<std::string, FT_Face> g_faces;
	bool g_isInitialized = false;

	void init() {
		if (g_isInitialized) return;
		assert(!FT_Init_FreeType(&g_ttfLibrary));
		g_isInitialized = true;
	}

	inline int dfr_max(int a, int b) {
		return a - ((a - b) & (a - b) >> 31);
	}

	inline int dfr_min(int a, int b) {
		return a + (((b - a) >> 31) & (b - a));
	}

	sRenderInfo drawText(
		const std::string& in_text,
		const sImage& in_outputImage,
		const sFont& in_font,
		const sFormating& in_formating,
		const sColor& in_color) {

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring wText = converter.from_bytes(in_text);

		return drawText(wText, in_outputImage, in_font, in_formating, in_color);
	}

	sRenderInfo drawText(
		const std::wstring& in_text,
		const sImage& in_outputImage,
		const sFont& in_font,
		const sFormating& in_formating,
		const sColor& in_color) {

		sRenderInfo result;
		sPoint pen = { 0, 0 };
		FT_Face face;

		g_ttfFacesMutex.lock();
		{
			const auto& it = g_faces.find(in_font.filename);
			if (it == g_faces.end()) {
				assert(!FT_New_Face(
					g_ttfLibrary,
					in_font.filename.c_str(),
					0,
					&face));
				g_faces[in_font.filename] = face;
			}
			else {
				face = it->second;
			}
		}
		g_ttfFacesMutex.unlock();

		int pointSize = in_font.pointSize;
		struct sLine {
			int width = 0;
			int from = 0;
			int to = 0;
		};
		while (true) {
			assert(!FT_Set_Pixel_Sizes(
				face,
				0,
				pointSize));

			FT_GlyphSlot	slot = face->glyph;
			int				n, x, y, k;
			int				num_chars = (int) in_text.size();
			wchar_t*			text = (wchar_t*) in_text.c_str();
			unsigned char	alpha;
			FT_Error		error;
			int lineHeight = face->size->metrics.height >> 6;
			int maxW = 0;
			pen = { 0, 0 };
			int alignOffsetX = 0;
			int alignOffsetY = 0;
			int lastWordStart = -1;
			int lastWordWidth = 0;
			std::vector<sLine> lines;
			sLine currentLine;
			result.renderedRect.x = in_outputImage.width;
			result.renderedRect.y = in_outputImage.height;
			result.renderedRect.w = 0;
			result.renderedRect.h = 0;

			for (n = 0; n < num_chars; ++n) {
				// Get the glyph metrics only
				FT_UInt  glyph_index;
				glyph_index = FT_Get_Char_Index(face, text[n]);
				error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
				if (error) continue;

				int advance = slot->advance.x >> 6;
				if (n > 0) {
					FT_UInt leftGlyph = FT_Get_Char_Index(face, text[n - 1]);
					FT_Vector kerning;
					FT_Get_Kerning(face, leftGlyph, glyph_index, FT_KERNING_DEFAULT, &kerning);
					advance += kerning.x >> 6;
				}
				int gW = advance;

				if (text[n] == '\n') {
					// We will new line and ignore that return
					currentLine.width = pen.x;
					currentLine.to = n - 1;
					lines.push_back(currentLine);
					currentLine.from = currentLine.to + 2;
					lastWordWidth = 0;
					pen.x = 0;
					pen.y += lineHeight;
					continue;
				}

				if (pen.x + gW >= in_outputImage.width && in_formating.wordWrap) {
					maxW = dfr_max(maxW, pen.x);

					if (text[n] == ' ') {
						// We will new line and ignore that space
						currentLine.width = pen.x;
						currentLine.to = n - 1;
						lines.push_back(currentLine);
						currentLine.from = currentLine.to + 1;
						lastWordWidth = 0;
						pen.x = 0;
						pen.y += lineHeight;
						continue;
					}

					// This character will go over, we will break line
					if (lastWordStart == -1) {
						// We have to cut this word in half, its bigger
						// than the row!
						currentLine.width = pen.x;
						currentLine.to = n - 1;
						lines.push_back(currentLine);
						currentLine.from = currentLine.to + 1;
						lastWordWidth = 0;
						pen.x = 0;
						pen.y += lineHeight;
						--n;
						continue;
					}
					else {
						currentLine.width = lastWordWidth;
						currentLine.to = lastWordStart - 1;
						lines.push_back(currentLine);
						currentLine.from = currentLine.to + 1;
						pen.x = 0;
						pen.y += lineHeight;
						n = lastWordStart - 1;
						lastWordWidth = 0;
						lastWordStart = -1;
						continue;
					}
				}

				if (text[n] == ' ') {
					lastWordWidth = pen.x;
					lastWordStart = n + 1;
				}

				pen.x += gW;
			}

			if (pen.x) {
				maxW = dfr_max(maxW, pen.x);
				currentLine.width = pen.x;
				currentLine.to = n - 1;
				lines.push_back(currentLine);
			}

			// Now render line by line
			pen.x = 0;

			// Align vertically
			if (in_formating.align & ALIGNV_TOP)
				pen.y = face->ascender >> 6;
			else if (in_formating.align & ALIGNV_CENTER)
				pen.y = (in_outputImage.height - (int) (lines.size() - 1) * lineHeight - (face->descender >> 6)) / 2;
			else if (in_formating.align & ALIGNV_BOTTOM)
				pen.y = in_outputImage.height + (face->descender >> 6) - ((int) lines.size() - 1) * (lineHeight >> 6);

			// It is going to fit? Try a smaller fixe it not
			if (in_formating.minPointSize && pointSize > in_formating.minPointSize) {
				if (maxW > in_outputImage.width ||
					((int) lines.size() * lineHeight) > in_outputImage.height) {
					--pointSize;
					continue;
				}
			}

			size_t lineCpt = 0;
			for (const auto& line : lines) {
				if (in_formating.align & ALIGNH_LEFT)
					pen.x = 0;
				else if (in_formating.align & ALIGNH_CENTER)
					pen.x = (in_outputImage.width - line.width) / 2;
				else if (in_formating.align & ALIGNH_RIGHT)
					pen.x = in_outputImage.width - line.width;
				bool allowJustify = lineCpt < lines.size() - 1 && in_formating.align & ALIGNH_JUSTIFY;
				if (allowJustify) {
					if (lines[lineCpt + 1].to - lines[lineCpt + 1].from <= 0) allowJustify = false;
				}
				int leftOver = in_outputImage.width - line.width;
				int cCount = line.to - line.from + 1;
				if (allowJustify) pen.x = 0;
				// 	for (n = line.from; n <= line.to; ++n)
				if (in_formating.rightToLeft) n = line.to;
				else n = line.from;
				while (true) {
					if (in_formating.rightToLeft) { if (n < line.from) break; }
					else if (n > line.to) break;

					// Get the glyph
					FT_UInt  glyph_index;
					glyph_index = FT_Get_Char_Index(face, text[n]);
					error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
					if (error) continue;
					error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
					if (error) continue;

					int glyphX = pen.x + (slot->metrics.horiBearingX >> 6);
					int glyphY = pen.y - (slot->metrics.horiBearingY >> 6);

					// Justify
					if (allowJustify) {
						int i = n - line.from;
						if (i > 0) {
							glyphX += leftOver * i / cCount;
						}
					}

					FT_Bitmap* bitmap = &slot->bitmap;

					int limits[4] = {
						dfr_max(0, glyphX) - glyphX,
						dfr_max(0, glyphY) - glyphY,
						dfr_min(in_outputImage.width, bitmap->width + glyphX) - glyphX,
						dfr_min(in_outputImage.height, bitmap->rows + glyphY) - glyphY,
					};
					result.renderedRect.x = dfr_min(limits[0] + glyphX, result.renderedRect.x);
					result.renderedRect.y = dfr_min(limits[1] + glyphY, result.renderedRect.y);
					result.renderedRect.w = dfr_max(limits[2] + glyphX, result.renderedRect.w);
					result.renderedRect.h = dfr_max(limits[3] + glyphY, result.renderedRect.h);
					for (y = limits[1]; y < limits[3]; y++) {
						for (x = limits[0]; x < limits[2]; x++) {
							k = ((y + glyphY) * in_outputImage.width + x + glyphX) * 4;
							alpha = dfr_max(in_outputImage.pData[k + 3], bitmap->buffer[y * bitmap->pitch + x]);
							in_outputImage.pData[k + 0] = in_color.r * alpha / 255;
							in_outputImage.pData[k + 1] = in_color.g * alpha / 255;
							in_outputImage.pData[k + 2] = in_color.b * alpha / 255;
							in_outputImage.pData[k + 3] = alpha;
						}
					}

					int advance = slot->advance.x >> 6;
					if (in_formating.rightToLeft) {
						if (n < line.to) {
							FT_UInt leftGlyph = FT_Get_Char_Index(face, text[n + 1]);
							FT_Vector kerning;
							FT_Get_Kerning(face, leftGlyph, glyph_index, FT_KERNING_DEFAULT, &kerning);
							advance += kerning.x;
						}
					}
					else {
						if (n > line.from) {
							FT_UInt leftGlyph = FT_Get_Char_Index(face, text[n - 1]);
							FT_Vector kerning;
							FT_Get_Kerning(face, leftGlyph, glyph_index, FT_KERNING_DEFAULT, &kerning);
							advance += kerning.x >> 6;
						}
					}
					pen.x += advance;

					if (in_formating.rightToLeft) --n;
					else ++n;
				}
				++lineCpt;
				if (lineCpt < (int) lines.size()) {
					pen.y += lineHeight;
				}
			}

			break;
		}

		result.renderedRect.w -= result.renderedRect.x;
		result.renderedRect.h -= result.renderedRect.y;
		result.renderedPointSize = pointSize;
		result.cursorPosition = pen;

		return result;
	}
};
