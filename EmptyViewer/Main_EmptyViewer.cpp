#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>

#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <vector>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;

// -------------------------------------------------
// Global Variables
// -------------------------------------------------
int Width = 512;
int Height = 512;
std::vector<float> OutputImage;
// -------------------------------------------------

// �⺻ ���� Ŭ����
struct Vec3 {
public:
	float x, y, z;
	Vec3(float x_ = 0, float y_ = 0, float z_ = 0) : x(x_), y(y_), z(z_) {}

	Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
	Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
	Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }

	float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
	Vec3 normalize() const { float len = std::sqrt(x * x + y * y + z * z); return Vec3(x / len, y / len, z / len); }
};


class Ray {
public:
	Vec3 point, direction;
	Ray(const Vec3& p, const Vec3& d) : point(p), direction(d.normalize()) {}
};

class Camera {
public:
	Vec3 eye;

	// x,y,z ������ ��������
	Vec3 u = Vec3(1, 0, 0);
	Vec3 v = Vec3(0, 1, 0);
	Vec3 w = Vec3(0, 0, 1);

	//left, right, bottom, top, direction
	float l, r, b, t, d;

	// image plain resolution
	int nx, ny;

	Camera() : eye(0, 0, 0), u(1, 0, 0), v(0, 1, 0), w(0, 0, 1),
		l(-0.1f), r(0.1f), b(-0.1f), t(0.1f), d(0.1f), nx(512), ny(512) {}

	Camera(float left, float right, float bottom, float top, float direction, int width, int height) :l(left), r(right), b(bottom), t(top), d(direction), nx(width), ny(height) {}

	Ray getRay(int i, int j) {
		float u_ = l + (r - l) * (i + 0.5f) / nx;
		float v_ = b + (t - b) * (j + 0.5f) / ny;
		Vec3 direction = (eye + u * u_ + v * v_ - w * d).normalize();;
		return Ray(eye, direction);
	}
};

class Surface {
public:
	// �ε����� true, �ƴϸ� false�� ��ȯ
	virtual bool intersect(const Ray& ray, float& t) const = 0;
};

class Plane : public Surface {
public:
	// ����� y��ǥ�� �ʿ���
	float y;

	// �ٵ� ���� ���� x,z��ǥ�� �ٸ� ����� ������ �� �ڵ� �����µ�
	//Ȯ�强�� ���ؼ� x,z��ǥ �ϴ� ���α�
	//float x, z;
	//Plane(float xPos, float yPos, float zPos) : x(xPos), y(yPos), z(zPos) {}

	Plane(float yPos) : y(yPos) {}


	bool intersect(const Ray& ray, float& t) const override {
		// ���� ray�� y�������� �� ����, ��鰡 ����ĥ ���� ����.
		// �׷��� �ε����� ����
		if (ray.direction.y == 0) return false;
		
		// ��� y��ǥ - Ray�� y��ǥ�� �ǹ̴�
		// ����� y��ǥ�� Ray�� y��ǥ���� ���� �ֳ� �Ʒ� �ֳ��� �ǹ�
		// ���� ��� y��ǥ�� ���� ������, ray�� ������ ������ ���ؾ� ��
		// �ݴ�� ��� y��ǥ�� �Ʒ��� ������, ray�� ������ �Ʒ��� ���ؾ� ��
		// �� ��찡 �ƴϸ� �ε����� �����Ƿ�, y�� �����ֱ�
		t = (y - ray.point.y) / ray.direction.y;
		return t > 0;
	}
};

class Sphere : public Surface {
public:
	// �߽�
	Vec3 center;
	// ������
	float radius;

	Sphere(const Vec3& c, float r) : center(c), radius(r) {}

	bool intersect(const Ray& ray, float& t) const override {
		// Sphere Intersection�� ���� �Ǻ������� ���ϴ°��� �Ǻ���

		// �ϴ� ray�� ���������� ���� �߽��� ����
		Vec3 oc = ray.point - center;

		// ���� �Ǻ����� a, b, c �������
		float a = ray.direction.dot(ray.direction);
		float b = 2.0f * oc.dot(ray.direction);
		float c = oc.dot(oc) - radius * radius;
		float discriminant = b * b - 4 * a * c;

		// �Ǻ����� �̿��ؼ� ����ϴ� �� Ȯ��. ����ϴ� 0���� ŭ
		t = (-b - std::sqrt(discriminant)) / (2.0f * a);
		return t > 0;
	}
};

// ���(Scene) Ŭ����
class Scene {
public:
	// Surface���� ���� ����
	std::vector<Surface*> surfaces;

	// Surface �ֱ�
	void addObject(Surface* obj) { surfaces.push_back(obj); }

