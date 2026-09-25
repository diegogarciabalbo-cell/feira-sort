// ============================================================
//  FEIRA SORT - Versao 4
//  Jogo de puzzle para o publico 50+: organize as frutas da banca
//  do Seu Ze ate que cada caixote tenha um so tipo de fruta.
//
//  Novidades da versao 4 (inspiradas em Candy Crush e Magic Sort):
//   - mapa de fases com estrelas
//   - frutas escondidas no saquinho
//   - moedas, reforco "caixote extra" e desafio do dia
//
//  Arquivos:
//   regras.h - logica do jogo, niveis e resolvedor (DFS com backtracking)
//   arte.h   - todo o desenho (cenario, frutas, feirante, botoes)
//   som.h    - efeitos e musica gerados por codigo
//   main.cpp - telas, layout, progresso e controle do jogo (este arquivo)
// ============================================================

#include "raylib.h"
#include "regras.h"
#include "arte.h"
#include "som.h"
#include "capitulos.h"

#include <ctime>
#include <fstream>
#include <map>

// ============================================================
//  FUNCOES DO NAVEGADOR (so na versao web)
// ============================================================

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>

EM_JS(int, larguraNavegador, (), { return window.innerWidth; });
EM_JS(int, alturaNavegador, (), { return window.innerHeight; });
// Telas modernas tem mais pixels que o navegador informa (ate 2x)
EM_JS(double, densidadePixels, (), { return Math.min(window.devicePixelRatio || 1, 2); });
// O canvas desenha em alta resolucao, mas ocupa exatamente a janela
EM_JS(void, ajustarTamanhoCanvas, (int largura, int altura), {
    var c = Module.canvas;
    var w = largura + 'px', h = altura + 'px';
    if (c.style.width !== w || c.style.height !== h) {
        c.style.setProperty('width', w, 'important');
        c.style.setProperty('height', h, 'important');
    }
});
// Progresso guardado no localStorage do navegador
EM_JS(int, lerNumeroNavegador, (int id, int padrao), {
    try {
        var v = localStorage.getItem('feira_sort_' + id);
        if (v === null && id === 1) v = localStorage.getItem('feira_sort_fase');  // save da versao antiga
        return v === null ? padrao : parseInt(v);
    } catch (e) { return padrao; }
});
EM_JS(void, gravarNumeroNavegador, (int id, int valor), {
    try { localStorage.setItem('feira_sort_' + id, valor); } catch (e) {}
});
// Toque na tela lido direto do navegador (a raylib 4.2 conta os dedos errado
// no celular: depois que o dedo sai, ela acha que ele continua na tela)
EM_JS(int, toquesAtivos, (), { return window.feiraToque ? window.feiraToque.ativos : 0; });
EM_JS(int, contadorDeToques, (), { return window.feiraToque ? window.feiraToque.contador : 0; });
EM_JS(double, toqueX, (), { return window.feiraToque ? window.feiraToque.x : 0; });
EM_JS(double, toqueY, (), { return window.feiraToque ? window.feiraToque.y : 0; });
// Abre um link (usado para mandar o postal no WhatsApp)
EM_JS(void, abrirLink, (const char* endereco), { window.location.href = UTF8ToString(endereco); });
EM_JS(int, dataDeHoje, (), {
    var d = new Date();
    return d.getFullYear() * 10000 + (d.getMonth() + 1) * 100 + d.getDate();
});
#else
// Data de hoje no formato AAAAMMDD (ex.: 20260925)
int dataDeHoje() {
    time_t agora = time(nullptr);
    tm* local = localtime(&agora);
    return (local->tm_year + 1900) * 10000 + (local->tm_mon + 1) * 100 + local->tm_mday;
}
#endif

// ============================================================
//  PROGRESSO SALVO: fase liberada, moedas, estrelas de cada fase
// ============================================================

enum ChaveProgresso { P_FASE_LIBERADA = 1, P_MOEDAS = 2, P_DESAFIO = 3, P_TUTORIAL = 4, P_ESTRELAS = 1000 };

struct Progresso {
    map<int, int> valores;

#if defined(PLATFORM_WEB)
    void carregar() {}
    int obter(int id, int padrao) {
        auto it = valores.find(id);
        if (it != valores.end()) return it->second;
        int v = lerNumeroNavegador(id, padrao);
        valores[id] = v;
        return v;
    }
    void definir(int id, int valor) {
        valores[id] = valor;
        gravarNumeroNavegador(id, valor);
    }
#else
    string caminho() const { return string(GetApplicationDirectory()) + "progresso.txt"; }

    // Arquivo com linhas "chave valor". A versao antiga tinha so um numero (a fase).
    void carregar() {
        ifstream arquivo(caminho());
        vector<int> numeros;
        int n;
        while (arquivo >> n) numeros.push_back(n);
        if (numeros.size() == 1) valores[P_FASE_LIBERADA] = numeros[0];
        for (size_t i = 0; i + 1 < numeros.size(); i += 2) valores[numeros[i]] = numeros[i + 1];
    }
    int obter(int id, int padrao) {
        auto it = valores.find(id);
        return it == valores.end() ? padrao : it->second;
    }
    void definir(int id, int valor) {
        valores[id] = valor;
        ofstream arquivo(caminho());
        for (auto& [chave, v] : valores) arquivo << chave << " " << v << "\n";
    }
#endif
};

// ============================================================
//  UTILIDADES
// ============================================================

// Fonte Poppins com os acentos do portugues
Font carregarFonte() {
    string caminho = string(GetApplicationDirectory()) + "Poppins-Bold.ttf";
    if (!FileExists(caminho.c_str())) return GetFontDefault();
    vector<int> letras;
    for (int c = 32; c < 127; c++) letras.push_back(c);
    const char* acentuadas = "áàâãéêíóôõúüçÁÀÂÃÉÊÍÓÔÕÚÜÇ";
    for (int i = 0; acentuadas[i] != '\0';) {
        int tamanho = 0;
        letras.push_back(GetCodepoint(acentuadas + i, &tamanho));
        i += tamanho;
    }
    Font f = LoadFontEx(caminho.c_str(), 96, letras.data(), (int)letras.size());
    SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    return f;
}

float suavizar(float t) { return t * t * (3 - 2 * t); }

// Curva que passa um pouco do tamanho final e volta ("pulinho")
float efeitoMola(float t) {
    t = min(1.0f, max(0.0f, t));
    const float c1 = 1.70158f, c3 = c1 + 1;
    return 1 + c3 * powf(t - 1, 3) + c1 * powf(t - 1, 2);
}

const char* sortear(const vector<const char*>& frases) {
    return frases[GetRandomValue(0, (int)frases.size() - 1)];
}

float distancia(Vector2 a, Vector2 b) { return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)); }

// Em telas muito altas, o conteudo fica centralizado na vertical
float deslocamentoVertical(float H, bool retrato) {
    float util = min(H, (retrato ? 1280.0f : 720.0f) * 1.08f);
    return (H - util) / 2;
}

// ---------------- Economia do jogo ----------------
const int MOEDAS_INICIAIS = 100;   // presente de boas-vindas
const int CUSTO_CAIXOTE = 60;      // reforco "caixote extra"
const int PREMIO_DESAFIO = 100;    // desafio do dia

// ============================================================
//  LAYOUT DA TELA DE JOGO: muda se a tela esta deitada
//  (computador) ou em pe (celular).
// ============================================================

struct Layout {
    float W = 0, H = 0;
    bool retrato = false;
    vector<Rectangle> caixotes;
    vector<float> prateleiras;
    Vector2 feirante;
    float escalaFeirante = 0.8f;
    Rectangle balao;
    Vector2 pontaBalao;
    Rectangle pilulaFase, pilulaMoedas;
    Botao desfazer, dica, reiniciar, caixote, mapa, som;
};

void descer(Rectangle& r, float dy) { r.y += dy; }

Layout montarLayout(float W, float Htotal, bool retrato, int total, bool somLigado) {
    Layout L;
    L.W = W;
    L.H = Htotal;
    float dy = deslocamentoVertical(Htotal, retrato);
    float H = Htotal - 2 * dy;
    L.retrato = retrato;
    Icone iconeSom = somLigado ? ICONE_SOM : ICONE_MUDO;
    Color marrom = COR_MARROM;

    auto linhaDeCaixotes = [&](int inicio, int quantidade, float baseY) {
        float espaco = min(24.0f, (W - 30 - quantidade * CAIXOTE_LARGURA) / max(1, quantidade - 1));
        float largura = quantidade * CAIXOTE_LARGURA + (quantidade - 1) * espaco;
        float x0 = (W - largura) / 2;
        for (int i = 0; i < quantidade; i++) {
            L.caixotes[inicio + i] = {x0 + i * (CAIXOTE_LARGURA + espaco), baseY - CAIXOTE_ALTURA, CAIXOTE_LARGURA,
                                      CAIXOTE_ALTURA};
        }
        L.prateleiras.push_back(baseY);
    };
    L.caixotes.resize(total);

    if (!retrato) {
        L.feirante = {112, 232};
        L.escalaFeirante = 0.78f;
        L.balao = {200, 80, min(540.0f, W - 620), 112};
        L.pontaBalao = {172, 150};
        L.pilulaFase = {W - 392, 82, 180, 52};
        L.pilulaMoedas = {W - 200, 82, 180, 52};
        L.mapa = {{W - 392, 146, 180, 54}, "MAPA", marrom, ICONE_MAPA, 24};
        L.som = {{W - 200, 146, 180, 54}, "SOM", marrom, iconeSom, 24};
        linhaDeCaixotes(0, total, H - 118);
        float bw = 206, bh = 64, esp = 18;
        float x0 = (W - (4 * bw + 3 * esp)) / 2, y = H - 84;
        L.desfazer = {{x0, y, bw, bh}, "DESFAZER", COR_AZUL, ICONE_DESFAZER, 26};
        L.dica = {{x0 + (bw + esp), y, bw, bh}, "DICA", COR_VERDE, ICONE_DICA, 26};
        L.reiniciar = {{x0 + 2 * (bw + esp), y, bw, bh}, "REINICIAR", marrom, ICONE_REINICIAR, 26};
        L.caixote = {{x0 + 3 * (bw + esp), y, bw, bh}, "CAIXOTE", COR_LARANJA, ICONE_CAIXOTE, 26};
    } else {
        L.feirante = {100, 222};
        L.escalaFeirante = 0.76f;
        L.balao = {186, 74, W - 206, 118};
        L.pontaBalao = {160, 146};
        L.pilulaFase = {W / 2 - 186, 212, 176, 48};
        L.pilulaMoedas = {W / 2 + 10, 212, 176, 48};
        int linhas = total <= 4 ? 1 : 2;
        int porLinha = (total + linhas - 1) / linhas;
        float base2 = H - 200;
        if (linhas == 1) {
            linhaDeCaixotes(0, total, base2);
        } else {
            float base1 = base2 - CAIXOTE_ALTURA - 82;
            linhaDeCaixotes(0, porLinha, base1);
            linhaDeCaixotes(porLinha, total - porLinha, base2);
        }
        float esp = 14, bw = (W - 40 - 2 * esp) / 3, bh = 66;
        float y1 = H - 172, y2 = H - 92;
        L.desfazer = {{20, y1, bw, bh}, "DESFAZER", COR_AZUL, ICONE_DESFAZER, 24};
        L.dica = {{20 + bw + esp, y1, bw, bh}, "DICA", COR_VERDE, ICONE_DICA, 24};
        L.reiniciar = {{20 + 2 * (bw + esp), y1, bw, bh}, "REINICIAR", marrom, ICONE_REINICIAR, 24};
        L.caixote = {{20, y2, bw, bh}, "CAIXOTE", COR_LARANJA, ICONE_CAIXOTE, 24};
        L.mapa = {{20 + bw + esp, y2, bw, bh}, "MAPA", marrom, ICONE_MAPA, 24};
        L.som = {{20 + 2 * (bw + esp), y2, bw, bh}, "SOM", marrom, iconeSom, 24};
    }

    // Aplica o deslocamento vertical em tudo
    for (Rectangle& r : L.caixotes) descer(r, dy);
    for (float& y : L.prateleiras) y += dy;
    L.feirante.y += dy;
    L.pontaBalao.y += dy;
    for (Rectangle* r : {&L.balao, &L.pilulaFase, &L.pilulaMoedas}) descer(*r, dy);
    for (Botao* b : {&L.desfazer, &L.dica, &L.reiniciar, &L.caixote, &L.mapa, &L.som}) descer(b->r, dy);
    return L;
}

