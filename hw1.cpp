//NAME: Miguel Trigueros
//cs335 Spring 2015 hw1
//This program demonstrates the use of OpenGL and XWindows
//
//Assignment is to modify this program.
//You will follow along with your instructor.
//
//Elements to be learned in this lab...
//
//. general animation framework
//. animation loop
//. object definition and movement
//. collision detection
//. mouse/keyboard interaction
//. object constructor
//. coding style
//. defined constants
//. use of static variables
//. dynamic memory allocation
//. simple opengl components
//. git
//
//elements we will add to program...
//. Game constructor
//. multiple particles
//. gravity
//. collision detection
//. more objects
//
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
extern "C" {
	#include "fonts.h"
}
#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600

#define MAX_PARTICLES 50000
#define GRAVITY 0.1

//global vars for colors
int rr;
int gg;
int bb;

//Used to turn on and off
int showParticles=0;

//X Windows variables
Display *dpy;
Window win;
GLXContext glc;

//Structures
struct Vec 
{
	float x, y, z;
};

struct Shape 
{
	float width, height;
	float radius;
	Vec center;
};

struct Particle 
{
	Shape s;
	Vec velocity;
};

struct Game 
{
	Shape box[5];
	Shape circle;
	Particle particle[MAX_PARTICLES];
	int n;
	int count;
	struct timespec pTimer;
};

//Function prototypes
void initXWindows(void);
void init_opengl(void);
void cleanupXWindows(void);
void check_mouse(XEvent *e, Game *game);
int check_keys(XEvent *e, Game *game);
void movement(Game *game);
void render(Game *game);

int main(void)
{
	int done=0;
	srand(time(NULL));
	initXWindows();
	init_opengl();
	
	//declare game object
	Game game;
	game.n=0;
	game.count=0;
	
	//box1
	game.box[0].width = 100;
	game.box[0].height = 10;
	game.box[0].center.x = -125 + 5*65;
	game.box[0].center.y = 750 - 5*60;
	
	//box2
	game.box[1].width = 100;
	game.box[1].height = 10;
	game.box[1].center.x = -50 + 5*65;
	game.box[1].center.y = 675 - 5*60;
	
	//box3
	game.box[2].width = 100;
	game.box[2].height = 10;
	game.box[2].center.x = 25 + 5*65;
	game.box[2].center.y = 600 - 5*60;
	
	//box4
	game.box[3].width = 100;
	game.box[3].height = 10;
	game.box[3].center.x = 100 + 5*65;
	game.box[3].center.y = 525 - 5*60;
	
	//box5
	game.box[4].width = 100;
	game.box[4].height = 10;
	game.box[4].center.x = 175 + 5*65;
	game.box[4].center.y = 450 - 5*60;
	
	//init circle
	game.circle.center.x = 300 + 5*65;
	game.circle.center.y = 300 - 5*60;
	game.circle.radius = 50;
	//init random particle colors
	rr = -1;
	gg = -1;
	bb = -1;
	
	//start animation
	while(!done) {
		
		while(XPending(dpy)) {
			XEvent e;
			XNextEvent(dpy, &e);
			check_mouse(&e, &game);
			done = check_keys(&e, &game);
		}
		movement(&game);
		render(&game);
		glXSwapBuffers(dpy, win);
	}
	cleanupXWindows();
	cleanup_fonts();
	return 0;
}

void set_title(void)
{
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "335 Lab1   LMB for particle");
}

void cleanupXWindows(void) 
{
	//do not change
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

void initXWindows(void) 
{
	//do not change
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	int w=WINDOW_WIDTH, h=WINDOW_HEIGHT;
	dpy = XOpenDisplay(NULL);
	
	if (dpy == NULL) {
		std::cout << "\n\tcannot connect to X server\n" << std::endl;
		exit(EXIT_FAILURE);
	}
	
	Window root = DefaultRootWindow(dpy);
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	
	if(vi == NULL) {
		std::cout << "\n\tno appropriate visual found\n" << std::endl;
		exit(EXIT_FAILURE);
	} 
	
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
		ButtonPress | ButtonReleaseMask |
		PointerMotionMask |
		StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
			InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//
	//Set 2D mode (no perspective)
	glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -10, 1);
	//
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glDisable(GL_CULL_FACE);
	//
	//Set the screen background color
	glClearColor(0.1, 0.1, 0.1, 1.0);
	glEnable(GL_TEXTURE_2D);
	initialize_fonts();
}

void makeParticle(Game *game, int x, int y) 
{
	if (game->n >= MAX_PARTICLES) {
		return;
	}
	std::cout << "makeParticle() " << x << " " << y << std::endl;
	//position of particle
	Particle *p = &game->particle[game->n];
	p->s.center.x = x;
	p->s.center.y = y;
	// random directions
	int tempy = rand() % 201 + (-100);
	p->velocity.y = (double)tempy / 100.0;
	int tempx = rand() % 201 + (-100);
	p->velocity.x =  (double)tempx / 100.0;
	game->n++;
}

