#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <math.h>
#include <stdio.h>

#define CLASSNAME "TTTWnd"

#define SIGN(x) ((x) > 0 ? 1 : -1)

unsigned char key_states[256];
unsigned char game_started = 0;

#define SEL_BUF_SIZE 256
unsigned int sel_buf[SEL_BUF_SIZE];

HWND g_hWnd;
HDC g_hDC;
HGLRC g_hGL;

int g_pitch;
int g_yaw;

int g_width = 800;
int g_height = 480;

int g_lastMouseX;
int g_lastMouseY;

int g_hits[10][3];
int g_hitIndex;
int g_numHits;

double g_ptX;
double g_ptY;
double g_ptZ;

unsigned char board[3][3][3];
unsigned char turn = 1;
unsigned char winner;
unsigned char move_to_ascii[] = " XO";

GLYPHMETRICSFLOAT g_metrics[256];
int g_fontBase;

void CheckWins(int x, int y, int z)
{
	int xx, yy, zz;
	int res = board[x][y][z];
	unsigned char won;
	
	// Same x
	won = 1;
	for(xx = 0; xx <= 2; xx++) {
		if(board[xx][y][z] != res) {
			won = 0;
			break;
		}
	}
	if(won) {
		winner = res;
		return;
	}
	
	// Same y
	won = 1;
	for(yy = 0; yy <= 2; yy++) {
		if(board[x][yy][z] != res) {
			won = 0;
			break;
		}
	}
	if(won) {
		winner = res;
		return;
	}
	
	// Same z
	won = 1;
	for(zz = 0; zz <= 2; zz++) {
		if(board[x][y][zz] != res) {
			won = 0;
			break;
		}
	}
	if(won) {
		winner = res;
		return;
	}
	
	
	if(x == y) {
		won = 1;
		for(xx = 0; xx <= 2; xx++) {
			if(board[xx][xx][z] != res) {
				won = 0;
				break;
			}
		}
		if(won) {
			winner = res;
			return;
		}
	}
	
	if(x == 2 - y) {
		won = 1;
		for(xx = 0; xx <= 2; xx++) {
			if(board[2 - xx][xx][z] != res) {
				won = 0;
				break;
			}
		}
		if(won) {
			winner = res;
			return;
		}
	}
	
	if(y == z) {
		won = 1;
		for(yy = 0; yy <= 2; yy++) {
			if(board[x][yy][yy] != res) {
				won = 0;
				break;
			}
		}
		if(won) {
			winner = res;
			return;
		}
	}
	
	if(y == 2 - z) {
		won = 1;
		for(yy = 0; yy <= 2; yy++) {
			if(board[x][2 - yy][yy] != res) {
				won = 0;
				break;
			}
		}
		if(won) {
			winner = res;
			return;
		}
	}
	
	if(z == x) {
		won = 1;
		for(zz = 0; zz <= 2; zz++) {
			if(board[zz][y][zz] != res) {
				won = 0;
				break;
			}
		}	
		if(won) {
			winner = res;
			return;
		}
	}
	
	
	if(z == 2 - x) {
		won = 1;
		for(zz = 0; zz <= 2; zz++) {
			if(board[zz][y][2 - zz] != res) {
				won = 0;
				break;
			}
		}	
		if(won) {
			winner = res;
			return;
		}
	}
	
	if(x == y && x == z) {
		won = 1;
		for(zz = 0; zz <= 2; zz++) {
			if(board[zz][zz][zz] != res) {
				won = 0;
				break;
			}
		}
		if(won) {
			winner = res;
			return;
		}
	}
	
	if(x == 2 - y && x == z) {
		won = 1;
		for(zz = 0; zz <= 2; zz++) {
			if(board[zz][2 - zz][zz] != res) {
				won = 0;
				break;
			}
		}
		if(won) {
			winner = res;
			return;
		}
	}
	
	if(x == y && x == 2 - z) {
		won = 1;
		for(zz = 0; zz <= 2; zz++) {
			if(board[zz][zz][2 - zz] != res) {
				won = 0;
				break;
			}
		}
		if(won) {
			winner = res;
			return;
		}
	}
	
	if(x == 2 - y && x == 2 - z) {
		won = 1;
		for(zz = 0; zz <= 2; zz++) {
			if(board[zz][2 - zz][2 - zz] != res) {
				won = 0;
				break;
			}
		}
		if(won) {
			winner = res;
			return;
		}
	}
}

