#include "fssimplewindow.h"
#include "yssimplesound.h"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <math.h>
#include <queue>
#include <stdio.h>
#include <thread>
#include <time.h>
#include <tuple>
#include <vector>

// TODO Optimize deleting done with Particles
// TODO Audio integration
// TODO Random function helper

using namespace std;

const int WIDTH = 1024;
const int HEIGHT = 768;
const int MAX_TRAIL_PARTICLES = 1000;
const double PI = 3.1415927;

double randomInRange(double lo, double hi) {
  return lo + static_cast<double>(rand()) / static_cast<double>(RAND_MAX) *
                  (hi - lo);
}

class Color {
private:
  float r, g, b;

public:
  // Default constructor sets a white color
  Color() : r(1.0f), g(1.0f), b(1.0f) {}

  // Parameterized constructor for setting specific RGB values
  Color(float r, float g, float b) : r(r), g(g), b(b) {}

  // Getters for the RGB values
  float getR() const { return r; }
  float getG() const { return g; }
  float getB() const { return b; }

  // Setters for the RGB values
  void setR(float r) { this->r = r; }
  void setG(float g) { this->g = g; }
  void setB(float b) { this->b = b; }

  // Function to set the OpenGL color
  void use() const { glColor3f(r, g, b); }
};

class TrailParticle {
private:
  double x, y; // Position
  float r, g, b, a;

public:
  TrailParticle(double x, double y, float r, float g, float b, float a = 1)
      : x(x), y(y), r(r), g(g), b(b), a(a) {}

  double getX() const { return x; }

  double getY() const { return y; }

  double getAlpha() const { return a; }

  void Fade(int repeat = 1) {
    for (int i = 0; i < repeat; ++i) {
      if (rand() % 5 == 0) { // 1 in 5 chance to increase brightness
        a += 0.005;
        if (a > 1)
          a = 1; // Ensure alpha doesn't exceed 1
      } else {
        a -= 0.0025;
      }
    }
  }

  void UpdateColor() {
    // This is just an example; you can introduce more complex logic here
    r += 0.005;
    if (r > 1)
      r = 1;

    g -= 0.005;
    if (g < 0)
      g = 0;
  }

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
  double x, y;   // Position
  double vx, vy; // Velocity
  double ax = 0;
  double ay = 0.01; // Acceleration
  double decaySpeed;
  double fadeSpeed = 0.003;
  bool burst = false;
  bool hasBurst = false;

  deque<TrailParticle> trailParticles;

  // Color of the particle
  float r, g, b;
  float a = 1;

public:
  // Constructor to initialize a particle with given attributes
  Particle(double x, double y, double vx, double vy, float r, float g, float b,
           bool burst = false)
      : x(x), y(y), vx(vx), vy(vy), r(r), g(g), b(b), burst(burst) {
    decaySpeed = burst ? 9 : 7;
  }

  double getX() const { return x; }

  double getY() const { return y; }

  double getVx() const { return vx; }

  double getVy() const { return vy; }

  double getAlpha() const { return a; }

  // void

  // Update the particle's physics and visual properties
  void Update() {
    vx += ax;
    vy += ay;
    x += vx;
    y += vy;

    if (burst) {
      Fade(3);
    }

    if (!burst && !hasBurst && vy >= 0) {
      hasBurst = true;
    }

    if (burst || !hasBurst) {
      trailParticles.push_front(TrailParticle(x, y, r, g, b, a));
      if (trailParticles.size() > MAX_TRAIL_PARTICLES) {
        trailParticles.pop_front();
      }
    }

    for (auto &particle : trailParticles) {
      particle.Fade(decaySpeed);
      particle.UpdateColor();
    }

    while (!trailParticles.empty() && trailParticles.front().getAlpha() <= 0) {
      trailParticles.pop_front();
    }
  }

  void Fade(int repeat = 1) {
    for (int i = 0; i < repeat; ++i) {
      a -= fadeSpeed;
    }
  }

  void drawTrail() {
    for (auto &particle : trailParticles) {
      particle.draw();
    }
  }

  // draw the particle on the screen
  void draw() {
    glPointSize(2.5);
    glBegin(GL_POINTS);
    glColor4f(r, g, b, a);
    glVertex2d(x, y);
    glEnd();
    drawTrail();
  }
};

