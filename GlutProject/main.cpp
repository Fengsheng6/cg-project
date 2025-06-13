#include <GL/freeglut.h>
#include <math.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
using namespace std;

#define GLUT_WHEEL_UP 3
#define GLUT_WHEEL_DOWN 4
#define root_2 1.414
#define root_3 1.732

GLint winWidth = 1080, winHeight = 720; // Initial display window size
GLfloat camera_x = -500, camera_y = 170.0, camera_z = 500; // viewing co-ordinate origin
GLfloat lookat_x = -567.667, lookat_y = 160.0, lookat_z = 2203.86; // look-at point
GLfloat Vx = 0.0, Vy = 1.0, Vz = 0.0; // view-up vector

GLfloat fov = 60.0f, aspect = (float)winWidth / (float)winHeight;
GLfloat dnear = 10.0f, dfar = 15000.0f;

// 添加鼠标控制相关变量
GLfloat yaw = -90.0f;   // 偏航角（左右旋转）
GLfloat pitch = 0.0f;  // 俯仰角（上下旋转）
GLfloat lastX = winWidth / 2.0f;  // 上次鼠标X位置
GLfloat lastY = winHeight / 2.0f; // 上次鼠标Y位置
bool firstMouse = true;  // 首次鼠标移动标记
bool mouseCaptured = false; // 鼠标是否被捕获标记

void mouseButton(int button, int state, int x, int y);
void idle(void);

// initiate states and speeds
float n = 485;	// view rotate degree
GLfloat step = 10;	// move speed

// arrays to save different textures
GLuint textures[10000];

//world
GLfloat world_radius = 5000; // Radius of sphere
GLfloat tree_degree = 0;	// wave degree of tree
GLint time_state = 0;

// 新增光源参数结构体
struct LightParams {
    GLfloat ambient[4];  // 环境光
    GLfloat diffuse[4];  // 漫反射光
    GLfloat specular[4]; // 镜面光
    GLfloat position[4]; // 光源位置
    GLfloat intensity;   // 光照强度
};

// 三种时间状态的光源参数
LightParams dayLight = {
    {0.3f, 0.3f, 0.3f, 1.0f},
    {1.0f, 1.0f, 0.9f, 1.0f},
    {1.0f, 1.0f, 0.9f, 1.0f},
    {100.0f, 200.0f, 100.0f, 1.0f},
    1.0f
};

LightParams duskLight = {
    {0.4f, 0.2f, 0.1f, 1.0f},
    {0.8f, 0.4f, 0.2f, 1.0f},
    {0.6f, 0.3f, 0.1f, 1.0f},
    {200.0f, 100.0f, 150.0f, 1.0f},
    0.7f
};

LightParams nightLight = {
    {0.1f, 0.1f, 0.2f, 1.0f},
    {0.2f, 0.2f, 0.5f, 1.0f},
    {0.1f, 0.1f, 0.3f, 1.0f},
    {-100.0f, 150.0f, 50.0f, 1.0f},
    0.3f
};

struct point
{
	GLfloat x, y, z;
};
point solar = { -27.4, 550.6 ,-367.4 }; // Solar's coordinates

// 定义顶点结构体
struct Vertex {
    float x, y, z;
};

// 定义纹理坐标结构体
struct TexCoord {
    float u, v;
};

// 定义法线结构体
struct Normal {
    float x, y, z;
};

// 定义面结构体
struct Face {
    int v[3], vt[3], vn[3];
};

// 添加材质数据结构
struct Material {
    std::string name;               // 材质名称
    GLfloat Ka[4] = {0.2f, 0.2f, 0.2f, 1.0f};  // 环境光
    GLfloat Kd[4] = {0.8f, 0.8f, 0.8f, 1.0f};  // 漫反射
    GLfloat Ks[4] = {0.5f, 0.5f, 0.5f, 1.0f}; // 镜面光
    GLfloat Ke[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // 新增：发射光
    GLfloat Ns = 32.0f;      // 高光指数
    GLfloat d = 1.0f;           // 透明度
};

struct Model {
    std::vector<Vertex> vertices;
    std::vector<TexCoord> texCoords;
    std::vector<Normal> normals;
    std::vector<Face> faces;
    std::map<std::string, Material> materials;
    std::string currentMaterial;
};

Model bangbuModel; // 全局模型对象

// 全局变量存储模型数据


point shaft = { 1, 0, 0 }; // Autorotation shaft
GLfloat planet_rotation = 0.0;
GLboolean rotateFlag = FALSE;
GLint Rotatestep = 0;

struct Image
{
	GLint width, height;
	GLubyte* data;

	Image() = default;
	~Image() {
		free(data);
	}

	void LoadFromFile(const string& filename)
	{
		GLint image_width;
		GLint image_height;
		GLint pixel_len;
		GLubyte* image_data;
		// Read in and open an image file
		FILE* pfile = NULL;
		fopen_s(&pfile, filename.c_str(), "rb"); // read the image in binary mode
		if (pfile == 0) {
			throw runtime_error("failed to load file from: " + filename + ".");
		}
		fseek(pfile, 0x0012, SEEK_SET);
		fread(&image_width, sizeof(image_width), 1, pfile);
		fread(&image_height, sizeof(image_height), 1, pfile);
		pixel_len = image_width * 3;
		while (pixel_len % 4 != 0)
			pixel_len++;
		pixel_len *= image_height;		// total size to save the image

		image_data = (GLubyte*)malloc(pixel_len);	// memory allocation
		if (image_data == 0) {
			throw runtime_error("failed to allocate memory.");
		}
		fseek(pfile, 54, SEEK_SET);
		fread(image_data, pixel_len, 1, pfile);
		fclose(pfile);
		for (int i = 0; i < pixel_len / 3; i++) {		// the original order of color information from the file is BGR, change the order to RGB
			GLubyte x = image_data[i * 3];
			GLubyte z = image_data[i * 3 + 2];
			image_data[i * 3 + 2] = x;
			image_data[i * 3] = z;
		}
		width = image_width;
		height = image_height;
		data = image_data;
	}

	Image(const string& filename) {
		LoadFromFile(filename);
	}

