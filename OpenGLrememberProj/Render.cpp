#include "Render.h"


#include <windows.h>

#include <GL\gl.h>
#include <GL\glu.h>
#include "GL\glext.h"

#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "MyShaders.h"

#include "ObjLoader.h"
#include "GUItextRectangle.h"

#include "Texture.h"

#include <iostream>
#include <sstream>



GuiTextRectangle rec;
class DOT
{
public:
	double x, y, z;
public:
	DOT(double xx, double yy, double zz) : x(xx), y(yy), z(zz) {}
	DOT() {};
	double* operator & ()
	{
		static double B[3];
		B[0] = x;
		B[1] = y;
		B[2] = z;
		return B;
	}
	DOT operator + (DOT& Z)
	{
		DOT A(x, y, z);
		A.z += Z.z;
		A.x += Z.x;
		A.y += Z.y;
		return A;
	}
	DOT operator - (DOT& B)
	{
		DOT A(x, y, z);
		A.x = A.x - B.x;
		A.y = A.y - B.y;
		A.z = A.z - B.z;
		return A;
	}
	DOT operator / (double a)
	{
		DOT D(x, y, z);
		D.x /= a;
		D.y /= a;
		D.z /= a;
		return D;
	}
	DOT operator -- ()
	{
		DOT n(x, y, z);
		n.x = -n.x;
		n.y = -n.y;
		n.z = -n.z;
		return n;
	}
};

bool textureMode = true;
bool lightMode = true;
bool sun = 1;


//небольшой дефайн для упрощения кода
#define POP glPopMatrix()
#define PUSH glPushMatrix()


ObjFile *model;

Texture texture1;
Texture sTex;
Texture rTex;
Texture tBox;

Shader s[10];  //массивчик для десяти шейдеров
Shader frac;
Shader cassini;


OpenGL* ogl;

//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;

	
	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}

	
	//считает позицию камеры, исходя из углов поворота, вызывается движком
	virtual void SetUpCamera()
	{

		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist*cos(fi2)*cos(fi1),
			camDist*cos(fi2)*sin(fi1),
			camDist*sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}
	auto Look() {
		return (lookPoint - pos).normolize();
	}


}  camera;   //создаем объект камеры

//класс недоделан!
class WASDcamera :public CustomCamera
{
public:
		
	float camSpeed;

	WASDcamera()
	{
		camSpeed = 0.4;
		pos.setCoords(5, 5, 5);
		lookPoint.setCoords(0, 0, 0);
		normal.setCoords(0, 0, 1);
	}

	virtual void SetUpCamera()
	{

		if (OpenGL::isKeyPressed('W'))
		{
			Vector3 forward = (lookPoint - pos).normolize()*camSpeed;
			pos = pos + forward;
			lookPoint = lookPoint + forward;
			
		}
		if (OpenGL::isKeyPressed('S'))
		{
			Vector3 forward = (lookPoint - pos).normolize()*(-camSpeed);
			pos = pos + forward;
			lookPoint = lookPoint + forward;
			
		}

		LookAt();
	}

} WASDcam;

class sun : public Light 
{
public:
	/*double sc;
	sun (Vector3 position, double scale)
	{
		pos = position;
		sc = scale;
	}*/
	sun()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}
	void DrawLightGhismo()
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		Shader::DontUseShaders();
		bool f1 = glIsEnabled(GL_LIGHTING);
		glDisable(GL_LIGHTING);
		bool f2 = glIsEnabled(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_2D);
		bool f3 = glIsEnabled(GL_DEPTH_TEST);

		glIsEnabled(GL_DEPTH_TEST);
		glColor3d(0.9, 0, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale * 0.02;
		extern bool sun;
		if ( sun == 1)
			s.Show();
	}
	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		//glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.1);

		glEnable(GL_LIGHT0);
	}
} Sun;
//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}

	
	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		Shader::DontUseShaders();
		bool f1 = glIsEnabled(GL_LIGHTING);
		glDisable(GL_LIGHTING);
		bool f2 = glIsEnabled(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_2D);
		bool f3 = glIsEnabled(GL_DEPTH_TEST);
		
		glDisable(GL_DEPTH_TEST);
		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale*0.08;
		s.Show();

		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
				glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale*1.5;
			c.Show();
		}
		/*
		if (f1)
			glEnable(GL_LIGHTING);
		if (f2)
			glEnable(GL_TEXTURE_2D);
		if (f3)
			glEnable(GL_DEPTH_TEST);
			*/
	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света



