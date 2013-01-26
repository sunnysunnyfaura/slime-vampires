#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include <vector>
#include <algorithm>
#include <iostream>

using namespace ci;
using namespace ci::app;
using namespace std;

const size_t N_BALLS = 1000;

template<typename T>
T clamp(T v, T low, T high) {
	if ( v <= low )
		return low;
	if ( v >= high )
		return high;
	return v;
}

struct Vector2 {
	float x, y;
	
	Vector2() {}
	Vector2(float x, float y) : x(x), y(y) {}
	
	Vector2 &operator+=(const Vector2 &rhs) {
		x += rhs.x;
		y += rhs.y;
		return *this;
	}
	
	Vector2 &operator-=(const Vector2 &rhs) {
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}
	
	Vector2 operator+(const Vector2 &rhs) const {
		return Vector2(x + rhs.x, y + rhs.y);
	}
	
	Vector2 operator-(const Vector2 &rhs) const {
		return Vector2(x - rhs.x, y - rhs.y);
	}
	float operator*(const Vector2 &rhs) const {
		return x * rhs.x + y * rhs.y;
	}
	
	Vector2 operator*(float s) const {
		return Vector2(s * x, s * y);
	}
	
	Vector2 operator-() const {
		return Vector2(-x, -y);
	}
	
	Vector2 perp() const {
		return Vector2(-y, x);
	}
	float sqMag() const { return x * x + y * y; }
	
	float mag() const { return sqrt(sqMag()); }
	
	void normalize() {
		float d = mag();
		if ( mag() < 0.00001 ) return;
		d = 1.0 / d;
		
		x *= d;
		y *= d;
	}
	Vec2f toV() const {
		return Vec2f(x, y);
	}
};

Vector2 operator*(float s, const Vector2 &rhs) {
	return Vector2(rhs.x * s, rhs.y * s);
}

Vector2 reflect(const Vector2 &incident, const Vector2 &normal) {
	Vector2 n = normal * ((incident * normal) * -2.0);
	return incident + n;
}

struct Circle {
	Vector2 center, vel;
	float radius;
	
	float mass() const { return radius * radius * 3.14159265358979323846264338327 + 1; }
};

class UniformGrid {
	const float world_width, world_height;
	const size_t cols, rows;
	const float cell_width, cell_height;
	
	vector< vector<Circle*> > grid;
public:

	UniformGrid(float world_width, float world_height, size_t cols, size_t rows) :
	world_width(world_width), world_height(world_height), cols(cols), rows(rows),
	cell_width(world_width/cols), cell_height(world_height/rows), grid(rows * cols) {
		
	}
	
	const size_t nRows() const { return rows; }
	const size_t nCols() const { return cols; }
	const size_t cellWidth() const { return cell_width; }
	const size_t cellHeight() const { return cell_height; }
	
	size_t getCellIndexForPosition(float x, float y) const {
		int col =  clamp((int)floor(x / cell_width), 0, (int)(cols - 1)),
		row = clamp((int)floor(y / cell_height), 0, (int)(rows - 1));
		return getCellIndex(col, row);
	}
	
	size_t getCellIndex(size_t c, size_t r ) const {
		auto index = cols * r + c;
		assert( index < cols * rows );
		return index;
	}
	
	vector<Circle*>& getCell(size_t c, size_t r) {
		return grid[getCellIndex(c, r)];
	}
	
	vector<Circle*>& getCellForPosition(float x, float y) {
		auto idx = getCellIndexForPosition(x, y);
		return grid[idx];
	}
	
	void put(Circle &c) {
		getCellForPosition(c.center.x, c.center.y).push_back(&c);
	}
	
	size_t cellIndexOf(const Circle &c) {
		for ( size_t i = 0; i < grid.size(); ++i ) {
			for ( size_t j = 0; j < grid[i].size(); ++j ) {
				if ( grid[i][j] == &c ) {
					return i;
				}
			}
		}
		return -1;
	}
	
	
	vector<Circle*> getObjectsInRect(float centerX, float centerY, float w, float h) {
		vector<Circle*> ret;
		w *= 0.5;
		h *= 0.5;
		const int
		x_min = clamp(int((centerX - w)/cell_width), 0, (int)cols-1),
		x_max = clamp(int((centerX + w)/cell_width), 0, (int)cols-1),
		
		y_min = clamp(int((centerY - h)/cell_height), 0, (int)rows-1),
		y_max = clamp(int((centerY + h)/cell_height), 0, (int)rows-1);
		
		for ( int c = x_min; c <= x_max; ++c ) {
			for ( int r = y_min; r <= y_max; ++r ) {
				const auto &cell = getCell(c, r);
				ret.insert(ret.end(), cell.begin(), cell.end());
			}
		}
		return ret;
	}
};

