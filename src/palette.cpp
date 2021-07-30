#include <vector>

#pragma warning(push, 0)
#include <FL/Fl.H>
#include <FL/Fl_Image_Surface.H>
#pragma warning(pop)

#include "image.h"
#include "option-dialogs.h"

// Avoid "warning C4458: declaration of 'i' hides class member"
// due to Fl_Window's Fl_X *i
#pragma warning(push)
#pragma warning(disable : 4458)

static bool write_graphic_palette(const char *f, const std::vector<Palette> &palettes, size_t nc) {
	int w = (int)nc, h = (int)palettes.size();
	if (w == 256) { w /= 16; h *= 16; }
	Fl_Image_Surface *surface = new Fl_Image_Surface(w, h);
	surface->set_current();

	fl_rectf(0, 0, w, h, FL_BLACK);
	int i = 0;
	for (const Palette &palette : palettes) {
		int j = 0;
		for (Fl_Color c : palette) {
			fl_color(c);
			fl_point(j % w, i + j / w);
			j++;
		}
		i++;
	}

	Fl_RGB_Image *img = surface->image();
	delete surface;
	Fl_Display_Device::display_device()->set_current();

	Image::Result result = Image::write_image(f, img);
	delete img;

	return result == Image::Result::IMAGE_OK;
}

bool write_palette(const char *f, const std::vector<Palette> &palettes,
	Image_To_Tiles_Dialog::Palette_Format pal_fmt, size_t nc) {
	if (pal_fmt == Image_To_Tiles_Dialog::Palette_Format::PLTE) {
		// The tileset image is already written
		return true;
	}

	if (pal_fmt == Image_To_Tiles_Dialog::Palette_Format::PNG || pal_fmt == Image_To_Tiles_Dialog::Palette_Format::BMP) {
		// The file extension determines the image format
		return write_graphic_palette(f, palettes, nc);
	}

	FILE *file = fl_fopen(f, "wb");
	if (!file) { return false; }

	if (pal_fmt == Image_To_Tiles_Dialog::Palette_Format::RGB) {
		int p = 0;
		for (const Palette &palette : palettes) {
			fprintf(file, "; palette %d\n", p++);
			for (Fl_Color c : palette) {
				uchar r, g, b;
				Fl::get_color(c, r, g, b);
				fprintf(file, "\tRGB %02d, %02d, %02d\n", (int)(r / 8), (int)(g / 8), (int)(b / 8));
			}
		}
	}
	else if (pal_fmt == Image_To_Tiles_Dialog::Palette_Format::JASC) {
		fprintf(file, "JASC-PAL\r\n0100\r\n%zu\r\n", palettes.size() * nc);
		for (const Palette &palette : palettes) {
			for (Fl_Color c : palette) {
				uchar r, g, b;
				Fl::get_color(c, r, g, b);
				fprintf(file, "%d %d %d\r\n", (int)r, (int)g, (int)b);
			}
		}
	}
	else if (pal_fmt == Image_To_Tiles_Dialog::Palette_Format::ACT) {
		for (const Palette &palette : palettes) {
			for (Fl_Color c : palette) {
				uchar r, g, b;
				Fl::get_color(c, r, g, b);
				fprintf(file, "%c%c%c", r, g, b);
			}
		}
		for (size_t i = palettes.size(); i < 256; i++) {
			fputc(0, file);
			fputc(0, file);
			fputc(0, file);
		}
	}
	else if (pal_fmt == Image_To_Tiles_Dialog::Palette_Format::GPL) {
		const char *name = fl_filename_name(f);
		fprintf(file, "GIMP Palette\nName: %s\n", name);
		for (const Palette &palette : palettes) {
			for (Fl_Color c : palette) {
				uchar r, g, b;
				Fl::get_color(c, r, g, b);
				fprintf(file, "% 3d % 3d % 3d\t#%02x%02x%02x\n", (int)r, (int)g, (int)b, (int)r, (int)g, (int)b);
			}
		}
	}

	fclose(file);
	return true;
}

bool write_tilepal(const char *f, const std::vector<size_t> &tileset, const std::vector<int> &tile_palettes) {
	FILE *file = fl_fopen(f, "wb");
	if (!file) { return false; }

	fputs("pertilepals: MACRO\nrept _NARG / 2\n\tdn \\2, \\1\n\tshift\n\tshift\nendr\nENDM\n", file);
	size_t nc = 16;
	size_t nt = tileset.size();
	size_t np = std::max(nt, (size_t)(nc * 3));
	for (size_t i = 0; i < np; i++) {
		if (!(i % nc)) {
			fputs("\n\tpertilepals ", file);
		}
		if (i < nt) {
			size_t ti = tileset[i];
			int pi = ti < tile_palettes.size() ? tile_palettes[ti] : 0;
			fprintf(file, "%d", pi);
		}
		else {
			fputc('0', file);
		}
		if (i < np - 1 && i % nc != nc - 1) {
			fputs(", ", file);
		}
	}
	if (np % 2) {
		fputs(", 0", file);
	}
	fputc('\n', file);

	fclose(file);
	return true;
}

#pragma warning(pop)
