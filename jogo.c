#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "screen.h"  // desenho no terminal   [oai_citation:0‡GitHub](https://github.com/tgfb/cli-lib/blob/main/include/screen.h?raw=1)
#include "keyboard.h"  // leitura de teclado    [oai_citation:1‡GitHub](https://github.com/tgfb/cli-lib/blob/main/include/keyboard.h?raw=1)
#include "timer.h"     // controle de tempo     [oai_citation:2‡GitHub](https://github.com/tgfb/cli-lib/blob/main/include/timer.h?raw=1)

void screenSetChar(int x, int y, char ch);
/* ---------- Constantes de jogo ---------- */
#define FIELD_W   (MAXX - 2)   /* área útil dentro da moldura da cli-lib */
#define FIELD_H   (MAXY - 4)   /* duas linhas para placar + molduras */
#define PADDLE_W  7            /* largura do taco */
#define PUCK_CHAR 'O'
#define PAD_CHAR  '='
#define FRAME_MS  50           /* taxa de atualização do timer (20 fps) */
#define GAME_TIME 120          /* duração da partida em segundos */
#define HS_FILE   "high_scores.txt"
#define HS_LIMIT  10           /* salvar os 10 melhores placares */

/* ---------- Estruturas ---------- */
typedef struct {
    int x;                 /* posição horizontal (coluna esquerda) */
} Paddle;

typedef struct {
    double x, y;           /* posição (float para movimento suave) */
    double vx, vy;         /* velocidade */
} Puck;

/* Lista encadeada para high‑scores */
typedef struct ScoreEntry {
    char   name[20];
    int    gols;           /* gols marcados (placar final) */
    struct ScoreEntry *next;
} ScoreEntry;

/* ---------- Variáveis globais ---------- */
static Paddle pad1, pad2;
static Puck   puck;
static int    score1 = 0, score2 = 0;
static time_t start_time;
static ScoreEntry *hs_head = NULL;   /* início da lista de high‑scores */

/* ---------- Funções utilitárias ---------- */
static void draw_border(void) {
    for (int x = MINX; x <= MAXX; ++x) {
        screenSetChar(x, MINY, '#');
        screenSetChar(x, MAXY, '#');
    }
    for (int y = MINY; y <= MAXY; ++y) {
        screenSetChar(MINX, y, '#');
        screenSetChar(MAXX, y, '#');
    }
}

static void clear_field(void) {
    for (int y = MINY + 1; y < MAXY; ++y)
        for (int x = MINX + 1; x < MAXX; ++x)
            screenSetChar(x, y, ' ');
}

static void draw_paddles(void) {
    /* Jogador 1 (em cima) */
    for (int i = 0; i < PADDLE_W; ++i)
        screenSetChar(pad1.x + i, MINY + 1, PAD_CHAR);
    /* Jogador 2 (embaixo) */
    for (int i = 0; i < PADDLE_W; ++i)
        screenSetChar(pad2.x + i, MAXY - 1, PAD_CHAR);
}

static void draw_puck(void) {
    screenSetChar((int)puck.x, (int)puck.y, PUCK_CHAR);
}

static void draw_score_and_timer(void) {
    int elapsed = (int)difftime(time(NULL), start_time);
    int remaining = (GAME_TIME - elapsed > 0) ? GAME_TIME - elapsed : 0;
    char info[64];
    snprintf(info, sizeof(info), "P1 %d  |  Tempo: %02d:%02d  |  P2 %d", score1, remaining / 60, remaining % 60, score2);
    int len = (int)strlen(info);
    int start_col = (MAXX - MINX - len) / 2 + MINX + 1;
    for (int i = 0; i < len; ++i)
        screenSetChar(start_col + i, MINY + 2, info[i]);
}