// Centro da vaga "vaga" (0 = fundo do caixote)
Vector2 posicaoVaga(const Rectangle& r, int vaga) {
    return {r.x + r.width / 2, r.y + r.height - 20 - VAGA_ALTURA * (vaga + 0.5f)};
}

Rectangle areaDeClique(const Rectangle& r) {
    return {r.x - 8, r.y - 64, r.width + 16, r.height + 72};
}

// ============================================================
//  COMPARTILHAR O POSTAL NO WHATSAPP
// ============================================================

const char* ENDERECO_DO_JOGO = "https://diegogarciabalbo-cell.github.io/feira-sort/";

// Transforma o texto num formato que pode ir dentro de um link (ex.: espaco vira %20)
string codificarParaLink(const string& texto) {
    const char* hexa = "0123456789ABCDEF";
    string resultado;
    for (unsigned char c : texto) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            resultado += (char)c;
        } else {
            resultado += '%';
            resultado += hexa[c >> 4];
            resultado += hexa[c & 15];
        }
    }
    return resultado;
}

void compartilharPostal(int capitulo) {
    const Capitulo& c = CAPITULOS[capitulo];
    string mensagem = "Postal do Feira Sort: " + string(c.feira) + ", " + c.cidade + "\n\n" + c.curiosidade +
                      "\n\nReceita: " + c.receitaTitulo + "\n" + c.receita + "\n\nJogue também: " + ENDERECO_DO_JOGO;
    string link = "https://api.whatsapp.com/send?text=" + codificarParaLink(mensagem);
#if defined(PLATFORM_WEB)
    abrirLink(link.c_str());
#else
    OpenURL(link.c_str());
#endif
}

// ============================================================
//  O JOGO
// ============================================================

enum Tela { MENU, BRASIL, MAPA, JOGO };

struct Voo {
    bool ativo = false;
    int de = 0, para = 0, quantidade = 0, fruta = 0, tamanhoOrigem = 0, tamanhoDestino = 0;
    float progresso = 0;
};

// Estado do toque/arraste (usado no mapa)
struct Toque {
    bool segurando = false, moveu = false;
    Vector2 inicio = {0, 0};
    float rolagemInicial = 0;
};

const float ESPACO_NO_MAPA = 150;

struct Jogo {
    Tela tela = MENU;
    bool mostrarAjuda = false;
    Progresso progresso;
    Sons sons;

    int faseLiberada = 1, moedas = 0;

    // ----- Nivel em andamento -----
    int fase = 1;
    bool desafio = false;
    Nivel nivel;
    Feira feira;
    Escondidas escondidas;
    vector<Feira> historico;
    int jogadas = 0, dicasUsadas = 0;
    bool usouCaixote = false;
    int selecionado = -1;
    bool ganhou = false, travou = false;
    float tempoVitoria = 0, proximoConfete = 0;
    int estrelasGanhas = 0, moedasGanhas = 0;

    string fala;
    float tempoFala = 0;
    int humor = 0;

    int dicaDe = -1, dicaPara = -1;
    float tempoDica = 0;
    vector<Jogada> planoDica;  // seguindo as dicas, o jogador sempre termina
    Feira estadoDoPlano;
    int caixoteTremendo = -1;
    float tempoTremor = 0;
    vector<float> tempoPulo, tempoSelo;
    Voo voo;

    // ----- Mapas e postal -----
    float rolagemMapa = 0;
    Toque toque;
    int capituloMapa = 0, voltaMapa = 0;  // qual feira o mapa de fases mostra
    int postalAberto = -1;                // postal na tela (-1 = nenhum)
    bool postalDaConquista = false;       // aberto logo depois de completar a feira
    bool viajando = false;                // caminhao indo para a proxima feira
    int viagemDe = 0, viagemPara = 0;
    float tempoViagem = 0;

    int capituloDoJogo() const { return desafio ? dataDeHoje() % TOTAL_CAPITULOS : capituloDaFase(fase); }

    void carregarProgresso() {
        progresso.carregar();
        faseLiberada = max(1, progresso.obter(P_FASE_LIBERADA, 1));
        moedas = progresso.obter(P_MOEDAS, -1);
        if (moedas < 0) {
            moedas = MOEDAS_INICIAIS;
            progresso.definir(P_MOEDAS, moedas);
        }
    }

    int estrelasDaFase(int f) { return progresso.obter(P_ESTRELAS + f, 0); }
    bool desafioFeitoHoje() { return progresso.obter(P_DESAFIO, 0) == dataDeHoje(); }

    void falar(const string& texto, float segundos = 3.5f, int humorNovo = 0) {
        fala = texto;
        tempoFala = segundos;
        humor = humorNovo;
    }

    // ---------------- Comecar / reiniciar ----------------

    void comecarNivel(int f, bool ehDesafio) {
        fase = f;
        desafio = ehDesafio;
        nivel = desafio ? nivelDoDesafio(dataDeHoje()) : nivelDaFase(f);
        usouCaixote = false;
        reiniciarNivel();
        tela = JOGO;
        const Capitulo& cap = CAPITULOS[capituloDoJogo()];
        if (desafio) {
            falar("Desafio do dia! Vale " + to_string(PREMIO_DESAFIO) + " moedas. Boa sorte!", 5, 1);
        } else if (posicaoNoCapitulo(fase) == 0) {
            falar("Chegamos em " + string(cap.curto) + "! Aqui a banca tem " + NOMES_FRUTAS[cap.regionais[0]] + " e " +
                      NOMES_FRUTAS[cap.regionais[1]] + ".",
                  6, 1);
        } else if (fase == PRIMEIRA_FASE_ESCONDIDA) {
            falar("Novidade! Algumas frutas vêm no saquinho. Elas aparecem quando ficam em cima!", 7, 1);
        } else {
            falar(sortear({"Bom dia, freguês! Vamos arrumar a banca?", "Que bom te ver! Bora organizar as frutas?",
                           "A feira hoje está bonita! Vamos começar?"}),
                  4, 1);
        }
        if (!progresso.obter(P_TUTORIAL, 0)) {
            mostrarAjuda = true;
            progresso.definir(P_TUTORIAL, 1);
        }
    }

    void reiniciarNivel() {
        bool manterCaixote = usouCaixote;  // o caixote comprado continua valendo
        feira = nivel.feira;
        escondidas = nivel.escondidas;
        if (manterCaixote) {
            feira.push_back({});
            escondidas.push_back(vector<bool>(CAPACIDADE, false));
        }
        revelarTopos(feira, escondidas);
        historico.clear();
        jogadas = dicasUsadas = 0;
        selecionado = -1;
        ganhou = travou = false;
        tempoDica = 0;
        planoDica.clear();
        voo.ativo = false;
        tempoPulo.assign(feira.size(), 0);
        tempoSelo.assign(feira.size(), 1);
        particulas.clear();
    }

    void verificarTravamento() {
        vector<Jogada> caminho;
        bool estourou;
        travou = !resolver(feira, caminho, estourou) && !estourou;
        if (travou) falar("Opa, a banca travou! Toque em DESFAZER para voltar.", 5, 0);
    }

    int estrelas() const {
        if (dicasUsadas == 0 && !usouCaixote) return 3;
        if (dicasUsadas <= 2) return 2;
        return 1;
    }

    string falaAtual() const {
        if (tempoFala > 0) return fala;
        if (travou) return "Opa, a banca travou! Toque em DESFAZER para voltar.";
        if (selecionado >= 0) return "Agora toque no caixote onde quer colocar.";
        return "Toque num caixote para pegar as frutas de cima.";
    }

    // ---------------- Acoes do jogador ----------------

    void cliqueNoCaixote(int c) {
        if (selecionado == -1) {
            if (feira[c].empty()) {
                falar("Esse caixote está vazio. Escolha um com frutas!");
            } else {
                selecionado = c;
                sons.tocar(sons.pegar);
            }
        } else if (selecionado == c) {
            selecionado = -1;
        } else if (podeMover(feira, selecionado, c)) {
            voo = {true, selecionado, c, quantasMovem(feira, selecionado, c), feira[selecionado].back(),
                   (int)feira[selecionado].size(), (int)feira[c].size(), 0};
            selecionado = -1;
            tempoDica = 0;
            tempoFala = min(tempoFala, 0.8f);
        } else {
            if ((int)feira[c].size() >= CAPACIDADE) falar("Esse caixote já está cheio! Escolha outro.");
            else falar("Ih, aí não dá! Só em cima da mesma fruta ou num caixote vazio.");
            caixoteTremendo = c;
            tempoTremor = 0.4f;
            sons.tocar(sons.erro);
        }
    }

