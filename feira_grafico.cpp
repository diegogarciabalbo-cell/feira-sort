// ============================================================
//  FEIRA SORT - Versao 2 (grafica)
//  Organize as frutas da feira: cada caixote com um so tipo de fruta.
//  Feito em C++ com a biblioteca raylib.
//
//  Pensado para o publico 50+:
//   - sem cronometro e sem anuncios
//   - frutas grandes, com formatos diferentes (nao depende so da cor)
//   - desfazer ilimitado
//   - dica que sempre funciona (o jogo resolve a feira sozinho)
//   - aviso quando a feira trava, coisa que quase nenhum jogo do genero faz
// ============================================================

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

// ---------------- Configuracoes da tela e do jogo ----------------
const int LARGURA_TELA = 1000;
const int ALTURA_TELA = 720;

const int CAPACIDADE = 4;       // frutas por caixote
const int CAIXOTES_VAZIOS = 2;  // caixotes vazios para manobrar

const float CAIXOTE_LARGURA = 104;
const float CAIXOTE_ESPACO = 18;
const float VAGA_ALTURA = 80;  // espaco de cada fruta dentro do caixote
const float CAIXOTE_ALTURA = CAPACIDADE * VAGA_ALTURA + 22;
const float CAIXOTE_TOPO = 200;
const float RAIO_FRUTA = 31;
const float ALTURA_LEVANTADA = 50;  // quanto a fruta sobe quando e escolhida
const float DURACAO_VOO = 0.35f;   // segundos da animacao da fruta voando
const int LIMITE_BUSCA = 200000;   // limite de tentativas do "resolvedor"

enum Fruta { MACA, BANANA, UVA, LIMAO, LARANJA, MORANGO, TOTAL_FRUTAS };

// Um caixote e uma pilha: o fim do vetor (back) e a fruta de cima.
using Caixote = vector<int>;
using Feira = vector<Caixote>;
using Jogada = pair<int, int>;  // (de, para)

// ---------------- Cores ----------------
const Color COR_FUNDO = {250, 240, 220, 255};
const Color COR_MADEIRA = {196, 138, 74, 255};
const Color COR_MADEIRA_ESCURA = {140, 92, 45, 255};
const Color COR_MADEIRA_FUNDO = {105, 68, 36, 255};
const Color COR_RIPA = {130, 86, 48, 255};
const Color COR_TEXTO = {80, 50, 25, 255};
const Color COR_VERDE = {55, 150, 75, 255};
const Color COR_LARANJA_AVISO = {215, 110, 20, 255};
const Color COR_DESTAQUE = {255, 195, 40, 255};
const Color COR_FOLHA = {70, 150, 50, 255};
const Color COR_CABO = {100, 60, 30, 255};

// ============================================================
//  REGRAS DO JOGO
// ============================================================

// Caixote com todas as frutas iguais (vazio tambem conta)
bool uniforme(const Caixote& c) {
    for (int fruta : c) {
        if (fruta != c[0]) return false;
    }
    return true;
}