static void Fan(float r, float y, int slices)
{
	int k;
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0, SIGN(y), 0);
	glVertex3f(0, y, 0);
	
	for(k = 0; k <= slices; k++) {
		float theta = ((2.0f * M_PI) / slices) * k;
		glVertex3f(r * cos(theta), y, r * sin(theta));
	}
	
	glEnd();
}

void Cylinder(float r, float length, int slices)
{
	int k;
	float halfLength = length / 2.0f;
	
	Fan(r, -halfLength, slices);
	Fan(r, halfLength, slices);
	
	glBegin(GL_TRIANGLE_STRIP);
	
	for(k = 0; k <= slices; k++) {
		float theta = ((2.0f * M_PI) / slices) * k;
		glNormal3f(cos(theta), 0, sin(theta));
		glVertex3f(r * cos(theta), -halfLength, r * sin(theta));
		glVertex3f(r * cos(theta), halfLength, r * sin(theta));
	}
	
	glEnd();
}

void Sphere(float r, int n)
{
	int i, j;
	float theta1, theta2, theta3;
	
	for (j = 0; j < n / 2; j++) {
		theta1 = j * (M_PI * 2.0f) / n - (M_PI / 2.0f);
		theta2 = (j + 1) * (M_PI * 2.0f) / n - (M_PI / 2.0f);

		glBegin(GL_TRIANGLE_STRIP);
		for (i = 0; i <= n; i++) {
			theta3 = i * (M_PI * 2.0f) / n;
			
			float x = cos(theta2) * cos(theta3);
			float y = sin(theta2);
			float z = cos(theta2) * sin(theta3);

			glNormal3f(x, y, z);
			glVertex3f(r * x, r * y, r * z);

			x = cos(theta1) * cos(theta3);
			y = sin(theta1);
			z = cos(theta1) * sin(theta3);

			glNormal3f(x, y, z);
			glVertex3f(r * x, r * y, r * z);
		}
		glEnd();
	}
}

void DrawX(void)
{
	glPushMatrix();
		glRotatef(45, 0, 0, 1);
		Cylinder(0.25, 4, 16);
		glRotatef(90, 0, 0, 1);
		Cylinder(0.25, 4, 16);
	glPopMatrix();
}

void GridSet(void)
{
	glPushMatrix();
		glTranslatef(-2.0f, 0, 2);
		Cylinder(0.25f, 12, 16);
	glPopMatrix();
	
	glPushMatrix();
		glTranslatef(2.0f, 0, 2);
		Cylinder(0.25f, 12, 16);
	glPopMatrix();
	
	glPushMatrix();
		glTranslatef(-2.0f, 0, -2);
		Cylinder(0.25f, 12, 16);
	glPopMatrix();
	
	glPushMatrix();
		glTranslatef(2.0f, 0, -2);
		Cylinder(0.25f, 12, 16);
	glPopMatrix();
}