    void pedirDica() {
        selecionado = -1;
        if (travou) {
            falar("Primeiro toque em DESFAZER, freguês!");
            return;
        }
        bool planoValido = !planoDica.empty() && estadoDoPlano == feira &&
                           podeMover(feira, planoDica[0].first, planoDica[0].second);
        if (!planoValido) {
            bool estourou;
            if (!resolver(feira, planoDica, estourou)) planoDica.clear();
            estadoDoPlano = feira;
        }
        if (!planoDica.empty()) {
            dicaDe = planoDica[0].first;
            dicaPara = planoDica[0].second;
            tempoDica = 5;
            dicasUsadas++;
            falar("Pegue do caixote PEGUE e ponha no COLOQUE, ó!", 5, 1);
        } else {
            falar("Tente juntar as frutas iguais!");
        }
    }

    void desfazer() {
        if (historico.empty()) {
            falar("Ainda não tem jogada para desfazer.");
            return;
        }
        feira = historico.back();
        historico.pop_back();
        revelarTopos(feira, escondidas);
        jogadas--;
        selecionado = -1;
        travou = false;
        tempoFala = 0;
        verificarTravamento();
    }

    // Reforco estilo Magic Sort: mais um caixote vazio na banca
    void usarCaixoteExtra() {
        if (usouCaixote) {
            falar("Só dá para usar um caixote extra por fase.");
            return;
        }
        if (moedas < CUSTO_CAIXOTE) {
            falar("Faltam moedas! Você ganha moedas passando de fase e no desafio do dia.");
            sons.tocar(sons.erro);
            return;
        }
        moedas -= CUSTO_CAIXOTE;
        progresso.definir(P_MOEDAS, moedas);
        usouCaixote = true;
        feira.push_back({});
        for (Feira& antigo : historico) antigo.push_back({});
        escondidas.push_back(vector<bool>(CAPACIDADE, false));
        tempoPulo.push_back(0);
        tempoSelo.push_back(1);
        planoDica.clear();
        selecionado = -1;
        travou = false;
        verificarTravamento();
        sons.tocar(sons.pronto);
        falar("Mais um caixote na banca! Aproveite.", 3, 1);
    }

    void concluirNivel(const Layout& L) {
        ganhou = true;
        tempoVitoria = 0;
        proximoConfete = 0.9f;
        estrelasGanhas = estrelas();
        if (desafio) {
            moedasGanhas = PREMIO_DESAFIO;
            progresso.definir(P_DESAFIO, dataDeHoje());
        } else {
            int antes = estrelasDaFase(fase);
            moedasGanhas = antes == 0 ? 5 + 5 * estrelasGanhas : 5;  // repetir fase rende menos
            if (antes == 0 && ultimaFaseDoCapitulo(fase)) moedasGanhas += PREMIO_CAPITULO;  // feira completa!
            progresso.definir(P_ESTRELAS + fase, max(antes, estrelasGanhas));
            if (fase >= faseLiberada) {
                faseLiberada = fase + 1;
                progresso.definir(P_FASE_LIBERADA, faseLiberada);
            }
        }
        moedas += moedasGanhas;
        progresso.definir(P_MOEDAS, moedas);
        sons.tocar(sons.vitoria);
        for (int i = 0; i < 5; i++) soltarConfete({L.W * (0.1f + i * 0.2f), L.H * 0.55f}, 30, 900);
        falar(sortear({"Banca arrumadinha! Obrigado, freguês!", "Que capricho! Os fregueses vão adorar!",
                       "Nota dez! Você é bom de feira!"}),
              100, 1);
    }

    // ---------------- Atualizacao do jogo ----------------

    void atualizarJogo(float dt, Vector2 mouse, bool clicou, const Layout& L) {
        for (float& t : tempoPulo) t = max(0.0f, t - dt);
        for (float& t : tempoSelo) t += dt;

        if (voo.ativo) {
            voo.progresso += dt / 0.36f;
            if (voo.progresso >= 1) {
                historico.push_back(feira);
                bool seguiuPlano = !planoDica.empty() && estadoDoPlano == feira &&
                                   planoDica[0] == Jogada{voo.de, voo.para};
                mover(feira, voo.de, voo.para);
                revelarTopos(feira, escondidas);
                if (seguiuPlano) {
                    planoDica.erase(planoDica.begin());
                    estadoDoPlano = feira;
                } else {
                    planoDica.clear();
                }
                jogadas++;
                voo.ativo = false;
                tempoPulo[voo.para] = 0.25f;
                const Caixote& destino = feira[voo.para];
                const Rectangle& r = L.caixotes[voo.para];
                if (venceu(feira)) {
                    tempoSelo[voo.para] = 0;
                    concluirNivel(L);
                } else if (caixotePronto(destino)) {
                    sons.tocar(sons.pronto);
                    tempoSelo[voo.para] = 0;
                    soltarConfete({r.x + r.width / 2, r.y + 10}, 26, 620);
                    falar(sortear({"Que beleza!", "Caprichou!", "Isso mesmo!", "Ficou uma lindeza!", "Muito bem!"}),
                          2.2f, 1);
                    verificarTravamento();
                } else {
                    sons.tocar(sons.plim[destino.size() - 1]);
                    verificarTravamento();
                }
            }
            return;
        }

        if (ganhou) {
            tempoVitoria += dt;
            proximoConfete -= dt;
            if (proximoConfete <= 0 && tempoVitoria < 6) {
                soltarConfete({(float)GetRandomValue(100, (int)L.W - 100), L.H * 0.5f}, 18, 800);
                proximoConfete = 1.1f;
            }
            if (clicou && tempoVitoria > 0.8f) {
                if (dentro(mouse, botaoVitoriaMapa(L))) {
                    sons.tocar(sons.clique);
                    irParaMapaDaFase(fase, L.H);
                } else if (!desafio && dentro(mouse, botaoVitoriaProxima(L))) {
                    sons.tocar(sons.clique);
                    if (ultimaFaseDoCapitulo(fase)) {
                        // Completou a feira: ganha o postal
                        postalAberto = capituloDaFase(fase);
                        postalDaConquista = true;
                    } else {
                        comecarNivel(fase + 1, false);
                    }
                }
            }
            return;
        }

        if (!clicou) return;

        if (dentro(mouse, L.som.r)) {
            sons.ligado = !sons.ligado;
            sons.tocar(sons.clique);
        } else if (dentro(mouse, L.mapa.r)) {
            sons.tocar(sons.clique);
            irParaMapaDaFase(fase, L.H);
        } else if (dentro(mouse, L.desfazer.r)) {
            sons.tocar(sons.clique);
            desfazer();
        } else if (dentro(mouse, L.dica.r)) {
            sons.tocar(sons.clique);
            pedirDica();
        } else if (dentro(mouse, L.reiniciar.r)) {
            sons.tocar(sons.clique);
            reiniciarNivel();
            falar("Recomeçamos a mesma banca. Com calma!");
        } else if (dentro(mouse, L.caixote.r)) {
            usarCaixoteExtra();
        } else {
            int clicado = -1;
            for (int i = 0; i < (int)feira.size(); i++) {
                if (dentro(mouse, areaDeClique(L.caixotes[i]))) clicado = i;
            }
            if (clicado >= 0) cliqueNoCaixote(clicado);
            else selecionado = -1;
        }
    }

    // ---------------- Desenho do jogo ----------------

    void desenharJogo(const Layout& L, Vector2 mouse, float tempo) {
        const Capitulo& cap = CAPITULOS[capituloDoJogo()];
        desenharCeu(L.W, L.H, tempo, cap.ceuTopo, cap.ceuBase);
        desenharBandeirinhas(L.W, 70, tempo);
        desenharToldo(L.W, cap.toldoA, cap.toldoB);
        for (float y : L.prateleiras) desenharPrateleira(L.W, y);
        desenharToalha(L.W, L.prateleiras.back() + 26, L.H);

        desenharFeirante(L.feirante, L.escalaFeirante, tempo, humor);
        desenharBalao(L.balao, falaAtual(), L.pontaBalao, L.retrato ? 25 : 26);
        if (desafio) desenharPilula(L.pilulaFase, "Desafio", "do dia");
        else desenharPilula(L.pilulaFase, "Fase", to_string(fase));
        desenharPilulaMoedas(L.pilulaMoedas, moedas);

        bool podeInteragir = !voo.ativo && !ganhou && !mostrarAjuda;
        int total = feira.size();
        for (int i = 0; i < total; i++) {
            Rectangle r = L.caixotes[i];
            float tremor = (i == caixoteTremendo && tempoTremor > 0) ? sinf(tempo * 60) * 6 : 0;
            r.x += tremor;
            bool emCima = podeInteragir && dentro(mouse, areaDeClique(L.caixotes[i]));
            bool pronto = caixotePronto(feira[i]) && !(voo.ativo && i == voo.de);
            desenharCaixote(r, i == selecionado ? 2 : (emCima ? 1 : 0), pronto);

            // Dica piscando
            if (tempoDica > 0 && (i == dicaDe || i == dicaPara)) {
                float pulso = 0.5f + 0.5f * sinf(tempo * 7);
                contornoArredondado({r.x - 10, r.y - 10, r.width + 20, r.height + 20}, 0.12f, 8, 6, Fade(COR_VERDE, pulso));
                string etiqueta = i == dicaDe ? "PEGUE" : "COLOQUE";
                float tamanho = 20, largura = larguraTexto(etiqueta, tamanho) + 20;
                Rectangle tag = {r.x + r.width / 2 - largura / 2, r.y - 48, largura, 32};
                DrawRectangleRounded(tag, 0.5f, 8, COR_VERDE);
                textoCentro(etiqueta, r.x + r.width / 2, tag.y + 5, tamanho, WHITE);
            }

            // Frutas (as escondidas aparecem como saquinho)
            int visiveis = feira[i].size();
            if (voo.ativo && i == voo.de) visiveis -= voo.quantidade;
            int levantadas = (i == selecionado) ? frutasIguaisNoTopo(feira[i]) : 0;
            float pulo = tempoPulo[i] > 0 ? 1 + 0.14f * sinf(PI * (1 - tempoPulo[i] / 0.25f)) : 1;
            for (int k = 0; k < visiveis; k++) {
                Vector2 p = posicaoVaga(r, k);
                if (k >= visiveis - levantadas) p.y -= ALTURA_LEVANTADA + sinf(tempo * 5) * 3;
                else if (emCima && selecionado == -1 && k == visiveis - 1) p.y -= 4 + sinf(tempo * 8) * 2;
                if (escondidas[i][k]) desenharSaquinho(p, pulo);
                else desenharFruta(feira[i][k], p, pulo);
            }

            if (pronto) desenharSeloPronto(r, efeitoMola(tempoSelo[i] / 0.4f));
        }

        // Frutas voando em arco
        if (voo.ativo) {
            float t = suavizar(min(voo.progresso, 1.0f));
            for (int q = 0; q < voo.quantidade; q++) {
                Vector2 origem = posicaoVaga(L.caixotes[voo.de], voo.tamanhoOrigem - voo.quantidade + q);
                origem.y -= ALTURA_LEVANTADA;
                Vector2 destino = posicaoVaga(L.caixotes[voo.para], voo.tamanhoDestino + q);
                Vector2 p = {origem.x + (destino.x - origem.x) * t, origem.y + (destino.y - origem.y) * t - sinf(PI * t) * 110};
                desenharFruta(voo.fruta, p, 1.0f + 0.12f * sinf(PI * t));
            }
        }

        // Botoes
        for (const Botao* b : {&L.desfazer, &L.dica, &L.reiniciar, &L.caixote, &L.mapa, &L.som}) {
            bool ativo = true;
            if (b == &L.desfazer && historico.empty()) ativo = false;
            if (b == &L.caixote && usouCaixote) ativo = false;
            desenharBotao(*b, podeInteragir && dentro(mouse, b->r), ativo);
        }
        // Preco do caixote extra
        if (!usouCaixote) {
            Rectangle r = L.caixote.r;
            Rectangle etiqueta = {r.x + r.width - 62, r.y - 16, 72, 32};
            DrawRectangleRounded(etiqueta, 0.6f, 8, Color{90, 55, 25, 255});
            desenharMoeda({etiqueta.x + 17, etiqueta.y + 16}, 11);
            texto(to_string(CUSTO_CAIXOTE), etiqueta.x + 32, etiqueta.y + 4, 22, WHITE);
        }
        // Quando trava, o botao DESFAZER brilha
        if (travou && !voo.ativo) {
            float pulso = 0.5f + 0.5f * sinf(tempo * 6);
            Rectangle r = L.desfazer.r;
            contornoArredondado({r.x - 6, r.y - 6, r.width + 12, r.height + 14}, 0.45f, 10, 5, Fade(COR_OURO, pulso));
        }

        if (ganhou) desenharVitoria(L, mouse);
        desenharParticulas();  // confete por cima de tudo
    }