// Venceu quando todo caixote esta vazio ou cheio de uma fruta so
bool venceu(const Feira& feira) {
    for (const Caixote& c : feira) {
        if (c.empty()) continue;
        if ((int)c.size() != CAPACIDADE || !uniforme(c)) return false;
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
//  RESOLVEDOR: busca em profundidade (DFS) com memoria.
//  Usado para: garantir que toda feira criada tem solucao,
//  dar dicas e avisar quando o jogador travou.
// ============================================================

// Transforma a feira num texto unico. Ordenamos os caixotes porque
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

    // Passo 0: jogadas sobre a mesma fruta (costumam ser as melhores)
    // Passo 1: jogadas para um caixote vazio
    for (int passo = 0; passo < 2; passo++) {
        for (int de = 0; de < total; de++) {
            for (int para = 0; para < total; para++) {
                if (!podeMover(feira, de, para)) continue;
                bool destinoVazio = feira[para].empty();
                if (passo == 0 && destinoVazio) continue;
                if (passo == 1 && !destinoVazio) continue;
                // Mudar um caixote inteiro de lugar nao ajuda em nada
                if (destinoVazio && uniforme(feira[de])) continue;
                // Todos os caixotes vazios sao iguais: basta testar um
                if (destinoVazio && para != primeiroVazio) continue;

                Feira antes = feira;
                mover(feira, de, para);
                caminho.push_back({de, para});
                if (buscar(feira, caminho, vistos, tentativas, estourouLimite)) return true;
                caminho.pop_back();
                feira = antes;
                if (estourouLimite) return false;
            }
        }
    }
    return false;
}

// Retorna true se achou solucao. "estourouLimite" indica que
// a busca desistiu antes de ter certeza.
bool resolver(Feira feira, vector<Jogada>& caminho, bool& estourouLimite) {
    unordered_set<string> vistos;
    int tentativas = 0;
    estourouLimite = false;
    caminho.clear();
    return buscar(feira, caminho, vistos, tentativas, estourouLimite);
}

// ============================================================
//  CRIACAO DAS FASES
// ============================================================

// Fase 1: 3 frutas, fase 2: 4 frutas... ate 6 frutas
int frutasDaFase(int fase) {
    return min(2 + fase, (int)TOTAL_FRUTAS);
}

Feira criarFeira(int fase, mt19937& gerador) {
    int qtdFrutas = frutasDaFase(fase);

    while (true) {
        // Sorteia quais frutas entram nesta fase
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
        bool temProntoNoInicio = false;
        for (int c = 0; c < qtdFrutas; c++) {
            if (uniforme(feira[c])) temProntoNoInicio = true;
        }
        if (temProntoNoInicio) continue;

        // So aceita a feira se ela tiver solucao
        vector<Jogada> caminho;
        bool estourou;
        if (resolver(feira, caminho, estourou)) return feira;
    }
}

// ============================================================
//  DESENHO DAS FRUTAS
//  Cada fruta tem um formato proprio, para quem tem dificuldade
//  de distinguir cores.
// ============================================================

Vector2 deslocar(Vector2 p, float dx, float dy) {
    return {p.x + dx, p.y + dy};
}

void desenharBrilho(Vector2 c, float r) {
    DrawCircleV(deslocar(c, -r * 0.35f, -r * 0.35f), r * 0.2f, Fade(WHITE, 0.45f));
}

void desenharFruta(int fruta, Vector2 c) {
    float r = RAIO_FRUTA;

    // Sombra no chao do caixote
    DrawEllipse((int)c.x, (int)(c.y + r * 0.9f), r * 0.8f, r * 0.15f, Fade(BLACK, 0.2f));

    switch (fruta) {
        case MACA: {
            DrawLineEx(deslocar(c, 0, -r * 0.7f), deslocar(c, r * 0.12f, -r * 1.2f), 4, COR_CABO);
            DrawEllipse((int)(c.x + r * 0.38f), (int)(c.y - r * 1.02f), r * 0.32f, r * 0.14f, COR_FOLHA);
            DrawCircleV(c, r, Color{200, 30, 40, 255});
            DrawCircleV(deslocar(c, r * 0.08f, r * 0.05f), r * 0.85f, Color{222, 50, 55, 255});
            desenharBrilho(c, r);
            break;
        }
        case BANANA: {
            // A banana e desenhada como varios circulos ao longo de um arco
            Vector2 centro = deslocar(c, 0, -r * 0.75f);
            float raioArco = r * 1.05f;
            for (int camada = 0; camada < 2; camada++) {
                for (int a = 20; a <= 160; a += 4) {
                    float ang = a * DEG2RAD;
                    float meio = sinf(PI * (a - 20) / 140.0f);  // mais grossa no meio
                    Vector2 p = {centro.x + cosf(ang) * raioArco, centro.y + sinf(ang) * raioArco};
                    float grossura = r * (0.1f + 0.24f * meio);
                    if (camada == 0) DrawCircleV(p, grossura + 2, Color{190, 150, 20, 255});
                    else DrawCircleV(p, grossura, Color{250, 212, 55, 255});
                }
            }
            for (int a : {20, 160}) {
                float ang = a * DEG2RAD;
                Vector2 ponta = {centro.x + cosf(ang) * raioArco, centro.y + sinf(ang) * raioArco};
                DrawCircleV(ponta, r * 0.12f, COR_CABO);
            }
            break;
        }
        case UVA: {
            DrawLineEx(deslocar(c, 0, -r * 0.6f), deslocar(c, r * 0.05f, -r * 1.1f), 4, COR_CABO);
            DrawEllipse((int)(c.x + r * 0.35f), (int)(c.y - r * 0.95f), r * 0.3f, r * 0.13f, COR_FOLHA);
            Vector2 bagos[] = {{-0.5f, -0.35f}, {0, -0.42f}, {0.5f, -0.35f},
                               {-0.27f, 0.12f}, {0.27f, 0.12f}, {0, 0.58f}};
            for (Vector2 b : bagos) {
                Vector2 p = deslocar(c, b.x * r, b.y * r);
                DrawCircleV(p, r * 0.34f, Color{95, 35, 130, 255});
                DrawCircleV(p, r * 0.3f, Color{135, 60, 175, 255});
                DrawCircleV(deslocar(p, -r * 0.1f, -r * 0.1f), r * 0.09f, Fade(WHITE, 0.45f));
            }
            break;
        }
        case LIMAO: {
            Color verde = {100, 185, 60, 255};
            DrawEllipse((int)c.x, (int)c.y, r * 1.05f + 2, r * 0.78f + 2, Color{60, 130, 35, 255});
            DrawEllipse((int)c.x, (int)c.y, r * 1.05f, r * 0.78f, verde);
            DrawCircleV(deslocar(c, -r * 1.02f, 0), r * 0.15f, verde);
            DrawCircleV(deslocar(c, r * 1.02f, 0), r * 0.15f, verde);
            DrawEllipse((int)(c.x - r * 0.35f), (int)(c.y - r * 0.32f), r * 0.32f, r * 0.12f, Fade(WHITE, 0.45f));
            break;
        }
        case LARANJA: {
            DrawCircleV(c, r, Color{225, 115, 15, 255});
            DrawCircleV(c, r * 0.93f, Color{248, 150, 30, 255});
            // Textura da casca
            Vector2 pontos[] = {{0.3f, 0.2f}, {-0.2f, 0.4f}, {0.5f, -0.2f}, {-0.5f, 0.1f}, {0.1f, 0.6f}};
            for (Vector2 p : pontos) DrawCircleV(deslocar(c, p.x * r, p.y * r), 2, Color{220, 115, 15, 255});
            DrawCircleV(deslocar(c, 0, -r * 0.85f), r * 0.14f, COR_FOLHA);
            DrawEllipse((int)(c.x + r * 0.35f), (int)(c.y - r * 0.95f), r * 0.3f, r * 0.13f, COR_FOLHA);
            desenharBrilho(c, r);
            break;
        }
        case MORANGO: {
            Color vermelho = {225, 45, 80, 255};
            DrawCircleV(deslocar(c, -r * 0.42f, -r * 0.2f), r * 0.52f, vermelho);
            DrawCircleV(deslocar(c, r * 0.42f, -r * 0.2f), r * 0.52f, vermelho);
            DrawTriangle(deslocar(c, -r * 0.92f, -r * 0.1f), deslocar(c, 0, r * 0.95f),
                         deslocar(c, r * 0.92f, -r * 0.1f), vermelho);
            // Sementinhas
            Vector2 sementes[] = {{-0.45f, -0.2f}, {0, -0.25f}, {0.45f, -0.2f}, {-0.25f, 0.2f},
                                  {0.25f, 0.2f},   {0, 0.55f}};
            for (Vector2 s : sementes) DrawCircleV(deslocar(c, s.x * r, s.y * r), 2.2f, Color{255, 225, 120, 255});
            // Folhinhas verdes em cima
            DrawEllipse((int)(c.x - r * 0.3f), (int)(c.y - r * 0.68f), r * 0.34f, r * 0.13f, COR_FOLHA);
            DrawEllipse((int)(c.x + r * 0.3f), (int)(c.y - r * 0.68f), r * 0.34f, r * 0.13f, COR_FOLHA);
            DrawCircleV(deslocar(c, 0, -r * 0.72f), r * 0.16f, COR_FOLHA);
            break;
        }
    }
}

// ============================================================
//  POSICOES NA TELA
// ============================================================

Rectangle retanguloCaixote(int i, int total) {
    float larguraTotal = total * CAIXOTE_LARGURA + (total - 1) * CAIXOTE_ESPACO;
    float inicioX = (LARGURA_TELA - larguraTotal) / 2;
    return {inicioX + i * (CAIXOTE_LARGURA + CAIXOTE_ESPACO), CAIXOTE_TOPO, CAIXOTE_LARGURA,
            CAIXOTE_ALTURA};
}

// Area clicavel: o caixote e um pouco acima dele (onde a fruta levanta)
Rectangle areaDeClique(int i, int total) {
    Rectangle r = retanguloCaixote(i, total);
    return {r.x - 8, r.y - 60, r.width + 16, r.height + 68};
}

// Centro da vaga "vaga" (0 = fundo do caixote)
Vector2 posicaoVaga(int i, int vaga, int total) {
    Rectangle r = retanguloCaixote(i, total);
    return {r.x + r.width / 2, r.y + r.height - 12 - VAGA_ALTURA * (vaga + 0.5f)};
}

// ============================================================
//  DESENHO DO CENARIO
// ============================================================

// Fonte do jogo (carregada no main). Aceita acentos do portugues.
Font fonte;

// Escreve um texto centralizado em centroX, com o topo em y
void desenharTexto(const char* texto, float centroX, float y, int tamanho, Color cor) {
    Vector2 medida = MeasureTextEx(fonte, texto, (float)tamanho, 1);
    DrawTextEx(fonte, texto, {centroX - medida.x / 2, y}, (float)tamanho, 1, cor);
}

// Carrega a fonte com as letras acentuadas do portugues
Font carregarFonte() {
    string caminho = string(GetApplicationDirectory()) + "Poppins-Bold.ttf";
    if (!FileExists(caminho.c_str())) return GetFontDefault();

    vector<int> letras;
    for (int c = 32; c < 127; c++) letras.push_back(c);
    const char* acentuadas = "áàâãéêíóôõúüçÁÀÂÃÉÊÍÓÔÕÚÜÇ";
    int posicao = 0;
    while (acentuadas[posicao] != '\0') {
        int tamanhoLetra = 0;
        letras.push_back(GetCodepoint(acentuadas + posicao, &tamanhoLetra));
        posicao += tamanhoLetra;
    }

    Font f = LoadFontEx(caminho.c_str(), 64, letras.data(), (int)letras.size());
    SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);  // deixa a letra suave
    return f;
}

