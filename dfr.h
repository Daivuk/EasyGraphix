#pragma once

#include <string>

namespace dfr {

	enum eAlignV {
		ALIGNV_TOP		= 0x1,
		ALIGNV_CENTER	= 0x2,
		ALIGNV_BOTTOM	= 0x4,
	};

	enum eAlignH {
		ALIGNH_LEFT		= 0x8,
		ALIGNH_CENTER	= 0x10,
		ALIGNH_RIGHT	= 0x20,
		ALIGNH_JUSTIFY	= 0x40
	};

	enum eAlign {
		ALIGN_TOP_LEFT				= ALIGNV_TOP | ALIGNH_LEFT,
		ALIGN_TOP					= ALIGNV_TOP | ALIGNH_CENTER,
		ALIGN_TOP_RIGHT				= ALIGNV_TOP | ALIGNH_RIGHT,
		ALIGN_LEFT					= ALIGNV_CENTER | ALIGNH_LEFT,
		ALIGN_CENTER				= ALIGNV_CENTER | ALIGNH_CENTER,
		ALIGN_RIGHT					= ALIGNV_CENTER | ALIGNH_RIGHT,
		ALIGN_BOTTOM_LEFT			= ALIGNV_BOTTOM | ALIGNH_LEFT,
		ALIGN_BOTTOM				= ALIGNV_BOTTOM | ALIGNH_CENTER,
		ALIGN_BOTTOM_RIGHT			= ALIGNV_BOTTOM | ALIGNH_RIGHT,
		ALIGN_TOP_LEFT_JUSTIFY		= ALIGNV_TOP | ALIGNH_LEFT | ALIGNH_JUSTIFY,
		ALIGN_TOP_JUSTIFY			= ALIGNV_TOP | ALIGNH_CENTER | ALIGNH_JUSTIFY,
		ALIGN_TOP_RIGHT_JUSTIFY		= ALIGNV_TOP | ALIGNH_RIGHT | ALIGNH_JUSTIFY,
		ALIGN_LEFT_JUSTIFY			= ALIGNV_CENTER | ALIGNH_LEFT | ALIGNH_JUSTIFY,
		ALIGN_CENTER_JUSTIFY		= ALIGNV_CENTER | ALIGNH_CENTER | ALIGNH_JUSTIFY,
		ALIGN_RIGHT_JUSTIFY			= ALIGNV_CENTER | ALIGNH_RIGHT | ALIGNH_JUSTIFY,
		ALIGN_BOTTOM_LEFT_JUSTIFY	= ALIGNV_BOTTOM | ALIGNH_LEFT | ALIGNH_JUSTIFY,
		ALIGN_BOTTOM_JUSTIFY		= ALIGNV_BOTTOM | ALIGNH_CENTER | ALIGNH_JUSTIFY,
		ALIGN_BOTTOM_RIGHT_JUSTIFY	= ALIGNV_BOTTOM | ALIGNH_RIGHT | ALIGNH_JUSTIFY
	};

	struct sPoint {
		int x, y;
	};

	struct sRect {
		int x, y, w, h;
	};

	struct sColor {
		unsigned char r, g, b;
	};

	struct sRenderInfo {
		sRect	renderedRect;		/** Area that was rendered */
		int		renderedPointSize;	/** Point size that was used for the render */
		sPoint	cursorPosition;		/** The cursor position after the render. y is the current baseline */
	};

	struct sImage {
		unsigned char*	pData;			/** Pointeur to image data */ 
		int				width, height;	/** Dimensions of the image */
	};

	struct sFont {
		std::string filename;	/** Filename to the .ttf file */
		int			pointSize;
	};

	struct sFormating {
		bool	wordWrap;
		eAlign	align;
		int		minPointSize;	/** Minimum point size for autoresize. Set to 0 for no autoresize */
		bool	rightToLeft;	/** For arabic languages */
	};

	void init();

	/**
		Draw text using specific font and point size onto an image buffer.

		@param in_text Text to render. i.e: "Hello World!"

		@param in_font sFont structure defining the Font. Filename, pointsize

		@param in_outputImage sImage structure defining the target image.

		@param in_formating sFormating structure defining text formating, alignement, wordwrap and autosize.

		@param in_color Color of the text RGB. Each component in range 0-255

		@return sRenderInfo structure containing information about the render. Rectangle, cursor position, etc.
	*/
	sRenderInfo drawText(
		const std::string& in_text,
		const sImage& in_outputImage,
		const sFont& in_font,
		const sFormating& in_formating = { },
		const sColor& in_color = { 255, 255, 255 });
	sRenderInfo drawText(
		const std::wstring& in_text,
		const sImage& in_outputImage,
		const sFont& in_font,
		const sFormating& in_formating = {},
		const sColor& in_color = { 255, 255, 255 });
};
