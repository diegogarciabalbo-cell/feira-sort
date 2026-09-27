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
#include "mundo.h"

const int FASES_POR_CAPITULO = 10;
const int TOTAL_CAPITULOS = 10;      // 5 feiras no Brasil + 5 no mundo
const int CAPITULOS_POR_REGIAO = 5;  // regiao 0 = Brasil, 1 = mundo
const int PREMIO_CAPITULO = 50;  // moedas extras ao completar uma feira

struct Capitulo {
    const char* feira;       // nome da feira
    const char* cidade;      // cidade e estado
    const char* curto;       // nome curto da cidade
    const char* rotulo;      // nome escrito no mapa
    const char* lembranca;   // titulo do postal
    const char* chegada;     // o que o Seu Ze fala ao chegar
    const float* posicao;    // posicao no mapa (0 a 1)
    int regionais[2];        // frutas da regiao (sempre aparecem)
    int opcionais[5];        // outras frutas que podem aparecer
    Color toldoA, toldoB;    // listras do toldo
    Color ceuTopo, ceuBase;  // cores do ceu
    const char* curiosidade;
    const char* receitaTitulo;
    const char* receita;
};

const Capitulo CAPITULOS[TOTAL_CAPITULOS] = {
    {"Mercadão de São Paulo", "São Paulo (SP)", "São Paulo", "São Paulo", "Lembrança de São Paulo", "Chegamos em São Paulo!", CIDADE_SP,
     {MORANGO, UVA}, {MACA, BANANA, LIMAO, LARANJA, MELANCIA},
     {210, 52, 48, 255}, {255, 247, 232, 255}, {138, 204, 242, 255}, {255, 238, 206, 255},
     "Inaugurado em 1933, o Mercadão tem vitrais do artista Conrado Sorgenicht Filho que mostram a produção de "
     "alimentos. O famoso sanduíche de mortadela surgiu lá nos anos 60.",
     "Salada de frutas",
     "Pique 1 maçã, 2 bananas, 1 xícara de uvas, 1 xícara de morangos e 2 fatias de melancia. Regue com o suco "
     "de 1 laranja e sirva gelada."},

    {"Mercado Central de Belo Horizonte", "Belo Horizonte (MG)", "Belo Horizonte", "Belo Horizonte", "Lembrança de Belo Horizonte", "Chegamos em Belo Horizonte!", CIDADE_BH,
     {GOIABA, JABUTICABA}, {BANANA, LARANJA, MACA, LIMAO, MORANGO},
     {46, 96, 176, 255}, {255, 247, 232, 255}, {150, 200, 236, 255}, {250, 232, 206, 255},
     "Criado em 1929, o Mercado Central quase acabou em 1964, quando o terreno foi vendido. Os próprios "
     "comerciantes se juntaram e compraram o mercado para salvá-lo.",
     "Romeu e Julieta",
     "Corte uma fatia de queijo minas e uma fatia de goiabada do mesmo tamanho. Coloque uma sobre a outra e "
     "sirva. Fica ótimo com um cafezinho!"},

    {"Feira de Caruaru", "Caruaru (PE)", "Caruaru", "Caruaru", "Lembrança de Caruaru", "Chegamos em Caruaru!", CIDADE_CARUARU,
     {CAJU, MANGA}, {BANANA, MELANCIA, LARANJA, LIMAO, UVA},
     {238, 186, 36, 255}, {62, 148, 72, 255}, {246, 190, 120, 255}, {255, 236, 196, 255},
     "A Feira de Caruaru é Patrimônio Imaterial do Brasil desde 2006 e ficou famosa na música de Onildo "
     "Almeida gravada por Luiz Gonzaga.",
     "Suco de caju",
     "Bata no liquidificador 4 cajus sem a castanha, 1 litro de água gelada e açúcar a gosto. Coe, se "
     "preferir, e sirva com gelo."},

    {"Ver-o-Peso", "Belém (PA)", "Belém", "Belém", "Lembrança de Belém", "Chegamos em Belém!", CIDADE_BELEM,
     {ACAI, CUPUACU}, {BANANA, LARANJA, LIMAO, MANGA, MELANCIA},
     {36, 132, 92, 255}, {255, 244, 214, 255}, {132, 200, 206, 255}, {236, 240, 206, 255},
     "O nome Ver-o-Peso vem do antigo posto que conferia o peso das mercadorias, instalado em 1625. É "
     "considerado a maior feira livre da América Latina.",
     "Creme de cupuaçu",
     "Bata 300 g de polpa de cupuaçu, 1 lata de leite condensado e 1 caixinha de creme de leite. Leve à "
     "geladeira por 3 horas e sirva bem gelado."},

    {"Mercado Público de Porto Alegre", "Porto Alegre (RS)", "Porto Alegre", "Porto Alegre", "Lembrança de Porto Alegre", "Chegamos em Porto Alegre!", CIDADE_POA,
     {BERGAMOTA, PESSEGO}, {UVA, MACA, MORANGO, BANANA, MELANCIA},
     {44, 122, 64, 255}, {204, 44, 44, 255}, {160, 196, 230, 255}, {246, 234, 214, 255},
     "Inaugurado em 1869, o Mercado Público de Porto Alegre já enfrentou quatro incêndios e continua firme "
     "no centro histórico da cidade.",
     "Sagu de suco de uva",
     "Deixe 1 xícara de sagu de molho por 1 hora. Cozinhe em 1 litro de suco de uva integral com 1 pau de "
     "canela e açúcar a gosto, mexendo, até as bolinhas ficarem transparentes. Sirva frio."},

    // ---------------- O MUNDO ----------------
    {"Mercado do Bolhão", "Porto (Portugal)", "Porto", "Portugal", "Lembrança do Porto", "Chegamos ao Porto, em Portugal!",
     MUNDO_PORTO, {PERA, CEREJA}, {UVA, LARANJA, MORANGO, BANANA, LIMAO},
     {0, 110, 64, 255}, {206, 28, 44, 255}, {150, 200, 236, 255}, {250, 236, 212, 255},
     "O prédio do Mercado do Bolhão é de 1914 e reabriu em 2022, depois de quatro anos de restauro. O nome vem "
     "de um riacho que formava uma bolha de água no terreno.",
     "Pera cozida com canela",
     "Descasque 4 peras e cozinhe por 20 minutos em água com açúcar, 1 pau de canela e 1 casca de limão. Sirva "
     "fria, regada com a calda."},

    {"La Boqueria", "Barcelona (Espanha)", "Barcelona", "Espanha", "Lembrança de Barcelona", "Chegamos a Barcelona, na Espanha!",
     MUNDO_BARCELONA, {ROMA, AZEITONA}, {LARANJA, UVA, MELANCIA, BANANA, PESSEGO},
     {198, 32, 44, 255}, {250, 200, 40, 255}, {140, 196, 238, 255}, {255, 232, 196, 255},
     "La Boqueria é citada pela primeira vez em 1217, como um mercado perto da porta da cidade. A cobertura de "
     "ferro, de 1914, está lá até hoje.",
     "Salada de laranja com azeitonas",
     "Descasque e fatie 3 laranjas. Junte azeitonas pretas, cebola roxa em rodelas finas, um fio de azeite e uma "
     "pitada de sal."},

    {"Mercato Centrale", "Florença (Itália)", "Florença", "Itália", "Lembrança de Florença", "Chegamos a Florença, na Itália!",
     MUNDO_FLORENCA, {FIGO, LIMAO_SICILIANO}, {UVA, LARANJA, MELANCIA, MORANGO, PESSEGO},
     {0, 140, 70, 255}, {250, 248, 240, 255}, {160, 200, 236, 255}, {248, 232, 200, 255},
     "O Mercato Centrale foi inaugurado em 1874, todo de ferro e vidro. O projeto é de Giuseppe Mengoni, o mesmo "
     "arquiteto da famosa Galeria Vittorio Emanuele II, em Milão.",
     "Figos assados com mel",
     "Corte 6 figos em cruz, regue com mel e asse por 15 minutos em forno médio. Sirva morno, com queijo branco "
     "ou sorvete."},

    {"Souk el Tayeb", "Beirute (Líbano)", "Beirute", "Líbano", "Lembrança de Beirute", "Chegamos a Beirute, no Líbano!",
     MUNDO_BEIRUTE, {TAMARA, DAMASCO}, {UVA, LIMAO, MELANCIA, BANANA, MORANGO},
     {214, 36, 44, 255}, {250, 248, 240, 255}, {140, 196, 232, 255}, {252, 238, 214, 255},
     "O Souk el Tayeb foi criado em 2004 por Kamal Mouzawak para valorizar os pequenos produtores. Hoje reúne "
     "agricultores e cozinheiros de todo o Líbano.",
     "Tâmaras recheadas",
     "Abra 12 tâmaras com uma faquinha e tire o caroço. Recheie cada uma com uma noz ou amêndoa e sirva com um "
     "cafezinho."},

    {"Mercado de Tsukiji", "Tóquio (Japão)", "Tóquio", "Japão", "Lembrança de Tóquio", "Chegamos a Tóquio, no Japão!",
     MUNDO_TOQUIO, {CAQUI, NASHI}, {MORANGO, MELANCIA, UVA, MACA, LIMAO},
     {196, 20, 52, 255}, {252, 248, 244, 255}, {236, 196, 214, 255}, {255, 242, 236, 255},
     "Aberto em 1935, Tsukiji foi o maior mercado de peixes do mundo. Em 2018 o atacado mudou para Toyosu, mas "
     "o mercado externo continua aberto e cheio de barraquinhas.",
     "Sanduíche de frutas (fruit sando)",
     "Passe chantili em 2 fatias de pão de forma sem casca. Coloque morangos e fatias de caqui no meio, feche, "
     "embrulhe em filme e gele por 1 hora antes de cortar ao meio."},
};

