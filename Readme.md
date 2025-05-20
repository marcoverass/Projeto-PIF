# 🏒 Projeto Air Hockey - PIF 2025.1

> Jogo desenvolvido para a disciplina de Programação Imperativa e Funcional (CESAR School).

## 🎮 Sobre o Jogo

Air Hockey de 2 jogadores, em linha de comando, com:
- Tacos controlados por teclado
- Disco com rebotes e aceleração
- Placar, tempo e sistema de high scores
- Animação via terminal com `cli-lib`

## 👨‍💻 Controles

- **Jogador 1**: `A` (esquerda), `D` (direita)
- **Jogador 2**: `←` e `→` (setas do teclado)
- **Q**: Finaliza o jogo manualmente

## 🧱 Estrutura do Código

- `jogo.c`: Lógica principal
- `cli-lib/`: Biblioteca gráfica obrigatória
- `Makefile`: Script de compilação
- `high_scores.txt`: Arquivo gerado com os melhores resultados

## ⚙️ Como Rodar

```bash
git clone https://github.com/marcoverass/Projeto-PIF.git
cd Projeto-PIF
make
./air_hockey
