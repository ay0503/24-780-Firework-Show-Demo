# Fireworks and Stars Animation Demo

This demo provides a demo of a fireworks show using the class graphics and sound library.

## Features:

1. **Gradient Background**: A gradient background, starting from a dark blue at the top to a darker shade at the bottom representing the night sky.

2. **Randomly Positioned Stars**: Stars scattered across the window, which twinkle between dimmer and brighter shades.

3. **Dynamic Fireworks**:
   - Fireworks are fired from the bottom, each with its distinct color and initial velocity.
   - When it reaches the peak, the firework explodes into colorful burst particles.
   - There are various burst patterns: normal, chrysanthemum, spiral, and double burst with a delay.
   - Every particle leaves behind a colorful, gradient, fading trail.

4. **Engaging Sound Effects**: A hissing sound plays through the launch of fireworks, and a distinct popping sound marks the firework burst at the peak.

5. **Real-time Simulation**: Newtonian Physics of fireworks (position, velocity, and acceleration) are simulated. Particle effects, trails, and decay as well are simulated as well.

## Code Structure:

- **Constants and Helper Functions**: Initial setup with constants for window dimensions, sound volume, and a function for random value generation.

- **Color Class**: Manages RGBA colors and keeps it as an object to make passing it easier.

- **TrailParticle Class**: Represents the trail particles left behind by fireworks. Handles positioning, color, fading, and rendering. This is used in a double-ended queue to control old and new particles without the overhead of enqueueing and dequeueing simply to draw or update the particle.

- **Particle Class**: The main component of the fireworks, controlling the physics, color, trail effects, and burst patterns.

- **Firework Class**: The main class representing the firework. Manages primary and burst particles, sound effects, and different burst behaviors.

- **Star Class**: Simulates a twinkling star, managing position, brightness, and rendering.

- **Demo Class**: The main app manager, following MVC conventions with update and draw, managing vectors of fireworks and stars, background rendering, and the main game loop.

- **Main Loop**: Controls user keyboard input for the `esc` key. The main loop simply calls the update and draw for the demo object.