void Pick(void)
{
	int viewport[4];
	int i, j;
	unsigned int names, *ptr;
	float closest_z = 9999.0f;
	int closest_index = 0;
	
	glRenderMode(GL_SELECT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glGetIntegerv(GL_VIEWPORT, viewport);
	gluPickMatrix(g_lastMouseX, viewport[3] - g_lastMouseY, 10, 10, viewport);
	gluPerspective(45.0f, (float) g_width / g_height, 1.0f, 50.0f);
	
	glMatrixMode(GL_MODELVIEW);
	glInitNames();
	
	glPushMatrix();
		glRotatef(g_pitch, 1, 0, 0);
		glRotatef(g_yaw, 0, 1, 0);
		
		int x, y, z;
		for(z = 0; z <= 2; z++) {
			glPushName(z);
			for(y = 0; y <= 2; y++) {
				glPushName(y);
				for(x = 0; x <= 2; x++) {
					glPushName(x);
					glPushMatrix();
						glTranslatef((x - 1) * 4, (y - 1) * 4, (z - 1) * 4);
						Sphere(2, 8);
					glPopMatrix();
					glPopName();
				}
				glPopName();
			}
			glPopName();
		}
		
	glPopMatrix();
	
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	
	g_numHits = glRenderMode(GL_RENDER);
	
	ptr = (unsigned int *) sel_buf;
	for (i = 0; i < g_numHits; i++) {
		names = *ptr;
		ptr++;
		
		float z = (float) *ptr / 0x7fffffff;
		if(z < closest_z) {
			closest_z = z;
			closest_index = i;
		}
			
		ptr += 2;
		
		for (j = 0; j < names; j++) {  
			g_hits[i][j] = *ptr;
			ptr++;
		}
	}
	
	g_hitIndex = closest_index;
}

void MouseMoved(int x, int y)
{
	if(game_started) {
		Pick();
	}
	g_lastMouseX = x;
	g_lastMouseY = y;
}

void MouseDragged(int x, int y)
{
	if(game_started) {
		g_yaw -= (g_lastMouseX - x);
		g_pitch -= (g_lastMouseY - y);
		
		g_yaw %= 360;
		g_pitch %= 360;
		
		if(g_pitch > 45) g_pitch = 45;
		if(g_pitch < -45) g_pitch = -45;
	}
	
	g_lastMouseX = x;
	g_lastMouseY = y;
}

void MouseClicked(int x, int y)
{
	if(game_started) {
		int m_x = g_hits[g_hitIndex][2];
		int m_y = g_hits[g_hitIndex][1];
		int m_z = g_hits[g_hitIndex][0];
		printf("Placing: %d, %d, %d\n", m_x, m_y, m_z);
		if(board[m_x][m_y][m_z] == 0) {
			board[m_x][m_y][m_z] = turn;
			CheckWins(m_x, m_y, m_z);
			turn = turn == 1 ? 2 : 1;
		}
	}

	g_lastMouseX = x;
	g_lastMouseY = y;
}

void WindowResized(int w, int h)
{
	g_width = w;
	g_height = h;
	
	glViewport(0, 0, g_width, g_height);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, (float) g_width / g_height, 1.0f, 20.0f);
	
	glMatrixMode(GL_MODELVIEW);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	
	switch(message) {
		case WM_KEYDOWN:
			if(wParam == VK_TAB) {
				if(g_numHits > 1)
					g_hitIndex = (g_hitIndex + 1) % g_numHits;
			} else if(wParam == 'C') {
				memset(board, 0, 3 * 3 * 3);
			} else if(wParam == VK_RETURN) {
				game_started = 1;
				winner = 0;
				turn = 1;
				memset(board, 0, 3 * 3 * 3);
			}
			key_states[wParam] = 1;
			break;
		case WM_KEYUP:
			key_states[wParam] = 0;
			break;
		case WM_SIZE:
			WindowResized(LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_MOUSEMOVE:
			if(wParam & MK_RBUTTON)
				MouseDragged(LOWORD(lParam), HIWORD(lParam));
			else MouseMoved(LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_LBUTTONDOWN:
			MouseClicked(LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default: return DefWindowProc(hWnd, message, wParam, lParam);
	}
	
	return 0;
}

void SetMaterial(float ambr, float ambg, float ambb, float difr, float difg, float difb, float specr, float specg, float specb, float shine)
{
	float mat[4];
	
	mat[0] = ambr;
	mat[1] = ambg;
	mat[2] = ambb;
	mat[3] = 1.0;
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat);
	
	mat[0] = difr;
	mat[1] = difg;
	mat[2] = difb;
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat);
	
	mat[0] = specr;
	mat[1] = specg;
	mat[2] = specb;
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat);
	
	glMaterialf(GL_FRONT, GL_SHININESS, shine * 128.0);
}