	void BindToTexture(GLuint* texture_id)
	{
		glGenTextures(1, texture_id);

		glBindTexture(GL_TEXTURE_2D, *texture_id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE, 0);
	}
};

// 解析MTL材质文件
bool loadMTL(const std::string& mtlPath, std::map<std::string, Material>& materials) {
    std::ifstream file(mtlPath);
    if (!file.is_open()) {
        std::cerr << "无法打开MTL文件: " << mtlPath << std::endl;
        return false;
    }

    std::string line;
    Material currentMat;

    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "newmtl") {
            if (!currentMat.name.empty()) {
                materials[currentMat.name] = currentMat;
            }
            iss >> currentMat.name;
        } else if (type == "Ka") {
            iss >> currentMat.Ka[0] >> currentMat.Ka[1] >> currentMat.Ka[2];
        } else if (type == "Kd") {
            iss >> currentMat.Kd[0] >> currentMat.Kd[1] >> currentMat.Kd[2];
        } else if (type == "Ks") {
        iss >> currentMat.Ks[0] >> currentMat.Ks[1] >> currentMat.Ks[2];
    } else if (type == "Ke") {  // 添加发射光参数解析
        iss >> currentMat.Ke[0] >> currentMat.Ke[1] >> currentMat.Ke[2];
    } else if (type == "Ns") {
            iss >> currentMat.Ns;
        } else if (type == "d") {
            iss >> currentMat.d;
        }
    }

    // 添加最后一个材质
    if (!currentMat.name.empty()) {
        materials[currentMat.name] = currentMat;
        // 打印材质信息
        std::cout << "成功加载材质: " << currentMat.name << std::endl;
        std::cout << "  环境光(Ka): " << currentMat.Ka[0] << "," << currentMat.Ka[1] << "," << currentMat.Ka[2] << std::endl;
        std::cout << "  漫反射(Kd): " << currentMat.Kd[0] << "," << currentMat.Kd[1] << "," << currentMat.Kd[2] << std::endl;
        std::cout << "  镜面光(Ks): " << currentMat.Ks[0] << "," << currentMat.Ks[1] << "," << currentMat.Ks[2] << std::endl;
        std::cout << "  发射光(Ke): " << currentMat.Ke[0] << "," << currentMat.Ke[1] << "," << currentMat.Ke[2] << std::endl;
        std::cout << "  高光指数(Ns): " << currentMat.Ns << std::endl;
        std::cout << "  透明度(d): " << currentMat.d << std::endl;
    }

    return true;
}

// OBJ模型加载函数
bool loadOBJ(const std::string& path, Model& model) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "无法打开OBJ文件: " << path << std::endl;
        return false;
    }

    std::string line;
    std::string baseDir = path.substr(0, path.find_last_of("/\\") + 1);

    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        // 顶点坐标
        if (type == "v") {
            Vertex v;
            iss >> v.x >> v.y >> v.z;
            model.vertices.push_back(v);
        }
        // 纹理坐标
        else if (type == "vt") {
            TexCoord vt;
            iss >> vt.u >> vt.v;
            model.texCoords.push_back(vt);
        }
        // 法线向量
        else if (type == "vn") {
            Normal vn;
            iss >> vn.x >> vn.y >> vn.z;
            model.normals.push_back(vn);
        }
        // 面
        else if (type == "f") {
            Face f;
            char slash;
            for (int i = 0; i < 3; i++) {
                iss >> f.v[i] >> slash >> f.vt[i] >> slash >> f.vn[i];
                // OBJ索引从1开始，转换为0-based
                f.v[i]--;
                f.vt[i]--;
                f.vn[i]--;
            }
            model.faces.push_back(f);
        }
        // 材质库
        else if (type == "mtllib") {
            std::string mtlFile;
            iss >> mtlFile;
            std::cout << "正在加载材质库: " << mtlFile << std::endl;  // 添加调试输出
            if (loadMTL(baseDir + mtlFile, model.materials)) {
                std::cout << "材质库加载成功，共加载" << model.materials.size() << "种材质" << std::endl;
            } else {
                std::cerr << "材质库加载失败: " << baseDir + mtlFile << std::endl;
            }
        }
        // 使用材质
        else if (type == "usemtl") {
            iss >> model.currentMaterial;
            std::cout << "切换到材质: " << model.currentMaterial << std::endl;  // 添加材质切换调试
        }
    }

    std::cout << "成功加载OBJ模型: " << path << std::endl;
    std::cout << "顶点数: " << model.vertices.size() << std::endl;
    std::cout << "面数: " << model.faces.size() << std::endl;
    std::cout << "材质数: " << model.materials.size() << std::endl;

    return true;
}

// 添加鼠标移动回调函数
void mouseMovement(int x, int y) {
    // 如果未捕获鼠标则不处理
    if (!mouseCaptured) return;

    // 计算鼠标移动偏移
    GLfloat xoffset = x - lastX;
    GLfloat yoffset = lastY - y; // Y轴相反

    // 重置鼠标位置到窗口中央
    glutWarpPointer(lastX, lastY);

    // 鼠标灵敏度
    GLfloat sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // 更新偏航角(左右)和俯仰角(上下)
    yaw += xoffset;   // 偏航角控制主方向(p)的水平旋转
    pitch += yoffset;

    // 限制俯仰角，防止翻转
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // 根据角度计算观察方向
    GLfloat radYaw = yaw * 3.1415926f / 180.0f;
    GLfloat radPitch = pitch * 3.1415926f / 180.0f;

    GLfloat frontX = cos(radYaw) * cos(radPitch);
    GLfloat frontY = sin(radPitch);
    GLfloat frontZ = sin(radYaw) * cos(radPitch);

    // 更新观察点
    lookat_x = camera_x + frontX * 200.0f;
    lookat_y = camera_y + frontY * 200.0f;
    lookat_z = camera_z + frontZ * 200.0f;

    glutPostRedisplay();
}

