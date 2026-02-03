#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "include/game.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SDL_EVENT_QUIT SDL_QUIT
#define SDL_EVENT_KEY_DOWN SDL_KEYDOWN

typedef struct
{
    TTF_Font *large;
    TTF_Font *medium;
    TTF_Font *small;
    TTF_Font *tiny;
} Fonts;

typedef struct
{
    Uint8 r, g, b, a;
} Color;

const Color COLOR_BG = {10, 10, 20, 255};
const Color COLOR_GRID = {30, 30, 60, 80};
const Color COLOR_SNAKE_HEAD = {0, 255, 150, 255};
const Color COLOR_SNAKE_BODY = {0, 200, 120, 255};
const Color COLOR_FOOD_NORMAL = {255, 50, 50, 255};
const Color COLOR_FOOD_BONUS = {255, 200, 0, 255};
const Color COLOR_FOOD_MEGA = {255, 0, 255, 255};
const Color COLOR_TEXT = {255, 255, 255, 255};
const Color COLOR_ACCENT = {0, 255, 255, 255};

void set_color(SDL_Renderer *r, Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

void draw_rect(SDL_Renderer *r, int x, int y, int w, int h)
{
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(r, &rect);
}

void draw_text_centered(SDL_Renderer *r, TTF_Font *font, const char *text,
                        int y, Color color)
{
    SDL_Color c = {color.r, color.g, color.b, color.a};
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, c);
    if (!surf)
        return;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    if (!tex)
    {
        SDL_FreeSurface(surf);
        return;
    }

    SDL_Rect dst = {(WINDOW_WIDTH - surf->w) / 2, y, surf->w, surf->h};
    SDL_RenderCopy(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text,
               int x, int y, Color color)
{
    SDL_Color c = {color.r, color.g, color.b, color.a};
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, c);
    if (!surf)
        return;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    if (!tex)
    {
        SDL_FreeSurface(surf);
        return;
    }

    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

void draw_grid(SDL_Renderer *r, float anim)
{
    set_color(r, COLOR_GRID);

    for (int x = 0; x < GRID_WIDTH; x++)
    {
        for (int y = 0; y < GRID_HEIGHT; y++)
        {
            float pulse = sinf((x + y) * 0.2f + anim) * 0.5f + 0.5f;
            SDL_SetRenderDrawColor(r,
                                   COLOR_GRID.r + (int)(pulse * 20),
                                   COLOR_GRID.g + (int)(pulse * 20),
                                   COLOR_GRID.b + (int)(pulse * 40),
                                   COLOR_GRID.a);
            SDL_Rect rect = {x * CELL_SIZE, y * CELL_SIZE, 1, 1};
            SDL_RenderFillRect(r, &rect);
        }
    }
}

void draw_cell_glow(SDL_Renderer *r, int gx, int gy, Color color, float intensity)
{
    int px = gx * CELL_SIZE;
    int py = gy * CELL_SIZE;

    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, (int)(100 * intensity));
    SDL_Rect glow = {px - 2, py - 2, CELL_SIZE + 4, CELL_SIZE + 4};
    SDL_RenderFillRect(r, &glow);

    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 255);
    SDL_Rect cell = {px + 1, py + 1, CELL_SIZE - 2, CELL_SIZE - 2};
    SDL_RenderFillRect(r, &cell);
}

void draw_snake(SDL_Renderer *r, Snake *snake)
{
    for (int i = snake->length - 1; i > 0; i--)
    {
        float alpha = snake->trail_alpha[i];
        Color c = COLOR_SNAKE_BODY;
        c.a = (Uint8)(255 * alpha);

        int t = 255 - i * 10;
        if (t < 100)
            t = 100;
        SDL_SetRenderDrawColor(r, c.r, c.g, c.b, t);

        draw_rect(r, snake->body[i].x * CELL_SIZE + 1,
                  snake->body[i].y * CELL_SIZE + 1,
                  CELL_SIZE - 2, CELL_SIZE - 2);
    }

    draw_cell_glow(r, snake->body[0].x, snake->body[0].y,
                   COLOR_SNAKE_HEAD, 0.8f);
}

void draw_food(SDL_Renderer *r, Food *food)
{
    if (!food->active)
        return;

    Color color = COLOR_FOOD_NORMAL;
    if (food->type == FOOD_BONUS)
        color = COLOR_FOOD_BONUS;
    else if (food->type == FOOD_MEGA)
        color = COLOR_FOOD_MEGA;

    float pulse = sinf(food->pulse) * 0.3f + 0.7f;
    draw_cell_glow(r, food->pos.x, food->pos.y, color, pulse);
}

void draw_powerup(SDL_Renderer *r, PowerUp *pw)
{
    if (!pw->active)
        return;

    float t = (SDL_GetTicks() - pw->spawn_time) / 1000.0f;
    if (t > 5.0f)
        return;

    float pulse = sinf(t * 5) * 0.5f + 0.5f;
    draw_cell_glow(r, pw->pos.x, pw->pos.y, COLOR_ACCENT, pulse);
}

