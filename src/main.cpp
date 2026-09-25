// ============================================================
//  FEIRA SORT - Versao 3
//  Jogo de puzzle para o publico 50+: organize as frutas da banca
//  do Seu Ze ate que cada caixote tenha um so tipo de fruta.
//
//  Arquivos:
//   regras.h - logica do jogo e resolvedor (DFS com backtracking)
//   arte.h   - todo o desenho (cenario, frutas, feirante, botoes)
//   som.h    - efeitos e musica gerados por codigo
//   main.cpp - telas, layout e controle do jogo (este arquivo)
// ============================================================

#include "raylib.h"
#include "regras.h"
#include "arte.h"
#include "som.h"

#include <fstream>

// ============================================================
//  PROGRESSO SALVO
// ============================================================

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>

// No navegador, o progresso fica guardado no localStorage
EM_JS(int, lerFaseNavegador, (), {
    try { var v = localStorage.getItem('feira_sort_fase'); return v ? parseInt(v) : 0; } catch (e) { return 0; }
});
EM_JS(void, gravarFaseNavegador, (int fase), {
    try { localStorage.setItem('feira_sort_fase', fase); } catch (e) {}
});
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

int carregarFase(bool& primeiraVez) {
    int fase = lerFaseNavegador();
    primeiraVez = fase == 0;
    return max(1, fase);
}

void salvarFase(int fase) { gravarFaseNavegador(fase); }

#else

string caminhoProgresso() {
    return string(GetApplicationDirectory()) + "progresso.txt";
}

int carregarFase(bool& primeiraVez) {
    ifstream arquivo(caminhoProgresso());
    int fase = 1;
    primeiraVez = !(arquivo >> fase);
    return max(1, fase);
}

void salvarFase(int fase) {
    ofstream arquivo(caminhoProgresso());
    arquivo << fase;
}

#endif

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

// ============================================================
//  LAYOUT: onde fica cada coisa. Muda se a tela esta deitada
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
    Rectangle pilulaFase, pilulaJogadas;
    Botao desfazer, dica, reiniciar, nova, inicio, som;
};

// Desloca um retangulo na vertical
void descer(Rectangle& r, float dy) { r.y += dy; }

Layout montarLayout(float W, float Htotal, bool retrato, int total, bool somLigado) {
    Layout L;
    L.W = W;
    L.H = Htotal;
    // Em telas muito altas, o jogo fica centralizado em vez de espalhado
    float H = min(Htotal, (retrato ? 1280.0f : 720.0f) * 1.08f);
    float dy = (Htotal - H) / 2;
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
        L.pilulaJogadas = {W - 200, 82, 180, 52};
        L.inicio = {{W - 392, 146, 180, 54}, "INÍCIO", marrom, ICONE_CASA, 24};
        L.som = {{W - 200, 146, 180, 54}, "SOM", marrom, iconeSom, 24};
        linhaDeCaixotes(0, total, H - 118);
        float bw = 206, bh = 64, esp = 18;
        float x0 = (W - (4 * bw + 3 * esp)) / 2, y = H - 84;
        L.desfazer = {{x0, y, bw, bh}, "DESFAZER", COR_AZUL, ICONE_DESFAZER, 26};
        L.dica = {{x0 + (bw + esp), y, bw, bh}, "DICA", COR_VERDE, ICONE_DICA, 26};
        L.reiniciar = {{x0 + 2 * (bw + esp), y, bw, bh}, "REINICIAR", marrom, ICONE_REINICIAR, 26};
        L.nova = {{x0 + 3 * (bw + esp), y, bw, bh}, "NOVA", marrom, ICONE_NOVA, 26};
    } else {
        L.feirante = {100, 222};
        L.escalaFeirante = 0.76f;
        L.balao = {186, 74, W - 206, 118};
        L.pontaBalao = {160, 146};
        L.pilulaFase = {W / 2 - 186, 212, 176, 48};
        L.pilulaJogadas = {W / 2 + 10, 212, 176, 48};
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
        L.nova = {{20, y2, bw, bh}, "NOVA", marrom, ICONE_NOVA, 24};
        L.inicio = {{20 + bw + esp, y2, bw, bh}, "INÍCIO", marrom, ICONE_CASA, 24};
        L.som = {{20 + 2 * (bw + esp), y2, bw, bh}, "SOM", marrom, iconeSom, 24};
    }

    // Aplica o deslocamento vertical em tudo
    for (Rectangle& r : L.caixotes) descer(r, dy);
    for (float& y : L.prateleiras) y += dy;
    L.feirante.y += dy;
    L.pontaBalao.y += dy;
    for (Rectangle* r : {&L.balao, &L.pilulaFase, &L.pilulaJogadas})
        descer(*r, dy);
    for (Botao* b : {&L.desfazer, &L.dica, &L.reiniciar, &L.nova, &L.inicio, &L.som})
        descer(b->r, dy);
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
//  O JOGO
// ============================================================