void check_mouse(XEvent *e, Game *game)
{
	static int savex = 0;
	static int savey = 0;
	static int n = 0;
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button was pressed
			savex = e->xbutton.x;
			savey = e->xbutton.y;
			if(game->n < MAX_PARTICLES) {
				int y = WINDOW_HEIGHT - e->xbutton.y;
				makeParticle(game, e->xbutton.x, y);
			}
		}
		if (e->xbutton.button==3) {
			//Right button was pressed
			return;
		}
	}
	//Did the mouse move?
	if (savex != e->xbutton.x || savey != e->xbutton.y) {
		savex = e->xbutton.x;
		savey = e->xbutton.y;
		if (++n < 10) {
			return;
		}
	}
}

int check_keys(XEvent *e, Game *game)
{
	//Was there input from the keyboard?
	if (e->type == KeyPress) {
		int key = XLookupKeysym(&e->xkey, 0);
		if (key == XK_Escape) {
			return 1;
		}
		if (key == XK_b) {
			showParticles ^= 1;
			return 0;
		}
	}
	//You may check other keys here.
	return 0;
}

void movement(Game *game)
{
	Particle *p;
	Shape *s;
	if (game->n <= 0) {
		return;
	}
	for(int i = 0; i < game->n; i++) {
		p = &game->particle[i];
		p->s.center.x += p->velocity.x;
		p->s.center.y += p->velocity.y;
		//gravity
		p->velocity.y -= .3;
		//check for collision with shapes...
		for(int j = 0; j < 5; j++) {
			s = &game->box[j];
			if(p->s.center.y >= s->center.y - (s->height) &&
					p->s.center.y <= s->center.y + (s->height) &&
					p->s.center.x >= s->center.x - (s->width) &&
					p->s.center.x <= s->center.x + (s->width)) {
				if(p->velocity.x < 0.0) {
					p->velocity.x *= p->velocity.x;
				}
				p->velocity.y *= -.5;
			}
			//check for off-screen
			if (p->s.center.y < 0.0) {
				std::cout << "off screen" << std::endl;
				game->particle[i] = game->particle[game->n-1];
				game->n--;
			}

		}
		//circle collision
		/*
		   double dist = sqrt(((game->circle.center.x - p->s.center.x) * 
		   (game->circle.center.x - p->s.center.x)) +
		   ((game->circle.center.y - p->s.center.y) * 
		   (game->circle.center.y - p->s.center.y)));
		   if(dist < (game->circle.radius * game->circle.radius)) {
		   	p->velocity.y *= -.5;
		   	p->velocity.x *= -.5;
		   }
		 */
	}
}

void render(Game *game)
{
	float w, h;
	glClear(GL_COLOR_BUFFER_BIT);
	
	//Get random colors 
	if(rr < 0 && gg < 0 && bb < 0) {
		rr = rand() % 256;
		gg = rand() % 256;
		bb = rand() % 256;
	}
	//Draw Box
	for(int i = 0; i < 5; i++) {
		Shape *s;	
		glColor3ub(rr,gg,bb);
		s = &game->box[i];
		glPushMatrix();
		glTranslatef(s->center.x, s->center.y, s->center.z);
		w = s->width;
		h = s->height;
		glBegin(GL_QUADS);
		glVertex2i(-w,-h);
		glVertex2i(-w, h);
		glVertex2i( w, h);
		glVertex2i( w,-h);
		glEnd();
		glPopMatrix();
	}
	// Text
	Rect r[5];
	r[0].left = 140;
	r[1].left = 240;
	r[2].left = 315;
	r[3].left = 380;
	r[4].left = 450;
	r[0].bot = 445;
	r[1].bot = 370;
	r[2].bot = 295;
	r[3].bot = 220;
	r[4].bot = 145;
	r[0].center = 0;
	r[1].center = 0;
	r[2].center = 0;
	r[3].center = 0;
	r[4].center = 0;
	//Draw Text
	ggprint8b(&r[0], 16, 0x00ff0000, "Requirements");
	ggprint8b(&r[1], 16, 0x00ff0000, "Design");
	ggprint8b(&r[2], 16, 0x00ff0000, "Coding");
	ggprint8b(&r[3], 16, 0x00ff0000, "Testing");
	ggprint8b(&r[4], 16, 0x00ff0000, "Maintenance");
	//draw circle
	/*
	   Shape *c;
	   glColor3ub(rr,gg,bb);
	   c = &game->circle;
	   glPushMatrix();
	 */
	
	//draw all particles here
	if(showParticles) {
		int count = 0;
		while(count < 50) {
			int y = WINDOW_HEIGHT - 75;
			int x = WINDOW_WIDTH - 600;
			makeParticle(game, x, y);
			count++;
		}
	}
	glPushMatrix();
	
	for(int i = 0; i < game->n; i++) {
		int r = rand() % 256;
		int g = rand() % 256;
		int b = rand() % 256;
		glColor3ub(r,g,b);
		Vec *c = &game->particle[i].s.center;
		w = 2;
		h = 2;
		glBegin(GL_QUADS);
		glVertex2i(c->x-w, c->y-h);
		glVertex2i(c->x-w, c->y+h);
		glVertex2i(c->x+w, c->y+h);
		glVertex2i(c->x+w, c->y-h);
		glEnd();
		glPopMatrix();
	}
}




