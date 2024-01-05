# Battleship Game in C

## Overview
This multi-platform Battleship game, developed in C, offers an engaging experience on both Windows and Linux. It features Player vs. Player (PvP) and Player vs. Computer (PvC) modes, with an intelligent, adaptive AI for challenging gameplay. The game utilizes the SDL library to create an immersive user interface.

## Gameplay
### Placement Phase
In the placement phase, players can select which ship they want to place, determine its orientation, and navigate restrictions such as forbidden placements (touching another ship or out of bounds) indicated by a red border. Players have the flexibility to change the position and orientation of currently placed ships. Additional options include clearing the board or randomly placing the ships.

Watch the placement phase in action:
![Placement Phase Demo](Assets/Demos/PlacementPhaseDemo.gif)

### Game Phase - Initial Stage
During the initial game phase, players view their board on the left and the enemy board on the right side. They can shoot at the enemy, with hits marked in red and misses (ocean hits) indicated with a white marker. The turn ends when the player misses a shot. Following the player's turn, the AI takes its turn, allowing players to see where the computer shoots.

See the initial game phase demo:
![Game Phase Demo 1](Assets/Demos/GamePhaseDemo1.gif)

### Game Phase - Advanced Stage
In a more advanced stage of the game, with multiple ships already destroyed, the video demonstrates the AI's strategy post hitting a player's ship. The AI attempts to locate the remaining parts of the ship instead of randomly attacking other cells, showing a sophisticated approach to gameplay.

Experience the advanced game phase:
![Game Phase Demo 2](Assets/Demos/GamePhaseDemo2.gif)

---

Enjoy the strategic depths of this Battleship game and test your skills against the AI or another player!
