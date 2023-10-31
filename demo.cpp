#include "fssimplewindow.h"
#include "yssimplesound.h"
#include <cstdlib>
#include <iostream>
#include <math.h>
#include <queue>
#include <stdio.h>
#include <time.h>
#include <vector>
using namespace std;

// pattern constants
const int PATTERN_DEFAULT = 0;
const int PATTERN_CHRYSANTHEMUM = 1;
const int PATTERN_SPIRAL = 2;
const int PATTERN_TWICE = 3;

// other constants
const int WIDTH = 1024;
const int HEIGHT = 768;
const float VOLUME = 0.1;
const int NUM_STARS = 50;
const int MAX_TRAIL_PARTICLES = 1000;
const double PI = 3.1415927;

// custom randRange function that returns a random double in the range [lo, hi)
double randRange(double lo, double hi) {
  return lo + double(rand()) / double(RAND_MAX) * (hi - lo);
}

// color class to simplify passing colors as arguments
class Color {
private:
  float r, g, b, a; // color components

public:
  Color(float r, float g, float b, float a = 1) : r(r), g(g), b(b), a(a) {}

  float getR() const { return r; }
  float getG() const { return g; }
  float getB() const { return b; }
  float getA() const { return a; }

  void setR(float nr) { r = nr; }
  void setG(float ng) { g = ng; }
  void setB(float nb) { b = nb; }
  void setA(float na) { a = na; }

  void use() const { glColor4f(r, g, b, a); }
};

// trail particles are particles that only make up the trail of a main particle
class TrailParticle {
private:
  double x, y;      // position
  float r, g, b, a; // color

public:
  TrailParticle(double x, double y, Color color) : x(x), y(y) {
    r = color.getR();
    g = color.getG();
    b = color.getB();
    a = color.getA();
  }

  double getX() const { return x; }
  double getY() const { return y; }
  double getA() const { return a; }

  // they should fade the trail particles s that they disappear
  void fade(int repeat = 1) {
    for (int i = 0; i < repeat; ++i) {
      // 1 in 5 chance to increase brightness (flashing effect)
      if (rand() % 5 == 0) {
        a += 0.005;
        if (a > 1)
          a = 1;
      } else {
        a -= 0.0025;
      }
    }
  }

  // updates the color of a trail particle
  void updateColor() {
    r += 0.005;
    if (r > 1)
      r = 1;

    g -= 0.005;
    if (g < 0)
      g = 0;

    b += randRange(-0.005, 0.005); // add random change to blue component
    if (b > 1)
      b = 1;
    if (b < 0)
      b = 0;
  }

  // draws the trail particle
  void draw() const {
    glPointSize(2.5);
    glBegin(GL_POINTS);
    glColor4f(r, g, b, a);
    glVertex2d(x, y);
    glEnd();
  }
};

class Particle {
private:
  double x, y;              // position
  double vx, vy;            // velocity
  double ax = 0;            // x acceleration
  double ay = 0.01;         // y acceleration
  double at = 0.03;         // tangential acceleration
  double decaySpeed;        // number of times to fade for different particles
  double fadeSpeed = 0.003; // default increment for each fade call
  int burstPattern;         // burst pattern
  bool burst = false;       // whether particles are from a burst
  bool hasBurst = false;    // whether particles have burst yet

  deque<TrailParticle> trailParticles; // particles that make up the trail
  vector<Particle> secondaryParticles; // particles created from secondary burst
  bool hasSecondaryBurst = false;      // whether secondary burst has happened
  bool shouldBurstTwice = false; // whether the particle should burst twice
  int timeSincePrimaryBurst = 0; // time since primary burst

  Color color; // color of particle

public:
  Particle(double x, double y, double vx, double vy, Color color,
           bool burst = false, int burstPattern = PATTERN_DEFAULT)
      : x(x), y(y), vx(vx), vy(vy), color(color), burst(burst),
        burstPattern(burstPattern) {
    decaySpeed = burst ? 9 : 7;
  }

  double getX() const { return x; }
  double getY() const { return y; }
  double getVx() const { return vx; }
  double getVy() const { return vy; }

  void scheduleBurst() { shouldBurstTwice = true; }

  // update the particle's physics and visual properties
  void update() {
    vx += ax;
    vy += ay;

    if (burstPattern == PATTERN_SPIRAL) {
      vx += -at * sin(atan2(vy, vx));
      vy += at * cos(atan2(vy, vx));
    }

    x += vx;
    y += vy;

    if (burst) {
      fade(3);
    }

    if (!burst && !hasBurst && vy >= 0) {
      hasBurst = true;
    }

    if (shouldBurstTwice) {
      updateSecondaryBurst();
    }

    if (burst || !hasBurst) {
      trailParticles.push_front(TrailParticle(x, y, color));
      if (trailParticles.size() > MAX_TRAIL_PARTICLES) {
        trailParticles.pop_front();
      }
    }

    for (auto &particle : trailParticles) {
      particle.fade(decaySpeed);
      particle.updateColor();
    }

    while (!trailParticles.empty() && trailParticles.front().getA() <= 0) {
      trailParticles.pop_front();
    }
  }

