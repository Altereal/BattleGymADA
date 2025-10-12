#define _CRT_SECURE_NO_WARNINGS
#include <GL/freeglut.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>


// Размеры окна
const int WINDOW_WIDTH = 480 + 160; // 480 (поле) + 160 (панель)
const int WINDOW_HEIGHT = 480;

// Размеры игрового поля в мировых координатах
const float FIELD_WIDTH = 18.0f;
const float FIELD_HEIGHT = 18.0f;

// Размеры информационной панели
const float PANEL_WIDTH = 6.0f;
const float PANEL_HEIGHT = 18.0f;

// Толщина стен
const float WALL_THICKNESS = 0.5f;

// Размер ячейки игрового поля
const float CELL_SIZE = 0.5f;
const int GRID_WIDTH = 36;   // 18 / 0.5 = 36
const int GRID_HEIGHT = 36;  // 18 / 0.5 = 36

//------------
// Типы игровых объектов
typedef enum {
    OBJECT_EMPTY = 0,     // Пустое пространство
    OBJECT_BRICK = 1,     // Разрушаемая стена (кирпич)
    OBJECT_STEEL = 2,     // Неразрушаемая стена (сталь)
    OBJECT_BASE = 4,      // База игрока
    OBJECT_BRICK_BASE = 5     // Кирпичи у базы игрока
} ObjectType;

// Структура игрового объекта
typedef struct {
    ObjectType type;      // Тип объекта
    int durability;       // Прочность (0-3)
    bool passable;        // Можно ли пройти
    bool destructible;    // Можно ли разрушить
} GameObject;

// Игровая сетка (матрица объектов)
GameObject gameGrid[36][36];
//------------


//------------
typedef struct {
    float x, y;           // Позиция центра танка
    int direction;         // 0: вверх, 1: вправо, 2: вниз, 3: влево
    bool isMoving;         // Флаг движения
    float moveProgress;    // Прогресс движения (0.0-1.0)
    int targetX, targetY;  // Целевая позиция в клетках
} Tank;

Tank playerTank;  // Танк игрока
//------------


//------------
typedef struct {
    float x, y;           // Позиция центра пули
    int direction;         // Направление (0-3 как у танка)
    bool active; // Активна ли пуля
    bool isEnemy; // Вражеская ли
} Bullet;

#define MAX_BULLETS 50     // Максимальное количество пуль
Bullet bullets[MAX_BULLETS]; // Массив пуль
int lastShotTime = 0;       // Время последнего выстрела (мс)
const int SHOT_DELAY = 100; // Задержка между выстрелами (1 секунда)
//------------


//------------
typedef struct {
    float x, y;           // Позиция центра
    int direction;         // Направление (0-3)
    bool active;           // Активен ли танк
    int lastShotTime;      // Время последнего выстрела
    // Поля для управления движением
    bool isMoving;
    float moveProgress;
    int targetX, targetY;  // Целевая позиция в клетках
    int pathStep;            // Текущий шаг в пути
    int pathLength;          // Длина пути
    int* path;               // Массив точек пути (x0,y0,x1,y1,...)
    int moveDelay;           // Задержка между движениями
    int lastMoveTime;        // Время последнего движения
    int state; // 0: к линии атаки, 1: на линии атаки
} EnemyTank;


#define MAX_ENEMIES 5
EnemyTank enemies[MAX_ENEMIES];
int respawnDelay = 10000;  // Начальная задержка респавна (10 сек)
int lastRespawnTime = 0;   // Время последнего респавна
int enemiesSpawnCount = 1; // Количество врагов за респавн
int gameStartTime = 0;     // Время начала игры

// Позиции спавна (в мировых координатах)
float spawnPoints[5][2] = {
    {-7.5f, -4.0f},   // (2.5;9.5)
    {-7.5f, 7.0f},    // (2.5;31.5)
    {-4.5f, 7.0f},    // (8.5;31.5)
    {6.0f, 7.0f},     // (29.5;31.5)
    {4.5f, 3.0f}      // (26.5;23.5)
};
//------------


//------------
// Структура для узла поиска пути
typedef struct {
    int x, y;
    int parentX, parentY;
} PathNode;
//------------


//------------
// Состояния игры
typedef enum {
    GAME_MENU,
    GAME_PLAYING,
    GAME_WIN,
    GAME_LOSE
} GameState;

GameState gameState = GAME_MENU;

// Игровые параметры
int playerHP = 5;
int gameScore = 0;
int gameTime = 0; // В секундах
int fps = 0;                 // Текущее FPS, обновляем раз в ~секунду
int fpsFrameCount = 0;       // Счётчик кадров между обновлениями
int fpsLastTimeMs = 0;       // Время последнего обновления (ms)


// Кнопки меню
typedef struct {
    float x, y;
    float width, height;
    const char* text;
    float textOffsetX;   // новый: сдвиг текста по X
    float textOffsetY;   // новый: сдвиг текста по Y
} Button;

Button startButton;
Button exitButton;
Button okButton;
Button restartButton;  // новая
Button gameOverButton; // новая

// Флаги для определения, нажата ли кнопка
int mouseX, mouseY;
bool mouseLeftDown = false;
//-------------------------------------------------------------------------









// Сетка и недвижимые объекты
//--------------------------------------------------------------------------
// Функция для установки объекта в сетку
void setGridObject(int x, int y, ObjectType type, int durability, bool passable, bool destructible) {
    if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
        gameGrid[y][x].type = type;
        gameGrid[y][x].durability = durability;
        gameGrid[y][x].passable = passable;
        gameGrid[y][x].destructible = destructible;
    }
}