//старые координаты мыши
int mouseX = 0, mouseY = 0;




float offsetX = 0, offsetY = 0;
float zoom=1;
float Time = 0;
int tick_o = 0;
int tick_n = 0;



DOT P;
//обработчик движения мыши
void mouseEvent(OpenGL *ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01*dx;
		camera.fi2 += -0.01*dy;
	}


	if (OpenGL::isKeyPressed(VK_LBUTTON))
	{
		offsetX -= 1.0*dx/ogl->getWidth()/zoom; // ?????????????????????????????????????????????????????????????
		offsetY += 1.0*dy/ogl->getHeight()/zoom;
	}


	
	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y,60,ogl->aspect);

		double z = Sun.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k*r.direction.X() + r.origin.X();
		y = k*r.direction.Y() + r.origin.Y();

		Sun.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		Sun.pos = Sun.pos + Vector3(0, 0, 0.02 * dy);
	}
}
//обработчик вращения колеса  мыши
void mouseWheelEvent(OpenGL *ogl, int delta)
{


	float _tmpZ = delta*0.003;
	if (ogl->isKeyPressed('Z'))
		_tmpZ *= 10;
	zoom += 0.2*zoom*_tmpZ;


	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01*delta;
}

static bool xyz = 0, castom = 1;
//обработчик нажатия кнопок клавиатуры
void keyDownEvent(OpenGL *ogl, int key)
{
	static bool switcher = 0;
	if (OpenGL::isKeyPressed('U'))
	{
		if (castom) castom = 0;
		else castom = 1;
	}
	if (OpenGL::isKeyPressed('I'))
	{
		if (xyz)
		{
			xyz = 0;
		}
		else
		{
			xyz = 1;
		}
	}
	if (OpenGL::isKeyPressed('O'))
	{
		if (!switcher)
		{
			sun = 0;
			switcher = 1;
		}
		else
		{
			sun = 1;
			switcher = 0;
		}
	}
	if (key == 'S')
	{
		frac.LoadShaderFromFile();
		frac.Compile();

		s[0].LoadShaderFromFile();
		s[0].Compile();

		cassini.LoadShaderFromFile();
		cassini.Compile();
	}

	if (key == 'Q')
		Time = 0;
}

void keyUpEvent(OpenGL *ogl, int key)
{

}


void DrawQuad()
{
	double A[] = { 0,0 };
	double B[] = { 1,0 };
	double C[] = { 1,1 };
	double D[] = { 0,1 };
	glBegin(GL_QUADS);
	glColor3d(.5, 0, 0);
	glNormal3d(0, 0, 1);
	glTexCoord2d(0, 0);
	glVertex2dv(A);
	glTexCoord2d(1, 0);
	glVertex2dv(B);
	glTexCoord2d(1, 1);
	glVertex2dv(C);
	glTexCoord2d(0, 1);
	glVertex2dv(D);
	glEnd();
}


ObjFile objModel, monkey, bulldozer, wizard, flower, stuff, floor1, coffin;


Texture TBulldozer, Twizard, tfloor, tstuff, tcoffin[4], tcolors[4];


