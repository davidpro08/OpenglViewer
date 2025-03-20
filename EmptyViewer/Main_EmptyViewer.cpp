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

// 기본 벡터 클래스
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

	// x,y,z 방향의 단위벡터
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
	// 부딪히면 true, 아니면 false를 반환
	virtual bool intersect(const Ray& ray, float& t) const = 0;
};

class Plane : public Surface {
public:
	// 평면은 y좌표만 필요함
	float y;

	// 근데 나중 가서 x,z좌표가 다른 평면이 나오면 이 코드 못쓰는데
	//확장성을 위해서 x,z좌표 일단 놔두기
	//float x, z;
	//Plane(float xPos, float yPos, float zPos) : x(xPos), y(yPos), z(zPos) {}

	Plane(float yPos) : y(yPos) {}


	bool intersect(const Ray& ray, float& t) const override {
		// 만약 ray가 y방향으로 안 가면, 평면가 마주칠 일이 없음.
		// 그러면 부딪히지 않음
		if (ray.direction.y == 0) return false;
		
		// 평면 y좌표 - Ray의 y좌표의 의미는
		// 평면의 y좌표가 Ray의 y좌표보다 위에 있냐 아래 있냐의 의미
		// 만약 평면 y좌표가 위에 있으면, ray의 방향은 위쪽을 향해야 함
		// 반대로 평면 y좌표가 아래에 있으면, ray의 방향은 아래를 향해야 함
		// 이 경우가 아니면 부딪히지 않으므로, y로 나눠주기
		t = (y - ray.point.y) / ray.direction.y;
		return t > 0;
	}
};

class Sphere : public Surface {
public:
	// 중심
	Vec3 center;
	// 반지름
	float radius;

	Sphere(const Vec3& c, float r) : center(c), radius(r) {}

	bool intersect(const Ray& ray, float& t) const override {
		// Sphere Intersection을 보면 판별식으로 접하는가를 판별함

		// 일단 ray의 시작점에서 원의 중심을 빼줌
		Vec3 oc = ray.point - center;

		// 이후 판별식의 a, b, c 만들어줌
		float a = ray.direction.dot(ray.direction);
		float b = 2.0f * oc.dot(ray.direction);
		float c = oc.dot(oc) - radius * radius;
		float discriminant = b * b - 4 * a * c;

		// 판별식을 이용해서 통과하는 지 확인. 통과하는 0보다 큼
		t = (-b - std::sqrt(discriminant)) / (2.0f * a);
		return t > 0;
	}
};

// 장면(Scene) 클래스
class Scene {
public:
	// Surface들의 저장 공간
	std::vector<Surface*> surfaces;

	// Surface 넣기
	void addObject(Surface* obj) { surfaces.push_back(obj); }

	// Ray를 받고 물체가 있는지를 알려줌
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

		// 아무것도 없으면 null
		// 뭔가 있으면 있다고 해줌
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

	// 원래라면 image plane의 여러가지 요소를 지정해야 함
	// 근데 이미 기본형으로 지정해놔서 괜찮음

	// 근데 일단 해봄
	// left = -0.1, right = 0.1, bottom = -0.1, top = 0.1, direction = 0.1
	// image resolution은 512 * 512이므로 그렇게 지정
	Camera camera = Camera(-0.1f, 0.1f, -0.1f, 0.1f, 0.1f, Width, Height);
	Scene scene;
	Vec3 whiteColor = Vec3(1.0f, 1.0f, 1.0f); // 물체가 있으면 하얀색
	Vec3 blackColor = Vec3(0.0f, 0.0f, 0.0f); // 물체가 있으면 검은색

	scene.addObject(new Sphere(Vec3(-4, 0, -7), 1));
	scene.addObject(new Sphere(Vec3(0, 0, -7), 2));
	scene.addObject(new Sphere(Vec3(4, 0, -7), 1));
	scene.addObject(new Plane(-2));

	// 일단 이미지 초기화
	OutputImage.clear();

	for (int y = 0; y < Height; ++y) 
	{
		for (int x = 0; x < Width; ++x) 
		{
			// 현재 위치에서 ray 쏘기
			Ray ray = camera.getRay(x, y);

			// 뭔가 부딪히는 물체가 있나요~
			// 이거 나중에 tMin을 0으로 설정하면 오류 생기기 때문ㅇ
			// Shading 들어갈 때 문제 생기면 여기 고치기
			bool hit = scene.trace(ray, 0, std::numeric_limits<float>::infinity());

			// 만약 부딪히면 RGB 조정
			if (hit) {
				OutputImage.push_back(whiteColor.x); // R (OpenGL의 GL_FLOAT는 0.0 ~ 1.0)
				OutputImage.push_back(whiteColor.y); // G
				OutputImage.push_back(whiteColor.z); // B
			}
			else {
				OutputImage.push_back(blackColor.x);   // R (OpenGL의 GL_FLOAT는 0.0 ~ 1.0)
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