// ============================================================
//  SONS: gerados pelo proprio codigo, sem arquivos de audio
// ============================================================

const int TAXA_AMOSTRAGEM = 44100;

// Cria um som a partir de uma lista de notas (frequencia em Hz, duracao em segundos)
Sound criarSom(const vector<pair<float, float>>& notas, float volume) {
    vector<short> amostras;
    for (auto [frequencia, duracao] : notas) {
        int total = (int)(duracao * TAXA_AMOSTRAGEM);
        for (int i = 0; i < total; i++) {
            float t = (float)i / TAXA_AMOSTRAGEM;
            float ataque = min(1.0f, i / 200.0f);        // evita estalo no inicio
            float queda = expf(-5.0f * t / duracao);      // som vai sumindo
            float onda = sinf(2 * PI * frequencia * t) + 0.3f * sinf(4 * PI * frequencia * t);
            amostras.push_back((short)(onda * ataque * queda * volume * 20000));
        }
    }
    Wave onda = {(unsigned int)amostras.size(), TAXA_AMOSTRAGEM, 16, 1, amostras.data()};
    return LoadSoundFromWave(onda);  // a raylib copia os dados
}

// ============================================================
//  PROGRESSO: guarda a fase alcancada num arquivo de texto
// ============================================================

string arquivoProgresso() {
    return string(GetApplicationDirectory()) + "progresso.txt";
}

