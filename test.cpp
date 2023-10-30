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

using namespace std;

const int WIDTH = 1024;
const int HEIGHT = 768;
const float VOLUME = 0.1;
const int NUM_STARS = 50;
const int MAX_TRAIL_PARTICLES = 1000;
const double PI = 3.1415927;

double randomInRange(double lo, double hi) {
    return lo + static_cast<double>(rand()) / static_cast<double>(RAND_MAX) * (hi - lo);
}

class Color {
    // ... [No changes here, the original Color class code]
};

class TrailParticle {
    // ... [No changes here, the original TrailParticle class code]
};

class Particle {
    // ... [No changes here, the original Particle class code]
};

class Firework {
private:
    Particle mainParticle;
    vector<Particle> burstParticles;
    YsSoundPlayer &player;
    YsSoundPlayer::SoundData burstSound;

    // ... [rest of the private members]

public:
    Firework(double startX, double startY, double startVx, double startVy,
             float r, float g, float b, YsSoundPlayer &player)
        : r(r), g(g), b(b), player(player),
          mainParticle(startX, startY, startVx, startVy, r, g, b) {
        burstSound.LoadWav("burst.wav");
        player.SetVolume(burstSound, VOLUME);
    }

    // ... [rest of the Firework methods, no changes]
};

class Star {
    // ... [No changes here, the original Star class code]
};

class Game {
private:
    vector<Firework*> fireworks;
    vector<Star> stars;
    vector<Color> fireworkColors;
    YsSoundPlayer player;
    YsSoundPlayer::SoundData hissSound;
    int timeElapsed = 0;

    // ... [rest of the private methods]

public:
    Game() {
        // ... [rest of the Game constructor]
    }

    ~Game() {
        for (auto &firework : fireworks) {
            delete firework;
        }
    }

    // ... [rest of the Game methods]
};

int main(void) {
    // ... [No changes here, the original main function code]
}