void InitGL(void)
{
	int format;
	HFONT font;
	PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR) };

	g_hDC = GetDC(g_hWnd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	format = ChoosePixelFormat(g_hDC, &pfd);
	DescribePixelFormat(g_hDC, format, sizeof pfd, &pfd);
	SetPixelFormat(g_hDC, format, &pfd);
	
	g_hGL = wglCreateContext(g_hDC);
	wglMakeCurrent(g_hDC, g_hGL);
	
	glViewport(0, 0, g_width, g_height);
	glClearColor(0, 0, 0, 0);
	
	glEnable(GL_DEPTH_TEST);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, (float) g_width / g_height, 1.0f, 50.0f);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.0f, 0.0f, 17.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	
	// Obsidian
	// SetMaterial(0.05375, 0.05, 0.06625, 0.18275, 0.17, 0.22525, 0.332741, 0.328634, 0.346435, 0.3);
	
	// Turquoise
	// SetMaterial(0.1, 0.18725, 0.1745, 0.396, 0.74151, 0.69102, 0.297254, 0.30829, 0.306678, 0.1);
	
	// Chrome
	//SetMaterial(0.25, 0.25, 0.25, 0.4, 0.4, 0.4, 0.774597, 0.774597, 0.774597, 0.6);
	
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	
	glSelectBuffer(SEL_BUF_SIZE, sel_buf);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	g_fontBase = glGenLists(256);
	
	font = CreateFont(-12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, 
			OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, 
			"Trebuchet MS");
	
	SelectObject(g_hDC, font);
	wglUseFontOutlines(g_hDC, 0, 255, g_fontBase, 0.0f, 0.2f,  
		WGL_FONT_POLYGONS, g_metrics); 
	DeleteObject(font);
}

void UnInitGL(void)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(g_hGL);
	ReleaseDC(g_hWnd, g_hDC);
}

float g_tRotation;
float g_tRotDir = 5;

float r[] = { 1, 0, 0, 1, 0, 1, 1 };
float g[] = { 0, 1, 0, 1, 1, 0, 1 };
float b[] = { 0, 0, 1, 0, 1, 1, 1 };

void RotatingText(const char *str, float size, unsigned char rainbow)
{
	float width = 0;
	int k, len = strlen(str);
	glListBase(g_fontBase);
	
	for(k = 0; k < len; k++)
		width += g_metrics[str[k]].gmfBlackBoxX;
	
	glPushMatrix();
	glEnable(GL_NORMALIZE);
	glScalef(size, size, size);
	
	glTranslatef(-width / 2, 0, -0.4f);
	for(k = 0; k < len; k++) {
		if(rainbow)
			glColor3f(r[k % 6], g[k % 6], b[k % 6]);
			
		glPushMatrix();
			glRotatef(sin(g_tRotation / 9) * 20, 1, 0, 0);
			glRotatef(cos(g_tRotation) * 30, 0, 1, 0);
			glTranslatef(-g_metrics[str[k]].gmfCellIncX / 2.0f, 0, 0.1);
			if(str[k] == 'i') // TODO fix this properly?! font metrics for 'i' is wrong
				glTranslatef(-0.1, 0, 0);
			glCallList(g_fontBase + str[k]);
		glPopMatrix();
		glTranslatef(g_metrics[str[k]].gmfCellIncX, 0, 0);
	}
	glPopMatrix();
	glDisable(GL_NORMALIZE);
	
	g_tRotation += M_PI / 12.0f;
}

void CenteredText(const char *str, float size)
{
	float width = 0;
	int k, len = strlen(str);
	glListBase(g_fontBase);
	
	for(k = 0; k < len; k++)
		width += g_metrics[str[k]].gmfCellIncX;
	
	glPushMatrix();
	glEnable(GL_NORMALIZE);
	glScalef(size, size, size);
	
	glTranslatef(-width / 2, 0, 0);
	
	glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
	
	glPopMatrix();
	glDisable(GL_NORMALIZE);
}