int carregarFase() {
    ifstream arquivo(arquivoProgresso());
    int fase = 1;
    if (arquivo >> fase && fase >= 1) return fase;
    return 1;
}

void salvarFase(int fase) {
    ofstream arquivo(arquivoProgresso());
    arquivo << fase;
}

// Toldo listrado de barraca de feira
void desenharToldo() {
    Color vermelho = {200, 55, 50, 255};
    Color creme = {255, 248, 235, 255};
    float listra = 50;
    for (int i = 0; i * listra < LARGURA_TELA; i++) {
        Color cor = (i % 2 == 0) ? vermelho : creme;
        DrawRectangle((int)(i * listra), 0, (int)listra, 42, cor);
        DrawCircle((int)(i * listra + listra / 2), 42, listra / 2, cor);
    }
    DrawRectangle(0, 0, LARGURA_TELA, 6, Color{150, 40, 35, 255});
}

void desenharCaixote(Rectangle r, bool mouseEmCima, bool selecionado) {
    // Sombra
    DrawRectangleRounded({r.x + 6, r.y + 10, r.width, r.height}, 0.08f, 6, Fade(BLACK, 0.15f));
    // Contorno de destaque
    if (selecionado) {
        DrawRectangleRounded({r.x - 6, r.y - 6, r.width + 12, r.height + 12}, 0.1f, 6, COR_DESTAQUE);
    } else if (mouseEmCima) {
        DrawRectangleRounded({r.x - 5, r.y - 5, r.width + 10, r.height + 10}, 0.1f, 6,
                             Fade(COR_DESTAQUE, 0.45f));
    }
    // Fundo e ripas de tras
    DrawRectangleRec(r, COR_MADEIRA_FUNDO);
    for (int k = 0; k < CAPACIDADE; k++) {
        float y = r.y + 10 + k * VAGA_ALTURA + VAGA_ALTURA * 0.3f;
        DrawRectangle((int)r.x, (int)y, (int)r.width, (int)(VAGA_ALTURA * 0.4f), COR_RIPA);
    }
    // Laterais e fundo do caixote
    DrawRectangle((int)r.x, (int)r.y, 10, (int)r.height, COR_MADEIRA);
    DrawRectangle((int)(r.x + r.width - 10), (int)r.y, 10, (int)r.height, COR_MADEIRA);
    DrawRectangle((int)r.x, (int)(r.y + r.height - 14), (int)r.width, 14, COR_MADEIRA);
    DrawRectangleLinesEx(r, 2, COR_MADEIRA_ESCURA);
    // Preguinhos
    DrawCircle((int)(r.x + 5), (int)(r.y + r.height - 7), 2, COR_MADEIRA_ESCURA);
    DrawCircle((int)(r.x + r.width - 5), (int)(r.y + r.height - 7), 2, COR_MADEIRA_ESCURA);
}