//выполняется перед первым рендером
void initRender(OpenGL* ogl)
{
	::ogl = ogl;
	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	
	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);




	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	//ogl->mainCamera = &WASDcam;
	ogl->mainLight = &light;
	ogl->mainLight = &Sun;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH);


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	/*
	//texture1.loadTextureFromFile("textures\\texture.bmp");   загрузка текстуры из файла
	*/


	frac.VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	frac.FshaderFileName = "shaders\\frac.frag"; //имя файла фрагментного шейдера
	frac.LoadShaderFromFile(); //загружаем шейдеры из файла
	frac.Compile(); //компилируем

	cassini.VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	cassini.FshaderFileName = "shaders\\cassini.frag"; //имя файла фрагментного шейдера
	cassini.LoadShaderFromFile(); //загружаем шейдеры из файла
	cassini.Compile(); //компилируем


	s[0].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[0].FshaderFileName = "shaders\\light.frag"; //имя файла фрагментного шейдера
	s[0].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[0].Compile(); //компилируем

	s[1].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[1].FshaderFileName = "shaders\\textureShader.frag"; //имя файла фрагментного шейдера
	s[1].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[1].Compile(); //компилируем



	//так как гит игнорит модели *.obj файлы, так как они совпадают по расширению с объектными файлами, 
	// создающимися во время компиляции, я переименовал модели в *.obj_m
   //loadModel("models\\lpgun6.obj_m", &objModel);

	std::string a = "2";
	glActiveTexture(GL_TEXTURE0);
	//loadModel("models\\monkey.obj_m", &monkey);
	//monkeyTex.loadTextureFromFile("textures//tex.bmp");
	//monkeyTex.bindTexture();

	loadModel("models\\untitled.obj_m", &bulldozer);
	TBulldozer.loadTextureFromFile("textures//RTex.bmp");

	loadModel("models\\newmini.obj_m", &wizard);
	Twizard.loadTextureFromFile("textures//2.bmp");

	tfloor.loadTextureFromFile("textures//floor2.bmp");

	tstuff.loadTextureFromFile("textures//stuff2.bmp");
	loadModel("models\\stuff2.obj_m", &stuff);
	loadModel("models\\skullMY.obj_m", &flower);
	tcolors[0].loadTextureFromFile("textures//skullred.bmp");
	tcolors[1].loadTextureFromFile("textures//skullgreen.bmp");
	tcolors[2].loadTextureFromFile("textures//skullblue.bmp");
	tcolors[3].loadTextureFromFile("textures//skullclear.bmp");
	loadModel("models\\floor.obj_m", &floor1);
	loadModel("models\\cofin.obj_m", &coffin);
	tcoffin[0].loadTextureFromFile("textures//coffinred.bmp");
	tcoffin[1].loadTextureFromFile("textures//coffingreen.bmp");
	tcoffin[2].loadTextureFromFile("textures//coffinblue.bmp");
	tcoffin[3].loadTextureFromFile("textures//coffinclear.bmp");
	tick_n = GetTickCount();
	tick_o = tick_n;

}

DOT dots[361];
double r;
double amount = 180;
int t_max = 0;
DOT Z(0, 0, 3);
struct Colors {
	bool Red = 0, Green = 0, Blue = 0, Clear = 0;
};
void DotsOfTheCircle(DOT O, double R, double angle)
{
	for (int i = 0; i <= amount; i++, r += angle / amount)
	{
		dots[i].x = O.x + R * cos(r);
		dots[i].y = O.y + R * sin(r);
	}
}
void AnimationSun()
{
		P.x = dots[t_max].x;
		P.y = dots[t_max].y;
		P.z = Z.z; 
	glColor3d(0, 0, 1);
	Sun.pos = Vector3(P.x, P.y, P.z);
}
double xb = 5, yb = 5;
void BorderPlace()
{
	double xbp = xb + 0.5, ybp = yb + 0.5;
	tfloor.bindTexture();
	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glColor3d(0.85, 0.85, 0.85);
	glTexCoord2d(0, 1);
	glVertex3d(xbp, ybp, 0);
	glTexCoord2d(1, 1);
	glVertex3d(xbp, -ybp, 0);
	glTexCoord2d(1, 0);
	glVertex3d(-xbp, -ybp, 0);
	glTexCoord2d(0, 0);
	glVertex3d(-xbp, ybp, 0);
	glEnd();
}
static double re, gr, bl;