enum Tela { MENU, JOGO };

struct Voo {
    bool ativo = false;
    int de = 0, para = 0, quantidade = 0, fruta = 0, tamanhoOrigem = 0, tamanhoDestino = 0;
    float progresso = 0;
};

struct Jogo {
    Tela tela = MENU;
    bool primeiraVez = false;
    bool mostrarAjuda = false;
    bool emAndamento = false;

    mt19937 gerador{random_device{}()};
    int fase = 1;
    Feira inicio, feira;
    vector<Feira> historico;
    int jogadas = 0, dicasUsadas = 0;
    int selecionado = -1;
    bool ganhou = false, travou = false;
    float tempoVitoria = 0, proximoConfete = 0;

    string fala;
    float tempoFala = 0;
    int humor = 0;

    int dicaDe = -1, dicaPara = -1;
    float tempoDica = 0;
    // Plano de solucao guardado: seguindo as dicas, o jogador sempre termina
    vector<Jogada> planoDica;
    Feira estadoDoPlano;
    int caixoteTremendo = -1;
    float tempoTremor = 0;
    vector<float> tempoPulo, tempoSelo;
    Voo voo;

    Sons sons;

    void falar(const string& texto, float segundos = 3.5f, int humorNovo = 0) {
        fala = texto;
        tempoFala = segundos;
        humor = humorNovo;
    }

    void comecarFase(bool montarNova) {
        if (montarNova) inicio = criarFeira(fase, gerador);
        feira = inicio;
        historico.clear();
        jogadas = dicasUsadas = 0;
        selecionado = -1;
        ganhou = travou = false;
        tempoDica = 0;
        planoDica.clear();
        voo.ativo = false;
        tempoPulo.assign(feira.size(), 0);
        tempoSelo.assign(feira.size(), 1);
        emAndamento = true;
        particulas.clear();
    }

    void verificarTravamento() {
        vector<Jogada> caminho;
        bool estourou;
        travou = !resolver(feira, caminho, estourou) && !estourou;
        if (travou) falar("Opa, a banca travou! Toque em DESFAZER para voltar.", 5, 0);
    }

    int estrelas() const { return dicasUsadas == 0 ? 3 : (dicasUsadas <= 2 ? 2 : 1); }

    // O que o Seu Ze diz quando nao tem nada especial acontecendo
    string falaAtual() const {
        if (tempoFala > 0) return fala;
        if (travou) return "Opa, a banca travou! Toque em DESFAZER para voltar.";
        if (selecionado >= 0) return "Agora toque no caixote onde quer colocar.";
        return "Toque num caixote para pegar as frutas de cima.";
    }

    // ---------------- Cliques ----------------

    void cliqueNoCaixote(int c, const Layout& L) {
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
        (void)L;
    }