void DrawCuboid(GLfloat cen_x, GLfloat cen_y, GLfloat cen_z, GLfloat len_x, GLfloat len_y, GLfloat len_z)
{
	GLfloat x_start = cen_x - len_x, x_end = cen_x + len_x;
	GLfloat y_start = cen_y - len_y, y_end = cen_y + len_y;
	GLfloat z_start = cen_z - len_z, z_end = cen_z + len_z;
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0), glVertex3f(x_start, y_start, z_end);
	glTexCoord2f(0.0, 1.0), glVertex3f(x_start, y_start, z_start);
	glTexCoord2f(1.0, 1.0), glVertex3f(x_end, y_start, z_start);
	glTexCoord2f(1.0, 0.0), glVertex3f(x_end, y_start, z_end);
	glEnd();
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0), glVertex3f(x_start, y_end, z_end);
	glTexCoord2f(0.0, 1.0), glVertex3f(x_start, y_end, z_start);
	glTexCoord2f(1.0, 1.0), glVertex3f(x_end, y_end, z_start);
	glTexCoord2f(1.0, 0.0), glVertex3f(x_end, y_end, z_end);
	glEnd();
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0), glVertex3f(x_start, y_start, z_end);
	glTexCoord2f(0.0, 1.0), glVertex3f(x_start, y_start, z_start);
	glTexCoord2f(1.0, 1.0), glVertex3f(x_start, y_end, z_start);
	glTexCoord2f(1.0, 0.0), glVertex3f(x_start, y_end, z_end);
	glEnd();
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0), glVertex3f(x_end, y_start, z_end);
	glTexCoord2f(0.0, 1.0), glVertex3f(x_end, y_start, z_start);
	glTexCoord2f(1.0, 1.0), glVertex3f(x_end, y_end, z_start);
	glTexCoord2f(1.0, 0.0), glVertex3f(x_end, y_end, z_end);
	glEnd();
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0), glVertex3f(x_start, y_start, z_end);
	glTexCoord2f(0.0, 1.0), glVertex3f(x_start, y_end, z_end);
	glTexCoord2f(1.0, 1.0), glVertex3f(x_end, y_end, z_end);
	glTexCoord2f(1.0, 0.0), glVertex3f(x_end, y_start, z_end);
	glTexCoord2f(1.0, 0.0), glVertex3f(x_end, y_start, z_end);
	glEnd();
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0), glVertex3f(x_start, y_start, z_start);
	glTexCoord2f(0.0, 1.0), glVertex3f(x_start, y_end, z_start);
	glTexCoord2f(1.0, 1.0), glVertex3f(x_end, y_end, z_start);
	glTexCoord2f(1.0, 0.0), glVertex3f(x_end, y_start, z_start);
	glEnd();
}

void DrawCone(GLfloat cen_x, GLfloat cen_y, GLfloat cen_z, GLfloat d1, GLfloat d2, GLfloat len, GLfloat degree)
{
	GLUquadricObj* objCylinder = gluNewQuadric();
	gluQuadricTexture(objCylinder, GL_TRUE);
	glPushMatrix();
	glTranslatef(cen_x, cen_y, cen_z);	// set the position
	glPushMatrix();
	glRotatef(degree, 1, 0, 0);	// rotate around x-axis
	gluCylinder(objCylinder, d1, d2, len, 32, 5);
	glPopMatrix();
	glPopMatrix();
}

void DrawHouse(GLfloat x_loc, GLfloat y_loc, GLfloat z_loc)
{
	glPushMatrix();
    glTranslatef(x_loc, y_loc, z_loc);

    glEnable(GL_TEXTURE_2D); // 确保纹理启用
    glColor3f(1.0f, 1.0f, 1.0f);

	// floor
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	DrawCuboid(0, 0, 0, 400, 5, 400);

	// ceiling
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	DrawCuboid(0, 400, 0, 400, 5, 400);

	// column
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor3f(1.0f, 1.0f, 1.0f);
	DrawCuboid(390, 200, 390, 10, 195, 10);
	DrawCuboid(-390, 200, 390, 10, 195, 10);
	DrawCuboid(390, 200, -390, 10, 195, 10);
	DrawCuboid(-390, 200, -390, 10, 195, 10);

	// wall
	glBindTexture(GL_TEXTURE_2D, textures[0]);

	DrawCuboid(0, 320, 395, 380, 75, 5);
	DrawCuboid(220, 125, 395, 160, 120, 5);
	DrawCuboid(-220, 125, 395, 160, 120, 5);

	DrawCuboid(0, 200, -395, 380, 195, 5);
	DrawCuboid(395, 200, 0, 5, 195, 380);
	DrawCuboid(-395, 200, 0, 5, 195, 380);

	// door
	glBindTexture(GL_TEXTURE_2D, textures[1]);

	glPushMatrix();
	glTranslatef(0.0, 0.0f, 460.0f);
	glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
	DrawCuboid(0, 125, 59, 60, 118, 5);
	glPopMatrix();

	// table
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor3f(1.0f, 1.0f, 1.0f);
	DrawCuboid(0, 100, 0, 100, 5, 100);

	glColor3f(0.0f, 0.0f, 0.0f);
	DrawCuboid(70, 50, 70, 5, 50, 5);
	DrawCuboid(-70, 50, 70, 5, 50, 5);
	DrawCuboid(70, 50, -70, 5, 50, 5);
	DrawCuboid(-70, 50, -70, 5, 50, 5);

	// teapot
	glColor3f(0.6f, 0.6f, 0.6f);

	glPushMatrix();
	glTranslatef(0, 120, 0);
	glutSolidTeapot(20.0f);
	glPopMatrix();

	// cups
	GLUquadricObj* pObj;
	pObj = gluNewQuadric();
	gluQuadricNormals(pObj, GLU_SMOOTH);

	glPushMatrix();
	glTranslatef(50, 120, 0);
	glRotatef(90, 1, 0, 0);
	gluCylinder(pObj, 10.0f, 8.0f, 15.0f, 26, 13);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-50, 120, 0);
	glRotatef(90, 1, 0, 0);
	gluCylinder(pObj, 10.0f, 8.0f, 15.0f, 26, 13);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, 120, 50);
	glRotatef(90, 1, 0, 0);
	gluCylinder(pObj, 10.0f, 8.0f, 15.0f, 26, 13);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, 120, -50);
	glRotatef(90, 1, 0, 0);
	gluCylinder(pObj, 10.0f, 8.0f, 15.0f, 26, 13);
	glPopMatrix();

	// bed
	glPushMatrix();

	glTranslatef(-320, 50, -280);
	glColor3f(229.0 / 256.0, 209.0 / 256.0, 31.0 / 256.0);
	DrawCuboid(0, 0, 0, 50, 5, 90);
	DrawCuboid(45, -25, 85, 5, 25, 5);
	DrawCuboid(-45, -25, 85, 5, 25, 5);
	DrawCuboid(45, -25, -85, 5, 25, 5);
	DrawCuboid(-45, -25, -85, 5, 25, 5);
	glColor3f(1.0, 1.0, 1.0);
	DrawCuboid(0, 15, -60, 50, 10, 30);
	glColor3f(1.0f, 0.0f, 0.0f);
	DrawCuboid(0, 15, 30, 50, 10, 60);

	glPopMatrix();

	// footstep1
	glPushMatrix();

	glTranslatef(180, 0, 0);
	glColor3f(229.0 / 256.0, 209.0 / 256.0, 31.0 / 256.0);
	DrawCuboid(0, 20, 0, 40, 20, 100);
	DrawCuboid(20, 60, 0, 20, 20, 100);

	glPopMatrix();

	//footstep2
	glPushMatrix();

	glRotatef(180.0, 0.0f, 1.0f, 0.0f);
	glTranslatef(180, 0, 0);
	glColor3f(229.0 / 256.0, 209.0 / 256.0, 31.0 / 256.0);
	DrawCuboid(0, 20, 0, 40, 20, 100);
	DrawCuboid(20, 60, 0, 20, 20, 100);

	glPopMatrix();

	// dumbbell
	glColor3f(0.0f, 0.0f, 0.0f);
	glPushMatrix();
	glTranslatef(-200, 0, 280);

	glPushMatrix();
	glTranslatef(-50, 25.0, 0);
	glutSolidSphere(25.0, 12, 10);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(50, 25.0, 0);
	glutSolidSphere(25.0, 12, 10);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, 25.0, 0.0);
	DrawCuboid(0, 0, 0, 50, 3, 3);
	glPopMatrix();

	glPopMatrix();

	glPopMatrix();
}