// Инициализация игрового поля на основе матрицы
void initGameGrid() {
    // Инициализируем всю сетку как пустые ячейки
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            setGridObject(x, y, OBJECT_EMPTY, 0, true, false);
        }
    }

    // Разрушаемые стены просто так
    for (int x = 5; x < 15; x++) {
        setGridObject(x, 27, OBJECT_BRICK, 3, false, true);
        setGridObject(x, 28, OBJECT_BRICK, 3, false, true);
    }
    for (int y = 27; y < 34; y++) {
        setGridObject(5, y, OBJECT_BRICK, 3, false, true);
        setGridObject(6, y, OBJECT_BRICK, 3, false, true);
    }
    for (int y = 5; y < 15; y++) {
        setGridObject(5, y, OBJECT_BRICK, 3, false, true);
        setGridObject(6, y, OBJECT_BRICK, 3, false, true);
    }

    for (int x = 24; x < 34; x++) {
        setGridObject(x, 28, OBJECT_BRICK, 3, false, true);
        setGridObject(x, 27, OBJECT_BRICK, 3, false, true);
    }
    for (int y = 19; y < 29; y++) {
        setGridObject(24, y, OBJECT_BRICK, 3, false, true);
        setGridObject(23, y, OBJECT_BRICK, 3, false, true);
    }

    for (int x = 19; x < 31; x++) {
        setGridObject(x, 20, OBJECT_BRICK, 3, false, true);
        setGridObject(x, 19, OBJECT_BRICK, 3, false, true);
    }
    for (int x = 1; x < 15; x++) {
        setGridObject(x, 20, OBJECT_BRICK, 3, false, true);
        setGridObject(x, 19, OBJECT_BRICK, 3, false, true);
    }

    for (int x = 27; x < 31; x++) {
        setGridObject(x, 12, OBJECT_BRICK, 3, false, true);
        setGridObject(x, 11, OBJECT_BRICK, 3, false, true);
    }
    for (int x = 11; x < 21; x++) {
        setGridObject(x, 12, OBJECT_BRICK, 3, false, true);
        setGridObject(x, 11, OBJECT_BRICK, 3, false, true);
    }

    // Неразрушаемые стены 
    for (int y = 0; y < 34; y++) {
        for (int x = 0; x < 34; x++) {
            if (x == 0 || x == 33 || y == 0 || y == 33) {
                setGridObject(x, y, OBJECT_STEEL, 0, false, false);
            }
        }
    }

    // База игрока (в центре снизу)
    setGridObject(17, 2, OBJECT_BASE, 1, false, true);
    setGridObject(18, 2, OBJECT_BASE, 1, false, true);
    setGridObject(17, 1, OBJECT_BASE, 1, false, true);
    setGridObject(18, 1, OBJECT_BASE, 1, false, true);

    // Кирпичи вокруг базы
    setGridObject(15, 1, OBJECT_BRICK_BASE, 3, false, true);
    setGridObject(15, 2, OBJECT_BRICK_BASE, 3, false, true);
    setGridObject(15, 3, OBJECT_BRICK_BASE, 3, false, true);
    setGridObject(15, 4, OBJECT_BRICK_BASE, 3, false, true);

    setGridObject(16, 1, OBJECT_BRICK_BASE, 3, false, true);
    setGridObject(16, 2, OBJECT_BRICK_BASE, 3, false, true);
    setGridObject(16, 3, OBJECT_BRICK_BASE, 3, false, true);
    setGridObject(16, 4, OBJECT_BRICK_BASE, 3, false, true);

    setGridObject(17, 3, OBJECT_BRICK_BASE, 3, false, true);
    setGridObject(17, 4, OBJECT_BRICK_BASE, 3, false, true);

    setGridObject(18, 3, OBJECT_BRICK_BASE, 3, false, true);
    setGridObject(18, 4, OBJECT_BRICK_BASE, 3, false, true);

    setGridObject(19, 1, OBJECT_BRICK_BASE, 3, false, true);
    setGridObject(19, 2, OBJECT_BRICK_BASE, 3, false, true);
    setGridObject(19, 3, OBJECT_BRICK_BASE, 3, false, true);
    setGridObject(19, 4, OBJECT_BRICK_BASE, 3, false, true);

    setGridObject(20, 1, OBJECT_BRICK_BASE, 3, false, true);
    setGridObject(20, 2, OBJECT_BRICK_BASE, 3, false, true);
    setGridObject(20, 3, OBJECT_BRICK_BASE, 3, false, true);
    setGridObject(20, 4, OBJECT_BRICK_BASE, 3, false, true);
}
//--------------------------------------------------------------------------












// Блок танка игрока
//--------------------------------------------------------------------------
void initTank() {
    playerTank.x = -3.0f;       // Стартовая позиция X
    playerTank.y = -7.0f;       // Стартовая позиция Y
    playerTank.direction = 0;  // Смотрит вверх
    playerTank.isMoving = false;
    playerTank.moveProgress = 0.0f;
    // Округляем к ближайшему центру сетки (центры лежат на -8.0 + n * CELL_SIZE)
    playerTank.targetX = (int)roundf((playerTank.x + 8.0f) / CELL_SIZE);
    playerTank.targetY = (int)roundf((playerTank.y + 8.0f) / CELL_SIZE);

    // Жёстко ставим танк в центр вычисленной клетки — так не будет стартового рывка
    playerTank.x = -8.0f + playerTank.targetX * CELL_SIZE;
    playerTank.y = -8.0f + playerTank.targetY * CELL_SIZE;
}

bool canMoveTo(int gridX, int gridY) {
    // Проверяем все 4 клетки под танком (2x2)
    for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
            int checkX = gridX + dx;
            int checkY = gridY + dy;

            // Проверка выхода за границы
            if (checkX < 0 || checkX >= GRID_WIDTH || checkY < 0 || checkY >= GRID_HEIGHT)
                return false;

            // Проверка проходимости клетки
            if (!gameGrid[checkY][checkX].passable)
                return false;
        }
    }
    return true;
}

void updateTank() {
    if (playerTank.isMoving) {
        playerTank.moveProgress += 0.020f;  // Скорость движения

        if (playerTank.moveProgress >= 1.0f) {
            playerTank.isMoving = false;
            playerTank.moveProgress = 1.0f;
        }

        // Рассчет новой позиции
        float targetWorldX = -8.5f + playerTank.targetX * 0.5f + 0.5f;
        float targetWorldY = -8.5f + playerTank.targetY * 0.5f + 0.5f;

        playerTank.x = playerTank.x + (targetWorldX - playerTank.x) * playerTank.moveProgress;
        playerTank.y = playerTank.y + (targetWorldY - playerTank.y) * playerTank.moveProgress;
    }
}

void drawTank() {
    glPushMatrix();
    glTranslatef(playerTank.x, playerTank.y, 0.0f);

    // Поворот в зависимости от направления
    switch (playerTank.direction) {
    case 0: break;  // Вверх
    case 1: glRotatef(90.0f, 0, 0, 1); break;
    case 2: glRotatef(180.0f, 0, 0, 1); break;
    case 3: glRotatef(270.0f, 0, 0, 1); break;
    }

    glColor3f(0.69f, 0.77f, 0.87f);  // Новый цвет игрока (#B0C4DE)


    // Корпус (основной прямоугольник)
    glBegin(GL_QUADS);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f(0.5f, -0.5f);
    glVertex2f(0.5f, 0.5f);
    glVertex2f(-0.5f, 0.5f);
    glEnd();

    // Пушка
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(-0.05f, 0.0f);
    glVertex2f(0.05f, 0.0f);
    glVertex2f(0.05f, 0.6f);
    glVertex2f(-0.05f, 0.6f);
    glEnd();

    // Гусеницы
    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_QUADS);
    // Левая
    glVertex2f(-0.6f, -0.4f);
    glVertex2f(-0.4f, -0.4f);
    glVertex2f(-0.4f, 0.4f);
    glVertex2f(-0.6f, 0.4f);

    // Правая
    glVertex2f(0.4f, -0.4f);
    glVertex2f(0.6f, -0.4f);
    glVertex2f(0.6f, 0.4f);
    glVertex2f(0.4f, 0.4f);
    glEnd();

    glPopMatrix();
}
//--------------------------------------------------------------------------







