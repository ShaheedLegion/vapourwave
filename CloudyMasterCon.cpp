#include <Windows.h>
#include "Renderer.hpp"
#include "FastNoise.h"

#define randf() (static_cast<float>(rand()))

namespace DXSS {
	const int particle_layers = 6;
	const int particle_x_dim = 60;
	const int particle_z_dim = 60;
	const int particles_total = 3600;
	float t = 0.0f;

	union pixel {
		unsigned int col;
		struct { unsigned char b, g, r, a; };
	};
	float lerp(float v0, float v1, float t) {
		return (1 - t) * v0 + t * v1;
	}
	struct range {
		pixel s, e;
		int ys, ye;

		unsigned int getCol(float y) {
			if (s.col == e.col) return s.col;

			float perc = (float)(y - ys) / (float)(ye - ys);
			pixel p;

			p.b = (unsigned char)lerp((float)s.b, (float)e.b, perc);
			p.g = (unsigned char)lerp((float)s.g, (float)e.g, perc);
			p.r = (unsigned char)lerp((float)s.r, (float)e.r, perc);
			p.a = (unsigned char)lerp((float)s.a, (float)e.a, perc);

			return p.col;
		}
	};

	range color_stops[particle_layers] = {
		{{0x00FFFFFF}, {0x00134685}, -100, -80},
		{{0x00134685}, {0x002568A7}, -80, -60},
		{{0x002568A7}, {0x00695605}, -60, 20},
		{{0x00695605}, {0x00329629}, 20, 40},
		{{0x00329629}, {0x00dce38d}, 40, 90},
		{{0x002B60C2}, {0x002B60C2}, 90, 850}
	};

	struct particle {
		float x, y, z;
		int tx, ty;
	};
	struct camera {
		float x, y, z, cx, cy, cz;

		void update(int direction) {
			bool directionChanged = false;
			switch (direction) {
				case 0: // left
					x -= 1.0f;
					directionChanged = true;
				break;
				case 1: // up
					y -= 1.0f;
					directionChanged = true;
				break;
				case 2: // right
					x += 1.0f;
					directionChanged = true;
				break;
				case 3: // down
					y += 1.0f;
					directionChanged = true;
				break;
				case 4: // forward
					z -= 1.0f;
					directionChanged = true;
				break;
				case 5: // back
					z += 1.0f;
					directionChanged = true;
				break;
			}
			if (directionChanged) {
				printf("Camera x[%f], y[%f], z[%f]\n", x, y, z);
			}
		}

		void transform(particle & p, int sw, int sh, int hsw, int hsh) {
			float ix = p.x + x;
			float iy = p.y + y;
			float iz = p.z + z;

			// Perform the perspective-correct transform here.
			p.tx = ((ix / iz) * sw) + hsw;
			p.ty = ((iy / iz) * sh) + hsh;
		}
	};
	const float _floor = 35.0f;
	struct particlefield {
		particle * particles;
		camera cam;
		particlefield() {
			particles = new particle[particles_total];
			int i = 0;
			int hspacer = 19; // world units
			int zspacer = 27;
			float start_z = (particle_z_dim * zspacer) + (particle_z_dim * 4);
			for (int z = 0; z < particle_z_dim; z++) {
				float start_x = -((particle_x_dim >> 1) * hspacer);
				for (int x = 0; x < particle_x_dim; x++) {
					particles[i].x = start_x;
					particles[i].y = _floor;
					particles[i].z = start_z;
					start_x += hspacer;
					++i;
				}
				start_z -= zspacer;
			}

			cam.x = 0;
			cam.y = 195.0f;
			cam.z = 80.0f;
			cam.cx = 1.0f;
			cam.cy = 1.0f;
			cam.cz = 1.0f;
		}

		~particlefield() {
			delete[] particles;
		}