void DrawGround()
{
	glColor3f(1.0f, 1.0f, 1.0f);

	GLfloat x_init = -(world_radius / 2) * root_2, x_len = world_radius * root_2;	// Set the size of ground
	GLfloat z_init = -(world_radius / 2) * root_2, z_len = world_radius * root_2;
	GLfloat tex_len = 512;	// Length of each texture

	// Bind texture
	glBindTexture(GL_TEXTURE_2D, textures[4]);
	glPushMatrix();
	glTranslatef(0, -5, 0);
	for (int i = 0; i < x_len / tex_len; i++) // Divide into many parts, each one use one texture(avoid repeat)
	{
		for (int j = 0; j < z_len / tex_len; j++)
		{
			glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0), glVertex3f(x_init + tex_len * i, 0, z_init + tex_len * (j + 1));
			glTexCoord2f(0.0, 1.0), glVertex3f(x_init + tex_len * i, 0, z_init + tex_len * j);
			glTexCoord2f(1.0, 1.0), glVertex3f(x_init + tex_len * (i + 1), 0, z_init + tex_len * j);
			glTexCoord2f(1.0, 0.0), glVertex3f(x_init + tex_len * (i + 1), 0, z_init + tex_len * (j + 1));
			glEnd();
		}
	}
	glPopMatrix();
}

void DrawTree(GLfloat x_loc, GLfloat y_loc, GLfloat z_loc)
{
	// using grass texture to draw the leaf
	glBindTexture(GL_TEXTURE_2D, textures[3]);
	glPushMatrix();
	glTranslatef(x_loc, y_loc, z_loc);
	glScalef(3.0, 3.0, 3.0);
	for (int i = 0; i < 6; i++) {
		// the lowest level leaf with the darkest color
		glColor3f(43 / 255.0, 80 / 255.0, 9 / 255.0);
		glPushMatrix();
		glRotatef(tree_degree + 90 + i * 60, 0, 1, 0);
		for (int j = -8; j < 6; j++)
			DrawCuboid(47 + j * 6, 240 - j * j * 1.7, 0, 3, 75, 1);
		glPopMatrix();
		// middle level leaf with middle color
		glColor3f(106 / 255.0, 150 / 255.0, 36 / 255.0);
		glPushMatrix();
		glRotatef(tree_degree + 110 + i * 60, 0, 1, 0);
		for (int j = -5; j < 8; j++)
			DrawCuboid(37 + j * 6, 274 - j * j * 1.5, 0, 3, 75, 0.5);
		glPopMatrix();
		// the highest level leaf with the shallowest color
		glColor3f(171 / 255.0, 211 / 255.0, 60 / 255.0);
		glPushMatrix();
		glRotatef(tree_degree + 130 + i * 60, 0, 1, 0);
		for (int j = -2; j < 8; j++)
			DrawCuboid(20 + j * 6, 315 - j * j * 1.5, 0, 3, 75, 0.5);
		glPopMatrix();
	}
	glPopMatrix();

	// using wood texture to draw the truck
	glBindTexture(GL_TEXTURE_2D, textures[5]);
	glPushMatrix();
	glTranslatef(x_loc, y_loc, z_loc);
	glScalef(3.0, 3.0, 3.0);
	glColor3f(0.65, 0.65, 0.65);
	DrawCone(0, 230, 0, 9, 13, 210, 90);
	DrawCone(0, 380, 0, 2, 9, 170, 90);

	glColor3f(1, 1, 1);
	glPopMatrix();
}

