// ============================================================
//  capitulos.h - A viagem do Seu Ze pelas feiras do Brasil.
//  Cada feira e um capitulo de 10 fases, com as frutas da regiao,
//  cores proprias e um postal (curiosidade + receita) no final.
//
//  As curiosidades foram conferidas em fontes publicas
//  (Wikipedia e imprensa local) em setembro de 2026.
// ============================================================
#pragma once

#include "raylib.h"
#include "regras.h"
#include "brasil.h"

const int FASES_POR_CAPITULO = 10;
const int TOTAL_CAPITULOS = 5;
const int PREMIO_CAPITULO = 50;  // moedas extras ao completar uma feira

struct Capitulo {
    const char* feira;       // nome da feira
    const char* cidade;      // cidade e estado
    const char* curto;       // nome curto, para o mapa
    const float* posicao;    // posicao no mapa do Brasil (0 a 1)
    int regionais[2];        // frutas da regiao (sempre aparecem)
    int opcionais[5];        // outras frutas que podem aparecer
    Color toldoA, toldoB;    // listras do toldo
    Color ceuTopo, ceuBase;  // cores do ceu
    const char* curiosidade;
    const char* receitaTitulo;
    const char* receita;
};

const Capitulo CAPITULOS[TOTAL_CAPITULOS] = {
    {"Mercadão de São Paulo", "São Paulo (SP)", "São Paulo", CIDADE_SP,
     {MORANGO, UVA}, {MACA, BANANA, LIMAO, LARANJA, MELANCIA},
     {210, 52, 48, 255}, {255, 247, 232, 255}, {138, 204, 242, 255}, {255, 238, 206, 255},
     "Inaugurado em 1933, o Mercadão tem vitrais do artista Conrado Sorgenicht Filho que mostram a produção de "
     "alimentos. O famoso sanduíche de mortadela surgiu lá nos anos 60.",
     "Salada de frutas",
     "Pique 1 maçã, 2 bananas, 1 xícara de uvas, 1 xícara de morangos e 2 fatias de melancia. Regue com o suco "
     "de 1 laranja e sirva gelada."},

    {"Mercado Central de Belo Horizonte", "Belo Horizonte (MG)", "Belo Horizonte", CIDADE_BH,
     {GOIABA, JABUTICABA}, {BANANA, LARANJA, MACA, LIMAO, MORANGO},
     {46, 96, 176, 255}, {255, 247, 232, 255}, {150, 200, 236, 255}, {250, 232, 206, 255},
     "Criado em 1929, o Mercado Central quase acabou em 1964, quando o terreno foi vendido. Os próprios "
     "comerciantes se juntaram e compraram o mercado para salvá-lo.",
     "Romeu e Julieta",
     "Corte uma fatia de queijo minas e uma fatia de goiabada do mesmo tamanho. Coloque uma sobre a outra e "
     "sirva. Fica ótimo com um cafezinho!"},

    {"Feira de Caruaru", "Caruaru (PE)", "Caruaru", CIDADE_CARUARU,
     {CAJU, MANGA}, {BANANA, MELANCIA, LARANJA, LIMAO, UVA},
     {238, 186, 36, 255}, {62, 148, 72, 255}, {246, 190, 120, 255}, {255, 236, 196, 255},
     "A Feira de Caruaru é Patrimônio Imaterial do Brasil desde 2006 e ficou famosa na música de Onildo "
     "Almeida gravada por Luiz Gonzaga.",
     "Suco de caju",
     "Bata no liquidificador 4 cajus sem a castanha, 1 litro de água gelada e açúcar a gosto. Coe, se "
     "preferir, e sirva com gelo."},

    {"Ver-o-Peso", "Belém (PA)", "Belém", CIDADE_BELEM,
     {ACAI, CUPUACU}, {BANANA, LARANJA, LIMAO, MANGA, MELANCIA},
     {36, 132, 92, 255}, {255, 244, 214, 255}, {132, 200, 206, 255}, {236, 240, 206, 255},
     "O nome Ver-o-Peso vem do antigo posto que conferia o peso das mercadorias, instalado em 1625. É "
     "considerado a maior feira livre da América Latina.",
     "Creme de cupuaçu",
     "Bata 300 g de polpa de cupuaçu, 1 lata de leite condensado e 1 caixinha de creme de leite. Leve à "
     "geladeira por 3 horas e sirva bem gelado."},

    {"Mercado Público de Porto Alegre", "Porto Alegre (RS)", "Porto Alegre", CIDADE_POA,
     {BERGAMOTA, PESSEGO}, {UVA, MACA, MORANGO, BANANA, MELANCIA},
     {44, 122, 64, 255}, {204, 44, 44, 255}, {160, 196, 230, 255}, {246, 234, 214, 255},
     "Inaugurado em 1869, o Mercado Público de Porto Alegre já enfrentou quatro incêndios e continua firme "
     "no centro histórico da cidade.",
     "Sagu de suco de uva",
     "Deixe 1 xícara de sagu de molho por 1 hora. Cozinhe em 1 litro de suco de uva integral com 1 pau de "
     "canela e açúcar a gosto, mexendo, até as bolinhas ficarem transparentes. Sirva frio."},
};