		void update(int direction, FastNoise& n) {
			cam.update(direction);
			int i = 0;
			int sw = _WIDTH;
			int sh = _HEIGHT;
			int hsw = sw >> 1;
			int hsh = sh >> 1;

			for (int z = 0; z < particle_z_dim; z++) {
				for (int x = 0; x < particle_x_dim; x++) {
					float height = n.GetNoise((float)x, (float)z - t) * 192.0f;
					particles[i].y = height;
					if (particles[i].y > 95.0f) particles[i].y = 95.0f;
					cam.transform(particles[i], sw, sh, hsw, hsh);
					++i;
				}
			}

			t += 0.1f;
		}
	};

	bool depthTest(float * db, float z, int tx, int ty, int sw) {
		if (z > db[(ty * sw) + tx]) return false;
		db[(ty * sw) + tx] = z;
		return true;
	}
}; // namespace DXSS


namespace DXDD {
	const float PI = 3.142857142857143f;
	struct pointf {
		float x, y;
	};
	struct image {
		int w;
		int h;
		unsigned int * pixels;

		image() : w(0), h(0), pixels(nullptr) {}
		void init(int _w, int _h) {
			w = _w;
			h = _h;

			if (pixels) {
				delete[] pixels;
				pixels = nullptr;
			}

			pixels = new unsigned int[w * h];

			int len = w * h;
			int i = 0;

			while (i < len) {
				pixels[i] = 0;
				i++;
			}
		}
		~image() { if (pixels) delete[] pixels; }
	};

	void pointOnCircumference(float radius, float angleInDegrees, pointf & origin, pointf & pt) {
		// Convert from degrees to radians via multiplication by PI/180        
		pt.x = (float)(radius * cos(angleInDegrees * PI / 180.0f)) + origin.x;
		pt.y = (float)(radius * sin(angleInDegrees * PI / 180.0f)) + origin.y;
	}

	void blur(image * img) {
		// 1-time blur operation.
		DXSS::pixel p;
		unsigned short r, g, b, a;
		for (int y = 1; y < img->w - 1; y++) {
			for (int x = 1; x < img->h - 1; x++) {

				p.col = img->pixels[(y * img->w) + (x - 1)];
				r = p.r; g = p.g; b = p.b; a = p.a;
				p.col = img->pixels[(y * img->w) + (x + 1)];
				r += p.r; g += p.g; b += p.b; a += p.a;
				p.col = img->pixels[((y - 1) * img->w) + x];
				r += p.r; g += p.g; b += p.b; a += p.a;
				p.col = img->pixels[((y + 1) * img->w) + x];
				r += p.r; g += p.g; b += p.b; a += p.a;

				p.r = r >> 2;
				p.g = g >> 2;
				p.b = b >> 2;
				p.a = a >> 2;
				img->pixels[(y * img->w) + x] = p.col;
			}
		}
	}

	void constructSky(image * sky, int sw, int sh) {
		sky->init(sw, sh);
		const int num_sky_stops = 6;
		DXSS::range sky_stops[num_sky_stops] = {
			{{0x00000000}, {0x001B1F40}, 0, 64},
			{{0x001B1F40}, {0x00403478}, 64, 100},
			{{0x00403478}, {0x009A228D}, 100, 120},
			{{0x009A228D}, {0x009A228D}, 120, 140},
			{{0x009A228D}, {0x00000000}, 140, 180},
			{{0x00000000}, {0x00000000}, 180, 300},
		};

		int stop_dist = sh / num_sky_stops;
		int prev_y = 0;
		for (int i = 0; i < num_sky_stops; i++) {
			sky_stops[i].ys = prev_y;
			prev_y += stop_dist;
			sky_stops[i].ye = prev_y;
		}

		detail::Uint32 sky_col = 0x00000000;
		for (int y = 0; y < sh; y++) {
			for (int t = 0; t < num_sky_stops; t++) {
				if ((y > sky_stops[t].ys) && (y < sky_stops[t].ye)) {
					sky_col = sky_stops[t].getCol(y);
					break;
				}
			}
			for (int x = 0; x < sw; x++) {
				sky->pixels[(y * sw) + x] = sky_col;
			}
		}
		blur(sky);
	}

