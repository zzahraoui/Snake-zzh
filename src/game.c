#include "include/game.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <SDL2/SDL.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ========== PARTICULES ==========

void particles_init(ParticleSystem *ps)
{
    ps->count = 0;
    memset(ps->particles, 0, sizeof(ps->particles));
}

void particles_add(ParticleSystem *ps, float x, float y, int r, int g, int b, int count)
{
    for (int i = 0; i < count && ps->count < MAX_PARTICLES; i++)
    {
        Particle *p = &ps->particles[ps->count++];
        p->x = x;
        p->y = y;

        // Vélocité aléatoire
        float angle = (rand() % 360) * M_PI / 180.0f;
        float speed = 2.0f + (rand() % 100) / 50.0f;
        p->vx = cosf(angle) * speed;
        p->vy = sinf(angle) * speed;

        p->life = 1.0f;
        p->r = r;
        p->g = g;
        p->b = b;
    }
}

void particles_update(ParticleSystem *ps)
{
    for (int i = 0; i < ps->count; i++)
    {
        Particle *p = &ps->particles[i];
        p->x += p->vx;
        p->y += p->vy;
        p->vy += 0.1f; // Gravité
        p->life -= 0.02f;

        // Supprimer si morte
        if (p->life <= 0)
        {
            ps->particles[i] = ps->particles[--ps->count];
            i--;
        }
    }
}

// ========== COMBO ==========

void combo_reset(Combo *combo)
{
    combo->count = 0;
    combo->multiplier = 1.0f;
    combo->fever_mode = false;
}

void combo_add(Combo *combo)
{
    unsigned int current_time = SDL_GetTicks();

    // Si plus de 3 secondes depuis le dernier fruit, reset
    if (current_time - combo->last_eat_time > 3000)
    {
        combo->count = 0;
    }

    combo->count++;
    combo->last_eat_time = current_time;

    // Calculer le multiplicateur
    if (combo->count >= 10)
    {
        combo->multiplier = 4.0f;
        combo->fever_mode = true;
    }
    else if (combo->count >= 7)
    {
        combo->multiplier = 3.0f;
    }
    else if (combo->count >= 5)
    {
        combo->multiplier = 2.5f;
    }
    else if (combo->count >= 3)
    {
        combo->multiplier = 2.0f;
    }
    else
    {
        combo->multiplier = 1.0f;
        combo->fever_mode = false;
    }
}

int combo_get_bonus(Combo *combo)
{
    if (combo->count >= 10)
        return 100;
    if (combo->count >= 7)
        return 50;
    if (combo->count >= 5)
        return 25;
    if (combo->count >= 3)
        return 10;
    return 0;
}

// ========== STATS ==========

void stats_load(Stats *stats)
{
    FILE *f = fopen("snake_stats.dat", "rb");
    if (f)
    {
        fread(stats, sizeof(Stats), 1, f);
        fclose(f);
    }
    else
    {
        // Valeurs par défaut
        stats->high_score = 0;
        stats->total_games = 0;
        stats->total_food_eaten = 0;
        stats->longest_snake = 0;
        stats->total_playtime = 0;
    }
}

void stats_save(Stats *stats)
{
    FILE *f = fopen("snake_stats.dat", "wb");
    if (f)
    {
        fwrite(stats, sizeof(Stats), 1, f);
        fclose(f);
    }
}

void stats_update(Stats *stats, Game *game)
{
    if (game->score > stats->high_score)
    {
        stats->high_score = game->score;
    }
    if (game->snake.length > stats->longest_snake)
    {
        stats->longest_snake = game->snake.length;
    }
}

// ========== UTILITAIRES ==========

static bool is_position_occupied(Snake *snake, Position pos)
{
    for (int i = 0; i < snake->length; i++)
    {
        if (snake->body[i].x == pos.x && snake->body[i].y == pos.y)
        {
            return true;
        }
    }
    return false;
}

static bool is_position_obstacle(Obstacle *obstacles, int count, Position pos)
{
    for (int i = 0; i < count; i++)
    {
        if (obstacles[i].active &&
            obstacles[i].pos.x == pos.x &&
            obstacles[i].pos.y == pos.y)
        {
            return true;
        }
    }
    return false;
}

// ========== NOURRITURE ==========

void food_spawn(Food *food, Snake *snake, Obstacle *obstacles, int obstacle_count)
{
    Position new_pos;
    int attempts = 0;

    do
    {
        new_pos.x = rand() % GRID_WIDTH;
        new_pos.y = rand() % GRID_HEIGHT;
        attempts++;
    } while ((is_position_occupied(snake, new_pos) ||
              is_position_obstacle(obstacles, obstacle_count, new_pos)) &&
             attempts < 100);

    food->pos = new_pos;
    food->active = true;
    food->pulse = 0;

    // Type aléatoire
    int rnd = rand() % 100;
    if (rnd < 5)
    {
        food->type = FOOD_MEGA; // 5% chance
    }
    else if (rnd < 20)
    {
        food->type = FOOD_BONUS; // 15% chance
    }
    else
    {
        food->type = FOOD_NORMAL; // 80% chance
    }
}