void DrawChair(GLfloat x_loc, GLfloat y_loc, GLfloat z_loc)
{
	glPushMatrix();

	glTranslatef(x_loc, y_loc, z_loc);	// set position
	glScalef(1.8f, 1.8f, 1.8f);

	// using wood texture
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	for (int i = 0; i < 4; i++) {// draw surfaces of the chair
		DrawCuboid(x_loc, 66, -15 + i * 10, 75, 1, 3.5);
	}

	glPushMatrix();	// draw the back of the chair
	glRotatef(15, 1, 0, 0);
	for (int i = 0; i < 4; i++) {
		DrawCuboid(x_loc, 81 + i * 10, 0, 75, 4, 1);
	}
	glPopMatrix();

	// using stone texture
	glBindTexture(GL_TEXTURE_2D, textures[6]);
	DrawCuboid(x_loc, 20, 0, 100, 1, 25);	// base of the chair

	// using black stone texture
	glBindTexture(GL_TEXTURE_2D, textures[6]);
	// leg of the chair
	for (int i = -1; i < 2; i += 2)
	{
		DrawCuboid(x_loc + 52 * i, 43, -16 * i, 5, 22, 1);
		DrawCuboid(x_loc + 52 * i, 64, 0, 5, 1, 17);
		DrawCuboid(x_loc - 52, 43, 16 * i, 5, 22, 1);
		DrawCuboid(x_loc + 52 * i, 43, 18, 3.5, 22, 1);
	}
	// connection of the back
	glPushMatrix();
	glRotatef(15, 1, 0, 0);
	DrawCuboid(x_loc + 52, 90, 2, 3.5, 27, 1);
	DrawCuboid(x_loc - 52, 90, 2, 3.5, 27, 1);
	glPopMatrix();

	glPopMatrix();
}

// 绘制模型的函数
void drawModel(const Model& model) {
    // 保存当前OpenGL状态
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_TEXTURE_2D); // 确保纹理启用

    for (const auto& face : model.faces) {
        // 设置当前材质
        if (!model.currentMaterial.empty() && model.materials.find(model.currentMaterial) != model.materials.end()) {
            const Material& mat = model.materials.at(model.currentMaterial);
            glMaterialfv(GL_FRONT, GL_AMBIENT, mat.Ka);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, mat.Kd);
            glMaterialfv(GL_FRONT, GL_SPECULAR, mat.Ks);
            glMaterialf(GL_FRONT, GL_SHININESS, mat.Ns);
            glMaterialfv(GL_FRONT, GL_EMISSION, mat.Ke);  // 添加发射光设置
            glMaterialf(GL_FRONT, GL_ALPHA, mat.d);
        }

        glBegin(GL_TRIANGLES);
        for (int i = 0; i < 3; i++) {
            // 设置法线
            if (face.vn[i] >= 0 && face.vn[i] < model.normals.size()) {
                const Normal& n = model.normals[face.vn[i]];
                glNormal3f(n.x, n.y, n.z);
            }

            // 设置纹理坐标
            if (face.vt[i] >= 0 && face.vt[i] < model.texCoords.size()) {
                const TexCoord& tc = model.texCoords[face.vt[i]];
                glTexCoord2f(tc.u, tc.v);
            }

            // 设置顶点
            if (face.v[i] >= 0 && face.v[i] < model.vertices.size()) {
                const Vertex& v = model.vertices[face.v[i]];
                glVertex3f(v.x * 100, v.y * 100, v.z * 100); // 缩放模型
            }
        }
        glEnd();
    }

    // 恢复OpenGL状态
    glPopAttrib();
}
 

void drawCylinder(GLfloat x, GLfloat y, GLfloat z, GLfloat r, GLfloat h, GLfloat direction, bool ball)
{
	GLUquadricObj* objCylinder = gluNewQuadric();
	glPushMatrix();
	glTranslatef(x, y, z);
	glPushMatrix();
	glRotatef(direction, 1, 0, 0);
	gluCylinder(objCylinder, r, r, h, 32, 5);
	if (ball)
	{	// Draw spheres at top and bottom to make it beautiful
		glutSolidSphere(r, 20, 20);
		glPushMatrix();
		glTranslatef(0, 0, h);
		glutSolidSphere(r, 20, 20);
		glPopMatrix();
	}
	glPopMatrix();
	glPopMatrix();
	gluDeleteQuadric(objCylinder);
}


void drawLamp(GLfloat x_loc, GLfloat y_loc, GLfloat z_loc)
{
	glDisable(GL_TEXTURE_2D);
	GLfloat day_emmission[] = { 0.0, 0.0, 0.0 };
	GLfloat night_emmission[] = { 0.8, 0.6, 0.2 };
	GLfloat third_emmission[] = { 0.2, 0.5, 0.8 };
	if (time_state == 2) {
		// 夜晚状态，设置发光属性为 emmission
		glMaterialfv(GL_FRONT, GL_EMISSION, day_emmission);
	}
	else if (time_state == 1) {
		// 白天状态，设置发光属性为另一种属性（假设是 day_emmission）
		glMaterialfv(GL_FRONT, GL_EMISSION, night_emmission);
	}
	else {
		// 第三状态，设置发光属性为另一种属性（假设是 third_emmission）
		glMaterialfv(GL_FRONT, GL_EMISSION, third_emmission);
	}

	glPushMatrix();
	glTranslatef(x_loc, y_loc, z_loc);
	glScalef(1.1, 1.1, 1.1);
	glColor4f(1.0, 1.0, 0.5, 1.0);
	drawCylinder(0, 0, 0, 15, 40, 90, true);	// Shine part of lamp
	glMaterialfv(GL_FRONT, GL_EMISSION, day_emmission);
	// Top and bottom frames
	glColor3f(0, 0, 0);
	drawCylinder(0, 14, 0, 10, 4, 90, false);
	drawCylinder(0, -50, 0, 10, 6, 90, false);
	glColor3f(1, 1, 1);
	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
}

void DrawRoad(GLfloat x, GLfloat y, GLfloat z, GLfloat degree)
{
	glBindTexture(GL_TEXTURE_2D, textures[6]);

	glPushMatrix();
	glRotated(degree, 0, 1, 0); // Change road direction
	// First part
	DrawCuboid(x - 40, y, z - 40, 18, 1, 16);
	DrawCuboid(x - 40, y, z - 11, 18, 1, 9);
	DrawCuboid(x - 40, y, z + 29, 18, 1, 27);
	// Second part
	DrawCuboid(x, y, z - 16, 18, 1, 40);
	DrawCuboid(x, y, z + 42, 18, 1, 14);
	// Third part
	DrawCuboid(x + 40, y, z - 46, 18, 1, 10);
	DrawCuboid(x + 40, y, z - 2, 18, 1, 30);
	DrawCuboid(x + 40, y, z + 44, 18, 1, 12);
	glPopMatrix();
}