    // ---------------- Tela de vitoria ----------------

    Rectangle botaoVitoriaMapa(const Layout& L) const {
        if (desafio) return {L.W / 2 - 180, L.H / 2 + 118, 360, 84};
        return {L.W / 2 - 272, L.H / 2 + 118, 240, 84};
    }
    Rectangle botaoVitoriaProxima(const Layout& L) const { return {L.W / 2 - 16, L.H / 2 + 118, 288, 84}; }

    void desenharVitoria(const Layout& L, Vector2 mouse) {
        float aparecer = min(1.0f, tempoVitoria / 0.3f);
        DrawRectangle(0, 0, (int)L.W, (int)L.H, Fade(BLACK, 0.45f * aparecer));
        float escala = efeitoMola(tempoVitoria / 0.45f);
        float pw = 620 * escala, ph = 520 * escala;
        Rectangle painel = {L.W / 2 - pw / 2, L.H / 2 - 270 * escala, pw, ph};
        DrawRectangleRounded({painel.x + 6, painel.y + 12, painel.width, painel.height}, 0.12f, 10, Fade(BLACK, 0.25f));
        DrawRectangleRounded(painel, 0.12f, 10, COR_CREME);
        DrawRectangleRounded({painel.x + 10, painel.y + 10, painel.width - 20, painel.height - 20}, 0.1f, 10, Color{255, 252, 244, 255});
        if (tempoVitoria < 0.35f) return;

        float topo = L.H / 2 - 270;
        textoContorno("MUITO BEM!", L.W / 2, topo + 28, 66, COR_VERDE, WHITE, 4);
        bool fimDaFeira = !desafio && ultimaFaseDoCapitulo(fase);
        string subtitulo = desafio ? "Desafio do dia concluído!"
                                   : (fimDaFeira ? "Você completou a feira de " + string(CAPITULOS[capituloDaFase(fase)].curto) + "!"
                                                 : "Fase " + to_string(fase) + " concluída!");
        textoCentro(subtitulo, L.W / 2, topo + 110, 30, COR_TEXTO);

        // Estrelas aparecendo uma de cada vez
        for (int i = 0; i < 3; i++) {
            float t = (tempoVitoria - 0.5f - i * 0.3f) / 0.35f;
            if (t <= 0) continue;
            float s = efeitoMola(t);
            Vector2 c = {L.W / 2 + (i - 1) * 118.0f, topo + 222 - (i == 1 ? 16 : 0)};
            float raio = (i == 1 ? 56 : 46) * s;
            desenharEstrela(mais(c, 3, 5), raio, Fade(BLACK, 0.15f));
            desenharEstrela(c, raio + 4, i < estrelasGanhas ? Color{214, 150, 20, 255} : Color{190, 180, 170, 255});
            desenharEstrela(c, raio, i < estrelasGanhas ? COR_OURO : Color{222, 214, 204, 255});
        }

        // Moedas ganhas
        if (tempoVitoria > 1.4f) {
            string ganho = "+" + to_string(moedasGanhas) + " moedas";
            float largura = larguraTexto(ganho, 32) + 50;
            float x = L.W / 2 - largura / 2;
            desenharMoeda({x + 18, topo + 318}, 18);
            texto(ganho, x + 46, topo + 300, 32, COR_LARANJA);
        }
        string recado = estrelasGanhas == 3 ? "Sem nenhuma ajuda! Parabéns!" : "Para 3 estrelas: sem dica e sem caixote extra.";
        textoCentro(recado, L.W / 2, topo + 348, 22, Fade(COR_TEXTO, 0.75f));

        Botao mapa = {botaoVitoriaMapa(L), desafio ? "VOLTAR AO MAPA" : "MAPA", COR_MARROM, ICONE_MAPA, 30};
        desenharBotao(mapa, dentro(mouse, mapa.r));
        if (!desafio) {
            Botao proxima = {botaoVitoriaProxima(L), fimDaFeira ? "VER POSTAL" : "PRÓXIMA", COR_VERDE, ICONE_JOGAR, 30};
            desenharBotao(proxima, dentro(mouse, proxima.r));
        }
    }

    // ============================================================
    //  VIAGEM PELO BRASIL: o mapa com as 5 feiras
    // ============================================================

    int capituloAtual() const { return capituloDaFase(faseLiberada); }
    int voltaAtual() const { return voltaDaFase(faseLiberada); }

    // Estado de cada feira: 0 bloqueada, 1 atual, 2 concluida
    int estadoDoCapitulo(int c) const {
        if (c == capituloAtual()) return 1;
        if (voltaAtual() > 0 || c < capituloAtual()) return 2;
        return 0;
    }

    // Em qual "volta" do Brasil esta a feira c (depois da 5a feira a viagem recomeca)
    int voltaDoCapitulo(int c) const {
        return (c <= capituloAtual() || voltaAtual() == 0) ? voltaAtual() : voltaAtual() - 1;
    }

    int estrelasDoCapitulo(int c) {
        int primeira = primeiraFaseDoCapitulo(c, voltaDoCapitulo(c)), total = 0;
        for (int f = primeira; f < primeira + FASES_POR_CAPITULO; f++) total += estrelasDaFase(f);
        return total;
    }

    void irParaBrasil() {
        tela = BRASIL;
        particulas.clear();
    }

    // Area do mapa do Brasil e dos cartoes das feiras na tela
    Rectangle areaDoBrasil(float W, float H) const {
        float dy = deslocamentoVertical(H, H > W);
        if (W > H) {
            float altura = H - 2 * dy - 290;
            return {W * 0.27f - altura * BRASIL_LARGURA / 2, 150 + dy, altura * BRASIL_LARGURA, altura};
        }
        float altura = min(W - 90, 560.0f);
        return {W / 2 - altura * BRASIL_LARGURA / 2, 150 + dy, altura * BRASIL_LARGURA, altura};
    }

    Vector2 pontoNoBrasil(const float* xy, float W, float H) const {
        Rectangle a = areaDoBrasil(W, H);
        return {a.x + xy[0] * a.height, a.y + xy[1] * a.height};
    }

    Rectangle cartaoDaFeira(int c, float W, float H) const {
        float dy = deslocamentoVertical(H, H > W);
        if (W > H) return {W * 0.52f, 204 + dy + c * 76.0f, W * 0.48f - 30, 68};
        Rectangle a = areaDoBrasil(W, H);
        return {30, a.y + a.height + 26 + c * 76.0f, W - 60, 68};
    }

    Rectangle botaoInicioTopo() const { return {18, 78, 180, 58}; }
    Rectangle botaoDesafioTopo(float W) const { return {W - 238, 78, 220, 58}; }
    Rectangle pilulaMoedasTopo(float W) const { return {W / 2 - 85, 80, 170, 54}; }
    Rectangle botaoJogarBaixo(float W, float H) const {
        float dy = deslocamentoVertical(H, H > W);
        return {W / 2 - 210, H - dy - 112, 420, 88};
    }

    void abrirFeira(int c, float H) {
        tela = MAPA;
        capituloMapa = c;
        voltaMapa = voltaDoCapitulo(c);
        particulas.clear();
        // Centraliza o mapa na fase atual (ou na primeira)
        int primeira = primeiraFaseDoCapitulo(capituloMapa, voltaMapa);
        int indice = max(1, min(FASES_POR_CAPITULO, faseLiberada - primeira + 1));
        rolagemMapa = (indice - 1) * ESPACO_NO_MAPA - (H - 220 - H * 0.55f);
        limitarRolagem(H);
    }

    void tocarNoDesafio() {
        if (desafioFeitoHoje()) {
            sons.tocar(sons.erro);
        } else {
            sons.tocar(sons.clique);
            comecarNivel(0, true);
        }
    }