  // creates a secondary burst when needed
  void updateSecondaryBurst() {
    if (!hasSecondaryBurst && timeSincePrimaryBurst >= 60) {
      // create secondary particles
      const int numParticles = 6;
      double dtheta = 2 * PI / numParticles;
      const double initialSpeed = 0.5;

      for (int i = 0; i < numParticles; ++i) {
        double angle = i * dtheta;
        double vx = initialSpeed * cos(angle);
        double vy = initialSpeed * sin(angle);

        secondaryParticles.push_back(Particle(x, y, vx, vy, color, true));
      }
      hasSecondaryBurst = true;
    }
    for (auto &secondary : secondaryParticles) {
      secondary.update();
    }
    timeSincePrimaryBurst++;
  }

  // fades the particle's color by fadeSpeed
  void fade(int repeat = 1) {
    for (int i = 0; i < repeat; ++i) {
      color.setA(color.getA() - fadeSpeed);
    }
  }

  // draws secondary particles
  void drawSecondary() {
    for (auto &secondary : secondaryParticles) {
      secondary.draw();
    }
  }

  // draws trail particles
  void drawTrail() {
    for (auto &particle : trailParticles) {
      particle.draw();
    }
  }

  // draw the particle on the screen
  void draw() {
    glPointSize(2.5);
    glBegin(GL_POINTS);
    color.use();
    glVertex2d(x, y);
    glEnd();
    drawTrail();
    drawSecondary();
  }
};

class Firework {
private:
  Particle mainParticle;               // The main particle before the burst
  vector<Particle> burstParticles;     // particles created from burst
  YsSoundPlayer &player;               // sound player for firework burst
  YsSoundPlayer::SoundData burstSound; // sound data for firework burst

  bool hasBurst = false;         // whether the firework has burst
  bool shouldBurstTwice = false; // whether the firework should burst twice
  Color color;                   // color of the firework

public:
  Firework(double startX, double startY, double startVx, double startVy,
           Color color, YsSoundPlayer &player, bool shouldBurstTwice = false)
      : color(color), player(player), shouldBurstTwice(shouldBurstTwice),
        mainParticle(startX, startY, startVx, startVy, color) {
    burstSound.LoadWav("burst.wav");
    player.SetVolume(burstSound, VOLUME);
  }

  // updates the firework's physics and visual properties
  void update() {
    mainParticle.update();
    if (!hasBurst && mainParticle.getVy() >= 0) {
      player.PlayOneShot(burstSound);
      int choice = rand() % 4;
      switch (choice) {
      case 0:
        burst(); // normal
        break;
      case 1:
        burst(); // chrysanthemum
        hasBurst = false;
        break;
      case 2:
        burstSpiral(); // spiral
        break;
      case 3:
        burstTwice(); // delayed twice
        break;
      }
      hasBurst = true;
    }
    for (auto &particle : burstParticles) {
      particle.update();
      if (shouldBurstTwice) {
        particle.updateSecondaryBurst();
      }
    }
  }

  // creates a burst with numParticles and initialSpeed
  void createBurst(int numParticles, double initialSpeed,
                   int burstPattern = PATTERN_DEFAULT) {
    double dtheta = 2 * PI / numParticles;
    int randomness = 2 * PI / 90; // 10 degrees randomness

    // these particles are brighter than the main particle
    float burstR = color.getR() + (1.0f - color.getR()) * 0.5f;
    float burstG = color.getG() + (1.0f - color.getG()) * 0.5f;
    float burstB = color.getB() + (1.0f - color.getB()) * 0.5f;

    // create burst particles
    for (int i = 0; i < numParticles; ++i) {
      double angle = i * dtheta + (rand() % randomness - randomness / 2);
      double vx = initialSpeed * cos(angle);
      double vy = initialSpeed * sin(angle);

      Particle burstParticle(mainParticle.getX(), mainParticle.getY(), vx, vy,
                             Color(burstR, burstG, burstB), true, burstPattern);
      burstParticles.push_back(burstParticle);
    }
  }

  // normal burst
  void burst() { createBurst(12, 1.0, PATTERN_DEFAULT); }

  void burstTwice() {
    createBurst(12, 1.0, PATTERN_TWICE);
    for (auto &particle : burstParticles) {
      particle.scheduleBurst();
    }
  }

  void burstSpiral() { createBurst(12, 1.0, PATTERN_SPIRAL); }