// ========== POWER-UP ==========

void powerup_spawn(PowerUp *powerup, Snake *snake, Food *food, Obstacle *obstacles, int obstacle_count)
{
    if (powerup->active)
        return;

    // 10% de chance d'apparaître après avoir mangé
    if (rand() % 100 < 10)
    {
        Position new_pos;
        int attempts = 0;

        do
        {
            new_pos.x = rand() % GRID_WIDTH;
            new_pos.y = rand() % GRID_HEIGHT;
            attempts++;
        } while ((is_position_occupied(snake, new_pos) ||
                  is_position_obstacle(obstacles, obstacle_count, new_pos) ||
                  (food->active && food->pos.x == new_pos.x && food->pos.y == new_pos.y)) &&
                 attempts < 100);

        powerup->pos = new_pos;
        powerup->active = true;
        powerup->spawn_time = SDL_GetTicks();

        // Type aléatoire
        powerup->type = (rand() % 4) + 1;

        // Durée selon le type
        switch (powerup->type)
        {
        case POWERUP_SHIELD:
            powerup->duration = 0;
            break; // Instantané
        case POWERUP_SLOW:
            powerup->duration = 8000;
            break;
        case POWERUP_SCORE_X2:
            powerup->duration = 10000;
            break;
        case POWERUP_GHOST:
            powerup->duration = 5000;
            break;
        default:
            powerup->duration = 5000;
        }
    }
}

// ========== OBSTACLES ==========

void obstacle_spawn(Game *game)
{
    // Ajouter des obstacles tous les 50 points
    int target_count = (game->score / 50) + 1;
    if (target_count > MAX_OBSTACLES)
        target_count = MAX_OBSTACLES;

    while (game->obstacle_count < target_count)
    {
        Position new_pos;
        int attempts = 0;

        do
        {
            new_pos.x = rand() % GRID_WIDTH;
            new_pos.y = rand() % GRID_HEIGHT;
            attempts++;
        } while ((is_position_occupied(&game->snake, new_pos) ||
                  (game->food.active && game->food.pos.x == new_pos.x && game->food.pos.y == new_pos.y) ||
                  is_position_obstacle(game->obstacles, game->obstacle_count, new_pos)) &&
                 attempts < 100);

        game->obstacles[game->obstacle_count].pos = new_pos;
        game->obstacles[game->obstacle_count].active = true;
        game->obstacle_count++;
    }
}

// ========== SERPENT ==========

void snake_move(Snake *snake)
{
    snake->direction = snake->next_direction;

    Position new_head = snake->body[0];

    switch (snake->direction)
    {
    case DIR_UP:
        new_head.y--;
        break;
    case DIR_DOWN:
        new_head.y++;
        break;
    case DIR_LEFT:
        new_head.x--;
        break;
    case DIR_RIGHT:
        new_head.x++;
        break;
    }

    for (int i = snake->length - 1; i > 0; i--)
    {
        snake->body[i] = snake->body[i - 1];
        snake->trail_alpha[i] = snake->trail_alpha[i - 1] * 0.95f;
    }

    snake->body[0] = new_head;
    snake->trail_alpha[0] = 1.0f;
}

// ========== COLLISIONS ==========

bool game_check_collision(Game *game)
{
    Position head = game->snake.body[0];

    // Ghost mode = traverse les murs
    if (game->active_powerup == POWERUP_GHOST)
    {
        // Téléportation
        if (head.x < 0)
            head.x = GRID_WIDTH - 1;
        if (head.x >= GRID_WIDTH)
            head.x = 0;
        if (head.y < 0)
            head.y = GRID_HEIGHT - 1;
        if (head.y >= GRID_HEIGHT)
            head.y = 0;
        game->snake.body[0] = head;
    }
    else
    {
        // Collision avec les murs
        if (head.x < 0 || head.x >= GRID_WIDTH ||
            head.y < 0 || head.y >= GRID_HEIGHT)
        {
            if (game->snake.has_shield)
            {
                game->snake.has_shield = false;
                return false;
            }
            return true;
        }
    }

    // Collision avec obstacles
    for (int i = 0; i < game->obstacle_count; i++)
    {
        if (game->obstacles[i].active &&
            head.x == game->obstacles[i].pos.x &&
            head.y == game->obstacles[i].pos.y)
        {
            if (game->snake.has_shield)
            {
                game->snake.has_shield = false;
                return false;
            }
            return true;
        }
    }

    // Collision avec soi-même
    for (int i = 1; i < game->snake.length; i++)
    {
        if (head.x == game->snake.body[i].x &&
            head.y == game->snake.body[i].y)
        {
            if (game->snake.has_shield)
            {
                game->snake.has_shield = false;
                return false;
            }
            return true;
        }
    }

    return false;
}

// ========== INITIALISATION ==========