// BFS
//--------------------------------------------------------------------------
// Очередь для BFS
PathNode pathQueue[1296];
int queueStart = 0;
int queueEnd = 0;

// Проверка, является ли точка линией атаки
bool isAttackLine(int x, int y) {
    return (y == 1) || (x == 17) || (x == 18);
}

// Поиск кратчайшего пути до ближайшей линии атаки
void findPathToAttackLine(EnemyTank* tank) {
    // Очищаем предыдущий путь
    if (tank->path) {
        free(tank->path);
        tank->path = NULL;
    }

    // Текущая позиция в сетке
    int startX = (int)roundf((tank->x + 8.0f) / 0.5f);
    int startY = (int)roundf((tank->y + 8.0f) / 0.5f);

    // Целевые точки атаки
    int targets[3][2] = { {17, 1}, {18, 1}, {startX, 1} };
    PathNode bestTarget = { -1, -1, -1, -1 };
    bool found = false;
    int shortestPath = INT_MAX;

    for (int t = 0; t < 3; t++) {
        int targetX = targets[t][0];
        int targetY = targets[t][1];

        // Массив посещенных точек
        bool visited[36][36] = { false };

        // Очередь инициализации
        queueStart = 0;
        queueEnd = 0;

        // Добавляем стартовую точку
        pathQueue[queueEnd++] = (PathNode){ startX, startY, -1, -1 };
        visited[startY][startX] = true;

        PathNode targetNode = { -1, -1, -1, -1 };
        bool pathFound = false;

        while (queueStart < queueEnd) {
            PathNode current = pathQueue[queueStart++];

            // Проверяем, достигли ли мы цели
            if (current.x == targetX && current.y == targetY) {
                targetNode = current;
                pathFound = true;
                break;
            }

            // Проверяем соседние клетки
            int dx[4] = { 0, 0, -1, 1 };
            int dy[4] = { -1, 1, 0, 0 };

            for (int i = 0; i < 4; i++) {
                int nx = current.x + dx[i];
                int ny = current.y + dy[i];

                // Проверяем границы
                if (nx < 0 || nx >= GRID_WIDTH || ny < 0 || ny >= GRID_HEIGHT)
                    continue;

                // Проверяем, можно ли пройти
                if (!visited[ny][nx] && canMoveTo(nx, ny)) {
                    visited[ny][nx] = true;
                    pathQueue[queueEnd++] = (PathNode){ nx, ny, current.x, current.y };
                }
            }
        }

        // Если путь найден, проверяем длину
        if (pathFound) {
            int length = 0;
            PathNode node = targetNode;
            while (node.parentX != -1) {
                length++;
                for (int i = 0; i < queueEnd; i++) {
                    if (pathQueue[i].x == node.parentX && pathQueue[i].y == node.parentY) {
                        node = pathQueue[i];
                        break;
                    }
                }
            }

            if (length < shortestPath) {
                shortestPath = length;
                bestTarget = targetNode;
                found = true;
            }
        }
    }

    // Если путь найден, сохраняем его
    if (found) {
        int length = shortestPath;
        tank->pathLength = length;
        tank->path = malloc(length * 2 * sizeof(int));
        tank->pathStep = 0;

        PathNode node = bestTarget;
        int index = length - 1;
        while (node.parentX != -1 && index >= 0) {
            tank->path[index * 2] = node.x;
            tank->path[index * 2 + 1] = node.y;
            index--;

            for (int i = 0; i < queueEnd; i++) {
                if (pathQueue[i].x == node.parentX && pathQueue[i].y == node.parentY) {
                    node = pathQueue[i];
                    break;
                }
            }
        }

        // Устанавливаем первую цель пути
        if (tank->pathLength > 0) {
            tank->targetX = tank->path[0];
            tank->targetY = tank->path[1];
        }
    }
}
//--------------------------------------------------------------------------





// Обнова времени
//--------------------------------------------------------------------------
void updateGameTime() {
    if (gameState == GAME_PLAYING) {
        int currentTime = glutGet(GLUT_ELAPSED_TIME);
        gameTime = (currentTime - gameStartTime) / 1000;

        // Проверка победы
        if (gameScore >= 2000) {
            gameState = GAME_WIN;
        }
    }
}

void timerCallback(int value);

void timerCallback(int value) {
    updateGameTime();
    glutTimerFunc(1000, timerCallback, 0);
}
//--------------------------------------------------------------------------








// Блок танков врагов
//--------------------------------------------------------------------------
void initEnemies() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].active = false;
    }
    lastRespawnTime = glutGet(GLUT_ELAPSED_TIME);
    gameStartTime = lastRespawnTime;
}

void respawnEnemies() {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    int gameDuration = (currentTime - gameStartTime) / 1000;

    enemiesSpawnCount = 1 + gameDuration / 60;
    if (enemiesSpawnCount > 5) enemiesSpawnCount = 5;

    if (currentTime - lastRespawnTime < respawnDelay) return;

    int spawned = 0;
    for (int i = 0; i < MAX_ENEMIES && spawned < enemiesSpawnCount; i++) {
        if (!enemies[i].active) {
            int spawnIndex = rand() % 5;
            enemies[i].x = spawnPoints[spawnIndex][0];
            enemies[i].y = spawnPoints[spawnIndex][1];
            enemies[i].direction = 2;
            enemies[i].active = true;
            enemies[i].lastShotTime = currentTime;

            // Инициализация параметров движения
            enemies[i].isMoving = false;
            enemies[i].moveProgress = 0.0f;
            enemies[i].targetX = (int)roundf((enemies[i].x + 8.0f) / 0.5f);
            enemies[i].targetY = (int)roundf((enemies[i].y + 8.0f) / 0.5f);
            enemies[i].path = NULL;
            enemies[i].pathStep = 0;
            enemies[i].pathLength = 0;
            // Случайная задержка между движениями (500-1000 мс)
            enemies[i].moveDelay = 500 + rand() % 500;
            enemies[i].lastMoveTime = currentTime;

            // Находим путь к линии атаки
            findPathToAttackLine(&enemies[i]);

            spawned++;
        }
    }

    lastRespawnTime = currentTime;
}