    void atualizarBrasil(Vector2 mouse, bool clicou, float dt, float W, float H) {
        if (viajando) {
            tempoViagem += dt;
            if (tempoViagem > 3.2f) viajando = false;
        }
        if (!clicou) return;
        if (dentro(mouse, botaoInicioTopo())) {
            sons.tocar(sons.clique);
            tela = MENU;
            return;
        }
        if (dentro(mouse, botaoDesafioTopo(W))) return tocarNoDesafio();
        if (dentro(mouse, botaoJogarBaixo(W, H))) {
            sons.tocar(sons.clique);
            comecarNivel(faseLiberada, false);
            return;
        }
        for (int c = 0; c < TOTAL_CAPITULOS; c++) {
            bool tocou = dentro(mouse, cartaoDaFeira(c, W, H)) ||
                         distancia(mouse, pontoNoBrasil(CAPITULOS[c].posicao, W, H)) < 36;
            if (!tocou) continue;
            if (estadoDoCapitulo(c) == 0) {
                sons.tocar(sons.erro);
            } else {
                sons.tocar(sons.clique);
                abrirFeira(c, H);
            }
            return;
        }
    }

    void desenharBrasil(float W, float H, Vector2 mouse, float tempo) {
        // Mar
        DrawRectangleGradientV(0, 0, (int)W, (int)H, Color{120, 192, 234, 255}, Color{80, 150, 206, 255});
        for (int i = 0; i < 14; i++) {
            float x = fmodf(i * 173.0f + tempo * 12, W + 80) - 40, y = 170 + (i * 97) % (int)max(200.0f, H - 250);
            DrawLineEx({x, y}, {x + 26, y - 4}, 3, Fade(WHITE, 0.35f));
            DrawLineEx({x + 26, y - 4}, {x + 52, y}, 3, Fade(WHITE, 0.35f));
        }

        // O Brasil: triangulos preenchidos + contorno
        Rectangle a = areaDoBrasil(W, H);
        auto ponto = [&](int i) { return Vector2{a.x + BRASIL_XY[i][0] * a.height, a.y + BRASIL_XY[i][1] * a.height}; };
        for (int camada = 0; camada < 2; camada++) {
            for (int t = 0; t < BRASIL_TRIANGULOS; t++) {
                Vector2 p1 = ponto(BRASIL_INDICES[t * 3]), p2 = ponto(BRASIL_INDICES[t * 3 + 1]), p3 = ponto(BRASIL_INDICES[t * 3 + 2]);
                if (camada == 0) triangulo(mais(p1, 6, 9), mais(p2, 6, 9), mais(p3, 6, 9), Fade(BLACK, 0.18f));
                else triangulo(p1, p2, p3, Color{132, 198, 98, 255});
            }
        }
        for (int i = 0; i < BRASIL_PONTOS; i++) DrawLineEx(ponto(i), ponto((i + 1) % BRASIL_PONTOS), 3, Color{72, 138, 66, 255});

        // Rota da viagem
        for (int c = 0; c + 1 < TOTAL_CAPITULOS; c++) {
            desenharRota(pontoNoBrasil(CAPITULOS[c].posicao, W, H), pontoNoBrasil(CAPITULOS[c + 1].posicao, W, H), 0.18f,
                         Fade(WHITE, 0.95f));
        }

        // Cidades
        for (int c = 0; c < TOTAL_CAPITULOS; c++) {
            Vector2 p = pontoNoBrasil(CAPITULOS[c].posicao, W, H);
            int estado = estadoDoCapitulo(c);
            if (viajando && c == viagemPara && tempoViagem < 2.6f) estado = 0;  // ainda nao chegou
            desenharMarcadorCidade(p, estado, tempo);
            string nome = CAPITULOS[c].curto;
            float largura = larguraTexto(nome, 22);
            bool esquerda = CAPITULOS[c].posicao[0] > 0.8f;
            float x = esquerda ? p.x - 28 - largura / 2 : p.x + 28 + largura / 2;
            textoContorno(nome, x, p.y - 13, 22, WHITE, Color{40, 70, 40, 255}, 2.5f);
        }

        // Caminhao do Seu Ze (viajando ou parado na feira atual)
        Vector2 onde = pontoNoBrasil(CAPITULOS[capituloAtual()].posicao, W, H);
        float direcao = 1;
        if (viajando) {
            Vector2 de = pontoNoBrasil(CAPITULOS[viagemDe].posicao, W, H), para = pontoNoBrasil(CAPITULOS[viagemPara].posicao, W, H);
            float t = suavizar(min(1.0f, tempoViagem / 2.6f));
            onde = pontoNaRota(de, para, 0.18f, t);
            direcao = para.x >= de.x ? 1.0f : -1.0f;
        }
        desenharCaminhao(mais(onde, 0, -44), 0.95f, tempo, direcao);

        // Cartoes das feiras
        if (W > H) textoContorno("A viagem do Seu Zé", W * 0.52f + (W * 0.48f - 30) / 2, 150 + deslocamentoVertical(H, false), 36,
                                 COR_CREME, Color{60, 90, 140, 255}, 3);
        for (int c = 0; c < TOTAL_CAPITULOS; c++) {
            Rectangle r = cartaoDaFeira(c, W, H);
            int estado = estadoDoCapitulo(c);
            bool emCima = dentro(mouse, r) && estado != 0 && !mostrarAjuda;
            DrawRectangleRounded({r.x + 3, r.y + 5, r.width, r.height}, 0.35f, 8, Fade(BLACK, 0.18f));
            DrawRectangleRounded(r, 0.35f, 8, estado == 0 ? Color{226, 222, 214, 255} : (emCima ? WHITE : COR_CREME));
            DrawRectangleRounded({r.x, r.y, 12, r.height}, 0.5f, 6, estado == 0 ? Color{180, 172, 162, 255} : CAPITULOS[c].toldoA);
            desenharFruta(CAPITULOS[c].regionais[0], {r.x + 46, r.y + r.height / 2 + 2}, 0.62f);
            desenharFruta(CAPITULOS[c].regionais[1], {r.x + 94, r.y + r.height / 2 + 2}, 0.62f);
            Color corTexto = estado == 0 ? Fade(COR_TEXTO, 0.45f) : COR_TEXTO;
            texto(CAPITULOS[c].curto, r.x + 128, r.y + 8, 26, corTexto);
            texto(CAPITULOS[c].feira, r.x + 128, r.y + 38, 17, Fade(corTexto, 0.7f));
            Vector2 direita = {r.x + r.width - 60, r.y + r.height / 2};
            if (estado == 0) {
                desenharCadeado(direita, 0.9f, Color{170, 162, 152, 255});
            } else if (estado == 1) {
                string f = "Fase " + to_string(faseLiberada);
                textoCentro(f, direita.x - 6, direita.y - 14, 22, COR_LARANJA);
            } else {
                desenharEstrela(mais(direita, -26, 0), 14, COR_OURO);
                texto(to_string(estrelasDoCapitulo(c)) + "/30", direita.x - 8, direita.y - 12, 22, COR_TEXTO);
            }
        }

        // Barra de cima
        desenharToldo(W);
        bool podeTocar = !mostrarAjuda && postalAberto < 0;
        desenharBotao({botaoInicioTopo(), "INÍCIO", COR_MARROM, ICONE_CASA, 24}, podeTocar && dentro(mouse, botaoInicioTopo()));
        desenharPilulaMoedas(pilulaMoedasTopo(W), moedas);
        desenharBotaoDesafio(W, mouse, tempo, podeTocar);

        // Aviso de viagem
        if (viajando) {
            string aviso = "Próxima parada: " + string(CAPITULOS[viagemPara].curto) + "!";
            float largura = larguraTexto(aviso, 30) + 50;
            float centro = a.x + a.width / 2;
            Rectangle faixa = {centro - largura / 2, a.y - 4, largura, 52};
            DrawRectangleRounded(faixa, 0.5f, 8, Fade(COR_LARANJA, 0.95f));
            textoCentro(aviso, centro, faixa.y + 9, 30, WHITE);
        }

        Rectangle jogar = botaoJogarBaixo(W, H);
        desenharBotao({jogar, "JOGAR FASE " + to_string(faseLiberada), COR_VERDE, ICONE_JOGAR, 34}, podeTocar && dentro(mouse, jogar));
    }

    void desenharBotaoDesafio(float W, Vector2 mouse, float tempo, bool podeTocar) {
        bool feito = desafioFeitoHoje();
        Rectangle b = botaoDesafioTopo(W);
        desenharBotao({b, feito ? "FEITO HOJE" : "DESAFIO", COR_LARANJA, ICONE_CALENDARIO, 24}, podeTocar && dentro(mouse, b), !feito);
        if (!feito) {  // bolinha chamando atencao
            float pulso = 1 + 0.15f * sinf(tempo * 6);
            DrawCircleV({b.x + b.width - 6, b.y + 4}, 13 * pulso, Color{226, 50, 50, 255});
            textoCentro("!", b.x + b.width - 6, b.y - 8, 22, WHITE);
        }
    }

    // ============================================================
    //  MAPA DE FASES DE UMA FEIRA (10 fases + o postal no fim)
    // ============================================================

    static const int PONTOS_NO_MAPA = FASES_POR_CAPITULO + 1;  // o ultimo e o postal

    int faseDoPonto(int i) const { return primeiraFaseDoCapitulo(capituloMapa, voltaMapa) + i - 1; }
    bool feiraConcluida() const { return faseDoPonto(FASES_POR_CAPITULO) < faseLiberada; }

    // Posicao do ponto i no mapa: um caminho em zigue-zague subindo
    Vector2 posicaoNoMapa(float i, float W, float H) const {
        float amplitude = min(W * 0.28f, 250.0f);
        return {W / 2 + sinf(i * 0.85f) * amplitude, H - 220 - (i - 1) * ESPACO_NO_MAPA + rolagemMapa};
    }

    void limitarRolagem(float H) {
        float maximo = (PONTOS_NO_MAPA - 1) * ESPACO_NO_MAPA - (H - 480);
        rolagemMapa = max(0.0f, min(rolagemMapa, max(0.0f, maximo)));
    }

    void irParaMapaDaFase(int f, float H) {
        if (desafio) return irParaBrasil();
        abrirFeira(capituloDaFase(f), H);
        voltaMapa = voltaDaFase(f);
    }