// Selo verde de "caixote pronto"
void desenharSeloPronto(Rectangle r) {
    Vector2 c = {r.x + r.width / 2, r.y - 24};
    DrawCircleV(c, 18, COR_VERDE);
    DrawLineEx(deslocar(c, -8, 0), deslocar(c, -2, 7), 4, WHITE);
    DrawLineEx(deslocar(c, -2, 7), deslocar(c, 9, -7), 4, WHITE);
}

struct Botao {
    Rectangle r;
    const char* texto;
    Color cor;
};

void desenharBotao(const Botao& b, bool mouseEmCima) {
    Color cor = b.cor;
    if (mouseEmCima) {  // clareia um pouco o botao
        cor.r = (unsigned char)min(255, cor.r + 30);
        cor.g = (unsigned char)min(255, cor.g + 30);
        cor.b = (unsigned char)min(255, cor.b + 30);
    }
    DrawRectangleRounded({b.r.x + 3, b.r.y + 5, b.r.width, b.r.height}, 0.35f, 8, Fade(BLACK, 0.2f));
    DrawRectangleRounded(b.r, 0.35f, 8, cor);
    desenharTexto(b.texto, b.r.x + b.r.width / 2, b.r.y + b.r.height / 2 - 15, 26, WHITE);
}

// ============================================================
//  PROGRAMA PRINCIPAL
// ============================================================

// Fruta voando de um caixote para outro
struct Voo {
    bool ativo = false;
    int de = 0, para = 0, quantidade = 0, fruta = 0;
    int tamanhoOrigem = 0, tamanhoDestino = 0;
    float progresso = 0;
};