int getDirectionToTarget(int currentX, int currentY, int targetX, int targetY) {
    int dx = targetX - currentX;
    int dy = targetY - currentY;

    // Приоритет движения по оси с наибольшей разницей
    if (abs(dx) > abs(dy)) {
        return (dx > 0) ? 1 : 3; // Вправо : Влево
    }
    else {
        return (dy > 0) ? 0 : 2; // Вверх : Вниз
    }
}

void drawEnemyTank(EnemyTank* tank) {
    glPushMatrix();
    glTranslatef(tank->x, tank->y, 0.0f);

    // Всегда используем актуальное направление из структуры
    switch (tank->direction) {
    case 0: break;
    case 1: glRotatef(90.0f, 0, 0, 1); break; // лево
    case 2: glRotatef(180.0f, 0, 0, 1); break; // низ
    case 3: glRotatef(-90.0f, 0, 0, 1); break; // право
    }

    glColor3f(1.0f, 0.85f, 0.73f);  // Новый цвет врага (#FFDAB9)


    // Корпус
    glBegin(GL_QUADS);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f(0.5f, -0.5f);
    glVertex2f(0.5f, 0.5f);
    glVertex2f(-0.5f, 0.5f);
    glEnd();

    // Пушка
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(-0.05f, 0.0f);
    glVertex2f(0.05f, 0.0f);
    glVertex2f(0.05f, 0.6f);
    glVertex2f(-0.05f, 0.6f);
    glEnd();

    // Гусеницы
    glColor3f(0.4f, 0.3f, 0.2f);
    glBegin(GL_QUADS);
    // Левая
    glVertex2f(-0.6f, -0.4f);
    glVertex2f(-0.4f, -0.4f);
    glVertex2f(-0.4f, 0.4f);
    glVertex2f(-0.6f, 0.4f);
    // Правая
    glVertex2f(0.4f, -0.4f);
    glVertex2f(0.6f, -0.4f);
    glVertex2f(0.6f, 0.4f);
    glVertex2f(0.4f, 0.4f);
    glEnd();

    glPopMatrix();
}

void drawEnemies() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            drawEnemyTank(&enemies[i]);
        }
    }
}
//--------------------------------------------------------------------------




// Блок работы пули
//--------------------------------------------------------------------------
void initBullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }
    lastShotTime = glutGet(GLUT_ELAPSED_TIME);
}

void createBullet() {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);

    // Проверяем задержку между выстрелами
    if (currentTime - lastShotTime < SHOT_DELAY) return;

    // Ищем свободный слот для пули
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = playerTank.x + (playerTank.direction - 2) % 2 * 0.5f;
            bullets[i].y = playerTank.y + (playerTank.direction - 1) % 2 * 0.5f;
            bullets[i].direction = playerTank.direction;
            bullets[i].active = true;
            bullets[i].isEnemy = false; // Помечаем как пулю игрока

            // Корректируем позицию в зависимости от направления
            switch (playerTank.direction) {
            case 0: bullets[i].y += 0.6f; break; // Вверх
            case 1: bullets[i].x += 0.6f; break; // Влево
            case 2: bullets[i].y -= 0.6f; break; // Вниз
            case 3: bullets[i].x -= 0.6f; break; // Вправо
            }

            lastShotTime = currentTime;
            return;
        }
    }
}

void createEnemyBullet(int enemyIndex) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = enemies[enemyIndex].x;
            bullets[i].y = enemies[enemyIndex].y;
            bullets[i].direction = enemies[enemyIndex].direction;
            bullets[i].active = true;
            bullets[i].isEnemy = true; // Помечаем как вражескую

            // Корректировка позиции
            switch (enemies[enemyIndex].direction) {
            case 0: bullets[i].y += 0.6f; break;
            case 1: bullets[i].x -= 0.6f; break;
            case 2: bullets[i].y -= 0.6f; break;
            case 3: bullets[i].x += 0.6f; break;
            }
            break;
        }
    }
}

void updateBullets() {
    const float BULLET_SPEED = 0.4f;

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;

        // Проверка попадания в игрока (только для вражеских пуль)
        if (bullets[i].isEnemy) {
            float distX = fabsf(bullets[i].x - playerTank.x);
            float distY = fabsf(bullets[i].y - playerTank.y);

            if (distX < 0.5f && distY < 0.5f) {
                bullets[i].active = false;
                // Обработка попадания в игрока
                playerHP--;

                // Проверка проигрыша по здоровью
                if (playerHP <= 0) {
                    gameState = GAME_LOSE;
                }
            }
        }
        // Проверка попадания во врагов (только для пуль игрока)
        if (!bullets[i].isEnemy) {
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (enemies[j].active) {
                    // Проверка столкновения с учетом размеров
                    float dx = fabsf(bullets[i].x - enemies[j].x);
                    float dy = fabsf(bullets[i].y - enemies[j].y);

                    // Размер танка: 1.0x1.0, размер пули: 0.3x0.3
                    if (dx < 0.65f && dy < 0.65f) {
                        bullets[i].active = false;
                        enemies[j].active = false;

                        // Начисление 50 очков за врага
                        gameScore += 50;

                        // Уменьшаем время респавна
                        respawnDelay -= 500;
                        if (respawnDelay < 1000) respawnDelay = 1000;
                        break;
                    }
                }
            }
        }

        // Двигаем пулю в зависимости от направления
        switch (bullets[i].direction) {
        case 0: bullets[i].y += BULLET_SPEED * 0.05f; break;
        case 1: bullets[i].x -= BULLET_SPEED * 0.05f; break;
        case 2: bullets[i].y -= BULLET_SPEED * 0.05f; break;
        case 3: bullets[i].x += BULLET_SPEED * 0.05f; break;
        }

        // Проверка выхода за границы поля
        if (bullets[i].x < -9.0f || bullets[i].x > 9.0f ||
            bullets[i].y < -9.0f || bullets[i].y > 9.0f) {
            bullets[i].active = false;
            continue;
        }

        // Проверка столкновений
        int gridX = (int)((bullets[i].x + 8.5f) / 0.5f);
        int gridY = (int)((bullets[i].y + 8.5f) / 0.5f);


        if (gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
            GameObject* obj = &gameGrid[gridY][gridX];

            // Уничтожение базы
            if (obj->type == OBJECT_BASE) {
                gameState = GAME_LOSE;
            }

            if (!obj->passable) {
                // Начисляем очки за разрушение кирпича
                if ((obj->type == OBJECT_BRICK || obj->type == OBJECT_BRICK_BASE) && obj->durability > 0) {
                    gameScore += 10;
                }

                // Для разрушаемых объектов уменьшаем прочность
                if (obj->destructible && obj->durability > 0) {
                    obj->durability--;

                    // Если прочность закончилась - удаляем объект
                    if (obj->durability <= 0) {
                        obj->type = OBJECT_EMPTY;
                        obj->passable = true;
                    }
                }

                bullets[i].active = false;
            }
        }
    }
}