    void pedirDica() {
        selecionado = -1;
        if (travou) {
            falar("Primeiro toque em DESFAZER, freguês!");
            return;
        }
        if (planoDica.empty() || estadoDoPlano != feira) {
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
        jogadas--;
        selecionado = -1;
        travou = false;
        tempoFala = 0;
        verificarTravamento();
    }

    // ---------------- Atualizacao ----------------

    void atualizarJogo(float dt, Vector2 mouse, bool clicou, const Layout& L) {
        for (float& t : tempoPulo) t = max(0.0f, t - dt);
        for (float& t : tempoSelo) t += dt;

        if (voo.ativo) {
            voo.progresso += dt / 0.36f;
            if (voo.progresso >= 1) {
                historico.push_back(feira);
                // Se a jogada seguiu o plano da dica, o plano continua valendo
                bool seguiuPlano = !planoDica.empty() && estadoDoPlano == feira &&
                                   planoDica[0] == Jogada{voo.de, voo.para};
                mover(feira, voo.de, voo.para);
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
                    ganhou = true;
                    tempoVitoria = 0;
                    proximoConfete = 0.9f;
                    salvarFase(fase + 1);
                    sons.tocar(sons.vitoria);
                    tempoSelo[voo.para] = 0;
                    for (int i = 0; i < 5; i++) soltarConfete({L.W * (0.1f + i * 0.2f), L.H * 0.55f}, 30, 900);
                    falar(sortear({"Banca arrumadinha! Obrigado, freguês!", "Que capricho! Os fregueses vão adorar!",
                                   "Nota dez! Você é bom de feira!"}),
                          100, 1);
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
            return;
        }

        if (!clicou) return;

        if (dentro(mouse, L.som.r)) {
            sons.ligado = !sons.ligado;
            sons.tocar(sons.clique);
        } else if (dentro(mouse, L.inicio.r)) {
            sons.tocar(sons.clique);
            tela = MENU;
        } else if (dentro(mouse, L.desfazer.r)) {
            sons.tocar(sons.clique);
            desfazer();
        } else if (dentro(mouse, L.dica.r)) {
            sons.tocar(sons.clique);
            pedirDica();
        } else if (dentro(mouse, L.reiniciar.r)) {
            sons.tocar(sons.clique);
            comecarFase(false);
            falar("Recomeçamos a mesma banca. Com calma!");
        } else if (dentro(mouse, L.nova.r)) {
            sons.tocar(sons.clique);
            comecarFase(true);
            falar("Chegou fruta nova! Vamos arrumar?");
        } else {
            int clicado = -1;
            for (int i = 0; i < (int)feira.size(); i++) {
                if (dentro(mouse, areaDeClique(L.caixotes[i]))) clicado = i;
            }
            if (clicado >= 0) cliqueNoCaixote(clicado, L);
            else selecionado = -1;
        }
    }

    // ---------------- Desenho do jogo ----------------

    void desenharCenario(const Layout& L, float tempo) {
        desenharCeu(L.W, L.H, tempo);
        desenharBandeirinhas(L.W, 70, tempo);
        desenharToldo(L.W);
        for (float y : L.prateleiras) desenharPrateleira(L.W, y);
        desenharToalha(L.W, L.prateleiras.back() + 26, L.H);
    }

    void desenharJogo(const Layout& L, Vector2 mouse, float tempo) {
        desenharCenario(L, tempo);
        desenharFeirante(L.feirante, L.escalaFeirante, tempo, humor);
        desenharBalao(L.balao, falaAtual(), L.pontaBalao, L.retrato ? 25 : 26);
        desenharPilula(L.pilulaFase, "Fase", to_string(fase));
        desenharPilula(L.pilulaJogadas, "Jogadas", to_string(jogadas));

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
                contornoArredondado({r.x - 10, r.y - 10, r.width + 20, r.height + 20}, 0.12f, 8, 6,
                                          Fade(COR_VERDE, pulso));
                string etiqueta = i == dicaDe ? "PEGUE" : "COLOQUE";
                float tamanho = 20, largura = larguraTexto(etiqueta, tamanho) + 20;
                Rectangle tag = {r.x + r.width / 2 - largura / 2, r.y - 48, largura, 32};
                DrawRectangleRounded(tag, 0.5f, 8, COR_VERDE);
                textoCentro(etiqueta, r.x + r.width / 2, tag.y + 5, tamanho, WHITE);
            }

            // Frutas
            int visiveis = feira[i].size();
            if (voo.ativo && i == voo.de) visiveis -= voo.quantidade;
            int levantadas = (i == selecionado) ? frutasIguaisNoTopo(feira[i]) : 0;
            float pulo = tempoPulo[i] > 0 ? 1 + 0.14f * sinf(PI * (1 - tempoPulo[i] / 0.25f)) : 1;
            for (int k = 0; k < visiveis; k++) {
                Vector2 p = posicaoVaga(r, k);
                if (k >= visiveis - levantadas) p.y -= ALTURA_LEVANTADA + sinf(tempo * 5) * 3;
                else if (emCima && selecionado == -1 && k == visiveis - 1) p.y -= 4 + sinf(tempo * 8) * 2;
                desenharFruta(feira[i][k], p, pulo);
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
                Vector2 p = {origem.x + (destino.x - origem.x) * t,
                             origem.y + (destino.y - origem.y) * t - sinf(PI * t) * 110};
                desenharFruta(voo.fruta, p, 1.0f + 0.12f * sinf(PI * t));
            }
        }

        // Botoes
        for (const Botao* b : {&L.desfazer, &L.dica, &L.reiniciar, &L.nova, &L.inicio, &L.som}) {
            bool ativo = !(b == &L.desfazer && historico.empty());
            desenharBotao(*b, podeInteragir && dentro(mouse, b->r), ativo);
        }
        // Quando trava, o botao DESFAZER brilha
        if (travou && !voo.ativo) {
            float pulso = 0.5f + 0.5f * sinf(tempo * 6);
            Rectangle r = L.desfazer.r;
            contornoArredondado({r.x - 6, r.y - 6, r.width + 12, r.height + 14}, 0.45f, 10, 5, Fade(COR_OURO, pulso));
        }

        if (ganhou) desenharVitoria(L, mouse, tempo);
        desenharParticulas();  // confete por cima de tudo
    }