	void constructSun(image * sun, int sw, int sh) {
		sun->init(sw, sh);
		//DXSS::range sun_range = { {0x00FFF4CD}, {0x00FF6D13}, 32, 96 };
		DXSS::range sun_range = { {0x00FFF4CD}, {0x00FF0004}, 32, 96 };

		// The sun will be 1/2 of the size of the given width / h.
		int sun_radius = sw >> 2;

		pointf pt;
		pointf origin;
		origin.x = sw >> 1;
		origin.y = sh >> 1;

		int highest_y = origin.y;
		int lowest_y = origin.y;

		for (int degrees = 180; degrees < 270; degrees++) {
			pointOnCircumference(sun_radius, degrees, origin, pt);

			// we now have a quadrant of a circle.
			// Duplicate the end points as a mirror of the x/y-axes.

			int sx = pt.x;
			int ex = (origin.x - pt.x) + origin.x;
			int sy = pt.y;
			int ey = (origin.y - pt.y) + origin.y;

			if (sy < lowest_y) lowest_y = sy;
			if (sy > highest_y) highest_y = ey;
			if (ey < lowest_y) lowest_y = ey;
			if (ey > highest_y) highest_y = ey;
			if ((sx > 0 && sx < sw) && (ex > 0 && ex < sw)) {
				if ((sy > 0 && sy < sh) && (ey > 0 && ey < sh)) {
					for (int y = sy; y < ey; y++) {
						// calculate the color for this line
						// then rasterize the line.
						detail::Uint32 sun_col = sun_range.getCol(y);
						for (int x = sx; x < ex; x++) {
							sun->pixels[(y * sw) + x] = sun_col;
						}
						// Add a border outline of the sun.
						//detail::Uint32 outline_col = 0x0065BFFF;
						//sun->pixels[(y * sw) + sx] = outline_col;
						//sun->pixels[(y * sw) + ex] = outline_col;
					}
				}
			}
		}
		int ty = highest_y;
		int tty = lowest_y;
		blur(sun);
	}
	void constructBG(image * bg, image * sky, image * sun, int bw, int bh) {
		// Have to copy the sun onto the bg . . 
		// This should be a straight copy, but we could copy + scale if we need to.
		image temp;
		temp.init(sky->w, sky->h);
		{
			int idx = 0;
			int len = sky->w * sky->h;
			unsigned short r, g, b, a;
			DXSS::pixel p;
			DXSS::pixel q;

			while (idx < len) {
				p.col = sky->pixels[idx];
				q.col = sun->pixels[idx];
				r = p.r; r += q.r;
				g = p.g; g += q.g;
				b = p.b; b += q.b;
				a = p.a; a += q.a;
				
				p.r = r > 255 ? 255 : r;
				p.g = g > 255 ? 255 : g;
				p.b = b > 255 ? 255 : b;
				p.a = a > 255 ? 255 : a;
				
				/*
				p.r = r > 255 ? r >> 1 : r;
				p.g = g > 255 ? g >> 1 : g;
				p.b = b > 255 ? b >> 1 : b;
				p.a = a > 255 ? a >> 1 : a;
				*/
				temp.pixels[idx] = p.col;
				idx++;
			}
		}


		int sy = 0;
		for (int y = 0; y < bh; y += 2) {
			int sx = 0;
			for (int x = 0; x < bw; x += 2) {
				unsigned int pix = temp.pixels[(sy * sky->w) + sx];
				bg->pixels[((y + 0) * bw) + (x + 0)] = pix;//  temp.pixels[(sy * sky->w) + sx];
				bg->pixels[((y + 1) * bw) + (x + 0)] = pix;//  temp.pixels[(sy * sky->w) + sx];
				bg->pixels[((y + 0) * bw) + (x + 1)] = pix;//  temp.pixels[(sy * sky->w) + sx];
				bg->pixels[((y + 1) * bw) + (x + 1)] = pix;//  temp.pixels[(sy * sky->w) + sx];
				sx++;
			}
			sy++;
		}
		/*
		for (int y = 0; y < sky->h; y++) {
			unsigned int *row1 = &bg->pixels[((y << 1) * bw)];
			unsigned int *row2 = &bg->pixels[(((y << 1) + 1) * bw)];
			for (int x = 0; x < sky->w; x++) {
				unsigned int pix = temp.pixels[(y * sky->w) + x];
				// Diamond pattern
				// row1[(x << 1)] = pix;
				// row2[(x << 1) + 1] = pix;
				
				row1[(x << 1)] = pix;
				row1[(x << 1) + 1] = pix;
				row2[(x << 1)] = pix;
				row2[(x << 1) + 1] = pix;
				// bg->pixels[((y << 1) * bw) + (x << 1)] = temp.pixels[(y * sky->w) + x];
				// bg->pixels[((y << 1) * bw) + (x << 1) + 1] = temp.pixels[(y * sky->w) + x];
			}
		}
		*/
	}
}; // namespace DXDD