class bouncierApp : public AppBasic {
	Circle circles[N_BALLS];
	UniformGrid *ug;
  public:
	bouncierApp() : ug(0) {}
	
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	~bouncierApp() {
		delete ug;
	}
};

void bouncierApp::setup()
{
	auto screenWidth = this->getWindowWidth(), screenHeight = this->getWindowHeight();
	ug = new UniformGrid(screenWidth, screenHeight, 10, 10);
	for ( size_t i = 0; i < N_BALLS; ++i ) {
		circles[i].center.y = screenHeight * randFloat();
		circles[i].center.x = screenWidth * randFloat();
		circles[i].vel.y = randFloat() * 10 - 5;
		circles[i].vel.x = randFloat() * 10 - 5;
		circles[i].radius = randFloat() * 5 + 4;
	
		ug->put(circles[i]);
	}
}

void bouncierApp::mouseDown( MouseEvent event )
{
}

void bouncierApp::update()
{
	const float screenWidth = this->getWindowWidth(), screenHeight = this->getWindowHeight();
	for ( size_t i = 0; i < N_BALLS; ++i ) {
		Circle &c = circles[i];
		const auto prevCenter = c.center;
		c.center += c.vel;
		
		if ( c.center.y - c.radius <= 0 ) {
			c.vel.y *= -1;
			c.center.y = c.radius;
		} else if ( c.center.y + c.radius >= screenHeight ) {
			c.vel.y *= -1;
			c.center.y = screenHeight - c.radius;
		}
		
		if ( c.center.x - c.radius <= 0 ) {
			c.vel.x *= -1;
			c.center.x = c.radius;
		} else if ( c.center.x + c.radius >= screenWidth ) {
			c.vel.x *= -1;
			c.center.x = screenWidth - c.radius;
		}
		
		auto candidates = ug->getObjectsInRect(prevCenter.x, prevCenter.y, ug->cellWidth() * 2, ug->cellHeight() * 2);
		
		for ( size_t j = 0; j < candidates.size(); ++j ) {
			Circle &o = *candidates[j];
			if ( &c == &o ) {
				continue;
			}

			auto d = o.center - c.center;
			float dsq = d.sqMag();
			if ( dsq <= (c.radius + o.radius) * (c.radius + o.radius) ) {
				auto mag = sqrt(dsq);
				d.x = d.x / mag;
				d.y = d.y / mag;
				
				Vector2 oprime = (o.vel * d) * d,
				cprime = (c.vel * d) * d;
				
				auto invTotalMass = 1.0f/(o.mass() + c.mass());
				Vector2 newO = (o.vel - oprime) +
				(o.mass() - c.mass()) * invTotalMass * oprime +
				2 * c.mass() * invTotalMass * cprime,
				newC = (c.vel - cprime) +
				(c.mass() - o.mass()) * invTotalMass * cprime +
				2 * o.mass() * invTotalMass * oprime;
				o.vel = newO;
				c.vel = newC;
				
				auto w = (c.radius + o.radius - mag) * 0.5;
				
				const auto prevOCenter = o.center;
				o.center += d * w;
				c.center -= d * w;
				
				//update the bin of o
				auto &OoldCell = ug->getCellForPosition(prevOCenter.x, prevOCenter.y);
				OoldCell.erase(find(OoldCell.begin(), OoldCell.end(), &o));
				ug->put(o);
			}
		}
		//update the bin of c
		auto &oldCell = ug->getCellForPosition(prevCenter.x, prevCenter.y);
		oldCell.erase(find(oldCell.begin(), oldCell.end(), &c));
		ug->put(c);

	}
}

void bouncierApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );
	gl::color(1, 1, 1);
	for ( size_t i = 0; i < N_BALLS; ++i ) {
		gl::drawSolidCircle(circles[i].center.toV(), circles[i].radius);
	}
	
}


CINDER_APP_BASIC( bouncierApp, RendererGl )