    void tocarNoMapa(Vector2 p, float W, float H) {
        if (dentro(p, botaoInicioTopo())) {
            sons.tocar(sons.clique);
            irParaBrasil();
            return;
        }
        if (dentro(p, botaoDesafioTopo(W))) return tocarNoDesafio();
        if (dentro(p, botaoJogarBaixo(W, H))) {
            sons.tocar(sons.clique);
            bool temFaseAqui = faseLiberada >= faseDoPonto(1) && faseLiberada <= faseDoPonto(FASES_POR_CAPITULO);
            if (temFaseAqui) comecarNivel(faseLiberada, false);
            else irParaBrasil();
            return;
        }
        for (int i = 1; i <= PONTOS_NO_MAPA; i++) {
            if (distancia(p, posicaoNoMapa((float)i, W, H)) >= 50) continue;
            if (i == PONTOS_NO_MAPA) {  // o postal
                if (feiraConcluida()) {
                    sons.tocar(sons.clique);
                    postalAberto = capituloMapa;
                    postalDaConquista = false;
                } else {
                    sons.tocar(sons.erro);
                }
            } else if (faseDoPonto(i) <= faseLiberada) {
                sons.tocar(sons.clique);
                comecarNivel(faseDoPonto(i), false);
            } else {
                sons.tocar(sons.erro);
            }
            return;
        }
    }

    void atualizarMapa(Vector2 ponteiro, bool pressionado, bool clicou, float W, float H) {
        rolagemMapa += GetMouseWheelMove() * 90;
        // Toque rapido no celular: encostou e soltou antes do quadro seguinte
        if (clicou && !pressionado && !toque.segurando) {
            tocarNoMapa(ponteiro, W, H);
            return;
        }
        if (pressionado && !toque.segurando) {
            toque = {true, false, ponteiro, rolagemMapa};
        }
        if (pressionado) {
            float arrasto = ponteiro.y - toque.inicio.y;
            if (fabsf(arrasto) > 14) toque.moveu = true;
            if (toque.moveu) rolagemMapa = toque.rolagemInicial + arrasto;
        }
        if (!pressionado && toque.segurando) {
            toque.segurando = false;
            if (!toque.moveu) tocarNoMapa(toque.inicio, W, H);
        }
        limitarRolagem(H);
    }

    void desenharMapa(float W, float H, Vector2 mouse, float tempo) {
        const Capitulo& cap = CAPITULOS[capituloMapa];
        // Campo verde
        DrawRectangleGradientV(0, 0, (int)W, (int)H, Color{168, 218, 118, 255}, Color{118, 186, 88, 255});

        // Enfeites do campo (flores e arbustos), rolam junto com o mapa
        for (int n = 0; n <= PONTOS_NO_MAPA + 2; n++) {
            Vector2 p = posicaoNoMapa((float)n, W, H);
            if (p.y < -120 || p.y > H + 120) continue;
            for (int lado : {-1, 1}) {
                float x = p.x + lado * (150 + (n * 37 % 60));
                if (x < 20 || x > W - 20) x = p.x - lado * (150 + (n * 37 % 60));
                float y = p.y + ((n * 53) % 50) - 25;
                if ((n + lado) % 3 == 0) {
                    DrawCircle((int)x, (int)y, 26, Color{86, 160, 70, 255});
                    DrawCircle((int)(x + 20), (int)(y + 6), 20, Color{96, 170, 76, 255});
                    DrawCircle((int)(x - 18), (int)(y + 8), 18, Color{80, 150, 64, 255});
                } else if ((n + lado) % 3 == 1) {
                    // Frutas da regiao espalhadas pelo campo
                    desenharFruta(cap.regionais[(n + (lado > 0)) % 2], {x, y}, 0.6f);
                } else {
                    Color cores[] = {{255, 255, 255, 255}, {255, 220, 80, 255}, {240, 120, 160, 255}};
                    Color cor = cores[(n + (lado > 0)) % 3];
                    for (int k = 0; k < 5; k++) {
                        float ang = k * 2 * PI / 5;
                        DrawCircle((int)(x + cosf(ang) * 7), (int)(y + sinf(ang) * 7), 6, cor);
                    }
                    DrawCircle((int)x, (int)y, 5, Color{240, 170, 40, 255});
                }
            }
        }

        // Caminho de terra
        for (int camada = 0; camada < 2; camada++) {
            for (float t = 1; t <= PONTOS_NO_MAPA; t += 0.06f) {
                Vector2 p = posicaoNoMapa(t, W, H);
                if (p.y < -60 || p.y > H + 60) continue;
                DrawCircleV(p, camada == 0 ? 27 : 22, camada == 0 ? Color{196, 160, 108, 255} : Color{236, 206, 150, 255});
            }
        }

        // As fases e o postal no fim
        for (int i = 1; i <= PONTOS_NO_MAPA; i++) {
            Vector2 p = posicaoNoMapa((float)i, W, H);
            if (p.y < -100 || p.y > H + 100) continue;
            if (i == PONTOS_NO_MAPA) {
                bool aberto = feiraConcluida();
                float pulo = aberto ? sinf(tempo * 4) * 4 : 0;
                DrawCircleV(mais(p, 0, 6), 50, Fade(BLACK, 0.2f));
                DrawCircleV(p, 50, WHITE);
                DrawCircleV(p, 46, aberto ? cap.toldoA : Color{168, 160, 150, 255});
                desenharEnvelope(mais(p, 0, pulo), 1.35f, COR_CREME, Color{150, 110, 70, 255});
                textoContorno(aberto ? "POSTAL" : "POSTAL", p.x, p.y + 52, 22, WHITE, Color{60, 60, 40, 255}, 2);
                continue;
            }
            int f = faseDoPonto(i);
            bool atual = f == faseLiberada, bloqueada = f > faseLiberada;
            float raio = atual ? 46 + sinf(tempo * 4) * 3 : 40;
            Color cor = bloqueada ? Color{168, 160, 150, 255} : (atual ? COR_LARANJA : COR_VERDE);
            DrawCircleV(mais(p, 0, 6), raio, Fade(BLACK, 0.2f));
            if (atual) DrawCircleV(p, raio + 7, COR_OURO);
            DrawCircleV(p, raio + 3, WHITE);
            DrawCircleV(p, raio, cor);
            DrawCircleV(mais(p, -raio * 0.3f, -raio * 0.35f), raio * 0.35f, Fade(WHITE, 0.25f));
            if (bloqueada) desenharCadeado(p, 1.3f, Color{240, 236, 230, 255});
            else textoContorno(to_string(f), p.x, p.y - 22, 38, WHITE, Fade(BLACK, 0.25f), 1.5f);
            int ganhas = estrelasDaFase(f);
            if (!bloqueada && !atual) {
                for (int k = 0; k < 3; k++) {
                    Vector2 e = {p.x + (k - 1) * 26.0f, p.y + raio + 10 - (k == 1 ? 6 : 0)};
                    desenharEstrela(e, 14, Color{150, 110, 40, 255});
                    desenharEstrela(e, 11, k < ganhas ? COR_OURO : Color{230, 222, 210, 255});
                }
            }
            if (atual) desenharFeirante({p.x, p.y - raio - 4 + sinf(tempo * 3) * 3}, 0.42f, tempo, 1);
        }

        // Faixas para os botoes fixos ficarem legiveis
        DrawRectangleGradientV(0, 50, (int)W, 190, Color{120, 180, 90, 235}, Color{120, 180, 90, 0});
        DrawRectangleGradientV(0, (int)(H - 170), (int)W, 170, Color{110, 176, 84, 0}, Color{110, 176, 84, 240});

        // Barra de cima e nome da feira
        desenharToldo(W, cap.toldoA, cap.toldoB);
        bool podeTocar = !mostrarAjuda && postalAberto < 0;
        desenharBotao({botaoInicioTopo(), "BRASIL", COR_MARROM, ICONE_MAPA, 24}, podeTocar && dentro(mouse, botaoInicioTopo()));
        desenharPilulaMoedas(pilulaMoedasTopo(W), moedas);
        desenharBotaoDesafio(W, mouse, tempo, podeTocar);
        float largura = min(W - 40, larguraTexto(cap.feira, 28) + 60);
        Rectangle faixa = {W / 2 - largura / 2, 150, largura, 50};
        DrawRectangleRounded({faixa.x + 2, faixa.y + 5, faixa.width, faixa.height}, 0.5f, 8, Fade(BLACK, 0.2f));
        DrawRectangleRounded(faixa, 0.5f, 8, cap.toldoA);
        textoCentro(cap.feira, W / 2, faixa.y + 9, 28, WHITE);

        // Botao grande embaixo
        Rectangle jogar = botaoJogarBaixo(W, H);
        bool temFaseAqui = faseLiberada >= faseDoPonto(1) && faseLiberada <= faseDoPonto(FASES_POR_CAPITULO);
        string rotulo = temFaseAqui ? "JOGAR FASE " + to_string(faseLiberada) : "MAPA DO BRASIL";
        desenharBotao({jogar, rotulo, temFaseAqui ? COR_VERDE : COR_AZUL, temFaseAqui ? ICONE_JOGAR : ICONE_MAPA, 34},
                      podeTocar && dentro(mouse, jogar));
    }

    // ============================================================
    //  POSTAL: curiosidade da feira + receita, para mandar no WhatsApp
    // ============================================================

    Rectangle cartaoPostal(float W, float H) const {
        float w = min(W - 30, 940.0f), h = W > H ? min(H - 40, 660.0f) : min(H - 80, 900.0f);
        return {W / 2 - w / 2, H / 2 - h / 2, w, h};
    }
    Rectangle botaoWhatsApp(float W, float H) const {
        Rectangle c = cartaoPostal(W, H);
        float bw = min(320.0f, (c.width - 60) / 2);
        return {W / 2 - bw - 10, c.y + c.height - 100, bw, 74};
    }
    Rectangle botaoFecharPostal(float W, float H) const {
        Rectangle b = botaoWhatsApp(W, H);
        return {W / 2 + 10, b.y, b.width, b.height};
    }

    void fecharPostal() {
        if (postalDaConquista) {
            // Depois do postal, o caminhao viaja ate a proxima feira
            viajando = true;
            viagemDe = capituloDaFase(faseLiberada - 1);
            viagemPara = capituloAtual();
            tempoViagem = 0;
            irParaBrasil();
        }
        postalAberto = -1;
        postalDaConquista = false;
    }

    void atualizarPostal(Vector2 mouse, bool clicou, float W, float H) {
        if (!clicou) return;
        if (dentro(mouse, botaoWhatsApp(W, H))) {
            sons.tocar(sons.clique);
            compartilharPostal(postalAberto);
        } else if (dentro(mouse, botaoFecharPostal(W, H))) {
            sons.tocar(sons.clique);
            fecharPostal();
        }
    }

