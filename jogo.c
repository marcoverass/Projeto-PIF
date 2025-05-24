#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "screen.h"
#include "keyboard.h"
#include "timer.h"

#define LARGURA_CAMPO (MAXX - 2)
#define ALTURA_CAMPO  (MAXY - 6)
#define LARGURA_TACO  9
#define SIMBOLO_TACO  '='
#define SIMBOLO_DISCO 'O'
#define TEMPO_FRAME   50
#define TEMPO_JOGO    90
#define ARQ_RANKING   "ranking.txt"
#define MAX_RANKING   10

void screenSetChar(int x, int y, char ch) {
    screenGotoxy(x, y);
    putchar(ch);
}

typedef struct {
    int x;
} Taco;

typedef struct {
    double x, y;
    double vx, vy;
} Disco;

typedef struct Ranking {
    char nome[20];
    int pontos;
    struct Ranking *next;
} Ranking;

Taco taco1, taco2;
Disco disco;
int pontos1 = 0, pontos2 = 0;
time_t inicio;
Ranking *inicioRanking = NULL;
char campo[ALTURA_CAMPO][LARGURA_CAMPO];

void limparCampo() {
    for (int y = 0; y < ALTURA_CAMPO; y++)
        for (int x = 0; x < LARGURA_CAMPO; x++)
            campo[y][x] = ' ';
}

void desenharBorda() {
    for (int x = MINX; x <= MAXX; x++) {
        screenSetChar(x, MINY + 2, '#');
        screenSetChar(x, MAXY, '#');
    }
    for (int y = MINY + 2; y <= MAXY; y++) {
        screenSetChar(MINX, y, '#');
        screenSetChar(MAXX, y, '#');
    }
}

void desenharPlacar() {
    int t = (int)difftime(time(NULL), inicio);
    int restante;
    if (TEMPO_JOGO - t > 0) {
        restante = TEMPO_JOGO - t;
    } else {
        restante = 0;
    }

    char info[64];
    snprintf(info, sizeof(info), "J1:%d | Tempo:%02d:%02d | J2:%d", pontos1, restante / 60, restante % 60, pontos2);
    int col = (MAXX - MINX - (int)strlen(info)) / 2 + MINX;
    for (int i = 0; info[i]; i++) screenSetChar(col + i, MINY, info[i]);
}

void desenharCampo() {
    for (int y = 0; y < ALTURA_CAMPO; y++) {
        for (int x = 0; x < LARGURA_CAMPO - 1; x++) {
            screenSetChar(x + MINX + 1, y + MINY + 3, campo[y][x]);
        }
    }
}

void desenharTacos() {
    for (int i = 0; i < LARGURA_TACO; i++)
        campo[1][taco1.x - MINX - 1 + i] = SIMBOLO_TACO;
    for (int i = 0; i < LARGURA_TACO; i++)
        campo[ALTURA_CAMPO - 2][taco2.x - MINX - 1 + i] = SIMBOLO_TACO;
}

void desenharDisco() {
    int x = (int)(disco.x - MINX - 1);
    int y = (int)(disco.y - MINY - 3);
    if (x >= 0 && x < LARGURA_CAMPO && y >= 0 && y < ALTURA_CAMPO)
        campo[y][x] = SIMBOLO_DISCO;
}

void renderizar() {
    limparCampo();
    desenharTacos();
    desenharDisco();
    desenharCampo();
    desenharBorda();
    desenharPlacar();
    screenUpdate();
}

void inicializarJogo() {
    taco1.x = taco2.x = MINX + (LARGURA_CAMPO - LARGURA_TACO) / 2;
    disco.x = MINX + LARGURA_CAMPO / 2;
    disco.y = MINY + ALTURA_CAMPO / 2 + 3;
    disco.vx = 1;
    disco.vy = 1;
    inicio = time(NULL);
}

void verificarColisoes() {
    if (disco.x <= MINX + 1 || disco.x >= MAXX - 1)
        disco.vx *= -1;

    if (disco.y <= MINY + 3) {
        pontos2++;
        inicializarJogo();
        return;
    } else if (disco.y >= MAXY - 1) {
        pontos1++;
        inicializarJogo();
        return;
    }

    if ((int)disco.y == MINY + 4) {
        if ((int)disco.x >= taco1.x && (int)disco.x <= taco1.x + LARGURA_TACO)
            disco.vy *= -1;
    }
    if ((int)disco.y == MAXY - 2) {
        if ((int)disco.x >= taco2.x && (int)disco.x <= taco2.x + LARGURA_TACO)
            disco.vy *= -1;
    }
}