class COOL
{
	public:
	double x = 0, y = 0;
	double xb = 1, yb = 1;
	double re = 0, gr = 0, bl = 0;
	bool SphereOfDeath(double xw, double yw, Colors& bp)
	{
		if (yw >= (y - yb) && yw <= (y + yb) && xw >= (x - xb) && xw <= (x + xb))
		{
			if (bp.Red)
			{
				re += 0.2;
				bp.Red = 0;
				return 1;
			}
			else if (bp.Green)
			{
				gr += 0.2;
				bp.Green = 0;
				return 1;
			}
			else if (bp.Blue)
			{
				bl += 0.2;
				bp.Blue = 0;
				return 1;
			}
			else if (bp.Clear)
			{
				re = 0;
				gr = 0;
				bl = 0;
				bp.Clear = 0;
				return 1;
			}
		}
		return 0;
	}
}SOD;
class items
{
	public:
		double x, y;
		double xb = 1, yb = 1;
		bool visible = 0;
		items(double x, double y) : x(x), y(y) {}
		bool TakeItem(double xw, double yw)
		{
			if (yw >= (y - yb) && yw <= (y + yb) && xw >= (x - xb) && xw <= (x + xb))
			{
				visible = 1;
				return 1;
			}
			return 0;
		}
};
items red_flower(-4, 4);
items green_flower(4, 4);
items blue_flower(4, -4);
items Clear_flower(-4, -4);
int st1 = 1, st2 = 0;
class Dude
{
	public:
	double x, y, z, velocity = 0.1, angle;
	int backpack[4] = { 0,0,0,0 };
	Colors BackPack;
	Dude(double x,double y, double z): x(x), y(y), z(z) {}
	void Move()
	{
		if (OpenGL::isKeyPressed('W'))
		{
			angle = 180;
			if(y <= yb)
				y += velocity;
			st1 = 1;
			st2 = 0;
			if (red_flower.TakeItem(x, y))
			{
				green_flower.visible = 0;
				blue_flower.visible = 0;
				Clear_flower.visible = 0;
				BackPack.Red = 1;
			}
			else if (green_flower.TakeItem(x, y))
			{
				red_flower.visible = 0;
				blue_flower.visible = 0;
				Clear_flower.visible = 0;
				BackPack.Green = 1;
			}
			else if (blue_flower.TakeItem(x, y))
			{
				red_flower.visible = 0;
				green_flower.visible = 0;
				Clear_flower.visible = 0;
				BackPack.Blue = 1;
			}
			else if (Clear_flower.TakeItem(x, y))
			{
				red_flower.visible = 0;
				green_flower.visible = 0;
				blue_flower.visible = 0;
				BackPack.Clear = 1;
			}

			if (SOD.SphereOfDeath(x, y, BackPack)) {
				green_flower.visible = 0;
				red_flower.visible = 0;
				blue_flower.visible = 0;
				Clear_flower.visible = 0;
			}
		}
		if (OpenGL::isKeyPressed('A'))
		{
			angle = -90;
			if(x >= -xb)
				x -= velocity;
			st1 = 0;
			st2 = 1;
			if (red_flower.TakeItem(x, y))
			{
				green_flower.visible = 0;
				blue_flower.visible = 0;
				Clear_flower.visible = 0;
				BackPack.Red = 1;
			}
			else if (green_flower.TakeItem(x, y))
			{
				red_flower.visible = 0;
				blue_flower.visible = 0;
				Clear_flower.visible = 0;
				BackPack.Green = 1;
			}
			else if (blue_flower.TakeItem(x, y))
			{
				red_flower.visible = 0;
				green_flower.visible = 0;
				Clear_flower.visible = 0;
				BackPack.Blue = 1;
			}
			else if (Clear_flower.TakeItem(x, y))
			{
				red_flower.visible = 0;
				green_flower.visible = 0;
				blue_flower.visible = 0;
				BackPack.Clear = 1;
			}

			if (SOD.SphereOfDeath(x, y, BackPack)) {
				green_flower.visible = 0;
				red_flower.visible = 0;
				blue_flower.visible = 0;
				Clear_flower.visible = 0;
			}
		}
		if (OpenGL::isKeyPressed('S'))
		{
			angle = 0;
			if(y >= -yb)
				y -= velocity;
			st1 = 1;
			st2 = 0;
			if (red_flower.TakeItem(x, y))
			{
				green_flower.visible = 0;
				blue_flower.visible = 0;
				Clear_flower.visible = 0;
				BackPack.Red = 1;
			}
			else if (green_flower.TakeItem(x, y))
			{
				red_flower.visible = 0;
				blue_flower.visible = 0;
				Clear_flower.visible = 0;
				BackPack.Green = 1;
			}
			else if (blue_flower.TakeItem(x, y))
			{
				red_flower.visible = 0;
				green_flower.visible = 0;
				Clear_flower.visible = 0;
				BackPack.Blue = 1;
			}
			else if (Clear_flower.TakeItem(x, y))
			{
				red_flower.visible = 0;
				green_flower.visible = 0;
				blue_flower.visible = 0;
				BackPack.Clear = 1;
			}

			if (SOD.SphereOfDeath(x, y, BackPack)) {
				green_flower.visible = 0;
				red_flower.visible = 0;
				blue_flower.visible = 0;
				Clear_flower.visible = 0;
			}
		}
		if (OpenGL::isKeyPressed('D'))
		{
			angle = 90;
			if(x <= xb)
				x += velocity;
			st1 = 0;
			st2 = 1;
			if (red_flower.TakeItem(x, y))
			{
				green_flower.visible = 0;
				blue_flower.visible = 0;
				Clear_flower.visible = 0;
				BackPack.Red = 1;
			}
			else if (green_flower.TakeItem(x, y))
			{
				red_flower.visible = 0;
				blue_flower.visible = 0;
				Clear_flower.visible = 0;
				BackPack.Green = 1;
			}
			else if (blue_flower.TakeItem(x, y))
			{
				red_flower.visible = 0;
				green_flower.visible = 0;
				Clear_flower.visible = 0;
				BackPack.Blue = 1;
			}
			else if (Clear_flower.TakeItem(x, y))
			{
				red_flower.visible = 0;
				green_flower.visible = 0;
				blue_flower.visible = 0;
				BackPack.Clear = 1;
			}

			if (SOD.SphereOfDeath(x, y, BackPack)) {
				green_flower.visible = 0;
				red_flower.visible = 0;
				blue_flower.visible = 0;
				Clear_flower.visible = 0;
			}
		}
	}
};
Dude tank(-2,0,0);