    void desenharPostal(float W, float H, Vector2 mouse) {
        const Capitulo& cap = CAPITULOS[postalAberto];
        DrawRectangle(0, 0, (int)W, (int)H, Fade(BLACK, 0.55f));
        Rectangle c = cartaoPostal(W, H);
        DrawRectangleRounded({c.x + 6, c.y + 12, c.width, c.height}, 0.05f, 8, Fade(BLACK, 0.3f));
        DrawRectangleRounded(c, 0.05f, 8, Color{255, 248, 232, 255});
        // Bordinha listrada de carta aerea
        for (float x = c.x + 20; x + 36 <= c.x + c.width - 20; x += 36) {
            DrawRectangle((int)x, (int)(c.y + 12), 18, 8, cap.toldoA);
            DrawRectangle((int)(x + 18), (int)(c.y + 12), 18, 8, Color{60, 110, 190, 255});
        }

        // Selo com a fruta da regiao
        Rectangle selo = {c.x + c.width - 130, c.y + 36, 100, 118};
        for (float x = selo.x; x <= selo.x + selo.width; x += 10) {
            DrawCircleV({x, selo.y}, 4, Color{255, 248, 232, 255});
            DrawCircleV({x, selo.y + selo.height}, 4, Color{255, 248, 232, 255});
        }
        DrawRectangleRec(selo, WHITE);
        DrawRectangleRec({selo.x + 6, selo.y + 6, selo.width - 12, selo.height - 12}, misturar(cap.toldoA, WHITE, 0.8f));
        DrawRectangleRec({selo.x + 6, selo.y + 6, selo.width - 12, 26}, cap.toldoA);
        desenharFruta(cap.regionais[0], {selo.x + selo.width / 2, selo.y + 72}, 1.0f);
        // Carimbo do correio: circulo com ondinhas
        Color tinta = Fade(Color{110, 80, 140, 255}, 0.55f);
        Vector2 carimbo = {selo.x - 16, selo.y + 84};
        DrawRing(carimbo, 30, 33, 0, 360, 36, tinta);
        for (int k = -1; k <= 1; k++) {
            for (float x = -60; x < 30; x += 3) {
                DrawCircleV({carimbo.x + x, carimbo.y + k * 12 + sinf(x * 0.15f) * 4}, 1.3f, tinta);
            }
        }

        // Textos: diminui a letra ate caber
        float margem = 36, larguraTextoMax = c.width - 2 * margem;
        float topo = c.y + 40;
        textoContorno("Lembrança de " + string(cap.curto), c.x + margem + larguraTexto("Lembrança de " + string(cap.curto), 40) / 2,
                      topo, 40, cap.toldoA, WHITE, 2);
        texto(cap.feira, c.x + margem, topo + 50, 24, Fade(COR_TEXTO, 0.7f));
        Rectangle zapRet = botaoWhatsApp(W, H);
        float yTexto = topo + 128, limite = zapRet.y - 120;
        float t1 = W > H ? 26 : 30, t2 = W > H ? 23 : 26;
        vector<string> curiosidade, receita;
        while (true) {
            curiosidade = quebrarLinhas(cap.curiosidade, larguraTextoMax - (yTexto < selo.y + selo.height ? 110 : 0), t1);
            receita = quebrarLinhas(cap.receita, larguraTextoMax, t2);
            float altura = curiosidade.size() * t1 * 1.25f + 24 + t1 * 1.4f + receita.size() * t2 * 1.25f;
            if (yTexto + altura <= limite || t2 <= 15) break;
            t1 -= 1;
            t2 -= 1;
        }
        float y = yTexto;
        for (const string& l : curiosidade) {
            texto(l, c.x + margem, y, t1, COR_TEXTO);
            y += t1 * 1.25f;
        }
        y += 10;
        DrawLineEx({c.x + margem, y}, {c.x + c.width - margem, y}, 2, Color{226, 210, 186, 255});
        y += 14;
        texto("Receita: " + string(cap.receitaTitulo), c.x + margem, y, t1 + 2, COR_LARANJA);
        y += t1 * 1.4f;
        for (const string& l : receita) {
            texto(l, c.x + margem, y, t2, COR_TEXTO);
            y += t2 * 1.25f;
        }

        // Assinatura
        desenharFeirante({c.x + 80, zapRet.y - 14}, 0.45f, (float)GetTime(), 1);
        texto("Um abraço do Seu Zé!", c.x + 138, zapRet.y - 64, 26, Fade(COR_TEXTO, 0.8f));

        Rectangle zap = zapRet, fechar = botaoFecharPostal(W, H);
        desenharBotao({zap, "WHATSAPP", Color{37, 170, 90, 255}, SEM_ICONE, 28}, dentro(mouse, zap));
        desenharBotao({fechar, postalDaConquista ? "CONTINUAR" : "FECHAR", COR_MARROM, SEM_ICONE, 28}, dentro(mouse, fechar));
    }

    // ============================================================
    //  TELA INICIAL
    // ============================================================

    Rectangle botaoJogar, botaoAjuda, botaoSomMenu;

    void montarMenu(float W, float H, bool retrato) {
        float dy = deslocamentoVertical(H, retrato);
        if (!retrato) {
            float cx = W * 0.58f;
            botaoJogar = {cx - 200, 272 + dy, 400, 100};
            botaoAjuda = {cx - 200, 394 + dy, 250, 66};
            botaoSomMenu = {cx + 64, 394 + dy, 136, 66};
        } else {
            botaoJogar = {W / 2 - 220, 520 + dy, 440, 110};
            botaoAjuda = {W / 2 - 220, 656 + dy, 280, 72};
            botaoSomMenu = {W / 2 + 76, 656 + dy, 144, 72};
        }
    }

    void atualizarMenu(Vector2 mouse, bool clicou) {
        if (!clicou) return;
        if (dentro(mouse, botaoJogar)) {
            sons.tocar(sons.clique);
            irParaBrasil();
        } else if (dentro(mouse, botaoAjuda)) {
            sons.tocar(sons.clique);
            mostrarAjuda = true;
        } else if (dentro(mouse, botaoSomMenu)) {
            sons.ligado = !sons.ligado;
            sons.tocar(sons.clique);
        }
    }

    void desenharMenu(float W, float H, bool retrato, Vector2 mouse, float tempo) {
        float dy = deslocamentoVertical(H, retrato);
        float util = H - 2 * dy;
        float mesa = (retrato ? util - 250 : util - 150) + dy;
        desenharCeu(W, H, tempo);
        desenharBandeirinhas(W, 70, tempo);
        desenharToldo(W);

        // Seu Ze atras do balcao
        Vector2 ze = retrato ? Vector2{W * 0.3f, mesa + 10} : Vector2{W * 0.16f, mesa + 10};
        float escalaZe = retrato ? 1.3f : 1.25f;
        desenharFeirante(ze, escalaZe, tempo, 1);
        desenharPrateleira(W, mesa);
        desenharToalha(W, mesa + 26, H);

        // Frutas expostas no balcao
        int quantas = retrato ? 3 : 7;
        float inicioX = retrato ? W * 0.58f : W * 0.42f;
        float passo = retrato ? 96 : 94;
        for (int i = 0; i < quantas; i++) {
            float pulo = fabsf(sinf(tempo * 2 + i * 0.8f)) * 6;
            desenharFruta((i + 1) % TOTAL_FRUTAS, {inicioX + i * passo, mesa - 34 - pulo}, 1.25f);
        }

        Rectangle balao = retrato ? Rectangle{W * 0.3f + 100, mesa - 270, W * 0.7f - 120, 100}
                                  : Rectangle{W * 0.16f + 100, mesa - 300, 240, 112};
        desenharBalao(balao, "Bem-vindo à minha banca!", {ze.x + 58 * escalaZe, ze.y - 168 * escalaZe}, 26);

        // Logo
        float cx = retrato ? W / 2 : W * 0.58f;
        float yLogo = (retrato ? 150 : 62) + dy;
        float tamanhoLogo = retrato ? 118 : 104;
        Color contorno = {150, 50, 40, 255};
        if (retrato) {
            textoContorno("FEIRA", W / 2, yLogo, tamanhoLogo, COR_CREME, contorno, 7);
            textoContorno("SORT", W / 2, yLogo + tamanhoLogo * 0.95f, tamanhoLogo, COR_OURO, contorno, 7);
        } else {
            textoContorno("FEIRA SORT", cx, yLogo, tamanhoLogo, COR_CREME, contorno, 7);
        }
        float larguraLogo = larguraTexto(retrato ? "FEIRA" : "FEIRA SORT", tamanhoLogo);
        float yFrutasLogo = retrato ? yLogo + tamanhoLogo : yLogo + tamanhoLogo * 0.55f;
        desenharFruta(MORANGO, {cx - larguraLogo / 2 - 60, yFrutasLogo + sinf(tempo * 2) * 6}, 1.3f);
        desenharFruta(LARANJA, {cx + larguraLogo / 2 + 60, yFrutasLogo + sinf(tempo * 2 + 1) * 6}, 1.3f);
        float ySub = retrato ? yLogo + tamanhoLogo * 2.05f : yLogo + tamanhoLogo + 8;
        textoCentro("Organize as frutas da banca do Seu Zé", cx, ySub, 30, COR_TEXTO);

        montarMenu(W, H, retrato);
        bool livre = !mostrarAjuda;
        desenharBotao({botaoJogar, "JOGAR", COR_VERDE, ICONE_JOGAR, 44}, livre && dentro(mouse, botaoJogar));
        textoCentro("Fase " + to_string(faseLiberada), botaoJogar.x + botaoJogar.width / 2, botaoJogar.y - 40, 28, COR_TEXTO);
        desenharBotao({botaoAjuda, "COMO JOGAR", COR_AZUL, ICONE_AJUDA, 24}, livre && dentro(mouse, botaoAjuda));
        desenharBotao({botaoSomMenu, "", COR_MARROM, sons.ligado ? ICONE_SOM : ICONE_MUDO, 34}, livre && dentro(mouse, botaoSomMenu));
    }

    // ============================================================
    //  COMO JOGAR
    // ============================================================

    Rectangle botaoEntendi(float W, float H) const { return {W / 2 - 150, H / 2 + 226, 300, 76}; }