    Rectangle botaoProximaFase(const Layout& L) const {
        return {L.W / 2 - 180, L.H / 2 + 92, 360, 84};
    }

    void desenharVitoria(const Layout& L, Vector2 mouse, float tempo) {
        float aparecer = min(1.0f, tempoVitoria / 0.3f);
        DrawRectangle(0, 0, (int)L.W, (int)L.H, Fade(BLACK, 0.45f * aparecer));
        float escala = efeitoMola(tempoVitoria / 0.45f);
        float pw = 600 * escala, ph = 470 * escala;
        Rectangle painel = {L.W / 2 - pw / 2, L.H / 2 - 250 * escala, pw, ph};
        DrawRectangleRounded({painel.x + 6, painel.y + 12, painel.width, painel.height}, 0.12f, 10, Fade(BLACK, 0.25f));
        DrawRectangleRounded(painel, 0.12f, 10, COR_CREME);
        DrawRectangleRounded({painel.x + 10, painel.y + 10, painel.width - 20, painel.height - 20}, 0.1f, 10, Color{255, 252, 244, 255});
        if (tempoVitoria < 0.35f) return;

        float topo = L.H / 2 - 250;
        textoContorno("MUITO BEM!", L.W / 2, topo + 30, 66, COR_VERDE, WHITE, 4);
        textoCentro("Fase " + to_string(fase) + " concluída em " + to_string(jogadas) + " jogadas", L.W / 2, topo + 112,
                    28, COR_TEXTO);

        // Estrelas aparecendo uma de cada vez
        int ganhas = estrelas();
        for (int i = 0; i < 3; i++) {
            float t = (tempoVitoria - 0.5f - i * 0.3f) / 0.35f;
            if (t <= 0) continue;
            float s = efeitoMola(t);
            Vector2 c = {L.W / 2 + (i - 1) * 118.0f, topo + 225 - (i == 1 ? 16 : 0)};
            float raio = (i == 1 ? 56 : 46) * s;
            desenharEstrela(mais(c, 3, 5), raio, Fade(BLACK, 0.15f));
            desenharEstrela(c, raio + 4, i < ganhas ? Color{214, 150, 20, 255} : Color{190, 180, 170, 255});
            desenharEstrela(c, raio, i < ganhas ? COR_OURO : Color{222, 214, 204, 255});
        }
        string recado = ganhas == 3 ? "Sem nenhuma dica! Parabéns!" : "Na próxima, tente com menos dicas!";
        textoCentro(recado, L.W / 2, topo + 300, 24, Fade(COR_TEXTO, 0.75f));

        Botao proxima = {botaoProximaFase(L), "PRÓXIMA FASE", COR_VERDE, ICONE_JOGAR, 32};
        desenharBotao(proxima, dentro(mouse, proxima.r));
        (void)tempo;
    }