class Firework {
private:
  Particle mainParticle;           // The main particle before the burst
  vector<Particle> burstParticles; // Particles created during the burst
  YsSoundPlayer::SoundData hissSound, burstSound;
  YsSoundPlayer player;

  int particleTimer = 0;
  bool hasBurst = false;
  float r, g, b;

public:
  Firework(double startX, double startY, double startVx, double startVy,
           float r, float g, float b)
      : r(r), g(g), b(b),
        mainParticle(startX, startY, startVx, startVy, r, g, b) {
          hissSound.LoadWav("hiss.wav");
          burstSound.LoadWav("burst.wav");
        }

  void Update() {
    mainParticle.Update();
    if (!hasBurst && mainParticle.getVy() >= 0) {
      int choice =
          rand() % 1; // 0 for normal, 1 for spiral, 2 for chrysanthemum
      switch (choice) {
      case 0:
        Burst();
        break;
      case 1:
        Burst();
        hasBurst = false;
        break; // Reuse Burst() for initial chrysanthemum burst
      case 2:
        BurstSpiral();
        break;
      }
      hasBurst = true;
    }

    for (auto it = burstParticles.begin(); it != burstParticles.end();) {
      it->Update();
      ++it;
    }

    // Handle post-burst logic like removing dead particles
    particleTimer++;
  }

  void BurstSpiral() {
    const int numParticles = 50; // Increase the number for more density
    const double initialSpeed = 1.0;
    double angleBetweenParticles = 2 * PI / numParticles;

    for (int i = 0; i < numParticles; ++i) {
      double angle = i * angleBetweenParticles;

      // Radial velocities (outward)
      double vx = initialSpeed * cos(angle);
      double vy = initialSpeed * sin(angle);

      // Tangential velocities (perpendicular to radial, for rotation)
      double tvx = -initialSpeed * sin(angle);
      double tvy = initialSpeed * cos(angle);

      // Combine radial and tangential velocities for spiral effect
      vx += tvx * 0.5; // Adjust the 0.5 factor for more or less spiral
      vy += tvy * 0.5;

      Particle burstParticle(mainParticle.getX(), mainParticle.getY(), vx, vy,
                             r, g, b, true);
      burstParticles.push_back(burstParticle);
    }
  }

  void SecondaryBurst(double x, double y) {
    const int numParticles = 12;
    const double initialSpeed = 0.5;
    double angleBetweenParticles = 2 * PI / numParticles;

    for (int i = 0; i < numParticles; ++i) {
      double angle = i * angleBetweenParticles;
      double vx = initialSpeed * cos(angle);
      double vy = initialSpeed * sin(angle);
      Particle burstParticle(x, y, vx, vy, r, g, b, true);
      burstParticles.push_back(burstParticle);
    }
  }

  void Burst() {
    const int numParticles = 12;
    burstParticles.reserve(burstParticles.size() + numParticles);
    const double initialSpeed = 1.0; // Adjust this value based on your needs
    double angleBetweenParticles = 2 * PI / numParticles; // Angle in radians
    int angleRandomness = 2 * PI / 90; // 10 degrees randomness

    float burstR =
        r + (1.0f - r) * 0.5f; // This makes the red component 50% brighter
    float burstG = g + (1.0f - g) * 0.5f; // Similarly for green
    float burstB = b + (1.0f - b) * 0.5f; // And blue

    for (int i = 0; i < numParticles; ++i) {
      double angle = i * angleBetweenParticles +
                     (rand() % angleRandomness - angleRandomness / 2);
      double vx = initialSpeed * cos(angle);
      double vy = initialSpeed * sin(angle);

      Particle burstParticle(mainParticle.getX(), mainParticle.getY(), vx, vy,
                             burstR, burstG, burstB, true);
      burstParticles.push_back(burstParticle);
    }
  }

  void draw() {
    mainParticle.draw();
    for (auto &particle : burstParticles) {
      particle.draw();
    }
  }
};

class Star {
private:
  double x, y;        // Position of the star
  float brightness;   // Brightness of the star
  float twinkleSpeed; // How fast the star changes its brightness

public:
  Star(double x, double y, float brightness)
      : x(x), y(y), brightness(brightness) {
    twinkleSpeed = ((rand() % 20) / 1000.0f -
                    0.01f); // Random speed between -0.01 and 0.01
  }