void drawBullets() {
    glColor3f(0.3f, 0.3f, 0.3f); // Серый цвет

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;

        glPushMatrix();
        glTranslatef(bullets[i].x, bullets[i].y, 0.0f);

        // Ориентация пули в зависимости от направления
        switch (bullets[i].direction) {
        case 0: case 2: // Вертикально
            glBegin(GL_QUADS);
            glVertex2f(-0.1f, -0.15f);
            glVertex2f(0.1f, -0.15f);
            glVertex2f(0.1f, 0.15f);
            glVertex2f(-0.1f, 0.15f);
            glEnd();
            break;

        case 1: case 3: // Горизонтально
            glBegin(GL_QUADS);
            glVertex2f(-0.15f, -0.1f);
            glVertex2f(0.15f, -0.1f);
            glVertex2f(0.15f, 0.1f);
            glVertex2f(-0.15f, 0.1f);
            glEnd();
            break;
        }

        glPopMatrix();
    }
}
//--------------------------------------------------------------------------







//функция имени ненависти к андрею
void updateEnemies() {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;

        // Текущая клетка врага
        int currentGridX = (int)roundf((enemies[i].x + 8.0f) / 0.5f);
        int currentGridY = (int)roundf((enemies[i].y + 8.0f) / 0.5f);

        // на линии атаки или нет
        enemies[i].state = isAttackLine(currentGridX, currentGridY) ? 1 : 0;

        // Стрельба раз в 2 секунды
        if (currentTime - enemies[i].lastShotTime > 2000) {
            createEnemyBullet(i);
            enemies[i].lastShotTime = currentTime;
        }

        // Движемся к целевой клетке
        if (enemies[i].isMoving) {
            enemies[i].moveProgress += 0.03f;

            // Завершили шаг
            if (enemies[i].moveProgress >= 1.0f) {
                enemies[i].isMoving = false;
                enemies[i].moveProgress = 1.0f;
                enemies[i].lastMoveTime = currentTime;

                // Фиксируем центр целевой клетки
                enemies[i].x = -8.5f + enemies[i].targetX * 0.5f + 0.5f;
                enemies[i].y = -8.5f + enemies[i].targetY * 0.5f + 0.5f;

                // Обновляем текущую клетку
                currentGridX = enemies[i].targetX;
                currentGridY = enemies[i].targetY;

                // Переходим к следующей точке пути
                if (enemies[i].path != NULL && enemies[i].pathStep < enemies[i].pathLength - 1) {
                    enemies[i].pathStep++;
                }
                // Путь завершён — очищаем
                else if (enemies[i].path != NULL) {
                    free(enemies[i].path);
                    enemies[i].path = NULL;
                    enemies[i].pathLength = 0;
                }
            }
            // В пути: выставляем направление по вектору на цель
            else {
                float targetWorldX = -8.5f + enemies[i].targetX * 0.5f + 0.5f;
                float targetWorldY = -8.5f + enemies[i].targetY * 0.5f + 0.5f;

                float dx = targetWorldX - enemies[i].x;
                float dy = targetWorldY - enemies[i].y;

                if (fabsf(dx) > fabsf(dy)) {
                    enemies[i].direction = (dx > 0.0f) ? 3 : 1; // вправо : влево
                }
                else {
                    enemies[i].direction = (dy > 0.0f) ? 0 : 2; // вверх : вниз
                }
            }
        }
        // Стоим: решаем, куда сделать следующий шаг
        else if (currentTime - enemies[i].lastMoveTime > enemies[i].moveDelay) {
            // A) Есть путь — идём по нему
            if (enemies[i].path != NULL && enemies[i].pathStep < enemies[i].pathLength) {
                int nextGridX = enemies[i].path[enemies[i].pathStep * 2];
                int nextGridY = enemies[i].path[enemies[i].pathStep * 2 + 1];

                if (canMoveTo(nextGridX, nextGridY)) {
                    enemies[i].targetX = nextGridX;
                    enemies[i].targetY = nextGridY;
                    enemies[i].isMoving = true;
                    enemies[i].moveProgress = 0.0f;
                }
            }
            // B) Путь пуст и мы на линии атаки 
            else if (enemies[i].path == NULL && enemies[i].state == 1) {
                int newDirection = -1;
                int nextGridX = currentGridX;
                int nextGridY = currentGridY;

                // Горизонтальная нижняя линия
                if (currentGridY == 1) {
                    if (currentGridX < 17) { newDirection = 3; nextGridX++; } // вправо
                    else if (currentGridX > 18) { newDirection = 1; nextGridX--; } // влево
                }
                // Вертикальные направляющие к базе
                else if (currentGridX == 17 || currentGridX == 18) {
                    if (currentGridY > 1) { newDirection = 2; nextGridY--; }      // вниз
                }

                if (newDirection != -1 && canMoveTo(nextGridX, nextGridY)) {
                    enemies[i].targetX = nextGridX;
                    enemies[i].targetY = nextGridY;
                    enemies[i].isMoving = true;
                    enemies[i].moveProgress = 0.0f;
                }
            }
        }

        // Стоим на линии атаки без пути — смотрим в нужную сторону
        if (!enemies[i].isMoving && enemies[i].path == NULL && enemies[i].state == 1) {
            if (currentGridY == 1) {
                if (currentGridX < 17)      enemies[i].direction = 3; // вправо
                else if (currentGridX > 18) enemies[i].direction = 1; // влево
                else                        enemies[i].direction = 2; // вниз
            }
            else if (currentGridX == 17 || currentGridX == 18) {
                enemies[i].direction = 2; // вниз
            }
        }
    }
}