// So, we'll need a smaller buffer onto which we can render the background
// and the sun, which we can then scale up . .

// This is the guts of the renderer, without this it will do nothing.
DWORD WINAPI Update(LPVOID lpParameter) {
	FastNoise noise; // Create a FastNoise object
	noise.SetNoiseType(FastNoise::SimplexFractal);

	Renderer *g_renderer = static_cast<Renderer *>(lpParameter);
	DXSS::particlefield field;
	DXDD::image sky, sun, bg;
	bg.init(_WIDTH, _HEIGHT);
	DXDD::constructSky(&sky, _WIDTH >> 1, _HEIGHT >> 1);
	DXDD::constructSun(&sun, _WIDTH >> 1, _HEIGHT >> 1);
	DXDD::constructBG(&bg, &sky, &sun, _WIDTH, _HEIGHT);

	while (g_renderer->IsRunning()) {
		field.update(g_renderer->GetDirection(), noise);

		detail::Uint32 *buffer = g_renderer->screen.GetPixels();
		float * depthBuffer = g_renderer->screen.GetDepthBuffer();
		{ // clear the screen.
			detail::Uint32 * b = buffer;
			detail::Uint32 *skyb = bg.pixels;
			float * db = depthBuffer;
			int idx = 0;
			int len = _WIDTH * _HEIGHT;
			while (idx < len) {
				*b = *skyb;
				*db = 9999.0f;
				b++;
				db++;
				skyb++;
				idx++;
			}
		}

		/*
		so one of the biggest issues with this is that we need to do a triple-nested loop, which runs in O(n^3) time,
		which is obviously not ideal.
		We need to select the color as part of the update step, so that the colors are part of the transformation calculation.
		*/
		detail::Uint32 col = 0xff0000ff;

		for (int i = 0; i < DXSS::particles_total; i++) {
			if (field.particles[i].tx >= 0 && field.particles[i].tx < _WIDTH) {
				if (field.particles[i].ty >= 0 && field.particles[i].ty < _HEIGHT) {
					if (DXSS::depthTest(depthBuffer, field.particles[i].z, field.particles[i].tx, field.particles[i].ty, _WIDTH)) {
						for (int t = 0; t < DXSS::particle_layers; t++) {
							if ((field.particles[i].y > DXSS::color_stops[t].ys) &&
								(field.particles[i].y < DXSS::color_stops[t].ye)) {
								col = DXSS::color_stops[t].getCol(field.particles[i].y);
								break;
							}
						}
						buffer[(field.particles[i].ty * _WIDTH) + field.particles[i].tx] = col;
					}
				}
			}
		}

		g_renderer->screen.Flip();
		g_renderer->updateThread.Delay(1);
	}

	return 0;
}

int main(int argc, char *args[]) {
	const char *const myclass = "Cloudy";
	Renderer renderer(myclass, &Update, nullptr);
	return 0;
}