void DrawEnvironment()
{
	DrawTree(-1000, -50, 1300);
	DrawTree(-400, -50, 1200);
	DrawTree(200, -50, 900);
	DrawTree(800, -50, 1500);

	DrawTree(-1300, -50, 400);
	DrawTree(-1500, -50, -200);
	DrawTree(-1200, -50, -800);

	DrawTree(-1000, -50, -1300);
	DrawTree(-400, -50, -800);
	DrawTree(200, -50, -900);
	DrawTree(800, -50, -1200);

	DrawTree(1500, -50, 1000);
	DrawTree(1200, -50, 400);
	DrawTree(900, -50, -200);
	DrawTree(1200, -50, -800);

	DrawChair(0, -40, 1500);

	for (int i = 0; i < 24; i++) {
		DrawRoad(-1440.0f + i * 120.0f, 1.0f, 1080.0f, 0.0f);
	}
	for (int i = 0; i < 24; i++) {
		DrawRoad(-1440.0f + i * 120.0f, 1.0f, -1080.0f, 0.0f);
	}
	for (int i = 0; i < 17; i++) {
		DrawRoad(1320.0f, 1.0f, -960 + i * 120.0f, 0.0f);
	}
	for (int i = 0; i < 17; i++) {
		DrawRoad(-1440.0f, 1.0f, -960 + i * 120.0f, 0.0f);
	}
	for (int i = 0; i < 5; i++) {
		DrawRoad(100.0f, 1.0f, 480.0f + i * 120.0f, 0.0f);
	}

	drawLamp(-80, 60, 430);

    drawLamp(250, 60, 430);

    
}

// draw skybox
void DrawSky() {
	// set position
	GLfloat x_start = -5000, x_end = 5000;
	GLfloat y_start = -800, y_mid = 0.0, y_end = 4200;
	GLfloat z_start = -5000, z_end = 5000;
	GLfloat lookat_y, y_rate;
	int daycurrent_time;	// choose sky picture by current_time state
	if (time_state == 0) {
		daycurrent_time = 7;
		lookat_y = 0.5, y_rate = 0.13;
	}
	else if (time_state == 2) {
		daycurrent_time = 12;
		lookat_y = 0.5, y_rate = 0.4;
	}
	else {
		daycurrent_time = 17;
		lookat_y = 0.5, y_rate = 0.4;
	}
	// bind textures
	//front side
	glBindTexture(GL_TEXTURE_2D, textures[daycurrent_time]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 1.0), glVertex3f(x_end, y_end, z_end);
	glTexCoord2f(0.0, lookat_y), glVertex3f(x_end, y_mid, z_end);
	glTexCoord2f(1.0, lookat_y), glVertex3f(x_start, y_mid, z_end);
	glTexCoord2f(1.0, 1.0), glVertex3f(x_start, y_end, z_end);
	glTexCoord2f(0.0, lookat_y), glVertex3f(x_end, y_mid, z_end);
	glTexCoord2f(0.0, y_rate), glVertex3f(x_end, y_start, z_end);
	glTexCoord2f(1.0, y_rate), glVertex3f(x_start, y_start, z_end);
	glTexCoord2f(1.0, lookat_y), glVertex3f(x_start, y_mid, z_end);
	glEnd();
	// right side
	glBindTexture(GL_TEXTURE_2D, textures[daycurrent_time + 1]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 1.0), glVertex3f(x_end, y_end, z_start);
	glTexCoord2f(0.0, lookat_y), glVertex3f(x_end, y_mid, z_start);
	glTexCoord2f(1.0, lookat_y), glVertex3f(x_end, y_mid, z_end);
	glTexCoord2f(1.0, 1.0), glVertex3f(x_end, y_end, z_end);
	glTexCoord2f(0.0, lookat_y), glVertex3f(x_end, y_mid, z_start);
	glTexCoord2f(0.0, y_rate), glVertex3f(x_end, y_start, z_start);
	glTexCoord2f(1.0, y_rate), glVertex3f(x_end, y_start, z_end);
	glTexCoord2f(1.0, lookat_y), glVertex3f(x_end, y_mid, z_end);
	glEnd();
	// back side
	glBindTexture(GL_TEXTURE_2D, textures[daycurrent_time + 2]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 1.0), glVertex3f(x_start, y_end, z_start);
	glTexCoord2f(0.0, lookat_y), glVertex3f(x_start, y_mid, z_start);
	glTexCoord2f(1.0, lookat_y), glVertex3f(x_end, y_mid, z_start);
	glTexCoord2f(1.0, 1.0), glVertex3f(x_end, y_end, z_start);
	glTexCoord2f(0.0, lookat_y), glVertex3f(x_start, y_mid, z_start);
	glTexCoord2f(0.0, y_rate), glVertex3f(x_start, y_start, z_start);
	glTexCoord2f(1.0, y_rate), glVertex3f(x_end, y_start, z_start);
	glTexCoord2f(1.0, lookat_y), glVertex3f(x_end, y_mid, z_start);
	glEnd();
	// left side
	glBindTexture(GL_TEXTURE_2D, textures[daycurrent_time + 3]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 1.0), glVertex3f(x_start, y_end, z_end);
	glTexCoord2f(0.0, lookat_y), glVertex3f(x_start, y_mid, z_end);
	glTexCoord2f(1.0, lookat_y), glVertex3f(x_start, y_mid, z_start);
	glTexCoord2f(1.0, 1.0), glVertex3f(x_start, y_end, z_start);
	glTexCoord2f(0.0, lookat_y), glVertex3f(x_start, y_mid, z_end);
	glTexCoord2f(0.0, y_rate), glVertex3f(x_start, y_start, z_end);
	glTexCoord2f(1.0, y_rate), glVertex3f(x_start, y_start, z_start);
	glTexCoord2f(1.0, lookat_y), glVertex3f(x_start, y_mid, z_start);
	glEnd();
	// top side
	glBindTexture(GL_TEXTURE_2D, textures[daycurrent_time + 4]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0), glVertex3f(x_start, y_end, z_end);
	glTexCoord2f(1.0, 0.0), glVertex3f(x_start, y_end, z_start);
	glTexCoord2f(1.0, 1.0), glVertex3f(x_end, y_end, z_start);
	glTexCoord2f(0.0, 1.0), glVertex3f(x_end, y_end, z_end);
	glEnd();
}