void draw_obstacles(SDL_Renderer *r, Obstacle *obs, int count)
{
    Color gray = {80, 80, 100, 255};
    for (int i = 0; i < count; i++)
    {
        if (obs[i].active)
        {
            draw_cell_glow(r, obs[i].pos.x, obs[i].pos.y, gray, 0.5f);
        }
    }
}

void draw_particles(SDL_Renderer *r, ParticleSystem *ps)
{
    for (int i = 0; i < ps->count; i++)
    {
        Particle *p = &ps->particles[i];
        SDL_SetRenderDrawColor(r, p->r, p->g, p->b, (Uint8)(255 * p->life));
        SDL_Rect rect = {(int)p->x, (int)p->y, 3, 3};
        SDL_RenderFillRect(r, &rect);
    }
}

void draw_hud(SDL_Renderer *r, Fonts *fonts, Game *game)
{
    char buf[128];

    snprintf(buf, sizeof(buf), "SCORE: %d", game->score);
    draw_text(r, fonts->medium, buf, 10, 10, COLOR_TEXT);

    if (game->combo.count >= 3)
    {
        snprintf(buf, sizeof(buf), "COMBO x%.1f", game->combo.multiplier);
        Color combo_color = game->combo.fever_mode ? (Color){255, 0, 255, 255} : (Color){255, 255, 0, 255};
        draw_text(r, fonts->small, buf, 10, 50, combo_color);
    }

    if (game->snake.has_shield)
    {
        draw_text(r, fonts->small, "SHIELD", 10, 80, COLOR_ACCENT);
    }

    if (game->active_powerup != POWERUP_NONE)
    {
        const char *name = "";
        switch (game->active_powerup)
        {
        case POWERUP_SLOW:
            name = "SLOW MOTION";
            break;
        case POWERUP_SCORE_X2:
            name = "SCORE x2";
            break;
        case POWERUP_GHOST:
            name = "GHOST MODE";
            break;
        default:
            break;
        }

        unsigned int elapsed = SDL_GetTicks() - game->powerup_start_time;
        int remaining = (game->powerup.duration - elapsed) / 1000;
        snprintf(buf, sizeof(buf), "%s (%ds)", name, remaining);
        draw_text(r, fonts->small, buf, WINDOW_WIDTH - 180, 10, COLOR_ACCENT);
    }
}

void draw_menu(SDL_Renderer *r, Fonts *fonts, float pulse)
{
    float scale = 1.0f + sinf(pulse) * 0.05f;

    draw_text_centered(r, fonts->large, "SNAKE PREMIUM",
                       (int)(100 * scale), COLOR_SNAKE_HEAD);

    draw_text_centered(r, fonts->medium, "Press SPACE to Play", 250, COLOR_TEXT);
    draw_text_centered(r, fonts->small, "Arrow Keys: Move", 330, COLOR_ACCENT);
    draw_text_centered(r, fonts->small, "P: Pause  |  S: Stats  |  ESC: Quit", 370, COLOR_ACCENT);
}

void draw_game_over(SDL_Renderer *r, Fonts *fonts, int score, int high)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 180);
    draw_rect(r, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    draw_text_centered(r, fonts->large, "GAME OVER", 150, COLOR_FOOD_NORMAL);

    char buf[64];
    snprintf(buf, sizeof(buf), "Score: %d", score);
    draw_text_centered(r, fonts->medium, buf, 230, COLOR_TEXT);

    if (score >= high)
    {
        draw_text_centered(r, fonts->small, "NEW HIGH SCORE!", 280, COLOR_FOOD_MEGA);
    }

    draw_text_centered(r, fonts->small, "R: Restart  |  ESC: Menu", 350, COLOR_ACCENT);
}