const char* NOMES_FRUTAS[TOTAL_FRUTAS] = {"maçã",    "banana",     "uva",  "limão", "laranja", "morango",
                                          "melancia", "goiaba",    "jabuticaba", "caju", "manga", "açaí",
                                          "cupuaçu", "bergamota", "pêssego"};

// Em qual capitulo fica cada fase (depois da ultima feira, a viagem recomeca)
int capituloDaFase(int fase) { return ((fase - 1) / FASES_POR_CAPITULO) % TOTAL_CAPITULOS; }
int posicaoNoCapitulo(int fase) { return (fase - 1) % FASES_POR_CAPITULO; }  // 0 a 9
int voltaDaFase(int fase) { return (fase - 1) / (FASES_POR_CAPITULO * TOTAL_CAPITULOS); }
int primeiraFaseDoCapitulo(int capitulo, int volta) {
    return volta * FASES_POR_CAPITULO * TOTAL_CAPITULOS + capitulo * FASES_POR_CAPITULO + 1;
}
bool ultimaFaseDoCapitulo(int fase) { return posicaoNoCapitulo(fase) == FASES_POR_CAPITULO - 1; }

// Dificuldade: sobe dentro de cada feira, e cada feira comeca um pouco mais dificil
int frutasDaFase(int fase) {
    int capitulo = min((fase - 1) / FASES_POR_CAPITULO, 2);
    return min(7, 3 + posicaoNoCapitulo(fase) * 4 / 9 + capitulo);
}

float chanceEscondida(int fase) {
    if (fase < PRIMEIRA_FASE_ESCONDIDA) return 0;
    return min(0.22f, 0.08f + 0.015f * (fase - PRIMEIRA_FASE_ESCONDIDA));
}

Nivel nivelDaFase(int fase) {
    const Capitulo& c = CAPITULOS[capituloDaFase(fase)];
    vector<int> regionais(c.regionais, c.regionais + 2), opcionais(c.opcionais, c.opcionais + 5);
    return gerarNivel(regionais, opcionais, frutasDaFase(fase), chanceEscondida(fase),
                      (unsigned)fase * 2654435761u + 12345u);
}

// Desafio do dia: a data (ex.: 20260925) vira a semente e escolhe a feira
Nivel nivelDoDesafio(int data) {
    const Capitulo& c = CAPITULOS[data % TOTAL_CAPITULOS];
    vector<int> regionais(c.regionais, c.regionais + 2), opcionais(c.opcionais, c.opcionais + 5);
    return gerarNivel(regionais, opcionais, 6, 0.3f, (unsigned)data * 7919u + 777u);
}