int main() {
    SetTraceLogLevel(LOG_WARNING);  // so mostra avisos e erros no terminal
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(LARGURA_TELA, ALTURA_TELA, "Feira Sort");
    SetTargetFPS(60);
    fonte = carregarFonte();

    // ---------- Sons ----------
    InitAudioDevice();
    bool temAudio = IsAudioDeviceReady();
    bool somLigado = true;
    Sound somErro = {}, somPronto = {}, somVitoria = {};
    vector<Sound> somFruta;  // um "plim" diferente para cada altura do caixote
    if (temAudio) {
        float notasPlim[] = {523, 587, 659, 784};  // do, re, mi, sol
        for (float nota : notasPlim) somFruta.push_back(criarSom({{nota, 0.18f}}, 0.5f));
        somErro = criarSom({{196, 0.15f}, {165, 0.2f}}, 0.35f);
        somPronto = criarSom({{784, 0.1f}, {1047, 0.25f}}, 0.4f);
        somVitoria = criarSom({{523, 0.14f}, {659, 0.14f}, {784, 0.14f}, {1047, 0.5f}}, 0.5f);
    }
    auto tocar = [&](Sound& som) {
        if (temAudio && somLigado) PlaySound(som);
    };

    random_device semente;
    mt19937 gerador(semente());

    // ---------- Estado do jogo ----------
    int fase = carregarFase();  // continua de onde parou
    Feira inicio = criarFeira(fase, gerador);
    Feira feira = inicio;
    vector<Feira> historico;  // para o "desfazer"
    int jogadas = 0;
    int selecionado = -1;
    bool ganhou = false;
    bool travou = false;

    string mensagem = "Boas-vindas! Sem pressa, pense com calma.";
    Color corMensagem = COR_TEXTO;
    float tempoMensagem = 5;

    int dicaDe = -1, dicaPara = -1;
    float tempoDica = 0;

    int caixoteTremendo = -1;
    float tempoTremor = 0;

    Voo voo;

    // ---------- Botoes ----------
    float larguraBotao = 190, alturaBotao = 60, espacoBotao = 22;
    float inicioBotoes = (LARGURA_TELA - (4 * larguraBotao + 3 * espacoBotao)) / 2;
    float yBotoes = 628;
    Botao botaoDesfazer = {{inicioBotoes, yBotoes, larguraBotao, alturaBotao}, "DESFAZER", {70, 110, 170, 255}};
    Botao botaoDica = {{inicioBotoes + (larguraBotao + espacoBotao), yBotoes, larguraBotao, alturaBotao}, "DICA", COR_VERDE};
    Botao botaoReiniciar = {{inicioBotoes + 2 * (larguraBotao + espacoBotao), yBotoes, larguraBotao, alturaBotao}, "REINICIAR", {150, 100, 60, 255}};
    Botao botaoNova = {{inicioBotoes + 3 * (larguraBotao + espacoBotao), yBotoes, larguraBotao, alturaBotao}, "NOVA FEIRA", {150, 100, 60, 255}};
    Botao botaoProxima = {{LARGURA_TELA / 2.0f - 140, 425, 280, 70}, "PRÓXIMA FASE", COR_VERDE};
    Botao botaoSom = {{LARGURA_TELA - 150.0f, 78, 130, 42}, "SOM: SIM", {120, 95, 70, 255}};

    // ---------- Funcoes auxiliares (lambdas) ----------
    auto avisar = [&](const string& texto, Color cor) {
        mensagem = texto;
        corMensagem = cor;
        tempoMensagem = 3.5f;
    };

    // Verifica se ainda existe saida a partir da feira atual
    auto verificarTravamento = [&]() {
        vector<Jogada> caminho;
        bool estourou;
        bool temSolucao = resolver(feira, caminho, estourou);
        travou = !temSolucao && !estourou;
    };

    auto comecarFeira = [&](bool novaMontagem) {
        if (novaMontagem) inicio = criarFeira(fase, gerador);
        feira = inicio;
        historico.clear();
        jogadas = 0;
        selecionado = -1;
        ganhou = false;
        travou = false;
        tempoDica = 0;
        voo.ativo = false;
    };

    auto cliqueNoCaixote = [&](int c) {
        if (selecionado == -1) {
            if (feira[c].empty()) {
                avisar("Esse caixote está vazio. Escolha um com frutas.", COR_TEXTO);
            } else {
                selecionado = c;
            }
        } else if (selecionado == c) {
            selecionado = -1;  // clicou de novo: solta a fruta
        } else if (podeMover(feira, selecionado, c)) {
            voo.ativo = true;
            voo.de = selecionado;
            voo.para = c;
            voo.quantidade = quantasMovem(feira, selecionado, c);
            voo.fruta = feira[selecionado].back();
            voo.tamanhoOrigem = feira[selecionado].size();
            voo.tamanhoDestino = feira[c].size();
            voo.progresso = 0;
            selecionado = -1;
            tempoDica = 0;
        } else {
            if ((int)feira[c].size() >= CAPACIDADE) {
                avisar("Caixote cheio! Escolha outro.", COR_LARANJA_AVISO);
            } else {
                avisar("Coloque sobre a mesma fruta ou num caixote vazio.", COR_LARANJA_AVISO);
            }
            caixoteTremendo = c;
            tempoTremor = 0.4f;
            tocar(somErro);
        }
    };

    // ---------- Laco principal ----------
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mouse = GetMousePosition();
        bool clicou = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        int total = feira.size();

        if (tempoMensagem > 0) tempoMensagem -= dt;
        if (tempoDica > 0) tempoDica -= dt;
        if (tempoTremor > 0) tempoTremor -= dt;

        // ----- Atualizacao -----
        if (voo.ativo) {
            voo.progresso += dt / DURACAO_VOO;
            if (voo.progresso >= 1) {
                // A fruta chegou: agora a jogada vale de verdade
                historico.push_back(feira);
                mover(feira, voo.de, voo.para);
                jogadas++;
                voo.ativo = false;

                const Caixote& destino = feira[voo.para];
                if (venceu(feira)) {
                    ganhou = true;
                    salvarFase(fase + 1);  // a proxima fase ja fica guardada
                    tocar(somVitoria);
                } else {
                    if ((int)destino.size() == CAPACIDADE && uniforme(destino)) tocar(somPronto);
                    else if (temAudio) tocar(somFruta[destino.size() - 1]);  // mais alto o caixote, mais aguda a nota
                    verificarTravamento();
                }
            }
        } else if (ganhou) {
            if (clicou && CheckCollisionPointRec(mouse, botaoProxima.r)) {
                fase++;
                comecarFeira(true);
                avisar("Fase " + to_string(fase) + ": agora com " + to_string(frutasDaFase(fase)) + " frutas!",
                       COR_TEXTO);
            }
        } else if (clicou) {
            if (CheckCollisionPointRec(mouse, botaoSom.r)) {
                somLigado = !somLigado;
                botaoSom.texto = somLigado ? "SOM: SIM" : "SOM: NÃO";
            } else if (CheckCollisionPointRec(mouse, botaoDesfazer.r)) {
                if (historico.empty()) {
                    avisar("Nada para desfazer.", COR_TEXTO);
                } else {
                    feira = historico.back();
                    historico.pop_back();
                    jogadas--;
                    selecionado = -1;
                    verificarTravamento();
                    if (!travou) avisar("Jogada desfeita.", COR_TEXTO);
                }
            } else if (CheckCollisionPointRec(mouse, botaoDica.r)) {
                selecionado = -1;
                if (travou) {
                    avisar("Primeiro toque em DESFAZER.", COR_LARANJA_AVISO);
                } else {
                    vector<Jogada> caminho;
                    bool estourou;
                    if (resolver(feira, caminho, estourou) && !caminho.empty()) {
                        dicaDe = caminho[0].first;
                        dicaPara = caminho[0].second;
                        tempoDica = 4;
                        avisar("Dica: pegue as frutas de DAQUI e coloque AQUI.", COR_VERDE);
                    } else {
                        avisar("Tente juntar frutas iguais.", COR_TEXTO);
                    }
                }
            } else if (CheckCollisionPointRec(mouse, botaoReiniciar.r)) {
                comecarFeira(false);
                avisar("Feira reiniciada.", COR_TEXTO);
            } else if (CheckCollisionPointRec(mouse, botaoNova.r)) {
                comecarFeira(true);
                avisar("Feira nova montada!", COR_TEXTO);
            } else {
                int clicado = -1;
                for (int i = 0; i < total; i++) {
                    if (CheckCollisionPointRec(mouse, areaDeClique(i, total))) clicado = i;
                }
                if (clicado >= 0) cliqueNoCaixote(clicado);
                else selecionado = -1;
            }
        }

        // Cursor de "maozinha" sobre o que da para clicar
        bool sobreClicavel = false;
        if (!voo.ativo && !ganhou) {
            for (int i = 0; i < total; i++) {
                if (CheckCollisionPointRec(mouse, areaDeClique(i, total))) sobreClicavel = true;
            }
            for (const Botao* b : {&botaoDesfazer, &botaoDica, &botaoReiniciar, &botaoNova, &botaoSom}) {
                if (CheckCollisionPointRec(mouse, b->r)) sobreClicavel = true;
            }
        } else if (ganhou && CheckCollisionPointRec(mouse, botaoProxima.r)) {
            sobreClicavel = true;
        }
        SetMouseCursor(sobreClicavel ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

        // ----- Desenho -----
        BeginDrawing();
        ClearBackground(COR_FUNDO);

        desenharToldo();
        desenharTexto("FEIRA SORT", LARGURA_TELA / 2.0f, 76, 44, COR_TEXTO);
        string info = "Fase " + to_string(fase) + "      Jogadas: " + to_string(jogadas);
        desenharTexto(info.c_str(), LARGURA_TELA / 2.0f, 126, 26, Fade(COR_TEXTO, 0.8f));

        float tempo = GetTime();
        for (int i = 0; i < total; i++) {
            Rectangle r = retanguloCaixote(i, total);
            float tremor = (i == caixoteTremendo && tempoTremor > 0) ? sinf(tempo * 60) * 6 : 0;
            r.x += tremor;

            bool mouseEmCima = !voo.ativo && !ganhou && CheckCollisionPointRec(mouse, areaDeClique(i, total));
            desenharCaixote(r, mouseEmCima, i == selecionado);

            // Contorno piscando da dica
            if (tempoDica > 0 && (i == dicaDe || i == dicaPara)) {
                float pulso = 0.5f + 0.5f * sinf(tempo * 7);
                DrawRectangleLinesEx({r.x - 8, r.y - 8, r.width + 16, r.height + 16}, 6, Fade(COR_VERDE, pulso));
                desenharTexto(i == dicaDe ? "DAQUI" : "AQUI", r.x + r.width / 2, r.y - 40, 24, COR_VERDE);
            }

            // Frutas do caixote
            int visiveis = feira[i].size();
            if (voo.ativo && i == voo.de) visiveis -= voo.quantidade;
            int levantadas = (i == selecionado) ? frutasIguaisNoTopo(feira[i]) : 0;
            for (int k = 0; k < visiveis; k++) {
                Vector2 p = posicaoVaga(i, k, total);
                p.x += tremor;
                if (k >= visiveis - levantadas) p.y -= ALTURA_LEVANTADA;
                desenharFruta(feira[i][k], p);
            }

            if ((int)feira[i].size() == CAPACIDADE && uniforme(feira[i]) && !(voo.ativo && i == voo.de)) {
                desenharSeloPronto(r);
            }
        }

        // Frutas voando (desenhadas por cima de tudo)
        if (voo.ativo) {
            float t = min(voo.progresso, 1.0f);
            float suave = t * t * (3 - 2 * t);
            for (int q = 0; q < voo.quantidade; q++) {
                Vector2 origem = posicaoVaga(voo.de, voo.tamanhoOrigem - voo.quantidade + q, total);
                origem.y -= ALTURA_LEVANTADA;
                Vector2 destino = posicaoVaga(voo.para, voo.tamanhoDestino + q, total);
                Vector2 p = {origem.x + (destino.x - origem.x) * suave,
                             origem.y + (destino.y - origem.y) * suave - sinf(PI * suave) * 80};
                desenharFruta(voo.fruta, p);
            }
        }

        // Mensagem (o aviso de travamento fica ate o jogador desfazer)
        if (ganhou) {
            // na tela de vitoria nao mostramos mensagem
        } else if (travou) {
            desenharTexto("Assim a feira travou. Toque em DESFAZER.", LARGURA_TELA / 2.0f, 566, 28, COR_LARANJA_AVISO);
        } else if (tempoMensagem > 0) {
            desenharTexto(mensagem.c_str(), LARGURA_TELA / 2.0f, 566, 28, corMensagem);
        }

        for (const Botao* b : {&botaoDesfazer, &botaoDica, &botaoReiniciar, &botaoNova, &botaoSom}) {
            desenharBotao(*b, !voo.ativo && !ganhou && CheckCollisionPointRec(mouse, b->r));
        }

        // Tela de vitoria
        if (ganhou) {
            DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, Fade(BLACK, 0.45f));
            Rectangle painel = {LARGURA_TELA / 2.0f - 290, 190, 580, 330};
            DrawRectangleRounded(painel, 0.12f, 8, COR_FUNDO);
            desenharTexto("MUITO BEM!", LARGURA_TELA / 2.0f, 220, 56, COR_VERDE);
            string texto = "Feira organizada em " + to_string(jogadas) + " jogadas";
            desenharTexto(texto.c_str(), LARGURA_TELA / 2.0f, 290, 30, COR_TEXTO);
            for (int f = 0; f < 3; f++) desenharFruta(f * 2, {LARGURA_TELA / 2.0f - 90 + f * 90.0f, 365});
            desenharBotao(botaoProxima, CheckCollisionPointRec(mouse, botaoProxima.r));
        }

        EndDrawing();
    }

    // Libera a memoria dos sons e da fonte
    if (temAudio) {
        for (Sound& s : somFruta) UnloadSound(s);
        UnloadSound(somErro);
        UnloadSound(somPronto);
        UnloadSound(somVitoria);
        CloseAudioDevice();
    }
    if (fonte.texture.id != GetFontDefault().texture.id) UnloadFont(fonte);
    CloseWindow();
    return 0;
}
