/*
 * plot3d.c
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 *
 *      Beep(1000, 100);
 * Implements a 3d graphics pipeline.
 * This fast and simple engine is only capable of projecting isolated points.
 */

#define _GNU_SOURCE

#include "plot3d.h"
#include "view.h"

// Global variables

extern BITMAPINFO bmInfo;
extern HDC myCompatibleDC;
extern HBITMAP myBitmap;
extern HDC hdc;

// Constants

#define PARALLEL    0
#define PERSPECTIVE 1

extern HWND g_hBitmap;
extern HDC hdc;
extern HWND mode0_rad, mode1_rad, mode2_rad;
extern Cell *latt0;

boolean showAxes  = true;
unsigned long timer = 0;
unsigned long begin;

// Colors

char gridcolor;
char X, Y, Z;

// Markers

Cell *marks;
int nmark;

int yy[WIDTH];

boolean rebuild = true;

int driftx = 0;
int drifty = 0;
int driftz = 0;

boolean momentum, wavefront, mode0, mode1, mode2, track, cube, plane, lattice;

extern pthread_mutex_t mutex;
extern pthread_barrier_t barrier;
extern HWND front_chk, track_chk, p_chk, plane_chk, cube_chk, latt_chk;

View view;

float t1[][3] =
{
  { BOXMIN, BOXMIN, BOXMIN },
  { BOXMIN, BOXMIN, BOXMIN },
  { BOXMIN, BOXMIN, BOXMIN },

  { BOXMAX, BOXMIN, BOXMIN },
  { BOXMAX, BOXMIN, BOXMIN },

  { BOXMIN, BOXMAX, BOXMIN },
  { BOXMIN, BOXMAX, BOXMIN },

  { BOXMIN, BOXMIN, BOXMAX },
  { BOXMIN, BOXMIN, BOXMAX },

  { BOXMAX, BOXMAX, BOXMAX },
  { BOXMAX, BOXMAX, BOXMAX },
  { BOXMAX, BOXMAX, BOXMAX },
};

float t2[][3] =
{
  { BOXMAX, BOXMIN, BOXMIN },
  { BOXMIN, BOXMAX, BOXMIN },
  { BOXMIN, BOXMIN, BOXMAX },

  { BOXMAX, BOXMAX, BOXMIN },
  { BOXMAX, BOXMIN, BOXMAX },

  { BOXMAX, BOXMAX, BOXMIN },
  { BOXMIN, BOXMAX, BOXMAX },

  { BOXMAX, BOXMIN, BOXMAX },
  { BOXMIN, BOXMAX, BOXMAX },

  { BOXMIN, BOXMAX, BOXMAX },
  { BOXMAX, BOXMIN, BOXMAX },
  { BOXMAX, BOXMAX, BOXMIN },
};

void initPlot()
{
  initScreen();
  initEngine(DISTANCE);
  //
  X = RED;
  Y = LIME;
  Z = CYAN;
  //
  setBackground(BLK);
  gridcolor = NAVY;
  //
  marks = malloc(TRACEBUF * sizeof(Cell));
  clearBuffer();
  char *s;
  asprintf((char **)&s, "Wait, please...");
  vprints(350, 380, s);
  //
  SetDIBits(myCompatibleDC, myBitmap, 0, HEIGHT, pixels, &bmInfo, 0);
  BitBlt(hdc, 0, 0, WIDTH, HEIGHT, myCompatibleDC, 0, 0, SRCCOPY);
  SendMessage(g_hBitmap, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)myBitmap);
}

void drawGroundPlane()
{
  if (!plane)
    return;
  float p1[3], p2[3];
  p1[0] = -1000;
  p1[1] = -1000;
  p1[2] = 0;
  p2[0] = +1000;
  p2[1] = -1000;
  p2[2] = 0;
  for(int i = 0; i < 100; i++)
  {
    line3d(p1, p2, gridcolor);
    p1[1] += 20; p2[1] += 20;
  }
  p1[0] = -1000;
  p1[1] = -1000;
  p2[0] = -1000;
  p2[1] = +1000;
  for(int i = 0; i < 100; i++)
  {
    line3d(p1, p2, gridcolor);
    p1[0] += 20; p2[0] += 20;
  }
  p1[0] = 0;
  p1[1] = 0;
  p2[0] = 0;
  p2[1] = 0;
  p2[2] = 500;
  line3d(p1, p2, YELLOW);
}