  void secondaryBurst(double x, double y) {
    const int numParticles = 12;
    const double initialSpeed = 0.5;
    double dtheta = 2 * PI / numParticles;
    int randomness = 2 * PI / 90; // 10 degrees randomness

    for (int i = 0; i < numParticles; ++i) {
      double angle = i * dtheta + (rand() % randomness - randomness / 2);
      double vx = initialSpeed * cos(angle);
      double vy = initialSpeed * sin(angle);
      Particle burstParticle(x, y, vx, vy, color, true);
      burstParticles.push_back(burstParticle);
    }
  }

  bool hasReachedBottom() const { return mainParticle.getY() > HEIGHT; }

  // draws the all particles of the firework
  void draw() {
    mainParticle.draw();
    for (auto &particle : burstParticles) {
      particle.draw();
    }
  }
};

class Star {
private:
  double x, y;        // position of star
  float a;            // brightness of star
  float twinkleSpeed; // speed of a oscillation

public:
  Star(double x, double y, float a) : x(x), y(y), a(a) {
    twinkleSpeed = randRange(-0.01, 0.01); // random speed between -0.01 and 0.01
  }

  // update a for twinkle effect
  void update() {
    a += twinkleSpeed;
    if (a > 1.0f || a < 0.0f) {
      twinkleSpeed *= -1;
    }
  }

  // draws the star
  void draw() const {
    glColor4f(1.0, 1.0, 1.0, a); // white color
    glBegin(GL_POINTS);
    glVertex2d(x, y);
    glEnd();
  }
};

class Demo {
private:
  vector<Firework *> fireworks;       // vector of all fireworks
  vector<Star> stars;                 // vector of all stars
  vector<Color> fireworkColors;       // vector of all firework colors
  YsSoundPlayer player;               // primary sound player for fireworks
  YsSoundPlayer::SoundData hissSound; // sound data for hissing sound
  int timeElapsed = 0;                // time elapsed to keep adding fireworks

  void addFireworkColors() {
    fireworkColors.push_back(Color(1.0f, 0.5f, 0.5f)); // reddish
    fireworkColors.push_back(Color(0.5f, 1.0f, 0.5f)); // greenish
    fireworkColors.push_back(Color(0.5f, 0.5f, 1.0f)); // bluish
    fireworkColors.push_back(Color(1.0f, 0.5f, 1.0f)); // purple
    fireworkColors.push_back(Color(1.0f, 1.0f, 0.5f)); // yellow
  }

  void addRandomFireworks(int count) {
    fireworks.reserve(fireworks.size() + count);
    for (int i = 0; i < count; ++i) {
      double x = randRange(WIDTH / 8, WIDTH * 7 / 8);
      double vx = (x < WIDTH / 2) ? randRange(0, 2) : randRange(-2, 0);
      double vy = randRange(-2.5, -3.5);
      Color color = fireworkColors[randRange(0, fireworkColors.size())];
      fireworks.push_back(new Firework(x, HEIGHT, vx, vy, color, player));
    }
  }

public:
  Demo() {
    addFireworkColors(); // initialize firework colors
    addRandomFireworks(6);
    stars.reserve(NUM_STARS); // reserve memory to avoid resizing overhead
    for (int i = 0; i < NUM_STARS; ++i) {
      stars.push_back(
          Star(randRange(0, WIDTH), randRange(0, HEIGHT), randRange(0, 1)));
    }
    // sound
    hissSound.LoadWav("hiss.wav");
    player.SetVolume(hissSound, VOLUME);
    player.Start();
    player.PlayOneShot(hissSound);
  }

  void update() {
    player.KeepPlaying();
    timeElapsed += 1;
    for (auto &star : stars) {
      star.update();
    }
    for (auto &firework : fireworks) {
      (*firework).update();
    }
    if (timeElapsed % 200 == 0) {
      addRandomFireworks(4);
    }
    for (auto firework = fireworks.begin(); firework != fireworks.end();) {
      if ((*firework)->hasReachedBottom()) {
        delete *firework;
        firework = fireworks.erase(firework);
      } else {
        ++firework;
      }
    }
  }

  void drawGradientBackground() {
    glBegin(GL_QUADS);
    // top color
    Color(23.0f / 255.0f, 53.0f / 255.0f, 97.0f / 255.0f).use();
    glVertex2i(0, 0);
    glVertex2i(WIDTH, 0);
    // bottom color
    Color(7.0f / 255.0f, 17.0f / 255.0f, 50.0f / 255.0f).use();
    glVertex2i(WIDTH, HEIGHT);
    glVertex2i(0, HEIGHT);
    glEnd();
  }

  void draw() {
    drawGradientBackground();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (auto &firework : fireworks) {
      (*firework).draw();
    }
    for (auto &star : stars) {
      star.draw();
    }
  }
};

int main(void) {
  srand(time(0));
  FsOpenWindow(0, 0, WIDTH, HEIGHT, 1);
  Demo app;
  while (true) { // main app loop
    FsPollDevice();
    if (FSKEY_ESC == FsInkey()) {
      break;
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    app.update();
    app.draw();
    FsSwapBuffers();
    FsSleep(10);
  }
  return 0;
}
