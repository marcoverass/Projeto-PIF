#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "screen.h"
#include "keyboard.h"
#include "timer.h"

void screenSetChar(int x, int y, char ch) {
    screenGotoxy(x, y);
    putchar(ch);
}

/* ---------- Constantes ---------- */
#define FIELD_W   (MAXX - 2)
#define FIELD_H   (MAXY - 4)
#define PADDLE_W  7
#define PUCK_CHAR 'O'
#define PAD_CHAR  '='
#define FRAME_MS  50
#define GAME_TIME 120
#define HS_FILE   "high_scores.txt"
#define HS_LIMIT  10

/* ---------- Structs ---------- */
typedef struct {
    int x;
} Paddle;

typedef struct {
    double x, y;
    double vx, vy;
} Puck;

typedef struct ScoreEntry {
    char name[20];
    int gols;
    struct ScoreEntry *next;
} ScoreEntry;

/* ---------- Globais ---------- */
static Paddle pad1, pad2;
static Puck puck;
static int score1 = 0, score2 = 0;
static time_t start_time;
static ScoreEntry *hs_head = NULL;
static char campo[FIELD_H][FIELD_W];  // MATRIZ ADICIONADA

/* ---------- Campo com matriz ---------- */
void campoLimpar() {
    for (int y = 0; y < FIELD_H; ++y)
        for (int x = 0; x < FIELD_W; ++x)
            campo[y][x] = ' ';
}

void campoDesenhar() {
    for (int y = 0; y < FIELD_H; ++y)
        for (int x = 0; x < FIELD_W; ++x)
            screenSetChar(x + MINX + 1, y + MINY + 1, campo[y][x]);
}

/* ---------- Utilitários ---------- */
void draw_border(void) {
    for (int x = MINX; x <= MAXX; ++x) {
        screenSetChar(x, MINY, '#');
        screenSetChar(x, MAXY, '#');
    }
    for (int y = MINY; y <= MAXY; ++y) {
        screenSetChar(MINX, y, '#');
        screenSetChar(MAXX, y, '#');
    }
}

void draw_paddles(void) {
    for (int i = 0; i < PADDLE_W; ++i)
        campo[0][pad1.x - MINX - 1 + i] = PAD_CHAR;
    for (int i = 0; i < PADDLE_W; ++i)
        campo[FIELD_H - 1][pad2.x - MINX - 1 + i] = PAD_CHAR;
}

void draw_puck(void) {
    int x = (int)puck.x - MINX - 1;
    int y = (int)puck.y - MINY - 1;
    if (x >= 0 && x < FIELD_W && y >= 0 && y < FIELD_H)
        campo[y][x] = PUCK_CHAR;
}

void draw_score_and_timer(void) {
    int elapsed = (int)difftime(time(NULL), start_time);
    int remaining;
    if (GAME_TIME - elapsed > 0)
        remaining = GAME_TIME - elapsed;
    else
        remaining = 0;

    char info[64];
    snprintf(info, sizeof(info), "P1 %d  |  Tempo: %02d:%02d  |  P2 %d",
             score1, remaining / 60, remaining % 60, score2);
    int len = (int)strlen(info);
    int start_col = (MAXX - MINX - len) / 2 + MINX + 1;
    for (int i = 0; i < len; ++i)
        screenSetChar(start_col + i, MINY + 2, info[i]);
}

/* ---------- High Scores ---------- */
void hs_insert(const char *name, int gols) {
    ScoreEntry *node = malloc(sizeof(ScoreEntry));
    if (!node) return;
    strncpy(node->name, name, sizeof(node->name) - 1);
    node->name[sizeof(node->name)-1] = '\0';
    node->gols = gols;
    node->next = NULL;

    if (!hs_head || gols > hs_head->gols) {
        node->next = hs_head;
        hs_head = node;
        return;
    }
    ScoreEntry *cur = hs_head;
    while (cur->next && cur->next->gols >= gols)
        cur = cur->next;
    node->next = cur->next;
    cur->next = node;
}

void hs_load(void) {
    FILE *f = fopen(HS_FILE, "r");
    if (!f) return;
    char name[20];
    int g;
    while (fscanf(f, "%19s %d", name, &g) == 2)
        hs_insert(name, g);
    fclose(f);
}

void hs_save(void) {
    FILE *f = fopen(HS_FILE, "w");
    if (!f) return;
    ScoreEntry *cur = hs_head;
    int count = 0;
    while (cur && count < HS_LIMIT) {
        fprintf(f, "%s %d\n", cur->name, cur->gols);
        cur = cur->next;
        ++count;
    }
    fclose(f);
}