void drawMark(Cell cell)
{
  if(cell.off > 0)
    return;
  int x0 = cell.off % SIDE;
  int z0 = cell.off / SIDE2;
  int y0 = cell.off / SIDE - SIDE * z0;

  int x = cell.off % SIDE;
  int z = cell.off / SIDE2;
  int y = cell.off / SIDE - SIDE * z;
  float p[3];
  p[0] = WIDE * (SIDE * (x0 + driftx) + x - DRIFT);
  p[1] = WIDE * (SIDE * (y0 + drifty) + y - DRIFT);
  p[2] = WIDE * (SIDE * (z0 + driftz) + z - DRIFT);
  float t1[3], t2[3];
  t1[0] = p[0];
  t1[1] = p[1];
  t1[2] = p[2];
  t2[0] = p[0];
  t2[1] = p[1];
  t2[2] = p[2];
  t1[0] += DEV;
  t2[0] -= DEV;
  line3d(t1, t2, ORANGE);
  t1[0] = p[0];
  t1[1] = p[1];
  t1[2] = p[2];
  t2[0] = p[0];
  t2[1] = p[1];
  t2[2] = p[2];
  t1[1] += DEV;
  t2[1] -= DEV;
  line3d(t1, t2, ORANGE);
  t1[0] = p[0];
  t1[1] = p[1];
  t1[2] = p[2];
  t2[0] = p[0];
  t2[1] = p[1];
  t2[2] = p[2];
  t1[2] += DEV;
  t2[2] -= DEV;
  line3d(t1, t2, ORANGE);
}

void drawMarks()
{
  Cell *ms = marks;
  for(int i = 0; i < nmark; i++)
  {
    drawMark(*ms);
    ms++;
  }
}

void putBlob(float xyz[3], int color)
{
  #define BLOB
  #ifdef BLOB
  putVoxel(xyz, color);
  xyz[0]++;
  putVoxel(xyz, color);
  xyz[0] -= 2;
  putVoxel(xyz, color);
  xyz[0]++;
  xyz[1]++;
  putVoxel(xyz, color);
  xyz[1] -= 2;
  putVoxel(xyz, color);
  xyz[1]++;
  xyz[2]++;
  putVoxel(xyz, color);
  xyz[2] -= 2;
  putVoxel(xyz, color);
  #else
  putVoxel(xyz, color);
  #endif
}

void drawCell(int t0[3], int t[3], Cell *cell)
{
  float xyz[3];
  xyz[0] = WIDE * (SIDE * (t0[0] + driftx) + t[0] - DRIFT);
  xyz[1] = WIDE * (SIDE * (t0[1] + drifty) + t[1] - DRIFT);
  xyz[2] = WIDE * (SIDE * (t0[2] + driftz) + t[2] - DRIFT);

  if(momentum && !ZERO(cell->p) && !ISSAT(cell->po))
    putBlob(xyz, CYAN);
  else if (wavefront && !ISSAT(cell->po) && BUSY(cell))
    putBlob(xyz, YELLOW);
  else if (lattice)
    putBlob(xyz, GRAY);
}

void drawEspacito(int t0[3], Cell *esp)
{
  int t[3];
  for(t[2] = 0; t[2] < SIDE; t[2]++)
    for(t[1] = 0; t[1] < SIDE; t[1]++)
      for(t[0] = 0; t[0] < SIDE; t[0]++)
      {
        drawCell(t0, t, esp);
        esp++;
      }
}

void drawModel()
{
  wavefront = SendMessage(front_chk, BM_GETCHECK, FRONT, 0);
  track     = SendMessage(track_chk, BM_GETCHECK, TRACK, 0);
  momentum  = SendMessage(p_chk,     BM_GETCHECK, MOMENTUM, 0);
  plane     = SendMessage(plane_chk, BM_GETCHECK, PLANE, 0);
  cube      = SendMessage(cube_chk,  BM_GETCHECK, CUBE, 0);
  lattice   = SendMessage(latt_chk,  BM_GETCHECK, LATTICE, 0);
  mode0     = SendMessage(mode0_rad, BM_GETCHECK, MODE0, 0);
  mode1     = SendMessage(mode1_rad, BM_GETCHECK, MODE1, 0);
  mode2     = SendMessage(mode2_rad, BM_GETCHECK, MODE2, 0);

  int t[3];
  Cell *espacito = latt0;
  for(t[2] = 0; t[2] < SIDE; t[2]++)
    for(t[1] = 0; t[1] < SIDE; t[1]++)
      for(t[0] = 0; t[0] < SIDE; t[0]++)
      {
        drawEspacito(t, espacito);
        espacito += SIDE3;
      }
  if (track)
    drawMarks();
}

void drawBox()
{
  if(cube)
    for(int i = 0; i < 12; i++)
      line3d(t1[i], t2[i], BOX);
}