/* ---------- High‑scores (lista encadeada) ---------- */
static void hs_insert(const char *name, int gols) {
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

static void hs_load(void) {
    FILE *f = fopen(HS_FILE, "r");
    if (!f) return;
    char name[20];
    int  g;
    while (fscanf(f, "%19s %d", name, &g) == 2)
        hs_insert(name, g);
    fclose(f);
}

static void hs_save(void) {
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

static void hs_free(void) {
    ScoreEntry *cur = hs_head;
    while (cur) {
        ScoreEntry *tmp = cur->next;
        free(cur);
        cur = tmp;
    }
}

/* ---------- Inicialização ---------- */
static void init_game(void) {
    pad1.x = MINX + (FIELD_W - PADDLE_W)/2;
    pad2.x = pad1.x;

    puck.x = MINX + FIELD_W / 2;
    puck.y = MINY + FIELD_H / 2;
    puck.vx = 1;
    puck.vy = 1;

    score1 = score2 = 0;
    start_time = time(NULL);
}

/* ---------- Colisão ---------- */
static void check_collisions(void) {
    /* Paredes laterais */
    if (puck.x <= MINX + 1 || puck.x >= MAXX - 1)
        puck.vx = -puck.vx;

    /* Goals (linhas superior e inferior) */
    if (puck.y <= MINY + 1) {
        ++score2; /* Gol do jogador 2 */
        init_game();
        return;
    }
    if (puck.y >= MAXY - 1) {
        ++score1; /* Gol do jogador 1 */
        init_game();
        return;
    }

    /* Tacos – linha do jogador 1 */
    if ((int)puck.y == MINY + 2) {
        if ((int)puck.x >= pad1.x && (int)puck.x <= pad1.x + PADDLE_W) {
            puck.vy = -puck.vy;
            if (puck.vy < 0) puck.vy -= 0.1; /* acelera */
        }
    }
    /* Tacos – linha do jogador 2 */
    if ((int)puck.y == MAXY - 2) {
        if ((int)puck.x >= pad2.x && (int)puck.x <= pad2.x + PADDLE_W) {
            puck.vy = -puck.vy;
            if (puck.vy > 0) puck.vy += 0.1; /* acelera */
        }
    }
}

/* ---------- Atualização de estado ---------- */
static void update_puck(void) {
    puck.x += puck.vx;
    puck.y += puck.vy;
    check_collisions();
}

static void handle_input(void) {
    if (!keyhit()) return;
    int ch = readch();
    switch (ch) {
        /* jogador 1 – A / D */
        case 'a': case 'A':
            if (pad1.x > MINX + 1) pad1.x -= 2;
            break;
        case 'd': case 'D':
            if (pad1.x + PADDLE_W < MAXX - 1) pad1.x += 2;
            break;
        /* jogador 2 – setas esquerda/direita */
        case 27: { /* esc seq */
            int n1 = readch();
            int n2 = readch();
            if (n1 == 91) {
                if (n2 == 68) { /* ← */
                    if (pad2.x > MINX + 1) pad2.x -= 2;
                } else if (n2 == 67) { /* → */
                    if (pad2.x + PADDLE_W < MAXX - 1) pad2.x += 2;
                }
            }
            break;
        }
        case 'q': case 'Q':
            /* encerrar antes do tempo */
            start_time -= GAME_TIME; /* força fim */
            break;
    }
}

/* ---------- Render ---------- */
static void render(void) {
    clear_field();
    draw_border();
    draw_paddles();
    draw_puck();
    draw_score_and_timer();
    screenUpdate();
}

/* ---------- Função principal ---------- */
int main(void) {
    /* Inicializações da cli‑lib */
    screenInit(1);
    keyboardInit();
    timerInit(FRAME_MS);

    /* High scores */
    hs_load();

    init_game();
    render();

    while (difftime(time(NULL), start_time) < GAME_TIME) {
        if (timerTimeOver())
            update_puck();

        handle_input();
        render();
    }

    /* Fim de jogo – exibe resultado e salva high‑score */
    clear_field();
    char msg[64];
    if (score1 > score2)
        snprintf(msg, sizeof(msg), "Jogador 1 venceu! (%d x %d)", score1, score2);
    else if (score2 > score1)
        snprintf(msg, sizeof(msg), "Jogador 2 venceu! (%d x %d)", score2, score1);
    else
        snprintf(msg, sizeof(msg), "Empate! (%d x %d)", score1, score2);

    /* Centraliza */
    int col = (MAXX - MINX - (int)strlen(msg)) / 2 + MINX + 1;
    int row = MINY + FIELD_H / 2;
    for (const char *p = msg; *p; ++p)
        screenSetChar(col++, row, *p);
    screenUpdate();

    /* Pega nome do vencedor ou empate */
    keyboardDestroy();
    printf("\nDigite seu nome para salvar no ranking: ");
    char nome[20];
    fgets(nome, sizeof(nome), stdin);
    nome[strcspn(nome, "\n")] = 0; /* remove '\n' */
    if (nome[0]) {
        hs_insert(nome, (score1 > score2) ? score1 : score2);
        hs_save();
    }

    /* exibe top scores */
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

/* ----------------- Makefile (salve como Makefile) -----------------
CC = gcc
CFLAGS = -Wall -std=c11 -I./cli-lib/include
LDFLAGS = -L./cli-lib/lib -lcli

all: air_hockey

air_hockey: air_hockey.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

clean:
	rm -f air_hockey
--------------------------------------------------------------------- */