void game_init(Game *game)
{
    // Serpent
    game->snake.length = 3;
    game->snake.direction = DIR_RIGHT;
    game->snake.next_direction = DIR_RIGHT;
    game->snake.has_shield = false;

    int start_x = GRID_WIDTH / 2;
    int start_y = GRID_HEIGHT / 2;

    game->snake.body[0].x = start_x;
    game->snake.body[0].y = start_y;
    game->snake.body[1].x = start_x - 1;
    game->snake.body[1].y = start_y;
    game->snake.body[2].x = start_x - 2;
    game->snake.body[2].y = start_y;

    for (int i = 0; i < MAX_SNAKE_LENGTH; i++)
    {
        game->snake.trail_alpha[i] = 0;
    }

    // Nourriture
    game->food.active = false;
    food_spawn(&game->food, &game->snake, game->obstacles, game->obstacle_count);

    // Power-up
    game->powerup.active = false;
    game->active_powerup = POWERUP_NONE;

    // Obstacles
    game->obstacle_count = 0;

    // Combo
    combo_reset(&game->combo);

    // Particules
    particles_init(&game->particles);

    // État
    game->state = GAME_MENU;
    game->score = 0;
    game->current_speed = BASE_SPEED;
    game->last_move_time = SDL_GetTicks();
    game->game_start_time = SDL_GetTicks();

    // Animations
    game->menu_pulse = 0;
    game->grid_anim = 0;

    // Stats
    stats_load(&game->stats);

    srand(time(NULL));
}

// ========== MISE À JOUR ==========

void game_update(Game *game)
{
    if (game->state != GAME_PLAYING)
        return;

    unsigned int current_time = SDL_GetTicks();

    // Vitesse progressive
    int speed_bonus = game->score / 50;
    game->current_speed = BASE_SPEED - (speed_bonus * SPEED_INCREMENT);
    if (game->current_speed < MIN_SPEED)
        game->current_speed = MIN_SPEED;

    // Slow motion power-up
    int move_speed = game->current_speed;
    if (game->active_powerup == POWERUP_SLOW)
    {
        move_speed *= 2;
    }

    // Déplacer le serpent
    if (current_time - game->last_move_time < (unsigned int)move_speed)
    {
        particles_update(&game->particles);
        return;
    }
    game->last_move_time = current_time;

    snake_move(&game->snake);

    // Vérifier power-up expiré
    if (game->active_powerup != POWERUP_NONE &&
        game->active_powerup != POWERUP_SHIELD)
    {
        if (current_time - game->powerup_start_time > game->powerup.duration)
        {
            game->active_powerup = POWERUP_NONE;
        }
    }

    // Collisions
    if (game_check_collision(game))
    {
        game->state = GAME_OVER;
        game->stats.total_games++;
        stats_update(&game->stats, game);
        stats_save(&game->stats);
        return;
    }

    // Manger nourriture
    Position head = game->snake.body[0];
    if (game->food.active &&
        head.x == game->food.pos.x &&
        head.y == game->food.pos.y)
    {

        // Points
        int points = 10;
        if (game->food.type == FOOD_BONUS)
            points = 25;
        else if (game->food.type == FOOD_MEGA)
            points = 50;

        if (game->active_powerup == POWERUP_SCORE_X2)
        {
            points *= 2;
        }

        points = (int)(points * game->combo.multiplier);
        game->score += points;

        // Combo
        combo_add(&game->combo);

        // Particules
        int r = (game->food.type == FOOD_MEGA) ? 255 : (game->food.type == FOOD_BONUS ? 255 : 255);
        int g = (game->food.type == FOOD_MEGA) ? 215 : (game->food.type == FOOD_BONUS ? 255 : 0);
        int b = (game->food.type == FOOD_MEGA) ? 0 : (game->food.type == FOOD_BONUS ? 0 : 0);
        particles_add(&game->particles, head.x * CELL_SIZE + CELL_SIZE / 2,
                      head.y * CELL_SIZE + CELL_SIZE / 2, r, g, b, 15);

        // Grandir
        if (game->snake.length < MAX_SNAKE_LENGTH)
        {
            int tail = game->snake.length;
            game->snake.body[tail] = game->snake.body[tail - 1];
            game->snake.length++;
        }

        // Stats
        game->stats.total_food_eaten++;

        // Nouvelle nourriture
        food_spawn(&game->food, &game->snake, game->obstacles, game->obstacle_count);

        // Peut-être spawner power-up
        powerup_spawn(&game->powerup, &game->snake, &game->food, game->obstacles, game->obstacle_count);

        // Obstacles
        obstacle_spawn(game);
    }

    // Ramasser power-up
    if (game->powerup.active &&
        head.x == game->powerup.pos.x &&
        head.y == game->powerup.pos.y)
    {

        game->active_powerup = game->powerup.type;
        game->powerup_start_time = current_time;
        game->powerup.active = false;

        if (game->active_powerup == POWERUP_SHIELD)
        {
            game->snake.has_shield = true;
            game->active_powerup = POWERUP_NONE;
        }

        particles_add(&game->particles, head.x * CELL_SIZE + CELL_SIZE / 2,
                      head.y * CELL_SIZE + CELL_SIZE / 2, 0, 255, 255, 20);
    }

    // Mise à jour particules
    particles_update(&game->particles);

    // Animations
    game->food.pulse += 0.1f;
}