void drawAxes()
{
  if(!showAxes)
    return;

  // Save status

  float position[3], direction[3], attitude[3];
  getCamera(position, direction, attitude);

  // Prepare matrix

  float tpos[3];
  tpos[0] = direction[0];
  tpos[1] = direction[1];
  tpos[2] = direction[2];
  normalize(tpos);
  scale3d(tpos, -30);
  setViewPort(0.0, 0.2, 0.0, 0.2);
  setWindow(-40, 40, -40, 40);
  setCamera(tpos, direction, attitude);
  newView2();
  newView3();

  float point[3];
  for(int i = 0; i < 35; i++)
  {
    double p = i;
    if(i < 30)
    {
      point[0] = p; point[1] = 0; point[2] = 0;
      plot(point, X);
      point[0] = p; point[1] = 0.3; point[2] = 0;
      plot(point, X);
      point[0] = p; point[1] = -0.3; point[2] = 0;
      plot(point, X);
      point[0] = p; point[1] = 0; point[2] = 0.3;
      plot(point, X);
      point[0] = p; point[1] = 0; point[2] = -0.3;
      plot(point, X);
      point[0] = 0; point[1] = p; point[2] = 0;
      plot(point, Y);
      point[0] = 0.3; point[1] = p; point[2] = 0;
      plot(point, Y);
      point[0] = -0.3; point[1] = p; point[2] = 0;
      plot(point, Y);
      point[0] = 0; point[1] = p; point[2] = 0.3;
      plot(point, Y);
      point[0] = 0; point[1] = p; point[2] = -0.3;
      plot(point, Y);
      point[0] = 0; point[1] = 0; point[2] = p;
      plot(point, Z);
      point[0] = 0; point[1] = 0.3; point[2] = p;
      plot(point, Z);
      point[0] = 0; point[1] = -0.3; point[2] = p;
      plot(point, Z);
      point[0] = 0.3; point[1] = 0; point[2] = p;
      plot(point, Z);
      point[0] = -0.3; point[1] = 0; point[2] = p;
      plot(point, Z);
    }
    else if(i == 34)
    {
      drawChar(p, 0, 0, X, 'x');
      drawChar(0, p, 0, Y, 'y');
      drawChar(0, -0.3, p, Z, 'z');
    }
  }

  // Restore standard configuration

  setViewPort(0.0, 1.0, 0.0, 1.0);
  setWindow(-WINDOW, WINDOW, -WINDOW, WINDOW);
  setCamera(position, direction, attitude);
}

void drawVectors()
{
  float p0[3];
  reset3d(p0);
  float position[3], direction[3], attitude[3];
  getCamera(position, direction, attitude);
  float di[3], at[3];
  at[0] = 0; at[1] = 0; at[2] = -1;
  di[0] = 1; di[1] = 0; di[2] = 0;
  setCamera(p0, di, at);
  setCamera(position, direction, attitude);
}

void update2d()
{
  char *s = NULL;
  asprintf(&s, "light: %lu tick: %lu", timer / LIGHT, timer);
  vprints(20, 20, s);
  asprintf(&s, "S: pause/resume");
  vprints(650, 740, s);
  setTextColor(ORANGE);
  if(isParallel())
    asprintf(&s, "PARALLEL");
  else
    asprintf(&s, "PERSPECTIVE");
  vprints(350, 740, s);
  setTextColor(WHT);
  if(stop)
  {
    asprintf(&s, "[PAUSED]");
    vprints(370, 395, s);
  }

  // Camera position

  float pos[3], dir[3], att[3];
  getCamera(pos, dir, att);
  asprintf(&s, "[%.0f, %.0f, %.0f]", pos[0], pos[1], pos[2]);
  int n = string_length(s);
  vprints(800-10*n, 150, s);
}

/**
 * Updates the screen continuously.
 */
void *DisplayLoop()
{
  pthread_detach(pthread_self());
  printf("Running...\n");
  //
  initPlot();
  flipBuffers();  // forces init of curr
  begin = GetTickCount64();
  pthread_barrier_wait(&barrier);
  while(true)
  {
    pthread_mutex_lock(&mutex);
    if(rebuild)
    {
      if(rebuild)
      {
        drawModel();
        flipBuffers();
      }

      // Voxels to pixels

      clearBuffer();
      setWindow(-WINDOW, WINDOW, -WINDOW, WINDOW);
      newView2();
      newProjection();
      newView3();
      update3d();

      drawBox();
      drawAxes();
      drawGroundPlane();
      //drawVectors();

      update2d();

      // Update screen

      SetDIBits(myCompatibleDC, myBitmap, 0, HEIGHT, pixels, &bmInfo, 0);
      BitBlt(hdc, 0, 0, WIDTH, HEIGHT, myCompatibleDC, 0, 0, SRCCOPY);
      //
      rebuild = false;
      pthread_mutex_unlock(&mutex);
      SendMessage(g_hBitmap, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)myBitmap);  // PATCH
    }
    else
    {
      pthread_mutex_unlock(&mutex);
      Sleep(80);
    }

    // Update elapsed time display.

    HWND hwnd = WindowFromDC(hdc);
    RECT rect;
    GetClientRect(hwnd, &rect);
    InvalidateRect(hwnd, &rect, TRUE);
  }
}