// Инит
//--------------------------------------------------------------------------
void initGame() {
    // Инициализация игровой сетки
    initGameGrid();

    // Инициализация танка игрока
    initTank();

    // Инициализация пуль
    initBullets();

    // Инициализация врагов
    initEnemies();

    // Настраиваем ортографическую проекцию
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-FIELD_WIDTH / 2, FIELD_WIDTH / 2 + PANEL_WIDTH,
        -FIELD_HEIGHT / 2, FIELD_HEIGHT / 2,
        -1.0, 1.0);

    // Переключаемся на модель-вид
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Инициализация кнопок меню
    startButton = (Button){ 0.0f, 0.0f, 6.0f, 1.5f, "Start Game", -2.0f, 0.0f };
    exitButton = (Button){ 0.0f, -2.0f, 6.0f, 1.5f, "Exit", -2.0f, 0.0f };
    okButton = (Button){ -3.0f, -3.0f, 6.0f, 1.5f, "Ok", -2.0f, 0.0f };

    gameOverButton = (Button){ -4.5f, -3.0f, 4.0f, 1.5f, "Game Over", -1.0f, 0.0f };
    restartButton = (Button){ 0.5f, -3.0f, 4.0f, 1.5f, "Restart", -1.0f, 0.0f };


    // Сброс игровых параметров
    playerHP = 5;
    gameScore = 0;
    gameTime = 0;
    gameState = GAME_MENU;
}

void init() {
    // Устанавливаем цвет очистки
    glClearColor(0.29f, 0.33f, 0.39f, 1.0f); // Новый серо-голубой фон (#4A5563)
    initGame();
}
//--------------------------------------------------------------------------












// Блок отрисовки
//--------------------------------------------------------------------------
void drawField() {
    // Рисуем игровое поле (зеленый квадрат)
    glColor3f(0.36f, 0.54f, 0.45f); // #5B8A72 — серо-зелёный металл
    glBegin(GL_QUADS);
    glVertex2f(-FIELD_WIDTH / 2, -FIELD_HEIGHT / 2);
    glVertex2f(FIELD_WIDTH / 2, -FIELD_HEIGHT / 2);
    glVertex2f(FIELD_WIDTH / 2, FIELD_HEIGHT / 2);
    glVertex2f(-FIELD_WIDTH / 2, FIELD_HEIGHT / 2);
    glEnd();
}

void drawWalls() {
    glColor3f(0.44f, 0.50f, 0.56f); // Тёмно-серый (#708090)




    // Левая стена
    glBegin(GL_QUADS);
    glVertex2f(-FIELD_WIDTH / 2, -FIELD_HEIGHT / 2);
    glVertex2f(-FIELD_WIDTH / 2 + WALL_THICKNESS, -FIELD_HEIGHT / 2);
    glVertex2f(-FIELD_WIDTH / 2 + WALL_THICKNESS, FIELD_HEIGHT / 2);
    glVertex2f(-FIELD_WIDTH / 2, FIELD_HEIGHT / 2);
    glEnd();

    // Правая стена
    glBegin(GL_QUADS);
    glVertex2f(FIELD_WIDTH / 2 - WALL_THICKNESS, -FIELD_HEIGHT / 2);
    glVertex2f(FIELD_WIDTH / 2, -FIELD_HEIGHT / 2);
    glVertex2f(FIELD_WIDTH / 2, FIELD_HEIGHT / 2);
    glVertex2f(FIELD_WIDTH / 2 - WALL_THICKNESS, FIELD_HEIGHT / 2);
    glEnd();

    // Верхняя стена
    glBegin(GL_QUADS);
    glVertex2f(-FIELD_WIDTH / 2, FIELD_HEIGHT / 2 - WALL_THICKNESS);
    glVertex2f(FIELD_WIDTH / 2, FIELD_HEIGHT / 2 - WALL_THICKNESS);
    glVertex2f(FIELD_WIDTH / 2, FIELD_HEIGHT / 2);
    glVertex2f(-FIELD_WIDTH / 2, FIELD_HEIGHT / 2);
    glEnd();

    // Нижняя стена
    glBegin(GL_QUADS);
    glVertex2f(-FIELD_WIDTH / 2, -FIELD_HEIGHT / 2);
    glVertex2f(FIELD_WIDTH / 2, -FIELD_HEIGHT / 2);
    glVertex2f(FIELD_WIDTH / 2, -FIELD_HEIGHT / 2 + WALL_THICKNESS);
    glVertex2f(-FIELD_WIDTH / 2, -FIELD_HEIGHT / 2 + WALL_THICKNESS);
    glEnd();
}

void drawGameObjects() {
    // Рассчитываем начальные координаты (левый нижний угол сетки)
    float startX = -FIELD_WIDTH / 2 + WALL_THICKNESS;
    float startY = -FIELD_HEIGHT / 2 + WALL_THICKNESS;

    // Проходим по всем ячейкам сетки
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            float posX = startX + x * CELL_SIZE;
            float posY = startY + y * CELL_SIZE;

            switch (gameGrid[y][x].type) {
            case OBJECT_BRICK: // Разрушаемая стена (кирпич)
                // Определяем цвет по прочности
                switch (gameGrid[y][x].durability) {
                case 3: glColor3f(0.80f, 0.44f, 0.44f); break; // основа
                case 2: glColor3f(0.88f, 0.56f, 0.56f); break; // средне
                case 1: glColor3f(0.96f, 0.68f, 0.68f); break; // почти всё
                }
                glBegin(GL_QUADS);
                glVertex2f(posX, posY);
                glVertex2f(posX + CELL_SIZE, posY);
                glVertex2f(posX + CELL_SIZE, posY + CELL_SIZE);
                glVertex2f(posX, posY + CELL_SIZE);
                glEnd();
                break;

            case OBJECT_BRICK_BASE: // Разрушаемая стена базы (кирпич)
                // Определяем цвет по прочности
                switch (gameGrid[y][x].durability) {
                case 3: glColor3f(0.80f, 0.44f, 0.44f); break; // основа
                case 2: glColor3f(0.88f, 0.56f, 0.56f); break; // средне
                case 1: glColor3f(0.96f, 0.68f, 0.68f); break; // почти всё
                }
                glBegin(GL_QUADS);
                glVertex2f(posX, posY);
                glVertex2f(posX + CELL_SIZE, posY);
                glVertex2f(posX + CELL_SIZE, posY + CELL_SIZE);
                glVertex2f(posX, posY + CELL_SIZE);
                glEnd();
                break;

            case OBJECT_STEEL: // Неразрушаемая стена (сталь)
                glColor3f(0.69f, 0.77f, 0.87f); // Цвет танка игрока (#B0C4DE)
                glBegin(GL_QUADS);
                glVertex2f(posX, posY);
                glVertex2f(posX + CELL_SIZE, posY);
                glVertex2f(posX + CELL_SIZE, posY + CELL_SIZE);
                glVertex2f(posX, posY + CELL_SIZE);
                glEnd();
                break;

            case OBJECT_BASE: // База игрока
                glColor3f(0.80f, 0.67f, 0.49f); // #CDAA7D — бронзово-песочный

                glBegin(GL_QUADS);
                glVertex2f(posX, posY);
                glVertex2f(posX + CELL_SIZE, posY);
                glVertex2f(posX + CELL_SIZE, posY + CELL_SIZE);
                glVertex2f(posX, posY + CELL_SIZE);
                glEnd();
                break;
            }
        }
    }
}