void Render(OpenGL *ogl)
{   
	//glClearColor(.1f, .1f, .1f, .0f);
	if(xyz) ogl->DrawAxes();

	glColor3d(SOD.re, SOD.gr,SOD.bl);
	Sphere sph;
	sph.pos.setCoords(0, 0, 1.5);
	sph.scale = sph.scale * 1;
	sph.Show();
	glEnable(GL_TEXTURE_2D);
	tick_o = tick_n;
	tick_n = GetTickCount();
	Time += (tick_n - tick_o) / 1000.0;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (lightMode)
		glEnable(GL_LIGHTING);

	//настройка материала
	GLfloat amb[] = { 0.2, 0.2, 0.1, 1. };
	GLfloat dif[] = { 0.4, 0.65, 0.5, 1. };
	GLfloat spec[] = { 0.9, 0.8, 0.3, 1. };
	GLfloat sh = 0.1f * 256;

	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	//размер блика
	glMaterialf(GL_FRONT, GL_SHININESS, sh);

	Shader::DontUseShaders();


	//Прогать тут  
	if (!sun)
	{
		s[1].UseShader();
	}

	BorderPlace();
	tank.Move();
	glPushMatrix();
		glTranslated(tank.x, tank.y, tank.z);
		Twizard.bindTexture();
		glRotated(90, 1, 0, 0);
		glRotated(tank.angle, 0, 1, 0);
		wizard.DrawObj();
	glPopMatrix();

	/*glPushMatrix();
		glTranslated(0,0,0);
		TBulldozer.bindTexture();
		glRotated(90, 1, 0, 0);
		bulldozer.DrawObj();
	glPopMatrix();*/

	glPushMatrix();
		glTranslated(tank.x - st1, tank.y - st2, 1);
		glScaled(5, 5, 5);
		glRotated(90, 1, 0, 0);
		tstuff.bindTexture();
		stuff.DrawObj();
	glPopMatrix();

	glPushMatrix();
		glTranslated(red_flower.x, red_flower.y, 0);
		glScaled(0.12, 0.12, 0.12);
		glRotated(40, 0, 0, 1);
		glRotated(90, 1, 0, 0);
		tcoffin[0].bindTexture();
		coffin.DrawObj();
	glPopMatrix();
	glPushMatrix();
		glTranslated(green_flower.x, green_flower.y, 0);
		glScaled(0.12, 0.12, 0.12);
		glRotated(-40, 0, 0, 1);
		glRotated(90, 1, 0, 0);
		tcoffin[1].bindTexture();
		coffin.DrawObj();
	glPopMatrix();
	glPushMatrix();
		glTranslated(blue_flower.x, blue_flower.y, 0);
		glScaled(0.12, 0.12, 0.12);
		glRotated(-120, 0, 0, 1);
		glRotated(90, 1, 0, 0);
		tcoffin[2].bindTexture();
		coffin.DrawObj();
	glPopMatrix();
	glPushMatrix();
		glTranslated(Clear_flower.x, Clear_flower.y, 0);
		glScaled(0.12, 0.12, 0.12);
		glRotated(120, 0, 0, 1);
		glRotated(90, 1, 0, 0);
		tcoffin[3].bindTexture();
		coffin.DrawObj();
	glPopMatrix();


	if (red_flower.visible)
	{
		tank.BackPack.Blue = 0;
		tank.BackPack.Green = 0;
		tank.BackPack.Clear = 0;
		PUSH;
		tcolors[0].bindTexture();
		glTranslated(tank.x, tank.y, 2.5);
		glScaled(0.1, 0.1, 0.1);
		glRotated(tank.angle, 0, 0, 1);
		glRotated(90, 1, 0, 0);
		flower.DrawObj();
		POP;
	}
	if (green_flower.visible)
	{
		tank.BackPack.Blue = 0;
		tank.BackPack.Red = 0;
		tank.BackPack.Clear = 0;
		PUSH;
		tcolors[1].bindTexture();
		glTranslated(tank.x, tank.y, 2.5);
		glScaled(0.1, 0.1, 0.1);
		glRotated(tank.angle, 0, 0, 1);
		glRotated(90, 1, 0, 0);
		flower.DrawObj();
		POP;
	}
	if (blue_flower.visible)
	{
		tank.BackPack.Red = 0;
		tank.BackPack.Green = 0;
		tank.BackPack.Clear = 0;
		PUSH;
		tcolors[2].bindTexture();
		glTranslated(tank.x, tank.y, 2.5);
		glScaled(0.1, 0.1, 0.1);
		glRotated(tank.angle, 0, 0, 1);
		glRotated(90, 1, 0, 0);
		flower.DrawObj();
		POP;
	}
	if (Clear_flower.visible)
	{
		tank.BackPack.Blue = 0;
		tank.BackPack.Green = 0;
		tank.BackPack.Red = 0;
		PUSH;
		tcolors[3].bindTexture();
		glTranslated(tank.x, tank.y, 2.5);
		glScaled(0.1, 0.1, 0.1);
		glRotated(tank.angle, 0, 0, 1);
		glRotated(90, 1, 0, 0);
		flower.DrawObj();
		POP;
	}
	static bool flag = 1;
	if (flag)
	{
		if (t_max < amount) 
			t_max += 1;
		else flag = 0;
	}
	else
	{
		t_max = 0;
		flag = 1;
	}
	if (sun)
	{
		double Pi = 2 * 3.14;
		DOT MidPoint(tank.x - st1, tank.y - st2, 1);
		DotsOfTheCircle(MidPoint, 0.1, Pi);
		if(castom)AnimationSun();
	}
	Shader::DontUseShaders();
}   //конец тела функции


bool gui_init = false;

//рисует интерфейс, вызывется после обычного рендера
void RenderGUI(OpenGL *ogl)
{
	
	Shader::DontUseShaders();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_LIGHTING);
	

	glActiveTexture(GL_TEXTURE0);
	GuiTextRectangle rec;		   //классик моего авторства для удобной работы с рендером текста.
	rec.setSize(300, 150);
	rec.setPosition(10, ogl->getHeight() - 150 - 10);

	std::stringstream ss;
	ss << "O - вкл/выкл освещение" << std::endl;
	ss << "WASD - управление (Y,-X,-Y,X)" << std::endl;
	ss << "I - вкл/выкл направление координат" << std::endl;
	ss << "U - вкл/выкл кастомное положение света, \n управление: G + ЛКМ" << std::endl;

	rec.setText(ss.str().c_str());
	rec.Draw();




	Shader::DontUseShaders(); 
}

void resizeEvent(OpenGL *ogl, int newW, int newH)
{
	rec.setPosition(10, newH - 100 - 10);
}