void hs_free(void) {
    ScoreEntry *cur = hs_head;
    while (cur) {
        ScoreEntry *tmp = cur->next;
        free(cur);
        cur = tmp;
    }
}

/* ---------- Inicialização ---------- */
void init_game(void) {
    pad1.x = MINX + (FIELD_W - PADDLE_W) / 2;
    pad2.x = pad1.x;

    puck.x = MINX + FIELD_W / 2;
    puck.y = MINY + FIELD_H / 2;
    puck.vx = 1;
    puck.vy = 1;

    score1 = score2 = 0;
    start_time = time(NULL);
}

/* ---------- Colisões ---------- */
void check_collisions(void) {
    if (puck.x <= MINX + 1 || puck.x >= MAXX - 1)
        puck.vx = -puck.vx;

    if (puck.y <= MINY + 1) {
        ++score2;
        init_game();
        return;
    }
    if (puck.y >= MAXY - 1) {
        ++score1;
        init_game();
        return;
    }

    if ((int)puck.y == MINY + 2) {
        if ((int)puck.x >= pad1.x && (int)puck.x <= pad1.x + PADDLE_W) {
            puck.vy = -puck.vy;
            if (puck.vy < 0) puck.vy -= 0.1;
        }
    }

    if ((int)puck.y == MAXY - 2) {
        if ((int)puck.x >= pad2.x && (int)puck.x <= pad2.x + PADDLE_W) {
            puck.vy = -puck.vy;
            if (puck.vy > 0) puck.vy += 0.1;
        }
    }
}

/* ---------- Atualizações ---------- */
void update_puck(void) {
    puck.x += puck.vx;
    puck.y += puck.vy;
    check_collisions();
}

void handle_input(void) {
    if (!keyhit()) return;
    int ch = readch();
    switch (ch) {
        case 'a': case 'A':
            if (pad1.x > MINX + 1) pad1.x -= 2;
            break;
        case 'd': case 'D':
            if (pad1.x + PADDLE_W < MAXX - 1) pad1.x += 2;
            break;
        case 27: {
            int n1 = readch();
            int n2 = readch();
            if (n1 == 91) {
                if (n2 == 68) {
                    if (pad2.x > MINX + 1) pad2.x -= 2;
                } else if (n2 == 67) {
                    if (pad2.x + PADDLE_W < MAXX - 1) pad2.x += 2;
                }
            }
            break;
        }
        case 'q': case 'Q':
            start_time -= GAME_TIME;
            break;
    }
}

/* ---------- Render ---------- */
void render(void) {
    campoLimpar();
    draw_paddles();
    draw_puck();
    campoDesenhar();
    draw_border();
    draw_score_and_timer();
    screenUpdate();
}

/* ---------- Main ---------- */
int main(void) {
    screenInit(1);
    keyboardInit();
    timerInit(FRAME_MS);
    hs_load();

    init_game();
    render();

    while (difftime(time(NULL), start_time) < GAME_TIME) {
        if (timerTimeOver())
            update_puck();
        handle_input();
        render();
    }

    clear_field();
    char msg[64];
    if (score1 > score2) {
        snprintf(msg, sizeof(msg), "Jogador 1 venceu! (%d x %d)", score1, score2);
    } else {
        if (score2 > score1) {
            snprintf(msg, sizeof(msg), "Jogador 2 venceu! (%d x %d)", score2, score1);
        } else {
            snprintf(msg, sizeof(msg), "Empate! (%d x %d)", score1, score2);
        }
    }

    int col = (MAXX - MINX - (int)strlen(msg)) / 2 + MINX + 1;
    int row = MINY + FIELD_H / 2;
    for (const char *p = msg; *p; ++p)
        screenSetChar(col++, row, *p);
    screenUpdate();

    keyboardDestroy();
    printf("\nDigite seu nome para salvar no ranking: ");
    char nome[20];
    fgets(nome, sizeof(nome), stdin);
    nome[strcspn(nome, "\n")] = 0;
    if (nome[0]) {
        if (score1 > score2)
            hs_insert(nome, score1);
        else if (score2 > score1)
            hs_insert(nome, score2);
        else
            hs_insert(nome, score1);
        hs_save();
    }

    printf("\n===== TOP %d SCORES =====\n", HS_LIMIT);
    ScoreEntry *cur = hs_head;
    int pos = 1;
    while (cur && pos <= HS_LIMIT) {
        printf("%2d. %-18s %d\n", pos, cur->name, cur->gols);
        cur = cur->next;
        ++pos;
    }

    hs_free();
    screenDestroy();
    timerDestroy();
    return 0;
}
