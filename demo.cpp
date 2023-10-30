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
const float VOLUME = 0.1;
const int NUM_STARS = 50;
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
  vector<Particle> secondaryParticles;
  bool hasSecondaryBurst = false;
  bool shouldBurstTwice = false;
  int timeSincePrimaryBurst = 0;

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

  void ScheduleBurst() { shouldBurstTwice = true; }

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

    if (shouldBurstTwice) {
      UpdateSecondaryBurst();
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

  void UpdateSecondaryBurst() {
    if (!hasSecondaryBurst && timeSincePrimaryBurst >= 60) {
      // Create secondary particles
      const int numParticles = 6; // Modify as needed
      double angleBetweenParticles = 2 * PI / numParticles;
      const double initialSpeed = 0.5;

      for (int i = 0; i < numParticles; ++i) {
        double angle = i * angleBetweenParticles;
        double vx = initialSpeed * cos(angle);
        double vy = initialSpeed * sin(angle);

        Particle secondary(x, y, vx, vy, r, g, b, true);
        secondaryParticles.push_back(secondary);
      }
      hasSecondaryBurst = true;
    }
    for (auto &secondary : secondaryParticles) {
      secondary.Update();
    }
    timeSincePrimaryBurst++;
  }

  void Fade(int repeat = 1) {
    for (int i = 0; i < repeat; ++i) {
      a -= fadeSpeed;
    }
  }

  void drawSecondary() {
    for (auto &secondary : secondaryParticles) {
      secondary.draw();
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
  YsSoundPlayer &player;
  YsSoundPlayer::SoundData burstSound;

  int particleTimer = 0;
  bool hasBurst = false;
  bool shouldBurstTwice = false;
  bool secondaryBurstDone = false;
  int timeSinceBurst = 0;
  float r, g, b;

public:
  Firework(double startX, double startY, double startVx, double startVy,
           float r, float g, float b, YsSoundPlayer &player,
           bool shouldBurstTwice = false)
      : r(r), g(g), b(b), player(player), shouldBurstTwice(shouldBurstTwice),
        mainParticle(startX, startY, startVx, startVy, r, g, b) {
    burstSound.LoadWav("burst.wav");
    player.SetVolume(burstSound, VOLUME);
  }

  void Update() {
    mainParticle.Update();
    if (!hasBurst && mainParticle.getVy() >= 0) {
      player.PlayOneShot(burstSound);
      int choice = rand() % 4;
      switch (choice) {
      case 0:
        cout << "normal" << endl;
        Burst(); // normal
        break;
      case 1:
        cout << "chrysanthemum" << endl;
        Burst(); // chrysanthemum
        hasBurst = false;
        break;
      case 2:
        cout << "spiral" << endl;
        BurstSpiral(); // spiral
        break;
      case 3:
        cout << "delayed second" << endl;
        BurstTwice(); // spiral
        break;
      }
      hasBurst = true;
    }
    for (auto &particle : burstParticles) {
      particle.Update();
      if (shouldBurstTwice) {
        particle.UpdateSecondaryBurst();
      }
    }

    // Handle post-burst logic like removing dead particles
    particleTimer++;
  }

  void BurstParticles(int numParticles, double initialSpeed) {
    double angleBetweenParticles = 2 * PI / numParticles;
    int angleRandomness = 2 * PI / 90; // 10 degrees randomness

    float burstR =
        r + (1.0f - r) * 0.5f; // Brightness adjustment can be customized
    float burstG = g + (1.0f - g) * 0.5f;
    float burstB = b + (1.0f - b) * 0.5f;

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

  void Burst() {
    BurstParticles(12, 1.0); // 12 particles with speed 1.0
  }

  void BurstTwice() {
    Burst();
    for (auto &particle : burstParticles) {
      particle.ScheduleBurst();
    }
  }

  void BurstSpiral() {
    const int numParticles = 12; // Increase the number for more density
    const double initialSpeed = 1.0;
    double angleBetweenParticles = 2 * PI / numParticles;
    int angleRandomness = 2 * PI / 90; // 10 degrees randomness

    for (int i = 0; i < numParticles; ++i) {
      double angle = i * angleBetweenParticles +
                     (rand() % angleRandomness - angleRandomness / 2);

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

  void draw() {
    mainParticle.draw();
    for (auto &particle : burstParticles) {
      particle.draw();
      particle.drawSecondary();
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
  vector<Firework *> fireworks;
  vector<Star> stars;
  vector<Color> fireworkColors;
  YsSoundPlayer player;
  YsSoundPlayer::SoundData hissSound;
  int timeElapsed = 0;

  void addFireworkColors() {
    fireworkColors.push_back(Color(1.0f, 0.5f, 0.5f)); // Reddish
    fireworkColors.push_back(Color(0.5f, 1.0f, 0.5f)); // Greenish
    fireworkColors.push_back(Color(0.5f, 0.5f, 1.0f)); // Bluish
    fireworkColors.push_back(Color(1.0f, 0.5f, 1.0f)); // Purple
    fireworkColors.push_back(Color(1.0f, 1.0f, 0.5f)); // Yellow
  }

  void addRandomFireworks(int count) {
    fireworks.reserve(fireworks.size() + count);
    for (int i = 0; i < count; ++i) {
      double startX = randomInRange(0, WIDTH);
      double startVx =
          (startX < WIDTH / 2) ? randomInRange(0, 2) : randomInRange(-2, 0);
      int colorIndex = rand() % fireworkColors.size();
      Color color = fireworkColors[colorIndex];
      double startVy = randomInRange(-2.5, -3.5);
      fireworks.push_back(new Firework(startX, HEIGHT, startVx, startVy,
                                       color.getR(), color.getG(), color.getB(),
                                       player));
    }
  }

public:
  Game() {
    addFireworkColors();
    addRandomFireworks(4);
    stars.reserve(NUM_STARS);
    for (int i = 0; i < NUM_STARS; ++i) {
      stars.push_back(Star(randomInRange(0, WIDTH), randomInRange(0, HEIGHT),
                           randomInRange(0, 1)));
    }
    hissSound.LoadWav("hiss.wav");
    player.SetVolume(hissSound, VOLUME);
    player.Start();
    player.PlayOneShot(hissSound);
    cout << "Game initialized" << endl;
  }

  void update() {
    player.KeepPlaying();
    timeElapsed += 1;
    for (auto &star : stars) {
      star.Update();
    }
    for (auto &firework : fireworks) {
      (*firework).Update();
    }
    if (timeElapsed >= 200) {
      addRandomFireworks(2);
      timeElapsed = 0;
    }
  }

  void drawGradientBackground() {
    const float topR = 23.0f / 255.0f, topG = 53.0f / 255.0f,
                topB = 97.0f / 255.0f;
    const float bottomR = 7.0f / 255.0f, bottomG = 17.0f / 255.0f,
                bottomB = 50.0f / 255.0f;
    glBegin(GL_QUADS);
    glColor3f(topR, topG, topB);
    glVertex2i(0, 0);
    glVertex2i(WIDTH, 0);
    glColor3f(bottomR, bottomG, bottomB);
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
    duration<double> time_span = duration_cast<duration<double> >(end - start);
    elapsed += time_span.count();
    if (time_span < desiredFrameTime * 0.01) {
      std::this_thread::sleep_for(desiredFrameTime - time_span);
    }
  }
  return 0;
}
