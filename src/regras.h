// ============================================================
//  regras.h - As regras do jogo e o "resolvedor"
//  Nada aqui desenha na tela: e so a logica pura do jogo.
// ============================================================
#pragma once

#include <algorithm>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

const int CAPACIDADE = 4;       // frutas por caixote
const int CAIXOTES_VAZIOS = 2;  // caixotes vazios para manobrar
const int LIMITE_BUSCA = 200000;

enum Fruta { MACA, BANANA, UVA, LIMAO, LARANJA, MORANGO, MELANCIA, TOTAL_FRUTAS };

// Um caixote e uma pilha: o fim do vetor (back) e a fruta de cima.
using Caixote = vector<int>;
using Feira = vector<Caixote>;
using Jogada = pair<int, int>;  // (de, para)

// Caixote com todas as frutas iguais (vazio tambem conta)
bool uniforme(const Caixote& c) {
    for (int fruta : c) {
        if (fruta != c[0]) return false;
    }
    return true;
}

bool caixotePronto(const Caixote& c) {
    return (int)c.size() == CAPACIDADE && uniforme(c);
}

// Venceu quando todo caixote esta vazio ou pronto
bool venceu(const Feira& feira) {
    for (const Caixote& c : feira) {
        if (!c.empty() && !caixotePronto(c)) return false;
    }
    return true;
}

bool podeMover(const Feira& feira, int de, int para) {
    if (de == para || feira[de].empty()) return false;
    if ((int)feira[para].size() >= CAPACIDADE) return false;
    if (!feira[para].empty() && feira[para].back() != feira[de].back()) return false;
    return true;
}

// Quantas frutas iguais estao juntas no topo do caixote
int frutasIguaisNoTopo(const Caixote& c) {
    int qtd = 0;
    for (int i = (int)c.size() - 1; i >= 0 && c[i] == c.back(); i--) qtd++;
    return qtd;
}

// Quantas frutas vao de fato se mover (limitado pelo espaco no destino)
int quantasMovem(const Feira& feira, int de, int para) {
    int espaco = CAPACIDADE - (int)feira[para].size();
    return min(frutasIguaisNoTopo(feira[de]), espaco);
}

void mover(Feira& feira, int de, int para) {
    int qtd = quantasMovem(feira, de, para);
    for (int i = 0; i < qtd; i++) {
        feira[para].push_back(feira[de].back());
        feira[de].pop_back();
    }
}

// ============================================================
//  RESOLVEDOR: busca em profundidade (DFS) com backtracking
//  e memoria dos estados ja visitados.
// ============================================================

// Transforma a feira num texto unico. Os caixotes sao ordenados porque
// a ordem deles nao muda se a feira tem solucao ou nao.
string chave(const Feira& feira) {
    vector<string> partes;
    for (const Caixote& c : feira) {
        string s;
        for (int fruta : c) s += char('A' + fruta);
        partes.push_back(s);
    }
    sort(partes.begin(), partes.end());
    string resultado;
    for (const string& p : partes) resultado += p + "|";
    return resultado;
}

bool buscar(Feira& feira, vector<Jogada>& caminho, unordered_set<string>& vistos,
            int& tentativas, bool& estourouLimite) {
    if (venceu(feira)) return true;
    if (++tentativas > LIMITE_BUSCA) {
        estourouLimite = true;
        return false;
    }
    if (!vistos.insert(chave(feira)).second) return false;  // ja visitado

    int total = feira.size();
    int primeiroVazio = -1;
    for (int i = 0; i < total; i++) {
        if (feira[i].empty()) {
            primeiroVazio = i;
            break;
        }
    }

    // Monta a lista de jogadas possiveis com uma "nota" para cada uma.
    // Testar primeiro as melhores faz a busca achar solucoes mais curtas.
    struct Candidata {
        int de, para, nota;
    };
    vector<Candidata> candidatas;
    for (int de = 0; de < total; de++) {
        for (int para = 0; para < total; para++) {
            if (!podeMover(feira, de, para)) continue;
            bool destinoVazio = feira[para].empty();
            if (destinoVazio && uniforme(feira[de])) continue;    // nao ajuda
            if (destinoVazio && para != primeiroVazio) continue;  // vazios sao iguais
            int qtd = quantasMovem(feira, de, para);
            int nota = 0;
            if (!destinoVazio && uniforme(feira[para])) nota += 4;             // empilha fruta igual
            if (qtd == frutasIguaisNoTopo(feira[de])) nota += 2;              // leva o grupo inteiro
            if ((int)feira[de].size() > qtd) {
                Caixote resto(feira[de].begin(), feira[de].end() - qtd);
                if (uniforme(resto)) nota += 1;                               // origem fica arrumada
            }
            if (destinoVazio) nota -= 3;                                      // gastar vazio e o ultimo recurso
            candidatas.push_back({de, para, nota});
        }
    }
    stable_sort(candidatas.begin(), candidatas.end(),
                [](const Candidata& a, const Candidata& b) { return a.nota > b.nota; });

    for (const Candidata& c : candidatas) {
        Feira antes = feira;
        mover(feira, c.de, c.para);
        caminho.push_back({c.de, c.para});
        if (buscar(feira, caminho, vistos, tentativas, estourouLimite)) return true;
        caminho.pop_back();
        feira = antes;  // backtracking: desfaz e tenta outra
        if (estourouLimite) return false;
    }
    return false;
}