const char* NOMES_FRUTAS[TOTAL_FRUTAS] = {"maçã",    "banana",     "uva",  "limão", "laranja", "morango",
                                          "melancia", "goiaba",    "jabuticaba", "caju", "manga", "açaí",
                                          "cupuaçu", "bergamota", "pêssego", "pera",   "cereja",     "romã",
                                          "azeitona", "figo",      "limão-siciliano", "tâmara", "damasco", "caqui",
                                          "nashi"};

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

// Altura dos caixotes ao longo de uma feira: fases "rapidinhas" de 3 para descansar
// e a ultima fase, antes do postal, e a "banca grande" de 5.
const int RITMO_DE_CAPACIDADE[FASES_POR_CAPITULO] = {4, 4, 3, 4, 4, 4, 3, 4, 4, 5};

int capacidadeDaFase(int fase) { return RITMO_DE_CAPACIDADE[posicaoNoCapitulo(fase)]; }

Nivel nivelDaFase(int fase) {
    const Capitulo& c = CAPITULOS[capituloDaFase(fase)];
    vector<int> regionais(c.regionais, c.regionais + 2), opcionais(c.opcionais, c.opcionais + 5);
    int capacidade = capacidadeDaFase(fase);
    int frutas = frutasDaFase(fase);
    if (capacidade == 5) frutas = max(3, frutas - 1);  // caixote mais alto ja e mais dificil
    return gerarNivel(regionais, opcionais, frutas, chanceEscondida(fase), (unsigned)fase * 2654435761u + 12345u,
                      capacidade);
}

// Nome do arquivo da foto de cada feira: fotos/<nome>.png (e o credito em fotos/<nome>.txt)
const char* ARQUIVO_FOTO[TOTAL_CAPITULOS] = {"sao-paulo", "belo-horizonte", "caruaru", "belem", "porto-alegre",
                                              "porto", "barcelona", "florenca", "beirute", "toquio"};

// Desafio do dia: a data (ex.: 20260925) vira a semente e escolhe a feira
Nivel nivelDoDesafio(int data) {
    const Capitulo& c = CAPITULOS[data % CAPITULOS_POR_REGIAO];  // desafio sempre nas feiras do Brasil
    vector<int> regionais(c.regionais, c.regionais + 2), opcionais(c.opcionais, c.opcionais + 5);
    return gerarNivel(regionais, opcionais, 6, 0.3f, (unsigned)data * 7919u + 777u);
}