  // Update brightness for twinkle effect
  void Update() {
    brightness += twinkleSpeed;
    if (brightness > 1.0f || brightness < 0.0f) {
      twinkleSpeed = -twinkleSpeed; // Reverse the direction when hitting bounds
    }
  }

  void Draw() const {
    glColor4f(1.0, 1.0, 1.0, brightness); // White color with given brightness
    glBegin(GL_POINTS);
    glVertex2d(x, y);
    glEnd();
  }
};

class Game {
private:
  vector<Firework> fireworks;
  vector<Star> stars;
  int timeElapsed = 0;

  vector<Color> fireworkColors = {
      Color(1.0f, 0.5f, 0.5f), // Reddish
      Color(0.5f, 1.0f, 0.5f), // Greenish
      Color(0.5f, 0.5f, 1.0f), // Bluish
      Color(1.0f, 0.5f, 1.0f), // Purple
      Color(1.0f, 1.0f, 0.5f)  // Yellow
  };

public:
  Game() {
    addRandomFireworks(4);
    stars.reserve(50);
    for (int i = 0; i < 50; ++i) {
      double x = rand() % WIDTH;  // Random x position
      double y = rand() % HEIGHT; // Random y position
      float brightness =
          (rand() % 100) / 100.0f; // Random brightness between 0 and 1
      stars.push_back(Star(x, y, brightness));
    }
  }

  void addRandomFireworks(int count) {
    fireworks.reserve(fireworks.size() + count);
    for (int i = 0; i < count; ++i) { // Creates 4 fireworks
      double startX =
          rand() % WIDTH; // Random x position within the canvas width

      double startVx;
      if (startX < WIDTH / 2) {
        // If on the left half, velocity is in range [0, 2]
        startVx = rand() % 181 / 100.0;
      } else {
        // If on the right half, velocity is in range [-2, 0]
        startVx = -(rand() % 181 / 100.0);
      }
      int colorIndex = rand() % fireworkColors.size();
      Color color = fireworkColors[colorIndex];

      double startVy =
          -2.5 -
          (rand() % 100 / 100.0); // Random velocity for y in range [-3, -4]
      fireworks.push_back(Firework(startX, HEIGHT, startVx, startVy,
                                   color.getR(), color.getG(), color.getB()));
    }
  }

  // updates the game and changes the state of the preview mark
  void update() {
    timeElapsed += 1;
    for (auto &star : stars) {
      star.Update();
    }

    for (auto it = fireworks.begin(); it != fireworks.end();) {
      it->Update();
      ++it;
    }
    if (timeElapsed >= 200) {
      addRandomFireworks(2);
      timeElapsed = 0;
    }
  }

  void drawGradientBackground() {
    // Top color (23, 53, 97)
    float topR = 23.0f / 255.0f;
    float topG = 53.0f / 255.0f;
    float topB = 97.0f / 255.0f;

    // Bottom color (7, 17, 50)
    float bottomR = 7.0f / 255.0f;
    float bottomG = 17.0f / 255.0f;
    float bottomB = 50.0f / 255.0f;

    glBegin(GL_QUADS);
    glColor3f(topR, topG, topB); // top color
    glVertex2i(0, 0);            // top-left corner
    glVertex2i(WIDTH, 0);        // top-right corner

    glColor3f(bottomR, bottomG, bottomB); // bottom color
    glVertex2i(WIDTH, HEIGHT);            // bottom-right corner
    glVertex2i(0, HEIGHT);                // bottom-left corner
    glEnd();
  }

  void draw() {
    drawGradientBackground();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (auto &firework : fireworks) {
      firework.draw();
    }
    for (auto &star : stars) {
      star.Draw();
    }
  }
};

int main(void) {
  using namespace std::chrono;
  srand(time(0));
  FsOpenWindow(0, 0, WIDTH, HEIGHT, 1);
  Game game;
  double elapsed = 0;
  while (true) {
    const milliseconds desiredFrameTime(1);
    high_resolution_clock::time_point start = high_resolution_clock::now();
    FsPollDevice();
    int key = FsInkey();
    if (FSKEY_ESC == key) {
      break;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    game.update();
    game.draw();

    FsSwapBuffers();
    FsSleep(10);

    high_resolution_clock::time_point end = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(end - start);
    elapsed += time_span.count();
    if (time_span < desiredFrameTime * 0.01) {
      // std::this_thread::sleep_for(desiredFrameTime - time_span);
    }
  }
  return 0;
}