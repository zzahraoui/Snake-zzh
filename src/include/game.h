#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

// Taille de la grille
#define GRID_WIDTH 40
#define GRID_HEIGHT 30
#define CELL_SIZE 20

// Taille de la fenêtre
#define WINDOW_WIDTH (GRID_WIDTH * CELL_SIZE)
#define WINDOW_HEIGHT (GRID_HEIGHT * CELL_SIZE)

// Vitesse de base (diminue = plus rapide)
#define BASE_SPEED 120
#define MIN_SPEED 50
#define SPEED_INCREMENT 2 // Diminue tous les N points

#define MAX_SNAKE_LENGTH (GRID_WIDTH * GRID_HEIGHT)
#define MAX_PARTICLES 100
#define MAX_OBSTACLES 10

// Types de nourriture
typedef enum
{
    FOOD_NORMAL, // +10 points
    FOOD_BONUS,  // +25 points (rare)
    FOOD_MEGA    // +50 points (très rare)
} FoodType;

// Types de power-ups
typedef enum
{
    POWERUP_NONE,
    POWERUP_SHIELD,   // Invincibilité 1 collision
    POWERUP_SLOW,     // Ralentit le jeu
    POWERUP_SCORE_X2, // Double les points
    POWERUP_GHOST     // Traverse les murs
} PowerUpType;

// Direction
typedef enum
{
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

// Position
typedef struct
{
    int x;
    int y;
} Position;

// Particule (pour les effets visuels)
typedef struct
{
    float x, y;
    float vx, vy; // Vélocité
    float life;   // Temps de vie (0.0 à 1.0)
    unsigned char r, g, b;
} Particle;

// Système de particules
typedef struct
{
    Particle particles[MAX_PARTICLES];
    int count;
} ParticleSystem;

// Serpent
typedef struct
{
    Position body[MAX_SNAKE_LENGTH];
    int length;
    Direction direction;
    Direction next_direction;
    bool has_shield;
    float trail_alpha[MAX_SNAKE_LENGTH]; // Pour l'effet de traînée
} Snake;

// Nourriture
typedef struct
{
    Position pos;
    bool active;
    FoodType type;
    float pulse; // Pour l'animation de pulsation
} Food;

// Power-up
typedef struct
{
    Position pos;
    bool active;
    PowerUpType type;
    float spawn_time;
    float duration; // Durée d'effet
} PowerUp;

// Obstacle
typedef struct
{
    Position pos;
    bool active;
} Obstacle;

// Combo system
typedef struct
{
    int count; // Nombre de fruits mangés dans le combo
    unsigned int last_eat_time;
    float multiplier; // Multiplicateur de score
    bool fever_mode;  // Mode FEVER (combo très élevé)
} Combo;

// État du jeu
typedef enum
{
    GAME_MENU,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_OVER,
    GAME_STATS // Écran de statistiques
} GameState;

// Statistiques du joueur
typedef struct
{
    int high_score;
    int total_games;
    int total_food_eaten;
    int longest_snake;
    float total_playtime; // En secondes
} Stats;

// Le jeu complet
typedef struct
{
    Snake snake;
    Food food;
    PowerUp powerup;
    Obstacle obstacles[MAX_OBSTACLES];
    int obstacle_count;

    Combo combo;
    ParticleSystem particles;

    GameState state;
    int score;
    int current_speed;
    unsigned int last_move_time;
    unsigned int game_start_time;

    // Power-up actif
    PowerUpType active_powerup;
    unsigned int powerup_start_time;

    Stats stats;

    // Animations
    float menu_pulse;
    float grid_anim;
} Game;

// Fonctions principales
void game_init(Game *game);
void game_update(Game *game);
bool game_check_collision(Game *game);
void snake_move(Snake *snake);
void food_spawn(Food *food, Snake *snake, Obstacle *obstacles, int obstacle_count);
void powerup_spawn(PowerUp *powerup, Snake *snake, Food *food, Obstacle *obstacles, int obstacle_count);
void obstacle_spawn(Game *game);

// Particules
void particles_init(ParticleSystem *ps);
void particles_add(ParticleSystem *ps, float x, float y, int r, int g, int b, int count);
void particles_update(ParticleSystem *ps);

// Combo
void combo_reset(Combo *combo);
void combo_add(Combo *combo);
int combo_get_bonus(Combo *combo);

// Stats
void stats_load(Stats *stats);
void stats_save(Stats *stats);
void stats_update(Stats *stats, Game *game);

#endif // GAME_H