void PointLight(GLfloat pos_x, GLfloat pos_y, GLfloat pos_z)
{
	GLfloat light_position[] = { pos_x, pos_y, pos_z, 1.0 };
	GLfloat light_ambient[] = { 0.3, 0.3, 0.3, 1.0 }, light_diffuse[] = { 0.6, 0.6, 0.6, 1.0 };
	GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };

	glEnable(GL_LIGHT2);

	glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 10.0);     // c 系数
	glLightfv(GL_LIGHT2, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT2, GL_POSITION, light_position);
	glLightfv(GL_LIGHT2, GL_SPECULAR, light_specular);
}

void ParallelLight()
{
	// propertities of the light
	GLfloat light_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
	GLfloat light_diffuse[] = { 0.3, 0.3, 0.3, 1.0 };
	GLfloat light_specular[] = { 0.6, 0.6, 0.6, 1.0 };
	GLfloat lightDirection[] = { 0.0, 0.6, 0.8, 0.0 };

	glEnable(GL_LIGHT3);
	// set propertities
	glLightfv(GL_LIGHT3, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT3, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT3, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT3, GL_POSITION, lightDirection);	// represent the direction of the light
}

void parallelLight()
{
	// Define propertities of the light
	GLfloat light_ambient[] = { 0.5, 0.5, 0.5, 1.0 };
	GLfloat light_diffuse[] = { 0.5, 0.5, 0.5, 1.0 };
	GLfloat light_specular[] = { 0.5, 0.5, 0.5, 1.0 };
	GLfloat lightDirection[] = { 1.0, 1.0, 1.0, 0.0 };
	// Set propertities
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, lightDirection);
}


// Spot lights --> Simulate the sun light and moon light, flash light
void spotLight()
{

	GLfloat light_ambient[] = { 1.0, 1.0, 0.1, 1.0 }, light_diffuse[] = { 1.0, 1.0, 0.4, 1.0 };
	GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };

	glPushMatrix();
	if (time_state == 0) // Day time
	{
		glRotatef(planet_rotation, shaft.x, shaft.y, shaft.z); // Autorotation shaft
		glEnable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
		// Set propertities

		glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
		glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
		// Special propertities of spot light

		glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 10);
		glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 30);
		glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.7);	// More intense spotlight
	}
	else
	{
		glRotatef(planet_rotation + 180, shaft.x, shaft.y, shaft.z); // Autorotation shaft
		glEnable(GL_LIGHT2);
		glDisable(GL_LIGHT1);
		// Set propertities

		glLightfv(GL_LIGHT2, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT2, GL_DIFFUSE, light_diffuse);
		glLightfv(GL_LIGHT2, GL_SPECULAR, light_specular);
		// Special propertities of spot light

		glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 10);
		glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 10);
		glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 0.005);	// More soft spotlight
	}
	glPopMatrix();
}

void setLight() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    // 根据时间状态选择光源参数
    LightParams* currentLight;
    switch(time_state) {
        case 1: currentLight = &duskLight; break;
        case 2: currentLight = &nightLight; break;
        default: currentLight = &dayLight; // 默认白天
    }
    
    // 设置光源属性（乘以强度系数）
    GLfloat ambient[4], diffuse[4], specular[4];
    for(int i=0; i<4; i++) {
        ambient[i] = currentLight->ambient[i] * currentLight->intensity;
        diffuse[i] = currentLight->diffuse[i] * currentLight->intensity;
        specular[i] = currentLight->specular[i] * currentLight->intensity;
    }
    
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_POSITION, currentLight->position);
    
    // 启用深度测试确保正确渲染
    glEnable(GL_DEPTH_TEST);
}

void DisplayFunc()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(camera_x, camera_y, camera_z, lookat_x, lookat_y, lookat_z, Vx, Vy, Vz);

	setLight();
	DrawGround();
	DrawHouse(100, 0, 0);

    DrawEnvironment();
    DrawSky();

	// 绘制bangbu模型
    glPushMatrix();
    glTranslatef(-500.0f, 0.0f, 800.0f); // 模型位置
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f); // 旋转调整朝向
    drawModel(bangbuModel);
    glPopMatrix();

	glutSwapBuffers();
}

void ReshapeFunc(GLint newWidth, GLint newHeight)
{
	glViewport(0, 0, newWidth, newHeight);
}

void IdleFunc()
{
	glutPostRedisplay();	// redisplay
}

void KeyboardFunc(unsigned char key, int x, int y)
{
    // 修正：同时考虑偏航角和俯仰角计算移动向量
    GLfloat radYaw = yaw * 3.1415926f / 180.0f;
    GLfloat radPitch = pitch * 3.1415926f / 180.0f;
    GLfloat frontX = cos(radYaw) * cos(radPitch);  // 主方向X分量
    GLfloat frontY = sin(radPitch);                // 主方向Y分量（新增）
    GLfloat frontZ = sin(radYaw) * cos(radPitch);  // 主方向Z分量
    GLfloat rightX = -sin(radYaw);                 // 右方向X分量
    GLfloat rightZ = cos(radYaw);                  // 右方向Z分量

    // WASD移动控制（添加Y轴更新）
    if (key == 'w' || key == 'W') {   // 向前移动
        camera_x += frontX * step;
        camera_y += frontY * step;  // 新增：Y轴跟随视角垂直移动
        camera_z += frontZ * step;
        lookat_x += frontX * step;
        lookat_y += frontY * step;  // 新增：观察点Y轴同步
        lookat_z += frontZ * step;
    }
    if (key == 's' || key == 'S') {   // 向后移动
        camera_x -= frontX * step;
        camera_y -= frontY * step;  // 新增：Y轴跟随视角垂直移动
        camera_z -= frontZ * step;
        lookat_x -= frontX * step;
        lookat_y -= frontY * step;  // 新增：观察点Y轴同步
        lookat_z -= frontZ * step;
    }
    if (key == 'd' || key == 'D') {   // 向垂直主方向的左方向移动
        camera_x += rightX * step;
        camera_z += rightZ * step;
        lookat_x += rightX * step;
        lookat_z += rightZ * step;
    }
    if (key == 'a' || key == 'A') {   // 向垂直主方向的右方向移动
        camera_x -= rightX * step;
        camera_z -= rightZ * step;
        lookat_x -= rightX * step;
        lookat_z -= rightZ * step;
    }

    // ESC键处理：释放鼠标捕获或退出
    if (key == 27) {
        if (mouseCaptured) {
            mouseCaptured = false;
            glutSetCursor(GLUT_CURSOR_INHERIT);
        } else {
            exit(0);
        }
    }

    glutPostRedisplay();
}

