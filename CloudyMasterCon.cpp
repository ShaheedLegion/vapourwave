#include <Windows.h>
#include "Renderer.hpp"
#include "FastNoise.h"

#define randf() (static_cast<float>(rand()))
/*
namespace DXSS {
	const int particle_layers = 6;
	const int particle_x_dim = 10;
	const int particle_z_dim = 10;
	const int particles_total = 100;
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
*/

namespace util {
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
	bool depthTestWrite(float * db, float z, int tx, int ty, int sw) {
		if (z > db[(ty * sw) + tx]) return false;
		db[(ty * sw) + tx] = z;
		return true;
	}
}; // namespace util

namespace DXDD {
	const float PI = 3.142857142857143f;
	struct pointf {
		float x, y, z;
		int tx, ty;
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
		util::pixel p;
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

	void constructSky(image * sky, FastNoise& n, int sw, int sh) {
		sky->init(sw, sh);
		const int num_sky_stops = 6;
		util::range sky_stops[num_sky_stops] = {
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
		util::pixel pix;
		for (int y = 0; y < sh; y++) {
			for (int t = 0; t < num_sky_stops; t++) {
				if ((y > sky_stops[t].ys) && (y < sky_stops[t].ye)) {
					sky_col = sky_stops[t].getCol(y);
					break;
				}
			}
			for (int x = 0; x < sw; x++) {
				// Need to get noise param at this point.
				float scale = 0.5f;// n.GetNoise((float)x, (float)y) * 64.0f;
				pix.col = sky_col;
				short bb = pix.b + (64.0f * scale);
				short rr = pix.r - (32.0f * scale);
				pix.b = (bb > 255 ? bb >> 1 : bb);
				pix.r = (rr < 0 ? pix.r : rr);
				sky->pixels[(y * sw) + x] = pix.col;
			}
		}
		blur(sky);
	}

	void constructSun(image * sun, int sw, int sh) {
		sun->init(sw, sh);
		//DXSS::range sun_range = { {0x00FFF4CD}, {0x00FF6D13}, 32, 96 };
		util::range sun_range = { {0x00FFFFFF}, {0x00FF0004}, 48, 80 };

		// The sun will be 1/2 of the size of the given width / h.
		int sun_radius = sw >> 3;

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
			util::pixel p;
			util::pixel q;

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

namespace sim {
	struct line {
		DXDD::pointf p;
		util::pixel c;
		bool h;
	};

	const int num_lines_h = 10;
	const int num_lines_v = 10;

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

		void transform(DXDD::pointf & p, int sw, int sh, int hsw, int hsh) {
			float ix = p.x + x;
			float iy = p.y + y;
			float iz = p.z + z;

			// Perform the perspective-correct transform here.
			p.tx = ((ix / iz) * sw) + hsw;
			p.ty = ((iy / iz) * sh) + hsh;
		}
	};

	struct simulation {
		line lines_h[num_lines_h];
		line lines_v[num_lines_v];
		camera cam;

		simulation() {
			int hspacer = 80; // world units
			int zspacer = 57; // world units
			float floor = 105.0f;
			float start_z = (num_lines_h * zspacer) + (num_lines_h * 4);
			for (int z = 0; z < num_lines_h; z++) {
				lines_h[z].p.x = -(num_lines_v * hspacer);
				lines_h[z].p.y = floor;
				lines_h[z].p.z = start_z;
				lines_h[z].c.col = 0x00FF8AE6;
				start_z -= zspacer;
			}

			float start_x = -((num_lines_v >> 1) * hspacer);
			start_z = (num_lines_h * zspacer) + (num_lines_h * 4);
			for (int x = 0; x < num_lines_v; x++) {
				lines_v[x].p.x = start_x;
				lines_v[x].p.y = floor;
				lines_v[x].p.z = start_z;
				lines_v[x].c.col = 0x00FF8AE6;
				start_x += hspacer;
			}

			cam.x = 0;
			cam.y = 5.0f;
			cam.z = 80.0f;
			cam.cx = 1.0f;
			cam.cy = 1.0f;
			cam.cz = 1.0f;
		}

		void update(int sw, int sh) {
			int hsw = sw >> 1;
			int hsh = sh >> 1;
			for (int h = 0; h < num_lines_h; h++) {
				cam.transform(lines_h[h].p, sw, sh, hsw, hsh);
			}
			for (int w = 0; w < num_lines_v; w++) {
				cam.transform(lines_v[w].p, sw, sh, hsw, hsh);
			}
		}

		void draw(detail::Uint32 * b, float * db, int sw, int sh) {
			// Draw the horizontal lines first
			for (int h = 0; h < num_lines_h; h++) {
				// This is going to be a fun operation (rofl)
				if (lines_h[h].p.ty > 0 && lines_h[h].p.ty < sh) {
					if (util::depthTestWrite(db, lines_h[h].p.z, lines_h[h].p.tx, lines_h[h].p.ty, sw)) {
						for (int x = 0; x < sw; x++) {
							b[(lines_h[h].p.ty * sw) + x] = lines_h[h].c.col;
						}
					}
				}
			}
			for (int w = 0; w < num_lines_v; w++) {
				if (lines_v[w].p.tx > 0 && lines_v[w].p.tx < sw) {
					if (util::depthTestWrite(db, lines_v[w].p.z, lines_v[w].p.tx, lines_v[w].p.ty, sw)) {
						for (int y = sh >> 1; y < sh; y++) {
							b[(y * sw) + lines_v[w].p.tx] = lines_v[w].c.col;
						}
					}
				}
			}
		}
	};
}; // namespace sim

namespace sim_m9 {
	unsigned int SampleCol(unsigned int * px, int w, int h, float x, float y) {
		int sx = (static_cast<int>(x * static_cast<float>(w))) % w;
		int sy = (static_cast<int>(y * static_cast<float>(h))) % h;
		if (sx < 0 || sx >= w || sy < 0 || sy >= h) return 0x00ff00;

		return px[sy * w + sx];
	}
	void LoadTexture(unsigned int * t, const char * fn, int sz) {
		/*
		FILE * fp = fopen(fn, "rb");
		if (fp) {
			fread(t, 4, sz, fp);
			fclose(fp);
		}
		else*/
		{
			int i = sz - 1;
			util::pixel * px = reinterpret_cast<util::pixel*>(t);
			while (i > 0) {
				float scale = ((float)rand() / (float)RAND_MAX); // between 0 and 1

				px[i].r = 64 + (unsigned char)((float)192.0f * scale);
				px[i].g = 96 + (unsigned char)((float)159.0f * scale);
				px[i].b = 128 + (unsigned char)((float)128.0f * scale);
				px[i].a = 64 + (unsigned char)((float)192.0f * scale);
				--i;
			}
		}


		util::pixel * px = reinterpret_cast<util::pixel*>(t);
		int i = sz - 1;
		while (i > 0) {
			util::pixel& p = px[i];
			p.a = 0;
			unsigned char r = p.r;
			p.r = p.b;
			p.b = r;
			--i;
		}
	}

	struct Simulation {
		float fWorldX;
		float fWorldY;
		float fWorldA;
		float fNear;
		float fFar;
		float fFoVHalf;

		unsigned int *ground;
		//ui *sky;
		float * fSeed;
		int nMapSize = 1024;

		Simulation() {
			fWorldX = 1000.0f;
			fWorldY = 1000.0f;
			fWorldA = 0.1f;
			fNear = -0.002f;
			fFar = 0.17f;
			fFoVHalf = 3.14159f / 4.0f;

			const int sz = nMapSize * nMapSize;

			ground = new unsigned int[sz];
			LoadTexture(ground, "test1024.data", sz);

			fSeed = new float[sz];
			int i = 0;
			while (i < sz) { fSeed[i] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); ++i; }
		}
		void Set(float fn, float ff) {
			fNear = fn;
			fFar = ff;
		}
		~Simulation() { delete[] ground; /*delete[] sky;*/ delete[] fSeed; }
/*
		float PNoise2D(int x, int y, int nOctaves, float fBias) {
			float fNoise = 0.0f;
			float fScaleAcc = 0.0f;
			float fScale = 1.0f;

			for (int o = 0; o < nOctaves; o++) {
				int nPitch = nMapSize >> o;
				int nSampleX1 = (x / nPitch) * nPitch;
				int nSampleY1 = (y / nPitch) * nPitch;

				int nSampleX2 = (nSampleX1 + nPitch) % nMapSize;
				int nSampleY2 = (nSampleY1 + nPitch) % nMapSize;

				float fBlendX = (float)(x - nSampleX1) / (float)nPitch;
				float fBlendY = (float)(y - nSampleY1) / (float)nPitch;

				float fSampleT = (1.0f - fBlendX) * fSeed[nSampleY1 * nMapSize + nSampleX1] + fBlendX * fSeed[nSampleY1 * nMapSize + nSampleX2];
				float fSampleB = (1.0f - fBlendX) * fSeed[nSampleY2 * nMapSize + nSampleX1] + fBlendX * fSeed[nSampleY2 * nMapSize + nSampleX2];

				fScaleAcc += fScale;
				fNoise += (fBlendY * (fSampleB - fSampleT) + fSampleT) * fScale;
				fScale = fScale / fBias;
			}
			// Scale to seed range
			return fNoise / fScaleAcc;
		}
*/
		void Update(detail::Uint32 * b, int w, int h) {
			float hw = static_cast<float>(w >> 1);
			float hh = static_cast<float>(h >> 1);
			int hwi = w >> 1;
			int hhi = h >> 1;

			//fWorldA += cc.d;

			fWorldX += cosf(fWorldA) * 0.001f;
			fWorldY += sinf(fWorldA) * 0.001f;

			// Create Frustum corner points
			float fFarX1 = fWorldX + cosf(fWorldA - fFoVHalf) * fFar;
			float fFarY1 = fWorldY + sinf(fWorldA - fFoVHalf) * fFar;

			float fNearX1 = fWorldX + cosf(fWorldA - fFoVHalf) * fNear;
			float fNearY1 = fWorldY + sinf(fWorldA - fFoVHalf) * fNear;

			float fFarX2 = fWorldX + cosf(fWorldA + fFoVHalf) * fFar;
			float fFarY2 = fWorldY + sinf(fWorldA + fFoVHalf) * fFar;

			float fNearX2 = fWorldX + cosf(fWorldA + fFoVHalf) * fNear;
			float fNearY2 = fWorldY + sinf(fWorldA + fFoVHalf) * fNear;
			// unsigned int color = 0xffffffff;
			// util::pixel * pixel = reinterpret_cast<util::pixel*>(&color);

			// Starting with furthest away line and work towards the camera point
			for (int y = 0; y < hhi; y++) {
				// Take a sample point for depth linearly related to rows down screen
				float fSampleDepth = (static_cast<float>(y) / h) + 0.1f;

				// Use sample point in non-linear (1/x) way to enable perspective
				// and grab start and end points for lines across the screen
				float fStartX = ((fFarX1 - fNearX1) / fSampleDepth) + fNearX1;
				float fStartY = ((fFarY1 - fNearY1) / fSampleDepth) + fNearY1;
				float fEndX = ((fFarX2 - fNearX2) / fSampleDepth) + fNearX2;
				float fEndY = ((fFarY2 - fNearY2) / fSampleDepth) + fNearY2;

				// Linearly interpolate lines across the screen
				for (int x = 0; x < w; x++) {
					float fSampleWidth = static_cast<float>(x) / static_cast<float>(w);
					float fSampleX = (fEndX - fStartX) * fSampleWidth + fStartX;
					float fSampleY = (fEndY - fStartY) * fSampleWidth + fStartY;

					// Wrap sample coordinates to give "infinite" periodicity on maps
					fSampleX = fmod(fSampleX, 1.0f);
					fSampleY = fmod(fSampleY, 1.0f);
					/*
					// Sample symbol and colour from map sprite, and draw the
					// pixel to the screen
					float scale = PNoise2D(static_cast<int>(fSampleX * nMapSize), static_cast<int>(fSampleY * nMapSize), 4, 2.1f);

					// Sample symbol and colour from sky sprite, we can use same
					// coordinates, but we need to draw the "inverted" y-location
					//col = SampleCol(sky, nMapSize, nMapSize, fSampleX, fSampleY);

					pixel->r = 0xf0;
					pixel->g = 0x00;
					pixel->b = 0xff - static_cast<unsigned char>(scale);
					pixel->a = 0;

					surf.PlotPixel(x, hhi - y, color);
					*/
					/*
					pixel->r = 0xff - t;
					pixel->g = 0xff - t;
					pixel->b = 0xff - t;
					pixel->a = 0;
					*/
					unsigned int col = SampleCol(ground, nMapSize, nMapSize, fSampleX, fSampleY);
					/*
					pix * f = reinterpret_cast<pix*>(&col);
					f->r -= u;
					f->g -= u;
					f->b -= u;
					*/
					b[((hhi + y) * w) + x] = col;
					//surf.PlotPixel(x, hhi + y, (color & col) & 0x326464);
				}
			}
		}
	};
}; // namespace sim_m9
// So, we'll need a smaller buffer onto which we can render the background
// and the sun, which we can then scale up . .

// This is the guts of the renderer, without this it will do nothing.
DWORD WINAPI Update(LPVOID lpParameter) {
	FastNoise noise; // Create a FastNoise object
	noise.SetNoiseType(FastNoise::SimplexFractal);

	Renderer *g_renderer = static_cast<Renderer *>(lpParameter);
	//DXSS::particlefield field;
	sim::simulation s;
	sim_m9::Simulation ss;
	DXDD::image sky, sun, bg;
	bg.init(_WIDTH, _HEIGHT);
	DXDD::constructSky(&sky, noise, _WIDTH >> 1, _HEIGHT >> 1);
	DXDD::constructSun(&sun, _WIDTH >> 1, _HEIGHT >> 1);
	DXDD::constructBG(&bg, &sky, &sun, _WIDTH, _HEIGHT);

	while (g_renderer->IsRunning()) {
		//field.update(g_renderer->GetDirection(), noise);
		s.update(_WIDTH, _HEIGHT);

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

		s.draw(buffer, depthBuffer, _WIDTH, _HEIGHT);
		ss.Update(buffer, _WIDTH, _HEIGHT);

		/*
		so one of the biggest issues with this is that we need to do a triple-nested loop, which runs in O(n^3) time,
		which is obviously not ideal.
		We need to select the color as part of the update step, so that the colors are part of the transformation calculation.
		*/
		detail::Uint32 col = 0xff0000ff;
		/*
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
		*/
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