	// Ray�� �ް� ��ü�� �ִ����� �˷���
	bool trace(const Ray& ray, float tMin, float tMax) {
		Surface* closestSurface = nullptr;
		float closestT = tMax;

		for (Surface* obj : surfaces) {
			float t;

			if (obj->intersect(ray, t) && t < closestT) {
				closestT = t;
				closestSurface = obj;
			}
		}

		// �ƹ��͵� ������ null
		// ���� ������ �ִٰ� ����
		return closestSurface != nullptr;
	}
};

void render()
{
	//Create our image. We don't want to do this in 
	//the main loop since this may be too slow and we 
	//want a responsive display of our beautiful image.
	//Instead we draw to another buffer and copy this to the 
	//framebuffer using glDrawPixels(...) every refresh

	// ������� image plane�� �������� ��Ҹ� �����ؾ� ��
	// �ٵ� �̹� �⺻������ �����س��� ������

	// �ٵ� �ϴ� �غ�
	// left = -0.1, right = 0.1, bottom = -0.1, top = 0.1, direction = 0.1
	// image resolution�� 512 * 512�̹Ƿ� �׷��� ����
	Camera camera = Camera(-0.1f, 0.1f, -0.1f, 0.1f, 0.1f, Width, Height);
	Scene scene;
	Vec3 whiteColor = Vec3(1.0f, 1.0f, 1.0f); // ��ü�� ������ �Ͼ��
	Vec3 blackColor = Vec3(0.0f, 0.0f, 0.0f); // ��ü�� ������ ������

	scene.addObject(new Sphere(Vec3(-4, 0, -7), 1));
	scene.addObject(new Sphere(Vec3(0, 0, -7), 2));
	scene.addObject(new Sphere(Vec3(4, 0, -7), 1));
	scene.addObject(new Plane(-2));

	// �ϴ� �̹��� �ʱ�ȭ
	OutputImage.clear();

	for (int y = 0; y < Height; ++y) 
	{
		for (int x = 0; x < Width; ++x) 
		{
			// ���� ��ġ���� ray ���
			Ray ray = camera.getRay(x, y);

			// ���� �ε����� ��ü�� �ֳ���~
			// �̰� ���߿� tMin�� 0���� �����ϸ� ���� ����� ������
			// Shading �� �� ���� ����� ���� ��ġ��
			bool hit = scene.trace(ray, 0, std::numeric_limits<float>::infinity());

			// ���� �ε����� RGB ����
			if (hit) {
				OutputImage.push_back(whiteColor.x); // R (OpenGL�� GL_FLOAT�� 0.0 ~ 1.0)
				OutputImage.push_back(whiteColor.y); // G
				OutputImage.push_back(whiteColor.z); // B
			}
			else {
				OutputImage.push_back(blackColor.x);   // R (OpenGL�� GL_FLOAT�� 0.0 ~ 1.0)
				OutputImage.push_back(blackColor.y); // G
				OutputImage.push_back(blackColor.z); // B
			}

			// ---------------------------------------------------
			// --- Implement your code here to generate the image
			// ---------------------------------------------------

			// draw a red rectangle in the center of the image
			//vec3 color = glm::vec3(0.5f, 0.5f, 0.5f); // grey color [0,1] in RGB channel
			//
			//if (i > Width / 4 && i < 3 * Width / 4 
			//	&& j > Height / 4 && j < 3 * Height / 4)
			//{
			//	color = glm::vec3(1.0f, 0.0f, 0.0f); // red color [0,1] in RGB channel
			//}
			
			// set the color
			//OutputImage.push_back(color.x); // R
			//OutputImage.push_back(color.y); // G
			//OutputImage.push_back(color.z); // B
		}
	}
}


void resize_callback(GLFWwindow*, int nw, int nh) 
{
	//This is called in response to the window resizing.
	//The new width and height are passed in so we make 
	//any necessary changes:
	Width = nw;
	Height = nh;
	//Tell the viewport to use all of our screen estate
	glViewport(0, 0, nw, nh);

	//This is not necessary, we're just working in 2d so
	//why not let our spaces reflect it?
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0.0, static_cast<double>(Width)
		, 0.0, static_cast<double>(Height)
		, 1.0, -1.0);

	//Reserve memory for our render so that we don't do 
	//excessive allocations and render the image
	OutputImage.reserve(Width * Height * 3);
	render();
}


int main(int argc, char* argv[])
{
	// -------------------------------------------------
	// Initialize Window
	// -------------------------------------------------

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(Width, Height, "OpenGL Viewer", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	//We have an opengl context now. Everything from here on out 
	//is just managing our window or opengl directly.

	//Tell the opengl state machine we don't want it to make 
	//any assumptions about how pixels are aligned in memory 
	//during transfers between host and device (like glDrawPixels(...) )
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	//We call our resize function once to set everything up initially
	//after registering it as a callback with glfw
	glfwSetFramebufferSizeCallback(window, resize_callback);
	resize_callback(NULL, Width, Height);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		//Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// -------------------------------------------------------------
		//Rendering begins!
		glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);
		//and ends.
		// -------------------------------------------------------------

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		//Close when the user hits 'q' or escape
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS
			|| glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