bool MoveRestriction(GLfloat pos_x, GLfloat pos_z)
{
	if (pos_z > 1600 || pos_z < -1600)
		return false;
	if (pos_x > 1600 || pos_x < -1600)
		return false;
	return true;
}

void SpecialKeyboardFunc(int key, int xx, int yy)
{
	// move the position of camera and lookat point, feel like human walking
	GLfloat ori_x = lookat_x, ori_z = lookat_z;	// record the position before moving
	GLfloat oriv_x = camera_x, oriv_z = camera_z;
	switch (key) {
	case GLUT_KEY_RIGHT:	// move left
		camera_x -= step * cos(n / 200);
		lookat_x -= step * cos(n / 200);
		camera_z += step * sin(n / 200);
		lookat_z += step * sin(n / 200);
		break;
	case GLUT_KEY_LEFT:	// move right
		camera_x += step * cos(n / 200);
		lookat_x += step * cos(n / 200);
		camera_z -= step * sin(n / 200);
		lookat_z -= step * sin(n / 200);
		break;
	case GLUT_KEY_DOWN:	// move forward
		camera_z -= step * cos(n / 200);
		lookat_z -= step * cos(n / 200);
		camera_x -= step * sin(n / 200);
		lookat_x -= step * sin(n / 200);
		break;
	case GLUT_KEY_UP:	// move back
		camera_z += step * cos(n / 200);
		lookat_z += step * cos(n / 200);
		camera_x += step * sin(n / 200);
		lookat_x += step * sin(n / 200);
		break;
	case GLUT_KEY_F1:
		time_state = 0;
		rotateFlag = FALSE;
		planet_rotation = 90; // Adjust the value for the evening state
		Rotatestep = 2; // Adjust the value for the evening state
		break;
	case GLUT_KEY_F2:
		time_state = 1;
		rotateFlag = FALSE;
		planet_rotation = 0;
		Rotatestep = 5; // A larger value
		break;
	case GLUT_KEY_F3:
		time_state = 2;
		rotateFlag = FALSE;
		planet_rotation = 180;
		Rotatestep = -5; // A larger value
		break;
	}

	if (!MoveRestriction(camera_x, camera_z))
	{
		lookat_x = ori_x, lookat_z = ori_z;
		camera_x = oriv_x, camera_z = oriv_z;
	}
}

void InitFunc()
{
	glutDisplayFunc(DisplayFunc);
	glutReshapeFunc(ReshapeFunc);
	glutIdleFunc(IdleFunc);
	glutKeyboardFunc(KeyboardFunc);
	glutSpecialFunc(SpecialKeyboardFunc);
	glutMotionFunc(mouseMovement);  // 鼠标拖动时
	glutPassiveMotionFunc(mouseMovement);  // 鼠标被动移动时

	glClearColor(0.5, 0.5, 0.4, 1.0);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	gluPerspective(fov, aspect, dnear, dfar);

	glEnable(GL_LIGHTING);	// enable lighting
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	glShadeModel(GL_FLAT);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	string path = "text/";
	for (int i = 0; i < 3; i++) {
		Image temp_image(path + "house" + to_string(i) + ".bmp");
		temp_image.BindToTexture(&textures[i]);
	}
	for (int i = 0; i < 2; i++) {
		Image temp_image(path + "grass" + to_string(i) + ".bmp");
		temp_image.BindToTexture(&textures[i + 3]);
	}
	Image tree_image(path + "tree.bmp");
	tree_image.BindToTexture(&textures[5]);

	Image rock_image(path + "rock.bmp");
	rock_image.BindToTexture(&textures[6]);

	for (int i = 0; i < 5; i++) {
		Image temp_image(path + "day" + to_string(i) + ".bmp");
		temp_image.BindToTexture(&textures[i + 7]);
	}
	for (int i = 0; i < 5; i++) {
		Image temp_image(path + "evening" + to_string(i) + ".bmp");
		temp_image.BindToTexture(&textures[i + 12]);
	}
	for (int i = 0; i < 5; i++) {
        Image temp_image(path + "night" + to_string(i) + ".bmp");
        temp_image.BindToTexture(&textures[i + 17]);
    }

    // 验证纹理加载状态
    for (int i = 0; i < 22; i++) {
        if (textures[i] == 0) {
            std::cerr << "警告：纹理" << i << "未成功加载！" << std::endl;
        }
    }

    // 加载OBJ模型
    loadOBJ("models/bangbu.obj", bangbuModel);
}

void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        mouseCaptured = true;
        glutSetCursor(GLUT_CURSOR_NONE);
        glutWarpPointer(winWidth / 2, winHeight / 2);
        lastX = winWidth / 2;
        lastY = winHeight / 2;
    }
}



void idle(void) {
    if (mouseCaptured) {
        glutWarpPointer(winWidth / 2, winHeight / 2);
        lastX = winWidth / 2;
        lastY = winHeight / 2;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	int s_width = glutGet(GLUT_SCREEN_WIDTH);
	int s_height = glutGet(GLUT_SCREEN_HEIGHT);
	glutInitWindowSize(winWidth, winHeight);
	glutInitWindowPosition((s_width - winWidth) / 2, (s_height - winHeight) / 2); // Center the window
	glutCreateWindow("GlutProject");

	InitFunc();
    glutMouseFunc(mouseButton);          // 鼠标点击回调
    glutIdleFunc(idle);
    glutMainLoop();

	return 0;
}