void draw_stats(SDL_Renderer *r, Fonts *fonts, Stats *stats)
{
    set_color(r, COLOR_BG);
    SDL_RenderClear(r);

    draw_text_centered(r, fonts->large, "STATISTICS", 50, COLOR_ACCENT);

    char buf[128];
    int y = 150;

    snprintf(buf, sizeof(buf), "High Score: %d", stats->high_score);
    draw_text_centered(r, fonts->medium, buf, y, COLOR_TEXT);
    y += 50;

    snprintf(buf, sizeof(buf), "Longest Snake: %d", stats->longest_snake);
    draw_text_centered(r, fonts->medium, buf, y, COLOR_TEXT);
    y += 50;

    snprintf(buf, sizeof(buf), "Total Games: %d", stats->total_games);
    draw_text_centered(r, fonts->medium, buf, y, COLOR_TEXT);
    y += 50;

    snprintf(buf, sizeof(buf), "Total Food Eaten: %d", stats->total_food_eaten);
    draw_text_centered(r, fonts->medium, buf, y, COLOR_TEXT);
    y += 50;

    int mins = (int)(stats->total_playtime / 60);
    int secs = (int)(stats->total_playtime) % 60;
    snprintf(buf, sizeof(buf), "Playtime: %dm %ds", mins, secs);
    draw_text_centered(r, fonts->medium, buf, y, COLOR_TEXT);

    draw_text_centered(r, fonts->small, "Press ESC to return", WINDOW_HEIGHT - 50, COLOR_ACCENT);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        printf("SDL Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0)
    {
        printf("TTF Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        printf("Mixer Warning: %s\n", Mix_GetError());
    }

    SDL_Window *win = SDL_CreateWindow("Snake Premium Edition",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);

    if (!win)
    {
        printf("Window Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *r = SDL_CreateRenderer(win, -1,
                                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!r)
    {
        printf("Renderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    Fonts fonts;
    fonts.large = TTF_OpenFont("assets/font.ttf", 48);
    fonts.medium = TTF_OpenFont("assets/font.ttf", 32);
    fonts.small = TTF_OpenFont("assets/font.ttf", 20);
    fonts.tiny = TTF_OpenFont("assets/font.ttf", 16);

    if (!fonts.large || !fonts.medium || !fonts.small)
    {
        printf("Font Error! Place font.ttf in assets/\n");
        SDL_DestroyRenderer(r);
        SDL_DestroyWindow(win);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    Game game;
    game_init(&game);

    printf("=== SNAKE PREMIUM EDITION ===\n");
    printf("Controls: Arrows, P=Pause, S=Stats\n\n");

    bool running = true;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN)
            {
                SDL_Keycode key = event.key.keysym.sym;

                if (key == SDLK_ESCAPE)
                {
                    if (game.state == GAME_STATS)
                    {
                        game.state = GAME_MENU;
                    }
                    else if (game.state != GAME_MENU)
                    {
                        game.state = GAME_MENU;
                    }
                    else
                    {
                        running = false;
                    }
                }
                else if (key == SDLK_SPACE && game.state == GAME_MENU)
                {
                    game_init(&game);
                    game.state = GAME_PLAYING;
                }
                else if (key == SDLK_s && game.state == GAME_MENU)
                {
                    game.state = GAME_STATS;
                }
                else if (key == SDLK_r && game.state == GAME_OVER)
                {
                    game_init(&game);
                    game.state = GAME_PLAYING;
                }
                else if (key == SDLK_p && (game.state == GAME_PLAYING || game.state == GAME_PAUSED))
                {
                    game.state = (game.state == GAME_PLAYING) ? GAME_PAUSED : GAME_PLAYING;
                }
                else if (game.state == GAME_PLAYING)
                {
                    if (key == SDLK_UP && game.snake.direction != DIR_DOWN)
                    {
                        game.snake.next_direction = DIR_UP;
                    }
                    else if (key == SDLK_DOWN && game.snake.direction != DIR_UP)
                    {
                        game.snake.next_direction = DIR_DOWN;
                    }
                    else if (key == SDLK_LEFT && game.snake.direction != DIR_RIGHT)
                    {
                        game.snake.next_direction = DIR_LEFT;
                    }
                    else if (key == SDLK_RIGHT && game.snake.direction != DIR_LEFT)
                    {
                        game.snake.next_direction = DIR_RIGHT;
                    }
                }
            }
        }

        if (game.state == GAME_PLAYING)
        {
            game_update(&game);
        }

        game.menu_pulse += 0.05f;
        game.grid_anim += 0.02f;

        set_color(r, COLOR_BG);
        SDL_RenderClear(r);

        if (game.state == GAME_MENU)
        {
            draw_grid(r, game.grid_anim);
            draw_menu(r, &fonts, game.menu_pulse);
        }
        else if (game.state == GAME_STATS)
        {
            draw_stats(r, &fonts, &game.stats);
        }
        else
        {
            draw_grid(r, game.grid_anim);
            draw_obstacles(r, game.obstacles, game.obstacle_count);
            draw_food(r, &game.food);
            draw_powerup(r, &game.powerup);
            draw_snake(r, &game.snake);
            draw_particles(r, &game.particles);
            draw_hud(r, &fonts, &game);

            if (game.state == GAME_OVER)
            {
                draw_game_over(r, &fonts, game.score, game.stats.high_score);
            }
            else if (game.state == GAME_PAUSED)
            {
                SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(r, 0, 0, 0, 180);
                draw_rect(r, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
                draw_text_centered(r, fonts.large, "PAUSED", 200, COLOR_ACCENT);
                draw_text_centered(r, fonts.small, "Press P to Resume", 280, COLOR_TEXT);
            }
        }

        SDL_RenderPresent(r);
        SDL_Delay(16);
    }

    unsigned int session_time = (SDL_GetTicks() - game.game_start_time) / 1000;
    game.stats.total_playtime += session_time;
    stats_save(&game.stats);

    TTF_CloseFont(fonts.large);
    TTF_CloseFont(fonts.medium);
    TTF_CloseFont(fonts.small);
    TTF_CloseFont(fonts.tiny);
    Mix_CloseAudio();
    TTF_Quit();
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(win);
    SDL_Quit();

    printf("\nThanks for playing!\n");
    return 0;
}