void Render(void)
{
	glPushMatrix();
		//glTranslatef(0, 0, -12.0f);
		glRotatef(g_pitch, 1, 0, 0);
		glRotatef(g_yaw, 0, 1, 0);
		
		glColor3f(0.5f, 0.5f, 0.5f);
		
		glPushMatrix();
			GridSet();
		glPopMatrix();
		
		glPushMatrix();
			glRotatef(90, 0, 0, 1);
			GridSet();
		glPopMatrix();
		
		glPushMatrix();
			glRotatef(90, 1, 0, 0);
			GridSet();
		glPopMatrix();
		
		int x, y, z;
		for(z = 0; z <= 2; z++) {
			for(y = 0; y <= 2; y++) {
				for(x = 0; x <= 2; x++) {
					glPushMatrix();
						glTranslatef((x - 1) * 4, (y - 1) * 4, (z - 1) * 4);
						glRotatef(-g_yaw, 0, 1, 0);
						glRotatef(-g_pitch, 1, 0, 0);
						if(board[x][y][z] == 1) {
							glColor3f(0.8f, 0, 0);
							DrawX();
						} else if(board[x][y][z] == 2) {
							glColor3f(0, 0, 0.8f);
							Sphere(2, 32);
						}
					glPopMatrix();
				}
			}
		}
		
		glPushMatrix();
			glTranslatef((g_hits[g_hitIndex][2] - 1) * 4, (g_hits[g_hitIndex][1] - 1) * 4, (g_hits[g_hitIndex][0] - 1) * 4);
			if(turn == 1) {
				glColor4f(0.8f, 0, 0, 0.5f);
				DrawX();
			} else {
				glColor4f(0, 0, 0.8f, 0.5f);
				Sphere(2, 32);
			}
		glPopMatrix();
	glPopMatrix();
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
		glLoadIdentity();
		glOrtho(0, g_width, 0, g_height, 0, 100);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
			glLoadIdentity();
			
			if(!game_started || winner != 0) {
				glPushMatrix();
					glTranslatef(0, 0, -90);
					glColor4f(0, 0, 0, 0.75f);
					glNormal3f(0, 0, 1);
					glRecti(0, 0, g_width, g_height);
				glPopMatrix();
			}
			
			if(!game_started) {
				glTranslatef(g_width / 2, g_height / 2 + 100, 0);
		
				RotatingText("Tic Tac Twist!", 72.0f, 1);
				glTranslatef(0, -50, 0);
				glColor3f(1, 1, 1);
				CenteredText("Created by Matt Laux", 20);
				
				glTranslatef(0, -100, 0);
				CenteredText("Press enter to play!", 36);
				glTranslatef(0, -50, 0);
				CenteredText("Right mouse button drag = rotate", 36);
				glTranslatef(0, -50, 0);
				CenteredText("Left mouse click = place piece", 36);
			} else if(winner != 0) {
				char str[64];
				sprintf(str, "%c wins!", move_to_ascii[winner]);
				if(winner == 1) {
					glColor3f(1, 0, 0);
				} else if(winner == 2) {
					glColor3f(0, 0, 1);
				}
				glTranslatef(g_width / 2, g_height / 2, 0);
				RotatingText(str, 150.0f, 0);
				
				glTranslatef(0, -100, 0);
				CenteredText("Press enter to restart", 36);
			}
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
	
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	RECT rect;
	DWORD start, end, sleep_time;
	DWORD frames = 0, last_fps_time = 0;

	WNDCLASSEX wc = { 0 };

	wc.style = CS_OWNDC;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = CLASSNAME;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wc.hIcon = wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	RegisterClassEx(&wc);
	
	rect.left = rect.top = 0;
	rect.right = g_width;
	rect.bottom = g_height;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	g_hWnd = CreateWindow(CLASSNAME, "Tic Tac Twist", WS_OVERLAPPEDWINDOW, 
			GetSystemMetrics(SM_CXSCREEN) / 2 - g_width / 2,
			GetSystemMetrics(SM_CYSCREEN) / 2 - g_height / 2,
			rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInst, NULL);

	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);
	
	InitGL();
	
	while(1) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if(msg.message == WM_QUIT)
				break;
				
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}
		
		start = GetTickCount();
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		Render();
		SwapBuffers(g_hDC);
		
		frames++;
		
		end = GetTickCount();
		if(last_fps_time == 0)
			last_fps_time = end;
		
		if(end - last_fps_time > 1000) {
			printf("%d fps\n", frames);
			last_fps_time = end;
			frames = 0;
		}
		
		sleep_time = 16;
		
		/*sleep_time = 16 - (end - start);
		if((signed) sleep_time < 0) {
			sleep_time = 0;
		}*/
		Sleep(sleep_time);
	}
	
	UnInitGL();

	return msg.wParam;
}