void atualizarDisco() {
    disco.x += disco.vx;
    disco.y += disco.vy;
    verificarColisoes();
}

void processarTeclas() {
    if (!keyhit()) return;
    int ch = readch();
    switch (ch) {
        case 'a': case 'A': if (taco1.x > MINX + 1) taco1.x -= 3; break;
        case 'd': case 'D': if (taco1.x + LARGURA_TACO < MAXX - 1) taco1.x += 3; break;
        case 27: {
            int n1 = readch(); int n2 = readch();
            if (n1 == 91 && n2 == 68 && taco2.x > MINX + 1) taco2.x -= 3;
            else if (n1 == 91 && n2 == 67 && taco2.x + LARGURA_TACO < MAXX - 1) taco2.x += 3;
            break;
        }
        case 'q': case 'Q': inicio -= TEMPO_JOGO; break;
    }
}

void inserirRanking(const char *nome, int pontos) {
    Ranking *new = malloc(sizeof(Ranking));
    strcpy(new->nome, nome);
    new->pontos = pontos;
    new->next = NULL;

    if (!inicioRanking || pontos > inicioRanking->pontos) {
        new->next = inicioRanking;
        inicioRanking = new;
        return;
    }
    Ranking *cur = inicioRanking;
    while (cur->next && cur->next->pontos >= pontos)
        cur = cur->next;
    new->next = cur->next;
    cur->next = new;
}

void salvarRanking() {
    FILE *f = fopen(ARQ_RANKING, "w");
    if (!f) return;
    Ranking *cur = inicioRanking;
    int count = 0;
    while (cur && count < MAX_RANKING) {
        fprintf(f, "%s %d\n", cur->nome, cur->pontos);
        cur = cur->next;
        count++;
    }
    fclose(f);
}

void carregarRanking() {
    FILE *f = fopen(ARQ_RANKING, "r");
    if (!f) return;
    char nome[20]; int pontos;
    while (fscanf(f, "%19s %d", nome, &pontos) == 2)
        inserirRanking(nome, pontos);
    fclose(f);
}

void liberarRanking() {
    Ranking *cur = inicioRanking;
    while (cur) {
        Ranking *tmp = cur->next;
        free(cur);
        cur = tmp;
    }
}

int main() {
    screenInit(1);
    keyboardInit();
    timerInit(TEMPO_FRAME);

    carregarRanking();
    inicializarJogo();

    // Mensagem de início
    screenClear();
    const char *msg = "Pressione 'P' para iniciar o jogo";
    int pos = (MAXX - (int)strlen(msg)) / 2;
    for (int i = 0; msg[i]; i++) {
        screenSetChar(pos + i, MAXY / 2, msg[i]);
    }
    screenUpdate();

    while (1) {
        if (keyhit()) {
            int tecla = readch();
            if (tecla == 'p' || tecla == 'P') break;
        }
    }

    inicio = time(NULL);  // começa o cronômetro após o 'P'

    while (difftime(time(NULL), inicio) < TEMPO_JOGO) {
        if (timerTimeOver()) atualizarDisco();
        processarTeclas();
        renderizar();
    }

    screenDestroy();
    keyboardDestroy();
    timerDestroy();

    printf("\nDigite seu nome: ");
    char nome[20];
    fgets(nome, sizeof(nome), stdin);
    nome[strcspn(nome, "\n")] = 0;
    int final;
    if (pontos1 > pontos2) {
        final = pontos1;
    } else {
        final = pontos2;
    }
    inserirRanking(nome, final);
    salvarRanking();

    printf("\n=== RANKING ===\n");
    Ranking *cur = inicioRanking;
    int posicao = 1;
    while (cur && posicao <= MAX_RANKING) {
        printf("%2d. %-15s %d\n", posicao++, cur->nome, cur->pontos);
        cur = cur->next;
    }

    liberarRanking();
    return 0;
}