void drawText(float x, float y, const char* text) {
    glColor3f(1.0f, 1.0f, 1.0f); // Белый цвет текста
    glRasterPos2f(x, y);

    // Используем шрифт с поддержкой символов
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
    }
}

void drawButton(Button button) {
    // Рисуем прямоугольник кнопки
    glColor3f(0.44f, 0.50f, 0.56f); // Основной цвет кнопки (#708090)
    glBegin(GL_QUADS);
    glVertex2f(button.x, button.y);
    glVertex2f(button.x + button.width, button.y);
    glVertex2f(button.x + button.width, button.y + button.height);
    glVertex2f(button.x, button.y + button.height);
    glEnd();

    // Рамка кнопки
    glColor3f(0.69f, 0.77f, 0.87f); // Обводка кнопки (#B0C4DE)
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(button.x, button.y);
    glVertex2f(button.x + button.width, button.y);
    glVertex2f(button.x + button.width, button.y + button.height);
    glVertex2f(button.x, button.y + button.height);
    glEnd();
    glLineWidth(1.0f);

    // Центрирование текста + учёт индивидуальных сдвигов
    int textWidth = 0;
    for (const char* c = button.text; *c != '\0'; c++) {
        textWidth += glutBitmapWidth(GLUT_BITMAP_9_BY_15, *c);
    }
    float textW = textWidth / 100.0f;                         // перевод «пикселей» шрифта в мир
    float textX = button.x + (button.width - textW) * 0.5f;  // центр по X
    float textY = button.y + (button.height * 0.5f) - 0.15f;  // центр по Y, чуть ниже

    drawText(textX + button.textOffsetX, textY + button.textOffsetY, button.text);

}

void drawInfoPanel() {
    // Рисуем панель
    float panelLeft = FIELD_WIDTH / 2;
    float panelRight = panelLeft + PANEL_WIDTH;
    float panelBottom = -PANEL_HEIGHT / 2;
    float panelTop = PANEL_HEIGHT / 2;

    glColor3f(0.29f, 0.33f, 0.39f); // Серо-голубой фон (#4A5563)
    glBegin(GL_QUADS);
    glVertex2f(panelLeft, panelBottom);
    glVertex2f(panelRight, panelBottom);
    glVertex2f(panelRight, panelTop);
    glVertex2f(panelLeft, panelTop);
    glEnd();

    // Рисуем информацию
    glColor3f(0.0f, 0.0f, 0.0f); // Черный

    // Здоровье игрока
    char hpText[20];
    snprintf(hpText, sizeof(hpText), "HP: %d", playerHP);
    drawText(panelLeft + 0.5f, panelTop - 1.0f, hpText);

    // Время игры
    int minutes = gameTime / 60;
    int seconds = gameTime % 60;
    char timeText[20];
    snprintf(timeText, sizeof(timeText), "Time: %02d:%02d", minutes, seconds);
    drawText(panelLeft + 0.5f, panelTop - 2.5f, timeText);

    // Очки
    char scoreText[20];
    snprintf(scoreText, sizeof(scoreText), "Score: %d", gameScore);
    drawText(panelLeft + 0.5f, panelTop - 4.0f, scoreText);

    // FPS
    char fpsText[20];
    snprintf(fpsText, sizeof(fpsText), "FPS: %d", fps);
    // Тот же левый отступ, на строку ниже очков (шаг 1.5 как у остальных строк)
    drawText(panelLeft + 0.5f, panelTop - 5.5f, fpsText);

}

void drawMenu() {
    glClearColor(0.29f, 0.33f, 0.39f, 1.0f); // Новый серо-голубой фон (#4A5563)
    glClear(GL_COLOR_BUFFER_BIT);

    // Рисуем заголовок
    glColor3f(0.0f, 0.0f, 0.0f);
    drawText(1.31f, 3.0f, "BATTLE GYM");

    // Рисуем кнопки
    drawButton(startButton);
    drawButton(exitButton);
}

void drawWinScreen() {
    glColor3f(0.35f, 0.40f, 0.46f); // #5A6675 примерно
    glBegin(GL_QUADS);
    glVertex2f(-FIELD_WIDTH / 2, -FIELD_HEIGHT / 2);
    glVertex2f(FIELD_WIDTH / 2, -FIELD_HEIGHT / 2);
    glVertex2f(FIELD_WIDTH / 2, FIELD_HEIGHT / 2);
    glVertex2f(-FIELD_WIDTH / 2, FIELD_HEIGHT / 2);
    glEnd();

    // Текст победы
    glColor3f(0.0f, 1.0f, 0.0f);
    drawText(-4.0f, 2.0f, "You win!");

    // Очки
    char scoreText[30];
    sprintf(scoreText, "Your score: %d", gameScore);
    drawText(-4.0f, 0.5f, scoreText);

    drawButton(okButton);
}

void drawLoseScreen() {
    glColor3f(0.35f, 0.40f, 0.46f); // фон панели в цвет фона

    glBegin(GL_QUADS);
    glVertex2f(-FIELD_WIDTH / 2, -FIELD_HEIGHT / 2);
    glVertex2f(FIELD_WIDTH / 2, -FIELD_HEIGHT / 2);
    glVertex2f(FIELD_WIDTH / 2, FIELD_HEIGHT / 2);
    glVertex2f(-FIELD_WIDTH / 2, FIELD_HEIGHT / 2);
    glEnd();


    // Текст проигрыша
    glColor3f(1.0f, 0.0f, 0.0f); // Красный
    drawText(-3.0f, 2.0f, "You loooose!");

    // Причина проигрыша
    if (playerHP <= 0) {
        drawText(-3.0f, 0.5f, "You died");
    }
    else {
        drawText(-3.0f, 0.5f, "Base was destroyed");
    }

    // Очки
    char scoreText[30];
    sprintf(scoreText, "Your score: %d", gameScore);
    drawText(-3.0f, -1.0f, scoreText);

    // Кнопки Game Over и Restart
    drawButton(gameOverButton);
    drawButton(restartButton);
}
//--------------------------------------------------------------------------