    void desenharAjuda(float W, float H, Vector2 mouse) {
        DrawRectangle(0, 0, (int)W, (int)H, Fade(BLACK, 0.5f));
        float pw = min(W - 40, 700.0f), ph = 660;
        Rectangle painel = {W / 2 - pw / 2, H / 2 - 330, pw, ph};
        DrawRectangleRounded({painel.x + 6, painel.y + 12, pw, ph}, 0.08f, 10, Fade(BLACK, 0.25f));
        DrawRectangleRounded(painel, 0.08f, 10, COR_CREME);
        textoContorno("COMO JOGAR", W / 2, painel.y + 20, 48, COR_LARANJA, WHITE, 3);

        struct Passo {
            int fruta;
            const char* texto;
        };
        Passo passos[] = {{MACA, "Toque num caixote: as frutas de cima sobem."},
                          {UVA, "Toque em outro caixote para colocar as frutas lá."},
                          {BANANA, "Só pode colocar em cima da mesma fruta ou num caixote vazio."},
                          {LARANJA, "Deixe cada caixote com um tipo só de fruta!"},
                          {-1, "Frutas no saquinho aparecem quando ficam em cima."}};
        float y = painel.y + 92;
        for (int i = 0; i < 5; i++) {
            Vector2 numero = {painel.x + 46, y + 34};
            DrawCircleV(numero, 21, COR_VERDE);
            textoCentro(to_string(i + 1), numero.x, numero.y - 15, 28, WHITE);
            if (passos[i].fruta >= 0) desenharFruta(passos[i].fruta, {painel.x + 110, y + 36}, 0.9f);
            else desenharSaquinho({painel.x + 110, y + 36}, 0.9f);
            vector<string> linhas = quebrarLinhas(passos[i].texto, pw - 190, 24);
            float yt = y + 36 - linhas.size() * 14.0f;
            for (const string& l : linhas) {
                texto(l, painel.x + 156, yt, 24, COR_TEXTO);
                yt += 28;
            }
            y += 82;
        }
        Rectangle b = botaoEntendi(W, H);
        desenharBotao({b, "ENTENDI!", COR_VERDE, SEM_ICONE, 32}, dentro(mouse, b));
    }
};

// ============================================================
//  UM QUADRO DO JOGO (chamado 60 vezes por segundo)
//  Fica numa funcao separada porque no navegador quem chama
//  e o proprio navegador, e nao um laco "while".
// ============================================================

Jogo* jogoAtual = nullptr;

void quadro() {
    Jogo& jogo = *jogoAtual;
    float dt = min(GetFrameTime(), 0.05f);
    float tempo = (float)GetTime();

#if defined(PLATFORM_WEB)
    // No navegador, a tela do jogo acompanha o tamanho da janela, em alta resolucao
    int larguraNav = larguraNavegador(), alturaNav = alturaNavegador();
    double densidade = densidadePixels();
    int larguraPixels = (int)(larguraNav * densidade), alturaPixels = (int)(alturaNav * densidade);
    if (larguraPixels != GetScreenWidth() || alturaPixels != GetScreenHeight()) SetWindowSize(larguraPixels, alturaPixels);
    ajustarTamanhoCanvas(larguraNav, alturaNav);
#else
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();
#endif

    // Tela virtual: 1280x720 deitada ou 720x1280 em pe, esticada para caber.
    // Escolhe a orientacao em que o jogo fica maior na tela.
    float larguraReal = (float)GetScreenWidth(), alturaReal = (float)GetScreenHeight();
    float escalaDeitada = min(larguraReal / 1280, alturaReal / 720);
    float escalaEmPe = min(larguraReal / 720, alturaReal / 1280);
    bool retrato = escalaEmPe > escalaDeitada;
    float escala = retrato ? escalaEmPe : escalaDeitada;
    float W = larguraReal / escala, H = alturaReal / escala;
    Camera2D camera = {};
    camera.zoom = escala;

    // Mouse ou toque na tela (celular)
#if defined(PLATFORM_WEB)
    static int contadorAntes = 0;
    int contador = contadorDeToques();
    bool toqueNovo = contador != contadorAntes;  // houve um toque desde o ultimo quadro
    contadorAntes = contador;
    int toques = toquesAtivos();
    Vector2 posicaoToque = {(float)(toqueX() * densidade), (float)(toqueY() * densidade)};
    Vector2 posicao = (toques > 0 || toqueNovo) ? posicaoToque : GetMousePosition();
#else
    static int toquesAntes = 0;
    int toques = GetTouchPointCount();
    bool toqueNovo = toques > 0 && toquesAntes == 0;
    toquesAntes = toques;
    Vector2 posicao = toques > 0 ? GetTouchPosition(0) : GetMousePosition();
#endif
    Vector2 mouse = GetScreenToWorld2D(posicao, camera);
    // Alguns celulares "imitam" um clique de mouse logo depois do toque:
    // esse clique falso e ignorado para nao contar o mesmo toque duas vezes
    static double ultimoToque = -10;
    if (toqueNovo || toques > 0) ultimoToque = GetTime();
    bool mouseValido = GetTime() - ultimoToque > 0.8;
    bool pressionado = (mouseValido && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) || toques > 0;
    // No celular nao existe "mouse em cima": depois que o dedo sai, nada fica destacado
    static bool usandoToque = false;
    Vector2 movimento = GetMouseDelta();
    if (toques > 0 || toqueNovo) usandoToque = true;
    else if (movimento.x != 0 || movimento.y != 0) usandoToque = false;
    Vector2 ponteiro = mouse;  // posicao real, usada no arraste do mapa
    if (usandoToque && toques == 0 && !toqueNovo) mouse = {-9999, -9999};
    bool clicou = (mouseValido && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || toqueNovo;

    jogo.sons.atualizarMusica();
    atualizarParticulas(dt);
    if (jogo.tempoFala > 0 && jogo.tempoFala < 50) jogo.tempoFala -= dt;
    if (jogo.tempoFala <= 0) jogo.humor = 0;
    if (jogo.tempoDica > 0) jogo.tempoDica -= dt;
    if (jogo.tempoTremor > 0) jogo.tempoTremor -= dt;

    Layout L = montarLayout(W, H, retrato, max(1, (int)jogo.feira.size()), jogo.sons.ligado);

    // ----- Atualizacao -----
    if (jogo.mostrarAjuda) {
        if (clicou && dentro(mouse, jogo.botaoEntendi(W, H))) {
            jogo.sons.tocar(jogo.sons.clique);
            jogo.mostrarAjuda = false;
        }
    } else if (jogo.postalAberto >= 0) {
        jogo.atualizarPostal(mouse, clicou, W, H);
    } else if (jogo.tela == MENU) {
        jogo.atualizarMenu(mouse, clicou);
    } else if (jogo.tela == BRASIL) {
        jogo.atualizarBrasil(mouse, clicou, dt, W, H);
    } else if (jogo.tela == MAPA) {
        jogo.atualizarMapa(ponteiro, pressionado, clicou, W, H);
    } else {
        jogo.atualizarJogo(dt, mouse, clicou, L);
    }

    // Recalcula o layout: o numero de caixotes pode ter mudado neste quadro
    L = montarLayout(W, H, retrato, max(1, (int)jogo.feira.size()), jogo.sons.ligado);

    // Cursor de maozinha sobre o que da para clicar
    bool sobreClicavel = false;
    if (jogo.mostrarAjuda) sobreClicavel = dentro(mouse, jogo.botaoEntendi(W, H));
    else if (jogo.tela == MENU)
        sobreClicavel = dentro(mouse, jogo.botaoJogar) || dentro(mouse, jogo.botaoAjuda) || dentro(mouse, jogo.botaoSomMenu);
    else if (jogo.postalAberto >= 0)
        sobreClicavel = dentro(mouse, jogo.botaoWhatsApp(W, H)) || dentro(mouse, jogo.botaoFecharPostal(W, H));
    else if (jogo.tela == MAPA || jogo.tela == BRASIL) {
        sobreClicavel = dentro(mouse, jogo.botaoInicioTopo()) || dentro(mouse, jogo.botaoDesafioTopo(W)) ||
                        dentro(mouse, jogo.botaoJogarBaixo(W, H));
        if (jogo.tela == BRASIL)
            for (int c = 0; c < TOTAL_CAPITULOS; c++)
                if (dentro(mouse, jogo.cartaoDaFeira(c, W, H)) && jogo.estadoDoCapitulo(c) != 0) sobreClicavel = true;
    }
    else if (jogo.ganhou)
        sobreClicavel = dentro(mouse, jogo.botaoVitoriaMapa(L)) || dentro(mouse, jogo.botaoVitoriaProxima(L));
    else {
        for (const Botao* b : {&L.desfazer, &L.dica, &L.reiniciar, &L.caixote, &L.mapa, &L.som})
            if (dentro(mouse, b->r)) sobreClicavel = true;
        for (const Rectangle& r : L.caixotes)
            if (dentro(mouse, areaDeClique(r))) sobreClicavel = true;
    }
    SetMouseCursor(sobreClicavel ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

    // ----- Desenho -----
    BeginDrawing();
    ClearBackground(COR_CREME);
    BeginMode2D(camera);
    if (jogo.tela == MENU) jogo.desenharMenu(W, H, retrato, mouse, tempo);
    else if (jogo.tela == BRASIL) jogo.desenharBrasil(W, H, mouse, tempo);
    else if (jogo.tela == MAPA) jogo.desenharMapa(W, H, mouse, tempo);
    else jogo.desenharJogo(L, mouse, tempo);
    if (jogo.postalAberto >= 0) jogo.desenharPostal(W, H, mouse);
    if (jogo.mostrarAjuda) jogo.desenharAjuda(W, H, mouse);
    EndMode2D();
    EndDrawing();
}

// ============================================================
//  MAIN
// ============================================================

int main() {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
#if defined(PLATFORM_WEB)
    InitWindow((int)(larguraNavegador() * densidadePixels()), (int)(alturaNavegador() * densidadePixels()), "Feira Sort");
    ajustarTamanhoCanvas(larguraNavegador(), alturaNavegador());
#else
    InitWindow(1280, 720, "Feira Sort");
    SetWindowMinSize(480, 480);
    SetTargetFPS(60);
#endif
    fonte = carregarFonte();

    static Jogo jogo;
    jogoAtual = &jogo;
    jogo.sons.carregar();
    jogo.carregarProgresso();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(quadro, 0, 1);  // o navegador chama quadro() a cada tela
#else
    while (!WindowShouldClose()) quadro();

    jogo.sons.descarregar();
    if (fonte.texture.id != GetFontDefault().texture.id) UnloadFont(fonte);
    CloseWindow();
#endif
    return 0;
}
