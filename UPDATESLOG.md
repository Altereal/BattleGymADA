This file contains a list of changes from version to version.
------------------
v0.1
The original version, presented last year for the show
------------------
v0.2
1. Button Structure
Two new fields have been added: textOffsetX and textOffsetY to individually shift the text of the buttons.

2. EnemyTank Structure
The attackLineType, targetLineX, and targetLoneY fields have been deleted.
The attack logic has been redesigned using path and state.

3. Shot logic
createBullet: Fixed the directions of the player's bullets (left/right).
createEnemyBullet: Fixed the directions of displacement of enemy bullets (left/right).
updateBullets: Reduced bullet speed (BULLET_SPEED = 0.4f).

4. Display system (rendering)
display: Added FPS calculation.
drawButton: Button colors have been changed, text centering has been improved, taking into account shifts.
drawEnemyTank: The color of the enemy tank has been changed, turns have been fixed.
drawField, drawWalls, drawInfoPanel, drawMenu: Updated colors to match the new palette.
drawGameObjects: Updated the colors of bricks, steel, and bases.
drawTank: The color of the player's tank has been changed.

5. Game completion screens
drawLoseScreen: The "Game Over" and "Restart" buttons have been added, and the translucent background has been removed.
drawWinScreen: Changed the background, adjusted the position of the text with glasses.
mouse: Added handling of the "Restart" and "Game Over" buttons on the lose screen.

6. Game logic and enemy AI
findPathToAttackLine: Improved coordinate conversion using roundf.
getDirectionToTarget: The logic of determining the direction (priority of axes) has been changed.
isAttackLine: The line of attack has been changed (y == 1 instead of y == 2).
updateEnemies: The motion logic has been completely redesigned:
A path is used to move towards the line of attack.
Direction and motion control has been simplified.
Added states: moving towards and being on the line of attack.
respawnEnemies: Fixed calculation of target positions using roundf.

7. Player control and movement
specialKeys: The player's movement step has been changed (1 cell instead of 2).
updateTank: The player's tank movement speed has been reduced.

8. Initialize and reset
the init game: The background color has been changed.
initGame: Added new buttons (gameOverButton, restartButton) with support for text shifts.
initTank: Improved tank positioning on the grid, removed the initial "jerk".

9. Time and victory
updateGameTime: The winning condition has been changed to gameScore >= 2000.

10. General improvements
Roundf functions are widely used to accurately transform coordinates.
Improved enemy path handling and movement.
The interface color scheme has been updated for a better visual experience.
------------------