// Чтоб картинка была
//--------------------------------------------------------------------------
void display() {
    // Очищаем буфер цвета
    glClear(GL_COLOR_BUFFER_BIT);


    // фэпэсы считаем
    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    if (fpsLastTimeMs == 0) fpsLastTimeMs = nowMs;      // инициализация при первом кадре
    fpsFrameCount++;
    int delta = nowMs - fpsLastTimeMs;
    if (delta >= 1000) {                                 // раз в ~секунду
        fps = (int)((fpsFrameCount * 1000.0f) / (float)delta);
        fpsFrameCount = 0;
        fpsLastTimeMs = nowMs;
    }


    // В зависимости от состояния игры рисуем разные экраны
    switch (gameState) {
    case GAME_MENU:
        drawMenu();
        break;

    case GAME_PLAYING:
        // Рисуем игровое поле
        drawField();
        drawWalls();
        drawGameObjects();
        drawInfoPanel();

        // Отрисовка игровых объектов
        updateTank();
        drawTank();

        respawnEnemies();
        updateEnemies();
        drawEnemies();

        updateBullets();
        drawBullets();
        break;

    case GAME_WIN:
        // Рисуем игровое поле на заднем плане
        drawField();
        drawWalls();
        drawGameObjects();
        drawInfoPanel();

        // Отрисовка игровых объектов
        drawTank();
        drawEnemies();
        drawBullets();

        // Рисуем экран победы поверх
        drawWinScreen();
        break;

    case GAME_LOSE:
        // Рисуем игровое поле на заднем плане
        drawField();
        drawWalls();
        drawGameObjects();
        drawInfoPanel();

        // Отрисовка игровых объектов
        drawTank();
        drawEnemies();
        drawBullets();

        // Рисуем экран проигрыша поверх
        drawLoseScreen();
        break;
    }

    // Обмен буферов
    glutSwapBuffers();
    glutPostRedisplay(); // Непрерывная перерисовка
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
}
//--------------------------------------------------------------------------










// Всё, что касается клавы и мыши
//--------------------------------------------------------------------------
void mouse(int button, int state, int x, int y) {
    // Преобразование координат мыши в мировые координаты
    mouseX = x;
    mouseY = y;

    if (button == GLUT_LEFT_BUTTON) {
        mouseLeftDown = (state == GLUT_DOWN);

        if (mouseLeftDown) {
            // Преобразуем координаты экрана в мировые
            GLint viewport[4];
            GLdouble modelview[16], projection[16];
            GLfloat winX, winY, winZ;
            GLdouble posX, posY, posZ;

            glGetIntegerv(GL_VIEWPORT, viewport);
            glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
            glGetDoublev(GL_PROJECTION_MATRIX, projection);

            winX = (float)x;
            winY = (float)viewport[3] - (float)y;
            glReadPixels(x, (int)winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);

            gluUnProject(winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);

            float worldX = (float)posX;
            float worldY = (float)posY;

            // Проверка нажатия на кнопки
            if (gameState == GAME_MENU) {
                // Кнопка "Начать игру"
                if (worldX >= startButton.x && worldX <= startButton.x + startButton.width &&
                    worldY >= startButton.y && worldY <= startButton.y + startButton.height) {
                    gameStartTime = glutGet(GLUT_ELAPSED_TIME);
                    gameState = GAME_PLAYING;
                }
                // Кнопка "Выход"
                else if (worldX >= exitButton.x && worldX <= exitButton.x + exitButton.width &&
                    worldY >= exitButton.y && worldY <= exitButton.y + exitButton.height) {
                    exit(0);
                }
            }

            else if (gameState == GAME_WIN) {
                // Нажатие на кнопку OK
                if (worldX >= okButton.x && worldX <= okButton.x + okButton.width &&
                    worldY >= okButton.y && worldY <= okButton.y + okButton.height) {

                    gameState = GAME_MENU;
                    return;
                }
            }

            // Экран проигрыша: две кнопки
            else if (gameState == GAME_LOSE) {
                // перезапуск игры
                if (worldX >= restartButton.x && worldX <= restartButton.x + restartButton.width &&
                    worldY >= restartButton.y && worldY <= restartButton.y + restartButton.height) {
                    initGame();                 // сбрасываем состояние
                    gameState = GAME_PLAYING;   // сразу возвращаемся в игру
                    return;
                }
                // GAME OVER 
                if (worldX >= gameOverButton.x && worldX <= gameOverButton.x + gameOverButton.width &&
                    worldY >= gameOverButton.y && worldY <= gameOverButton.y + gameOverButton.height) {
                    gameState = GAME_MENU;      // показываем экран меню
                    return;
                }
            }

        }
    }
}

void keyboard(unsigned char key, int x, int y) {
    if (gameState == GAME_PLAYING) {
        if (key == 27) { // ESC
            gameState = GAME_MENU;
            initGame(); // Сброс игры
        }
        if (key == ' ') { // Пробел
            createBullet();
        }
    }
}

void specialKeys(int key, int x, int y) {
    int newDirection = -1;

    switch (key) {
    case GLUT_KEY_UP: newDirection = 0; break;
    case GLUT_KEY_LEFT: newDirection = 1; break;
    case GLUT_KEY_DOWN: newDirection = 2; break;
    case GLUT_KEY_RIGHT: newDirection = 3; break;
    }

    if (newDirection != -1 && !playerTank.isMoving) {
        // Если нужно повернуться
        if (playerTank.direction != newDirection) {
            playerTank.direction = newDirection;
        }
        // Если можно двигаться
        else {
            int nextX = playerTank.targetX;
            int nextY = playerTank.targetY;

            switch (newDirection) {
            case 0: nextY += 1; break; // Вверх
            case 1: nextX -= 1; break; // Влево
            case 2: nextY -= 1; break; // Вниз
            case 3: nextX += 1; break; // Вправо
            }

            if (canMoveTo(nextX, nextY)) {
                playerTank.targetX = nextX;
                playerTank.targetY = nextY;
                playerTank.isMoving = true;
                playerTank.moveProgress = 0.0f;
            }
        }
    }
    glutPostRedisplay();
}
//--------------------------------------------------------------------------








// Мэйн
//--------------------------------------------------------------------------
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Battle Gym");

    glutInitDisplayString("rgba double depth>=16");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);

    // Таймер для обновления игрового времени
    glutTimerFunc(1000, timerCallback, 0);

    init();
    glutMainLoop();
    return 0;
}