    // ---------------- Tela inicial ----------------

    Rectangle botaoJogar, botaoAjuda, botaoSomMenu;

    // Em telas muito altas, o menu fica centralizado na vertical
    static float deslocamentoMenu(float H, bool retrato) {
        float util = min(H, (retrato ? 1280.0f : 720.0f) * 1.08f);
        return (H - util) / 2;
    }

    void montarMenu(float W, float H, bool retrato) {
        float dy = deslocamentoMenu(H, retrato);
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
            if (!emAndamento || ganhou) {
                if (ganhou) fase++;
                comecarFase(true);
            }
            tela = JOGO;
            falar(sortear({"Bom dia, freguês! Vamos arrumar a banca?", "Que bom te ver! Bora organizar as frutas?",
                           "A feira hoje está bonita! Vamos começar?"}),
                  4, 1);
            if (primeiraVez) {
                mostrarAjuda = true;
                primeiraVez = false;
            }
        } else if (dentro(mouse, botaoAjuda)) {
            sons.tocar(sons.clique);
            mostrarAjuda = true;
        } else if (dentro(mouse, botaoSomMenu)) {
            sons.ligado = !sons.ligado;
            sons.tocar(sons.clique);
        }
    }

    void desenharMenu(float W, float H, bool retrato, Vector2 mouse, float tempo) {
        float dy = deslocamentoMenu(H, retrato);
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

        // Balao do Seu Ze
        Rectangle balao = retrato ? Rectangle{W * 0.3f + 100, mesa - 270, W * 0.7f - 120, 100}
                                  : Rectangle{W * 0.16f + 100, mesa - 300, 240, 112};
        desenharBalao(balao, "Bem-vindo à minha banca!", {ze.x + 58 * escalaZe, ze.y - 168 * escalaZe}, 26);

        // Logo
        float cx = retrato ? W / 2 : W * 0.58f;
        float yLogo = (retrato ? 150 : 62) + dy;
        float tamanhoLogo = retrato ? 118 : 104;
        if (retrato) {
            textoContorno("FEIRA", W / 2, yLogo, tamanhoLogo, COR_CREME, Color{150, 50, 40, 255}, 7);
            textoContorno("SORT", W / 2, yLogo + tamanhoLogo * 0.95f, tamanhoLogo, COR_OURO, Color{150, 50, 40, 255}, 7);
        } else {
            textoContorno("FEIRA SORT", cx, yLogo, tamanhoLogo, COR_CREME, Color{150, 50, 40, 255}, 7);
        }
        float larguraLogo = retrato ? larguraTexto("FEIRA", tamanhoLogo) : larguraTexto("FEIRA SORT", tamanhoLogo);
        float yFrutasLogo = retrato ? yLogo + tamanhoLogo : yLogo + tamanhoLogo * 0.55f;
        desenharFruta(MORANGO, {cx - larguraLogo / 2 - 60, yFrutasLogo + sinf(tempo * 2) * 6}, 1.3f);
        desenharFruta(LARANJA, {cx + larguraLogo / 2 + 60, yFrutasLogo + sinf(tempo * 2 + 1) * 6}, 1.3f);
        float ySub = retrato ? yLogo + tamanhoLogo * 2.05f : yLogo + tamanhoLogo + 8;
        textoCentro("Organize as frutas da banca do Seu Zé", cx, ySub, 30, COR_TEXTO);

        montarMenu(W, H, retrato);
        bool livre = !mostrarAjuda;
        string rotuloJogar = (emAndamento && !ganhou) ? "CONTINUAR" : "JOGAR";
        desenharBotao({botaoJogar, rotuloJogar, COR_VERDE, ICONE_JOGAR, 44}, livre && dentro(mouse, botaoJogar));
        int faseMostrada = (emAndamento && ganhou) ? fase + 1 : fase;
        textoCentro("Fase " + to_string(faseMostrada), botaoJogar.x + botaoJogar.width / 2,
                    botaoJogar.y - 40, 28, COR_TEXTO);
        desenharBotao({botaoAjuda, "COMO JOGAR", COR_AZUL, ICONE_AJUDA, 24}, livre && dentro(mouse, botaoAjuda));
        desenharBotao({botaoSomMenu, "", COR_MARROM, sons.ligado ? ICONE_SOM : ICONE_MUDO, 34},
                      livre && dentro(mouse, botaoSomMenu));
    }

    // ---------------- Como jogar ----------------

    Rectangle botaoEntendi(float W, float H) const {
        return {W / 2 - 150, H / 2 + 190, 300, 76};
    }

    void desenharAjuda(float W, float H, Vector2 mouse) {
        DrawRectangle(0, 0, (int)W, (int)H, Fade(BLACK, 0.5f));
        float pw = min(W - 40, 680.0f), ph = 580;
        Rectangle painel = {W / 2 - pw / 2, H / 2 - 300, pw, ph};
        DrawRectangleRounded({painel.x + 6, painel.y + 12, pw, ph}, 0.08f, 10, Fade(BLACK, 0.25f));
        DrawRectangleRounded(painel, 0.08f, 10, COR_CREME);
        textoContorno("COMO JOGAR", W / 2, painel.y + 22, 50, COR_LARANJA, WHITE, 3);

        struct Passo {
            int fruta;
            const char* texto;
        };
        Passo passos[] = {{MACA, "Toque num caixote: as frutas de cima sobem."},
                          {UVA, "Toque em outro caixote para colocar as frutas lá."},
                          {BANANA, "Só pode colocar em cima da mesma fruta ou num caixote vazio."},
                          {LARANJA, "Deixe cada caixote com um tipo só de fruta!"}};
        float y = painel.y + 100;
        for (int i = 0; i < 4; i++) {
            Vector2 numero = {painel.x + 50, y + 34};
            DrawCircleV(numero, 22, COR_VERDE);
            textoCentro(to_string(i + 1), numero.x, numero.y - 16, 30, WHITE);
            desenharFruta(passos[i].fruta, {painel.x + 116, y + 36}, 0.95f);
            vector<string> linhas = quebrarLinhas(passos[i].texto, pw - 200, 25);
            float yt = y + 36 - linhas.size() * 15.0f;
            for (const string& l : linhas) {
                texto(l, painel.x + 164, yt, 25, COR_TEXTO);
                yt += 30;
            }
            y += 88;
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
    float baseW = retrato ? 720 : 1280, baseH = retrato ? 1280 : 720;
    float escala = min(larguraReal / baseW, alturaReal / baseH);
    float W = larguraReal / escala, H = alturaReal / escala;
    Camera2D camera = {};
    camera.zoom = escala;

    // Mouse ou toque na tela (celular)
    static int toquesAntes = 0;
    int toques = GetTouchPointCount();
    bool toqueNovo = toques > 0 && toquesAntes == 0;
    toquesAntes = toques;
    Vector2 posicao = toques > 0 ? GetTouchPosition(0) : GetMousePosition();
    Vector2 mouse = GetScreenToWorld2D(posicao, camera);
    bool clicou = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || toqueNovo;

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
    } else if (jogo.tela == MENU) {
        jogo.atualizarMenu(mouse, clicou);
    } else if (jogo.ganhou && clicou && dentro(mouse, jogo.botaoProximaFase(L)) && jogo.tempoVitoria > 0.6f) {
        jogo.sons.tocar(jogo.sons.clique);
        jogo.fase++;
        jogo.comecarFase(true);
        jogo.falar("Fase " + to_string(jogo.fase) + "! Chegou mais fruta na banca.", 4, 1);
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
    else if (jogo.ganhou) sobreClicavel = dentro(mouse, jogo.botaoProximaFase(L));
    else {
        for (const Botao* b : {&L.desfazer, &L.dica, &L.reiniciar, &L.nova, &L.inicio, &L.som})
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
    else jogo.desenharJogo(L, mouse, tempo);
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
    jogo.fase = carregarFase(jogo.primeiraVez);

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