// true = achou solucao. "estourouLimite" = desistiu antes de ter certeza.
bool resolver(Feira feira, vector<Jogada>& caminho, bool& estourouLimite) {
    unordered_set<string> vistos;
    int tentativas = 0;
    estourouLimite = false;
    caminho.clear();
    return buscar(feira, caminho, vistos, tentativas, estourouLimite);
}

// ============================================================
//  FASES
// ============================================================

// Quantas frutas diferentes cada fase tem
int frutasDaFase(int fase) {
    if (fase <= 2) return 3;
    if (fase <= 4) return 4;
    if (fase <= 6) return 5;
    if (fase <= 9) return 6;
    return 7;
}

// A partir da fase 6 aparecem frutas escondidas no saquinho (estilo Magic Sort)
const int PRIMEIRA_FASE_ESCONDIDA = 6;

float chanceEscondida(int fase) {
    if (fase < PRIMEIRA_FASE_ESCONDIDA) return 0;
    return min(0.55f, 0.15f + 0.05f * (fase - PRIMEIRA_FASE_ESCONDIDA));
}

// Monta uma feira com "qtdFrutas" tipos, sempre com solucao
Feira criarFeira(int qtdFrutas, mt19937& gerador) {
    while (true) {
        // Sorteia quais frutas entram
        vector<int> tipos;
        for (int f = 0; f < TOTAL_FRUTAS; f++) tipos.push_back(f);
        shuffle(tipos.begin(), tipos.end(), gerador);
        tipos.resize(qtdFrutas);

        vector<int> todas;
        for (int tipo : tipos) {
            for (int i = 0; i < CAPACIDADE; i++) todas.push_back(tipo);
        }
        shuffle(todas.begin(), todas.end(), gerador);

        Feira feira(qtdFrutas + CAIXOTES_VAZIOS);
        int pos = 0;
        for (int c = 0; c < qtdFrutas; c++) {
            for (int i = 0; i < CAPACIDADE; i++) feira[c].push_back(todas[pos++]);
        }

        // Nao queremos caixote ja pronto no inicio
        bool temPronto = false;
        for (int c = 0; c < qtdFrutas; c++) {
            if (uniforme(feira[c])) temPronto = true;
        }
        if (temPronto) continue;

        // So aceita a feira se ela tiver solucao
        vector<Jogada> caminho;
        bool estourou;
        if (resolver(feira, caminho, estourou)) return feira;
    }
}

// ------------------------------------------------------------
//  Um nivel completo: as frutas e quais estao escondidas.
//  Tudo sai da "semente": a mesma semente gera sempre o mesmo
//  nivel. Assim a fase 12 e igual para todo mundo, como no
//  Candy Crush, e da para jogar de novo buscando 3 estrelas.
// ------------------------------------------------------------

using Escondidas = vector<vector<bool>>;  // [caixote][posicao]

struct Nivel {
    Feira feira;
    Escondidas escondidas;
};

Nivel gerarNivel(int qtdFrutas, float chanceDeEsconder, unsigned semente) {
    mt19937 gerador(semente);
    Nivel n;
    n.feira = criarFeira(qtdFrutas, gerador);
    n.escondidas.assign(n.feira.size(), vector<bool>(CAPACIDADE, false));
    uniform_real_distribution<float> sorteio(0, 1);
    for (int c = 0; c < qtdFrutas; c++) {
        for (int k = 0; k < CAPACIDADE - 1; k++) {  // a fruta de cima nunca comeca escondida
            if (sorteio(gerador) < chanceDeEsconder) n.escondidas[c][k] = true;
        }
    }
    return n;
}

Nivel nivelDaFase(int fase) {
    return gerarNivel(frutasDaFase(fase), chanceEscondida(fase), (unsigned)fase * 2654435761u + 12345u);
}

// Desafio do dia: a data (ex.: 20260925) vira a semente
Nivel nivelDoDesafio(int data) {
    return gerarNivel(6, 0.3f, (unsigned)data * 7919u + 777u);
}

// Revela o grupo de frutas de cima de cada caixote
void revelarTopos(const Feira& feira, Escondidas& escondidas) {
    escondidas.resize(feira.size(), vector<bool>(CAPACIDADE, false));
    for (size_t c = 0; c < feira.size(); c++) {
        int tamanho = feira[c].size();
        int grupo = feira[c].empty() ? 0 : frutasIguaisNoTopo(feira[c]);
        for (int k = 0; k < CAPACIDADE; k++) {
            if (k >= tamanho || k >= tamanho - grupo) escondidas[c][k] = false;
        }
    }
}
