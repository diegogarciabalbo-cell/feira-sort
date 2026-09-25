// ============================================================
//  arte.h - Tudo que desenha: cenario, frutas, caixotes,
//  feirante, botoes, estrelas e particulas.
//  Nenhuma imagem e usada: tudo e feito com formas geometricas.
// ============================================================
#pragma once

#include "raylib.h"
#include "regras.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace std;

// ---------------- Medidas (em pixels da tela virtual) ----------------
const float CAIXOTE_LARGURA = 112;
const float VAGA_ALTURA = 80;
const float CAIXOTE_ALTURA = 4 * VAGA_ALTURA + 28;
const float RAIO_FRUTA = 33;
const float ALTURA_LEVANTADA = 58;

// ---------------- Paleta ----------------
const Color COR_TEXTO = {84, 52, 28, 255};
const Color COR_VERDE = {52, 158, 82, 255};
const Color COR_AZUL = {62, 118, 196, 255};
const Color COR_MARROM = {170, 108, 56, 255};
const Color COR_LARANJA = {232, 120, 30, 255};
const Color COR_OURO = {255, 196, 40, 255};
const Color COR_FOLHA = {76, 160, 58, 255};
const Color COR_FOLHA_ESCURA = {44, 112, 38, 255};
const Color COR_CABO = {110, 70, 36, 255};
const Color COR_CREME = {255, 248, 234, 255};

Font fonte;  // carregada no main

// ============================================================
//  Utilidades
// ============================================================

Vector2 V(float x, float y) { return {x, y}; }
Vector2 mais(Vector2 a, float dx, float dy) { return {a.x + dx, a.y + dy}; }

Color misturar(Color a, Color b, float t) {
    return {(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
            (unsigned char)(a.b + (b.b - a.b) * t), (unsigned char)(a.a + (b.a - a.a) * t)};
}
Color clarear(Color c, float t) { return misturar(c, Color{255, 255, 255, c.a}, t); }
Color escurecer(Color c, float t) { return misturar(c, Color{0, 0, 0, c.a}, t); }

// Triangulo desenhado nos dois sentidos (a raylib esconde um dos lados)
void triangulo(Vector2 a, Vector2 b, Vector2 c, Color cor) {
    DrawTriangle(a, b, c, cor);
    DrawTriangle(a, c, b, cor);
}

// Folha em forma de "olho", saindo de "base" na direcao do angulo (graus)
void desenharFolha(Vector2 base, float comprimento, float largura, float anguloGraus, Color cor) {
    float a = anguloGraus * DEG2RAD;
    Vector2 dir = {cosf(a), sinf(a)};
    Vector2 lado = {-dir.y, dir.x};
    const int PASSOS = 10;
    Vector2 esq[PASSOS + 1], dirt[PASSOS + 1];
    for (int i = 0; i <= PASSOS; i++) {
        float t = (float)i / PASSOS;
        float meia = largura * sinf(PI * t);
        Vector2 eixo = {base.x + dir.x * comprimento * t, base.y + dir.y * comprimento * t};
        esq[i] = {eixo.x + lado.x * meia, eixo.y + lado.y * meia};
        dirt[i] = {eixo.x - lado.x * meia, eixo.y - lado.y * meia};
    }
    for (int i = 0; i < PASSOS; i++) {
        triangulo(esq[i], esq[i + 1], dirt[i + 1], cor);
        triangulo(esq[i], dirt[i + 1], dirt[i], cor);
    }
    Vector2 ponta = {base.x + dir.x * comprimento, base.y + dir.y * comprimento};
    DrawLineEx(base, mais(ponta, -dir.x * comprimento * 0.2f, -dir.y * comprimento * 0.2f),
               max(1.5f, largura * 0.18f), escurecer(cor, 0.25f));
}

// Meio disco virado para baixo (usado na melancia)
void meioDisco(Vector2 centro, float raio, Color cor) {
    const int PASSOS = 24;
    for (int i = 0; i < PASSOS; i++) {
        float a1 = PI * i / PASSOS, a2 = PI * (i + 1) / PASSOS;
        triangulo(centro, mais(centro, cosf(a1) * raio, sinf(a1) * raio),
                  mais(centro, cosf(a2) * raio, sinf(a2) * raio), cor);
    }
}

void desenharEstrela(Vector2 c, float raio, Color cor) {
    Vector2 pontos[10];
    for (int i = 0; i < 10; i++) {
        float a = -PI / 2 + i * PI / 5;
        float r = (i % 2 == 0) ? raio : raio * 0.45f;
        pontos[i] = {c.x + cosf(a) * r, c.y + sinf(a) * r};
    }
    for (int i = 0; i < 10; i++) triangulo(c, pontos[i], pontos[(i + 1) % 10], cor);
}

// ---------------- Texto ----------------

float larguraTexto(const string& texto, float tamanho) {
    return MeasureTextEx(fonte, texto.c_str(), tamanho, 1).x;
}

void texto(const string& t, float x, float y, float tamanho, Color cor) {
    DrawTextEx(fonte, t.c_str(), {x, y}, tamanho, 1, cor);
}

void textoCentro(const string& t, float centroX, float y, float tamanho, Color cor) {
    texto(t, centroX - larguraTexto(t, tamanho) / 2, y, tamanho, cor);
}

// Texto com contorno grosso (para o logo)
void textoContorno(const string& t, float centroX, float y, float tamanho, Color cor, Color contorno,
                   float espessura) {
    float x = centroX - larguraTexto(t, tamanho) / 2;
    for (int i = 0; i < 16; i++) {
        float a = i * PI / 8;
        texto(t, x + cosf(a) * espessura, y + sinf(a) * espessura, tamanho, contorno);
    }
    texto(t, x, y + espessura * 0.9f, tamanho, contorno);
    texto(t, x, y, tamanho, cor);
}

// Quebra um texto em linhas que caibam na largura
vector<string> quebrarLinhas(const string& t, float larguraMax, float tamanho) {
    vector<string> linhas;
    string linha, palavra;
    for (size_t i = 0; i <= t.size(); i++) {
        if (i == t.size() || t[i] == ' ') {
            string teste = linha.empty() ? palavra : linha + " " + palavra;
            if (larguraTexto(teste, tamanho) > larguraMax && !linha.empty()) {
                linhas.push_back(linha);
                linha = palavra;
            } else {
                linha = teste;
            }
            palavra.clear();
        } else {
            palavra += t[i];
        }
    }
    if (!linha.empty()) linhas.push_back(linha);
    return linhas;
}

// ============================================================
//  FRUTAS
// ============================================================

void brilho(Vector2 c, float r) {
    DrawEllipse((int)(c.x - r * 0.36f), (int)(c.y - r * 0.4f), r * 0.17f, r * 0.1f, Fade(WHITE, 0.85f));
}

// Uma banana: varios circulos ao longo de um arco
void umaBanana(Vector2 centroArco, float raioArco, float grossura, Color base) {
    Color escura = escurecer(base, 0.3f), clara = clarear(base, 0.45f);
    for (int camada = 0; camada < 3; camada++) {
        for (int a = 22; a <= 158; a += 3) {
            float ang = a * DEG2RAD;
            float meio = sinf(PI * (a - 22) / 136.0f);
            float g = grossura * (0.3f + 0.7f * meio);
            Vector2 p = mais(centroArco, cosf(ang) * raioArco, sinf(ang) * raioArco);
            if (camada == 0) DrawCircleV(p, g + 2.5f, escura);
            else if (camada == 1) DrawCircleV(p, g, base);
            else DrawCircleV(mais(p, -cosf(ang) * g * 0.35f, -sinf(ang) * g * 0.35f), g * 0.4f, Fade(clara, 0.8f));
        }
    }
    for (int a : {22, 158}) {
        float ang = a * DEG2RAD;
        DrawCircleV(mais(centroArco, cosf(ang) * raioArco, sinf(ang) * raioArco), grossura * 0.28f, COR_CABO);
    }
}

void desenharFruta(int fruta, Vector2 c, float escala = 1.0f) {
    float r = RAIO_FRUTA * escala;

    // Sombra no chao do caixote
    DrawEllipse((int)c.x, (int)(c.y + r * 0.95f), r * 0.78f, r * 0.14f, Fade(BLACK, 0.22f));

    switch (fruta) {
        case MACA: {
            Color base = {218, 36, 48, 255}, escura = {130, 14, 26, 255}, clara = {255, 130, 120, 255};
            Vector2 partes[3] = {mais(c, -r * 0.36f, -r * 0.1f), mais(c, r * 0.36f, -r * 0.1f), mais(c, 0, r * 0.16f)};
            float raios[3] = {r * 0.66f, r * 0.66f, r * 0.8f};
            for (int i = 0; i < 3; i++) DrawCircleV(partes[i], raios[i] + 2.5f, escura);
            for (int i = 0; i < 3; i++) DrawCircleV(partes[i], raios[i], base);
            DrawCircleGradient((int)(c.x - r * 0.25f), (int)(c.y - r * 0.18f), r * 0.62f, Fade(clara, 0.9f), Fade(base, 0));
            DrawEllipse((int)c.x, (int)(c.y - r * 0.62f), r * 0.2f, r * 0.08f, Fade(escura, 0.7f));
            DrawLineEx(mais(c, 0, -r * 0.6f), mais(c, r * 0.1f, -r * 1.08f), max(3.0f, r * 0.12f), COR_CABO);
            desenharFolha(mais(c, r * 0.08f, -r * 0.92f), r * 0.6f, r * 0.2f, -25, COR_FOLHA);
            brilho(c, r);
            break;
        }
        case BANANA: {
            Color amarela = {250, 208, 50, 255};
            DrawLineEx(mais(c, -r * 0.95f, -r * 0.55f), mais(c, -r * 1.12f, -r * 0.85f), r * 0.16f, Color{120, 110, 40, 255});
            umaBanana(mais(c, r * 0.06f, -r * 1.12f), r * 1.02f, r * 0.26f, escurecer(amarela, 0.08f));
            umaBanana(mais(c, 0, -r * 0.78f), r * 1.08f, r * 0.3f, amarela);
            break;
        }
        case UVA: {
            Color base = {118, 48, 168, 255}, escura = {70, 22, 104, 255}, clara = {196, 140, 236, 255};
            DrawLineEx(mais(c, 0, -r * 0.7f), mais(c, r * 0.06f, -r * 1.12f), max(3.0f, r * 0.11f), COR_CABO);
            desenharFolha(mais(c, r * 0.05f, -r * 0.95f), r * 0.72f, r * 0.26f, -150, COR_FOLHA);
            Vector2 bagos[] = {{-0.52f, -0.42f}, {0, -0.48f}, {0.52f, -0.42f}, {-0.27f, 0.0f},
                               {0.27f, 0.0f},    {0, 0.42f}};
            float rb = r * 0.34f;
            for (Vector2 b : bagos) DrawCircleV(mais(c, b.x * r, b.y * r), rb + 2.2f, escura);
            for (Vector2 b : bagos) {
                Vector2 p = mais(c, b.x * r, b.y * r);
                DrawCircleV(p, rb, base);
                DrawCircleGradient((int)(p.x - rb * 0.3f), (int)(p.y - rb * 0.3f), rb * 0.75f, Fade(clara, 0.9f), Fade(base, 0));
                DrawCircleV(mais(p, -rb * 0.38f, -rb * 0.38f), rb * 0.17f, Fade(WHITE, 0.85f));
            }
            break;
        }
        case LIMAO: {
            Color base = {104, 188, 54, 255}, escura = {48, 118, 28, 255}, clara = {196, 236, 130, 255};
            DrawEllipse((int)c.x, (int)c.y, r * 1.02f + 2.5f, r * 0.78f + 2.5f, escura);
            DrawCircleV(mais(c, -r * 1.0f, 0), r * 0.17f + 2, escura);
            DrawCircleV(mais(c, r * 1.0f, 0), r * 0.17f + 2, escura);
            DrawEllipse((int)c.x, (int)c.y, r * 1.02f, r * 0.78f, base);
            DrawCircleV(mais(c, -r * 1.0f, 0), r * 0.17f, base);
            DrawCircleV(mais(c, r * 1.0f, 0), r * 0.17f, base);
            DrawEllipse((int)(c.x - r * 0.18f), (int)(c.y - r * 0.16f), r * 0.72f, r * 0.48f, Fade(clara, 0.45f));
            DrawEllipse((int)(c.x - r * 0.28f), (int)(c.y - r * 0.26f), r * 0.4f, r * 0.24f, Fade(clara, 0.5f));
            DrawEllipse((int)(c.x - r * 0.38f), (int)(c.y - r * 0.36f), r * 0.2f, r * 0.08f, Fade(WHITE, 0.85f));
            break;
        }
        case LARANJA: {
            Color base = {246, 142, 22, 255}, escura = {176, 84, 8, 255}, clara = {255, 206, 110, 255};
            DrawCircleV(c, r + 2.5f, escura);
            DrawCircleV(c, r, base);
            DrawCircleGradient((int)(c.x - r * 0.25f), (int)(c.y - r * 0.25f), r * 0.75f, Fade(clara, 0.9f), Fade(base, 0));
            Vector2 poros[] = {{0.4f, 0.25f}, {-0.3f, 0.45f}, {0.55f, -0.15f}, {-0.6f, 0.1f}, {0.1f, 0.62f}, {0.3f, -0.45f}};
            for (Vector2 p : poros) DrawCircleV(mais(c, p.x * r, p.y * r), r * 0.05f, Fade(escura, 0.5f));
            for (int i = 0; i < 5; i++) {
                float a = -PI / 2 + i * 2 * PI / 5;
                DrawCircleV(mais(c, cosf(a) * r * 0.1f, -r * 0.86f + sinf(a) * r * 0.1f), r * 0.07f, COR_FOLHA_ESCURA);
            }
            desenharFolha(mais(c, r * 0.05f, -r * 0.9f), r * 0.62f, r * 0.2f, -20, COR_FOLHA);
            brilho(c, r);
            break;
        }
        case MORANGO: {
            Color base = {232, 40, 72, 255}, escura = {140, 16, 40, 255}, clara = {255, 140, 150, 255};
            auto forma = [&](float extra, Color cor) {
                float k = 1 + extra / r;
                DrawCircleV(mais(c, -r * 0.42f, -r * 0.22f), r * 0.54f * k, cor);
                DrawCircleV(mais(c, r * 0.42f, -r * 0.22f), r * 0.54f * k, cor);
                triangulo(mais(c, -r * 0.94f * k, -r * 0.12f), mais(c, 0, r * 0.98f * k),
                          mais(c, r * 0.94f * k, -r * 0.12f), cor);
            };
            forma(2.5f, escura);
            forma(0, base);
            DrawCircleGradient((int)(c.x - r * 0.25f), (int)(c.y - r * 0.2f), r * 0.6f, Fade(clara, 0.8f), Fade(base, 0));
            Vector2 sementes[] = {{-0.5f, -0.2f}, {0, -0.28f}, {0.5f, -0.2f}, {-0.28f, 0.14f},
                                  {0.28f, 0.14f}, {0, 0.46f},  {-0.6f, 0.1f}, {0.6f, 0.1f}};
            for (Vector2 s : sementes) DrawEllipse((int)(c.x + s.x * r), (int)(c.y + s.y * r), r * 0.05f, r * 0.08f, Color{255, 226, 120, 255});
            for (float a : {-170.0f, -130.0f, -90.0f, -50.0f, -10.0f}) {
                desenharFolha(mais(c, 0, -r * 0.62f), r * 0.45f, r * 0.14f, a, COR_FOLHA);
            }
            DrawLineEx(mais(c, 0, -r * 0.65f), mais(c, r * 0.05f, -r * 1.0f), max(3.0f, r * 0.1f), COR_FOLHA_ESCURA);
            brilho(mais(c, 0, r * 0.05f), r);
            break;
        }
        case MELANCIA: {
            Vector2 centro = mais(c, 0, -r * 0.4f);
            float R = r * 1.12f;
            DrawRectangleRounded({centro.x - R - 2.5f, centro.y - 3, (R + 2.5f) * 2, 6}, 1, 4, Color{30, 90, 40, 255});
            meioDisco(centro, R + 2.5f, Color{30, 90, 40, 255});
            meioDisco(centro, R, Color{56, 150, 60, 255});
            meioDisco(centro, R * 0.9f, Color{190, 232, 150, 255});
            meioDisco(centro, R * 0.83f, Color{238, 58, 72, 255});
            meioDisco(mais(centro, -R * 0.12f, 0), R * 0.45f, Fade(Color{255, 130, 130, 255}, 0.35f));
            Vector2 sementes[] = {{-0.45f, 0.2f}, {0, 0.32f}, {0.45f, 0.2f}, {-0.22f, 0.52f}, {0.22f, 0.52f}};
            for (Vector2 s : sementes) DrawEllipse((int)(centro.x + s.x * R), (int)(centro.y + s.y * R), R * 0.05f, R * 0.08f, Color{40, 20, 20, 255});
            break;
        }
        case GOIABA: {
            // Goiaba cortada ao meio: casca verde e polpa rosada com sementes
            DrawCircleV(c, r + 2.5f, Color{96, 128, 34, 255});
            DrawCircleV(c, r, Color{176, 204, 74, 255});
            DrawCircleV(c, r * 0.84f, Color{244, 118, 118, 255});
            DrawCircleGradient((int)(c.x - r * 0.2f), (int)(c.y - r * 0.2f), r * 0.7f, Fade(Color{255, 176, 164, 255}, 0.9f), Fade(Color{244, 118, 118, 255}, 0));
            DrawCircleV(c, r * 0.42f, Color{250, 150, 140, 255});
            for (int i = 0; i < 9; i++) {
                float a = i * 2 * PI / 9;
                DrawCircleV(mais(c, cosf(a) * r * 0.55f, sinf(a) * r * 0.55f), r * 0.07f, Color{255, 232, 196, 255});
            }
            DrawEllipse((int)(c.x - r * 0.4f), (int)(c.y - r * 0.55f), r * 0.16f, r * 0.08f, Fade(WHITE, 0.6f));
            break;
        }
        case JABUTICABA: {
            // Tres jabuticabas pretinhas presas num galhinho
            DrawLineEx(mais(c, -r * 1.05f, -r * 0.35f), mais(c, r * 1.05f, -r * 0.55f), r * 0.22f, Color{120, 84, 52, 255});
            DrawLineEx(mais(c, -r * 1.05f, -r * 0.35f), mais(c, r * 1.05f, -r * 0.55f), r * 0.1f, Color{150, 108, 70, 255});
            Vector2 bagos[] = {{-0.44f, 0.12f}, {0.4f, 0.02f}, {-0.02f, 0.5f}};
            float raios[] = {0.44f, 0.46f, 0.42f};
            for (int i = 0; i < 3; i++) DrawCircleV(mais(c, bagos[i].x * r, bagos[i].y * r), raios[i] * r + 2.2f, Color{12, 6, 20, 255});
            for (int i = 0; i < 3; i++) {
                Vector2 p = mais(c, bagos[i].x * r, bagos[i].y * r);
                float rb = raios[i] * r;
                DrawCircleV(p, rb, Color{44, 22, 58, 255});
                DrawCircleGradient((int)(p.x - rb * 0.3f), (int)(p.y - rb * 0.3f), rb * 0.7f, Fade(Color{130, 96, 170, 255}, 0.8f), Fade(Color{44, 22, 58, 255}, 0));
                DrawCircleV(mais(p, -rb * 0.38f, -rb * 0.38f), rb * 0.16f, Fade(WHITE, 0.85f));
            }
            break;
        }
        case CAJU: {
            // Caju: a "fruta" amarela e vermelha com a castanha em cima
            Color vermelho = {226, 58, 44, 255}, amarelo = {252, 204, 44, 255}, escura = {150, 30, 20, 255};
            DrawCircleV(mais(c, 0, r * 0.28f), r * 0.72f + 2.5f, escura);
            DrawCircleV(mais(c, 0, -r * 0.2f), r * 0.56f + 2.5f, escura);
            DrawCircleV(mais(c, 0, r * 0.28f), r * 0.72f, vermelho);
            DrawCircleGradient((int)c.x, (int)(c.y - r * 0.2f), r * 0.56f, amarelo, vermelho);
            DrawEllipse((int)(c.x - r * 0.25f), (int)(c.y - r * 0.3f), r * 0.14f, r * 0.08f, Fade(WHITE, 0.7f));
            // Castanha
            Color castanha = {150, 132, 100, 255};
            DrawEllipse((int)(c.x + r * 0.12f), (int)(c.y - r * 0.86f), r * 0.36f + 2, r * 0.2f + 2, Color{96, 82, 58, 255});
            DrawEllipse((int)(c.x + r * 0.12f), (int)(c.y - r * 0.86f), r * 0.36f, r * 0.2f, castanha);
            DrawEllipse((int)(c.x + r * 0.2f), (int)(c.y - r * 0.8f), r * 0.14f, r * 0.08f, Color{120, 104, 76, 255});
            break;
        }
        case MANGA: {
            // Manga inclinada: verde, amarela e vermelha
            Color escura = {70, 110, 30, 255};
            const int N = 7;
            auto ponto = [&](int i) { float t = (float)i / (N - 1); return mais(c, (-0.5f + t) * r * 1.1f, (0.32f - t * 0.64f) * r); };
            auto raio = [&](int i) { float t = (float)i / (N - 1); return r * (0.5f + 0.2f * sinf(PI * t)); };
            for (int i = 0; i < N; i++) DrawCircleV(ponto(i), raio(i) + 2.5f, escura);
            for (int i = 0; i < N; i++) DrawCircleV(ponto(i), raio(i), Color{124, 180, 64, 255});
            for (int i = 3; i < N; i++) DrawCircleV(mais(ponto(i), r * 0.05f, -r * 0.05f), raio(i) * 0.85f, Fade(Color{246, 126, 44, 255}, 0.75f));
            DrawCircleGradient((int)(c.x + r * 0.15f), (int)(c.y - r * 0.1f), r * 0.6f, Fade(Color{252, 206, 64, 255}, 0.8f), Fade(Color{252, 206, 64, 255}, 0));
            DrawLineEx(mais(ponto(N - 1), r * 0.3f, -r * 0.3f), mais(ponto(N - 1), r * 0.5f, -r * 0.55f), max(3.0f, r * 0.1f), COR_CABO);
            desenharFolha(mais(ponto(N - 1), r * 0.42f, -r * 0.45f), r * 0.55f, r * 0.18f, -150, COR_FOLHA);
            DrawEllipse((int)(c.x - r * 0.3f), (int)(c.y - r * 0.05f), r * 0.16f, r * 0.08f, Fade(WHITE, 0.6f));
            break;
        }
        case ACAI: {
            // Acai servido na cuia, do jeito paraense
            Vector2 borda = mais(c, 0, -r * 0.18f);
            meioDisco(borda, r * 1.05f + 2.5f, Color{70, 38, 16, 255});
            meioDisco(borda, r * 1.05f, Color{146, 90, 44, 255});
            meioDisco(borda, r * 0.8f, Color{170, 110, 58, 255});
            DrawLineEx(mais(borda, -r * 0.9f, r * 0.35f), mais(borda, r * 0.9f, r * 0.35f), 2.5f, Color{110, 64, 28, 255});
            DrawEllipse((int)borda.x, (int)borda.y, r * 1.05f + 2, r * 0.3f + 2, Color{70, 38, 16, 255});
            DrawEllipse((int)borda.x, (int)borda.y, r * 1.02f, r * 0.28f, Color{76, 16, 64, 255});
            DrawEllipse((int)(borda.x - r * 0.2f), (int)(borda.y - r * 0.05f), r * 0.6f, r * 0.14f, Color{112, 36, 96, 255});
            Vector2 bagos[] = {{-0.3f, -0.28f}, {0.05f, -0.34f}, {0.35f, -0.26f}, {-0.1f, -0.18f}, {0.2f, -0.14f}};
            for (Vector2 b : bagos) {
                Vector2 p = mais(borda, b.x * r, b.y * r);
                DrawCircleV(p, r * 0.15f, Color{50, 10, 44, 255});
                DrawCircleV(mais(p, -r * 0.04f, -r * 0.04f), r * 0.05f, Fade(WHITE, 0.6f));
            }
            desenharFolha(mais(borda, r * 0.45f, -r * 0.3f), r * 0.55f, r * 0.16f, -60, COR_FOLHA);
            break;
        }
        case CUPUACU: {
            // Cupuacu: fruta grande, marrom e aveludada
            Color base = {150, 96, 52, 255}, escura = {82, 50, 22, 255};
            DrawEllipse((int)c.x, (int)c.y, r * 1.05f + 2.5f, r * 0.8f + 2.5f, escura);
            DrawEllipse((int)c.x, (int)c.y, r * 1.05f, r * 0.8f, base);
            DrawEllipse((int)(c.x - r * 0.18f), (int)(c.y - r * 0.18f), r * 0.7f, r * 0.45f, Fade(Color{192, 138, 86, 255}, 0.6f));
            Vector2 pontos[] = {{-0.6f, 0.2f}, {-0.3f, 0.45f}, {0.1f, 0.5f}, {0.5f, 0.3f}, {0.7f, -0.1f}, {0.35f, -0.4f},
                                {-0.05f, 0.1f}, {0.3f, 0.05f}, {-0.45f, -0.15f}};
            for (Vector2 p : pontos) DrawCircleV(mais(c, p.x * r, p.y * r), r * 0.05f, Fade(escura, 0.55f));
            DrawCircleV(mais(c, -r * 1.02f, 0), r * 0.12f, Color{100, 70, 36, 255});
            DrawEllipse((int)(c.x - r * 0.4f), (int)(c.y - r * 0.38f), r * 0.18f, r * 0.08f, Fade(WHITE, 0.45f));
            break;
        }
        case BERGAMOTA: {
            // Bergamota (mexerica): laranja achatada, com gomos marcados e folha escura
            Color base = {244, 108, 20, 255}, escura = {160, 56, 8, 255};
            DrawEllipse((int)c.x, (int)(c.y + r * 0.05f), r * 1.02f + 2.5f, r * 0.84f + 2.5f, escura);
            DrawEllipse((int)c.x, (int)(c.y + r * 0.05f), r * 1.02f, r * 0.84f, base);
            DrawCircleGradient((int)(c.x - r * 0.25f), (int)(c.y - r * 0.15f), r * 0.7f, Fade(Color{255, 176, 80, 255}, 0.85f), Fade(base, 0));
            for (float dx : {-0.5f, 0.0f, 0.5f}) {
                for (float t = -0.6f; t <= 0.6f; t += 0.08f) {
                    float curva = dx * (1 - t * t * 0.9f);
                    DrawCircleV(mais(c, curva * r, t * r * 0.8f + r * 0.05f), 1.4f, Fade(escura, 0.45f));
                }
            }
            DrawCircleV(mais(c, 0, -r * 0.76f), r * 0.1f, Color{40, 90, 30, 255});
            desenharFolha(mais(c, r * 0.02f, -r * 0.78f), r * 0.75f, r * 0.24f, -35, Color{44, 110, 40, 255});
            DrawEllipse((int)(c.x - r * 0.42f), (int)(c.y - r * 0.3f), r * 0.16f, r * 0.08f, Fade(WHITE, 0.7f));
            break;
        }
        case PESSEGO: {
            // Pessego: cor de pessego com bochecha rosada e o "risquinho"
            Color base = {252, 184, 110, 255}, escura = {196, 110, 64, 255};
            DrawCircleV(c, r + 2.5f, escura);
            DrawCircleV(c, r, base);
            DrawCircleGradient((int)(c.x + r * 0.3f), (int)(c.y + r * 0.1f), r * 0.85f, Fade(Color{232, 76, 70, 255}, 0.8f), Fade(Color{232, 76, 70, 255}, 0));
            for (int a = -80; a <= 60; a += 6) {
                float ang = a * DEG2RAD;
                DrawCircleV(mais(c, -r * 0.55f + cosf(ang) * r * 0.55f, sinf(ang) * r * 0.85f), 1.6f, Fade(escura, 0.6f));
            }
            DrawLineEx(mais(c, 0, -r * 0.85f), mais(c, r * 0.05f, -r * 1.1f), max(3.0f, r * 0.1f), COR_CABO);
            desenharFolha(mais(c, r * 0.05f, -r * 0.98f), r * 0.6f, r * 0.2f, -20, COR_FOLHA);
            DrawCircleV(mais(c, -r * 0.4f, -r * 0.35f), r * 0.18f, Fade(WHITE, 0.4f));
            break;
        }
    }
}

// Fruta escondida: saquinho de papel com "?" (estilo Magic Sort)
void desenharSaquinho(Vector2 c, float escala = 1.0f) {
    float r = RAIO_FRUTA * escala;
    DrawEllipse((int)c.x, (int)(c.y + r * 0.95f), r * 0.78f, r * 0.14f, Fade(BLACK, 0.22f));
    Rectangle saco = {c.x - r * 0.78f, c.y - r * 0.78f, r * 1.56f, r * 1.7f};
    DrawRectangleRounded({saco.x - 2.5f, saco.y - 2.5f, saco.width + 5, saco.height + 5}, 0.3f, 8, Color{140, 98, 52, 255});
    DrawRectangleRounded(saco, 0.3f, 8, Color{224, 186, 128, 255});
    // Dobra de cima em zigue-zague
    DrawRectangle((int)saco.x, (int)saco.y + 2, (int)saco.width, (int)(r * 0.3f), Color{198, 156, 96, 255});
    int dentes = 5;
    float largura = saco.width / dentes;
    for (int i = 0; i < dentes; i++) {
        float x = saco.x + i * largura;
        triangulo({x, saco.y + r * 0.3f + 2}, {x + largura, saco.y + r * 0.3f + 2}, {x + largura / 2, saco.y + r * 0.46f + 2},
                  Color{198, 156, 96, 255});
    }
    DrawRectangle((int)(saco.x + r * 0.2f), (int)(saco.y + r * 0.6f), (int)(r * 0.14f), (int)(r * 0.85f), Fade(WHITE, 0.25f));
    textoCentro("?", c.x + 1, c.y - r * 0.42f + 2, r * 1.15f, Fade(Color{90, 55, 25, 255}, 0.35f));
    textoCentro("?", c.x, c.y - r * 0.42f, r * 1.15f, Color{120, 76, 36, 255});
}

void desenharMoeda(Vector2 c, float r) {
    DrawCircleV(mais(c, 1, 2), r, Fade(BLACK, 0.2f));
    DrawCircleV(c, r, Color{200, 136, 18, 255});
    DrawCircleV(c, r * 0.84f, Color{255, 204, 52, 255});
    DrawCircleV(c, r * 0.64f, Color{242, 172, 30, 255});
    desenharEstrela(c, r * 0.46f, Color{255, 226, 110, 255});
    DrawCircleV(mais(c, -r * 0.4f, -r * 0.4f), r * 0.16f, Fade(WHITE, 0.7f));
}

void desenharCadeado(Vector2 c, float s, Color cor) {
    for (int a = 180; a <= 360; a += 10) {
        float ang = a * DEG2RAD;
        DrawCircleV({c.x + cosf(ang) * 8 * s, c.y - 5 * s + sinf(ang) * 9 * s}, 2.6f * s, cor);
    }
    DrawRectangleRounded({c.x - 12 * s, c.y - 5 * s, 24 * s, 19 * s}, 0.3f, 6, cor);
    DrawCircleV(mais(c, 0, 3 * s), 2.8f * s, Fade(BLACK, 0.35f));
    DrawRectangle((int)(c.x - 1.2f * s), (int)(c.y + 3 * s), (int)(2.4f * s), (int)(6 * s), Fade(BLACK, 0.35f));
}

// ============================================================
//  CAIXOTE DE MADEIRA
// ============================================================

// destaque: 0 = normal, 1 = mouse em cima, 2 = escolhido
void desenharCaixote(Rectangle r, int destaque, bool pronto) {
    DrawRectangleRounded({r.x + 7, r.y + 12, r.width, r.height}, 0.06f, 6, Fade(BLACK, 0.2f));
    if (destaque == 2) {
        DrawRectangleRounded({r.x - 8, r.y - 8, r.width + 16, r.height + 16}, 0.1f, 8, COR_OURO);
    } else if (pronto) {
        DrawRectangleRounded({r.x - 6, r.y - 6, r.width + 12, r.height + 12}, 0.1f, 8, Fade(COR_VERDE, 0.75f));
    } else if (destaque == 1) {
        DrawRectangleRounded({r.x - 6, r.y - 6, r.width + 12, r.height + 12}, 0.1f, 8, Fade(COR_OURO, 0.5f));
    }

    // Fundo e ripas de tras
    DrawRectangleGradientV((int)r.x, (int)r.y, (int)r.width, (int)r.height, Color{92, 56, 28, 255}, Color{64, 38, 18, 255});
    for (int k = 0; k < 4; k++) {
        float y = r.y + 14 + k * VAGA_ALTURA + VAGA_ALTURA * 0.28f;
        DrawRectangleGradientV((int)r.x, (int)y, (int)r.width, (int)(VAGA_ALTURA * 0.44f), Color{156, 102, 56, 255}, Color{124, 78, 40, 255});
        DrawLine((int)r.x + 14, (int)(y + 8), (int)(r.x + r.width - 14), (int)(y + 10), Fade(Color{90, 55, 28, 255}, 0.5f));
    }

    // Laterais e fundo, com veios da madeira
    Color claro = {222, 164, 98, 255}, medio = {186, 124, 64, 255};
    DrawRectangleGradientH((int)r.x, (int)r.y, 13, (int)r.height, claro, medio);
    DrawRectangleGradientH((int)(r.x + r.width - 13), (int)r.y, 13, (int)r.height, medio, claro);
    DrawRectangleGradientV((int)r.x, (int)(r.y + r.height - 20), (int)r.width, 20, claro, medio);
    DrawLine((int)r.x + 6, (int)r.y + 20, (int)r.x + 6, (int)(r.y + r.height - 26), Fade(Color{140, 90, 45, 255}, 0.6f));
    DrawLine((int)(r.x + r.width - 7), (int)r.y + 30, (int)(r.x + r.width - 7), (int)(r.y + r.height - 30), Fade(Color{140, 90, 45, 255}, 0.6f));
    DrawLine((int)r.x + 20, (int)(r.y + r.height - 10), (int)(r.x + r.width - 24), (int)(r.y + r.height - 11), Fade(Color{140, 90, 45, 255}, 0.6f));
    DrawRectangleLinesEx(r, 2, Color{96, 58, 28, 255});
    for (float px : {r.x + 6.5f, r.x + r.width - 6.5f}) {
        DrawCircleV({px, r.y + r.height - 10}, 2.5f, Color{80, 50, 30, 255});
        DrawCircleV({px, r.y + 10}, 2.5f, Color{80, 50, 30, 255});
    }
}

void desenharSeloPronto(Rectangle r, float escala) {
    Vector2 c = {r.x + r.width / 2, r.y - 26};
    float s = escala;
    DrawCircleV(mais(c, 0, 3), 21 * s, Fade(BLACK, 0.2f));
    DrawCircleV(c, 21 * s, WHITE);
    DrawCircleV(c, 18 * s, COR_VERDE);
    DrawLineEx(mais(c, -8 * s, 0), mais(c, -2 * s, 7 * s), 4.5f * s, WHITE);
    DrawLineEx(mais(c, -2 * s, 7 * s), mais(c, 9 * s, -7 * s), 4.5f * s, WHITE);
}

// ============================================================
//  CENARIO: ceu, nuvens, bandeirinhas, toldo, prateleiras, toalha
// ============================================================

void desenharCeu(float W, float H, float tempo, Color topo = {138, 204, 242, 255}, Color base = {255, 238, 206, 255}) {
    DrawRectangleGradientV(0, 0, (int)W, (int)H, topo, base);
    // Sol suave
    DrawCircleGradient((int)(W * 0.82f), 150, 150, Fade(Color{255, 240, 170, 255}, 0.8f), Fade(Color{255, 240, 170, 255}, 0));
    // Nuvens andando devagar
    for (int i = 0; i < 4; i++) {
        float x = fmodf(i * W / 3.2f + tempo * (8 + i * 3), W + 300) - 150;
        float y = 120 + i * 38;
        Color nuvem = Fade(WHITE, 0.75f);
        DrawCircle((int)x, (int)y, 26, nuvem);
        DrawCircle((int)(x + 30), (int)(y - 10), 32, nuvem);
        DrawCircle((int)(x + 62), (int)y, 24, nuvem);
        DrawRectangleRounded({x - 10, y, 90, 22}, 1, 6, nuvem);
    }
}

// Bandeirinhas de festa penduradas num barbante
void desenharBandeirinhas(float W, float y, float tempo) {
    Color cores[] = {{232, 64, 64, 255}, {255, 196, 40, 255}, {62, 150, 220, 255}, {76, 176, 80, 255}, {236, 110, 180, 255}, {250, 140, 40, 255}};
    float espaco = 48;
    int total = (int)(W / espaco) + 2;
    auto alturaFio = [&](float x) { return y + sinf(x / W * PI) * 26; };
    for (int i = 0; i < (int)W; i += 8) {
        DrawLineEx({(float)i, alturaFio((float)i)}, {(float)i + 8, alturaFio((float)i + 8)}, 2, Color{120, 90, 60, 255});
    }
    for (int i = 0; i < total; i++) {
        float x = i * espaco + 10;
        float yf = alturaFio(x);
        float balanco = sinf(tempo * 2 + i) * 3;
        Color cor = cores[i % 6];
        triangulo({x, yf}, {x + 34, yf}, {x + 17 + balanco, yf + 38}, cor);
        triangulo({x + 4, yf + 2}, {x + 14, yf + 2}, {x + 13 + balanco * 0.8f, yf + 20}, Fade(WHITE, 0.3f));
    }
}

void desenharToldo(float W, Color vermelho = {210, 52, 48, 255}, Color creme = {255, 247, 232, 255}) {
    float listra = 64;
    int total = (int)(W / listra) + 2;
    DrawRectangle(0, 0, (int)W, 58, Fade(BLACK, 0.1f));
    for (int i = 0; i < total; i++) {
        Color cor = (i % 2 == 0) ? vermelho : creme;
        float x = i * listra;
        DrawEllipse((int)(x + listra / 2), 60, listra / 2, 16, Fade(BLACK, 0.12f));
        DrawRectangleGradientV((int)x, 0, (int)listra, 50, escurecer(cor, 0.12f), cor);
        DrawCircle((int)(x + listra / 2), 48, listra / 2, cor);
        DrawRectangle((int)(x + 8), 8, 6, 36, Fade(WHITE, 0.12f));
    }
    DrawRectangle(0, 0, (int)W, 8, escurecer(vermelho, 0.3f));
}

// Tabua onde os caixotes ficam apoiados
void desenharPrateleira(float W, float y) {
    DrawRectangle(0, (int)(y + 24), (int)W, 10, Fade(BLACK, 0.15f));
    DrawRectangleGradientV(0, (int)y, (int)W, 26, Color{214, 150, 84, 255}, Color{162, 102, 52, 255});
    DrawLine(0, (int)y + 1, (int)W, (int)y + 1, Fade(WHITE, 0.4f));
    DrawLine(0, (int)y + 12, (int)W, (int)y + 13, Fade(Color{120, 72, 36, 255}, 0.5f));
}

// Toalha xadrez da banca
void desenharToalha(float W, float y, float H) {
    float q = 44;
    for (int linha = 0; y + linha * q < H; linha++) {
        for (int col = 0; col * q < W; col++) {
            bool vermelho = (linha + col) % 2 == 0;
            Color cor = vermelho ? Color{214, 78, 70, 255} : Color{252, 242, 226, 255};
            DrawRectangle((int)(col * q), (int)(y + linha * q), (int)q + 1, (int)q + 1, cor);
        }
    }
    DrawRectangleGradientV(0, (int)y, (int)W, 30, Fade(BLACK, 0.25f), Fade(BLACK, 0));
}

// ============================================================
//  FEIRANTE "SEU ZE"
// ============================================================

// base = centro de baixo do corpo. humor: 0 normal, 1 feliz
void desenharFeirante(Vector2 base, float s, float tempo, int humor) {
    float y0 = base.y + sinf(tempo * 2.2f) * 2 * s;
    float x = base.x;
    Color pele = {242, 192, 150, 255}, peleEscura = {214, 156, 116, 255};

    // Corpo: camisa azul e avental
    DrawRectangleRounded({x - 66 * s, y0 - 84 * s, 132 * s, 96 * s}, 0.45f, 10, Color{62, 110, 180, 255});
    DrawRectangleRounded({x - 40 * s, y0 - 64 * s, 80 * s, 80 * s}, 0.25f, 8, Color{250, 246, 236, 255});
    DrawLineEx({x - 36 * s, y0 - 62 * s}, {x - 26 * s, y0 - 84 * s}, 5 * s, Color{250, 246, 236, 255});
    DrawLineEx({x + 36 * s, y0 - 62 * s}, {x + 26 * s, y0 - 84 * s}, 5 * s, Color{250, 246, 236, 255});
    DrawRectangleRounded({x - 18 * s, y0 - 40 * s, 36 * s, 24 * s}, 0.3f, 6, Color{232, 222, 205, 255});
    // Pescoco
    DrawRectangle((int)(x - 14 * s), (int)(y0 - 98 * s), (int)(28 * s), (int)(18 * s), peleEscura);

    // Cabeca
    Vector2 cab = {x, y0 - 138 * s};
    DrawCircleV(mais(cab, -44 * s, 4 * s), 11 * s, peleEscura);
    DrawCircleV(mais(cab, 44 * s, 4 * s), 11 * s, peleEscura);
    DrawCircleV(cab, 46 * s, pele);
    DrawCircleV(mais(cab, -24 * s, 14 * s), 9 * s, Fade(Color{240, 120, 110, 255}, 0.45f));
    DrawCircleV(mais(cab, 24 * s, 14 * s), 9 * s, Fade(Color{240, 120, 110, 255}, 0.45f));

    // Olhos (piscam de vez em quando)
    bool piscando = fmodf(tempo, 4.0f) < 0.13f;
    for (int lado : {-1, 1}) {
        Vector2 olho = mais(cab, lado * 16 * s, -4 * s);
        if (piscando || humor == 1) {
            // Olho fechado / sorridente: um arquinho
            for (int a = 200; a <= 340; a += 20) {
                float ang = a * DEG2RAD;
                DrawCircleV(mais(olho, cosf(ang) * 6 * s, sinf(ang) * 6 * s + 4 * s), 2 * s, Color{60, 40, 30, 255});
            }
        } else {
            DrawCircleV(olho, 6 * s, Color{60, 40, 30, 255});
            DrawCircleV(mais(olho, -2 * s, -2 * s), 2 * s, WHITE);
        }
        DrawLineEx(mais(olho, -8 * s, -12 * s), mais(olho, 8 * s, -13 * s), 4 * s, Color{120, 110, 105, 255});
    }

    // Nariz, bigode e sorriso
    DrawCircleV(mais(cab, 0, 8 * s), 8 * s, peleEscura);
    if (humor == 1) {
        DrawEllipse((int)cab.x, (int)(cab.y + 30 * s), 13 * s, 9 * s, Color{150, 50, 50, 255});
        DrawEllipse((int)cab.x, (int)(cab.y + 34 * s), 8 * s, 4 * s, Color{230, 110, 110, 255});
    } else {
        for (int a = 20; a <= 160; a += 14) {
            float ang = a * DEG2RAD;
            DrawCircleV(mais(cab, cosf(ang) * 12 * s, 20 * s + sinf(ang) * 10 * s), 2.2f * s, Color{150, 60, 50, 255});
        }
    }
    Color bigode = {110, 100, 96, 255};
    DrawEllipse((int)(cab.x - 13 * s), (int)(cab.y + 19 * s), 15 * s, 7 * s, bigode);
    DrawEllipse((int)(cab.x + 13 * s), (int)(cab.y + 19 * s), 15 * s, 7 * s, bigode);

    // Chapeu de palha
    Color palha = {236, 196, 116, 255}, palhaEscura = {190, 146, 70, 255};
    DrawEllipse((int)cab.x, (int)(cab.y - 30 * s), 74 * s + 2, 17 * s + 2, palhaEscura);
    DrawEllipse((int)cab.x, (int)(cab.y - 30 * s), 74 * s, 17 * s, palha);
    DrawRectangleRounded({cab.x - 40 * s, cab.y - 74 * s, 80 * s, 46 * s}, 0.5f, 8, palha);
    DrawRectangle((int)(cab.x - 40 * s), (int)(cab.y - 44 * s), (int)(80 * s), (int)(11 * s), Color{200, 50, 46, 255});
    DrawLineEx(mais(cab, -30 * s, -66 * s), mais(cab, -28 * s, -46 * s), 2 * s, palhaEscura);
    DrawLineEx(mais(cab, 0, -70 * s), mais(cab, 0, -46 * s), 2 * s, palhaEscura);
    DrawLineEx(mais(cab, 30 * s, -66 * s), mais(cab, 28 * s, -46 * s), 2 * s, palhaEscura);
}

// Balao de fala com rabinho apontando para "ponta"
void desenharBalao(Rectangle r, const string& fala, Vector2 ponta, float tamanhoTexto) {
    Vector2 meio = {r.x + 18, r.y + r.height * 0.55f};
    DrawRectangleRounded({r.x + 4, r.y + 7, r.width, r.height}, 0.3f, 10, Fade(BLACK, 0.15f));
    triangulo(ponta, {meio.x, meio.y - 16}, {meio.x, meio.y + 16}, Color{226, 214, 196, 255});
    DrawRectangleRounded({r.x - 3, r.y - 3, r.width + 6, r.height + 6}, 0.3f, 10, Color{226, 214, 196, 255});
    DrawRectangleRounded(r, 0.3f, 10, WHITE);
    triangulo(mais(ponta, 4, 0), {meio.x + 4, meio.y - 12}, {meio.x + 4, meio.y + 12}, WHITE);

    float tamanho = tamanhoTexto;
    vector<string> linhas = quebrarLinhas(fala, r.width - 40, tamanho);
    while (linhas.size() * tamanho * 1.12f > r.height - 16 && tamanho > 16) {
        tamanho -= 2;
        linhas = quebrarLinhas(fala, r.width - 40, tamanho);
    }
    float alturaTotal = linhas.size() * tamanho * 1.12f;
    float y = r.y + (r.height - alturaTotal) / 2;
    for (const string& l : linhas) {
        texto(l, r.x + 22, y, tamanho, COR_TEXTO);
        y += tamanho * 1.12f;
    }
}

// ============================================================
//  BOTOES com icones
// ============================================================

enum Icone { SEM_ICONE, ICONE_DESFAZER, ICONE_DICA, ICONE_REINICIAR, ICONE_NOVA, ICONE_CASA, ICONE_SOM, ICONE_MUDO, ICONE_JOGAR, ICONE_AJUDA, ICONE_CAIXOTE, ICONE_CALENDARIO, ICONE_MAPA };

// Seta em arco (usada em desfazer e reiniciar)
void setaCurva(Vector2 c, float raio, float inicioGraus, float fimGraus, float espessura, Color cor) {
    float sentido = fimGraus >= inicioGraus ? 1.0f : -1.0f;  // horario ou anti-horario
    for (float a = inicioGraus; (a - fimGraus) * sentido <= 0; a += 6 * sentido) {
        DrawCircleV(mais(c, cosf(a * DEG2RAD) * raio, sinf(a * DEG2RAD) * raio), espessura / 2, cor);
    }
    float af = fimGraus * DEG2RAD;
    Vector2 p = mais(c, cosf(af) * raio, sinf(af) * raio);
    Vector2 tangente = {-sinf(af) * sentido, cosf(af) * sentido};
    Vector2 normal = {cosf(af), sinf(af)};
    float t = espessura * 1.6f;
    triangulo(mais(p, normal.x * t, normal.y * t), mais(p, -normal.x * t, -normal.y * t),
              mais(p, tangente.x * t * 1.3f, tangente.y * t * 1.3f), cor);
}

void desenharIcone(Icone icone, Vector2 c, float s, Color cor) {
    switch (icone) {
        case ICONE_DESFAZER:
            setaCurva(mais(c, 2 * s, 3 * s), 10 * s, 40, -180, 4.5f * s, cor);
            break;
        case ICONE_REINICIAR:
            setaCurva(c, 11 * s, -60, 250, 4.5f * s, cor);
            break;
        case ICONE_DICA:
            DrawCircleV(mais(c, 0, -3 * s), 10 * s, cor);
            DrawRectangle((int)(c.x - 5 * s), (int)(c.y + 6 * s), (int)(10 * s), (int)(7 * s), cor);
            DrawRectangle((int)(c.x - 4 * s), (int)(c.y + 14 * s), (int)(8 * s), (int)(2.5f * s), cor);
            break;
        case ICONE_NOVA:
            DrawRectangle((int)(c.x - 12 * s), (int)(c.y - 3 * s), (int)(24 * s), (int)(6 * s), cor);
            DrawRectangle((int)(c.x - 3 * s), (int)(c.y - 12 * s), (int)(6 * s), (int)(24 * s), cor);
            break;
        case ICONE_CASA:
            triangulo(mais(c, -14 * s, 0), mais(c, 14 * s, 0), mais(c, 0, -13 * s), cor);
            DrawRectangle((int)(c.x - 9 * s), (int)(c.y - 1 * s), (int)(18 * s), (int)(13 * s), cor);
            break;
        case ICONE_SOM:
        case ICONE_MUDO:
            DrawRectangle((int)(c.x - 13 * s), (int)(c.y - 5 * s), (int)(8 * s), (int)(10 * s), cor);
            triangulo(mais(c, -7 * s, -5 * s), mais(c, -7 * s, 5 * s), mais(c, 3 * s, -12 * s), cor);
            triangulo(mais(c, -7 * s, 5 * s), mais(c, 3 * s, 12 * s), mais(c, 3 * s, -12 * s), cor);
            if (icone == ICONE_SOM) {
                for (float a = -45; a <= 45; a += 8) {
                    DrawCircleV(mais(c, 4 * s + cosf(a * DEG2RAD) * 9 * s, sinf(a * DEG2RAD) * 9 * s), 1.6f * s, cor);
                }
            } else {
                DrawLineEx(mais(c, 7 * s, -6 * s), mais(c, 15 * s, 6 * s), 3 * s, cor);
                DrawLineEx(mais(c, 15 * s, -6 * s), mais(c, 7 * s, 6 * s), 3 * s, cor);
            }
            break;
        case ICONE_JOGAR:
            triangulo(mais(c, -9 * s, -13 * s), mais(c, -9 * s, 13 * s), mais(c, 14 * s, 0), cor);
            break;
        case ICONE_AJUDA:
            DrawCircleV(c, 13 * s, cor);
            break;
        case ICONE_CAIXOTE:
            // Caixote de ripas com um "+" (caixote extra)
            DrawRectangleLinesEx({c.x - 14 * s, c.y - 8 * s, 22 * s, 20 * s}, 3 * s, cor);
            DrawRectangle((int)(c.x - 13 * s), (int)(c.y + 0.5f * s), (int)(20 * s), (int)(3 * s), cor);
            DrawCircleV(mais(c, 10 * s, -9 * s), 7.5f * s, cor);
            DrawRectangle((int)(c.x + 6 * s), (int)(c.y - 10 * s), (int)(8 * s), (int)(2.5f * s), Fade(BLACK, 0.45f));
            DrawRectangle((int)(c.x + 8.75f * s), (int)(c.y - 13 * s), (int)(2.5f * s), (int)(8 * s), Fade(BLACK, 0.45f));
            break;
        case ICONE_CALENDARIO:
            DrawRectangleRounded({c.x - 13 * s, c.y - 10 * s, 26 * s, 24 * s}, 0.25f, 6, cor);
            DrawRectangle((int)(c.x - 13 * s), (int)(c.y - 4 * s), (int)(26 * s), (int)(2 * s), Fade(BLACK, 0.35f));
            DrawRectangle((int)(c.x - 8 * s), (int)(c.y - 14 * s), (int)(3 * s), (int)(7 * s), cor);
            DrawRectangle((int)(c.x + 5 * s), (int)(c.y - 14 * s), (int)(3 * s), (int)(7 * s), cor);
            for (int i = 0; i < 3; i++) {
                DrawRectangle((int)(c.x - 9 * s + i * 7 * s), (int)(c.y + 2 * s), (int)(4 * s), (int)(4 * s), Fade(BLACK, 0.3f));
            }
            break;
        case ICONE_MAPA:
            DrawCircleV(mais(c, 0, -5 * s), 10 * s, cor);
            triangulo(mais(c, -9 * s, -1 * s), mais(c, 9 * s, -1 * s), mais(c, 0, 14 * s), cor);
            DrawCircleV(mais(c, 0, -5 * s), 4 * s, Fade(BLACK, 0.3f));
            break;
        default:
            break;
    }
}

struct Botao {
    Rectangle r;
    string rotulo;
    Color cor;
    Icone icone;
    float tamanhoTexto;
};

// Contorno arredondado (a funcao mudou de nome na raylib 5.5)
void contornoArredondado(Rectangle r, float arredondado, int segmentos, float espessura, Color cor) {
#if defined(RAYLIB_VERSION_MAJOR) && (RAYLIB_VERSION_MAJOR * 100 + RAYLIB_VERSION_MINOR >= 505)
    DrawRectangleRoundedLinesEx(r, arredondado, segmentos, espessura, cor);
#else
    DrawRectangleRoundedLines(r, arredondado, segmentos, espessura, cor);
#endif
}

bool dentro(Vector2 p, Rectangle r) { return CheckCollisionPointRec(p, r); }

void desenharBotao(const Botao& b, bool mouseEmCima, bool ativo = true) {
    Color cor = ativo ? b.cor : Color{170, 160, 150, 255};
    float sobe = (mouseEmCima && ativo) ? 3 : 0;
    Rectangle r = {b.r.x, b.r.y - sobe, b.r.width, b.r.height};
    float arredondado = 0.4f;
    DrawRectangleRounded({r.x + 2, b.r.y + 9, r.width, r.height}, arredondado, 10, Fade(BLACK, 0.2f));
    DrawRectangleRounded({r.x, r.y + 6, r.width, r.height}, arredondado, 10, escurecer(cor, 0.3f));
    DrawRectangleRounded(r, arredondado, 10, mouseEmCima && ativo ? clarear(cor, 0.12f) : cor);
    DrawRectangleRounded({r.x + 6, r.y + 4, r.width - 12, r.height * 0.42f}, arredondado, 10, Fade(WHITE, 0.18f));

    float tamanho = b.tamanhoTexto;
    float larguraRotulo = b.rotulo.empty() ? 0 : larguraTexto(b.rotulo, tamanho);
    float larguraIcone = b.icone == SEM_ICONE ? 0 : tamanho * 1.15f;
    float espaco = (larguraIcone > 0 && larguraRotulo > 0) ? tamanho * 0.35f : 0;
    float total = larguraIcone + espaco + larguraRotulo;
    float x = r.x + (r.width - total) / 2;
    float centroY = r.y + r.height / 2;
    if (b.icone == ICONE_AJUDA) {
        desenharIcone(ICONE_AJUDA, {x + larguraIcone / 2, centroY}, tamanho / 26, WHITE);
        textoCentro("?", x + larguraIcone / 2, centroY - tamanho * 0.5f, tamanho, cor);
    } else if (b.icone != SEM_ICONE) {
        desenharIcone(b.icone, {x + larguraIcone / 2 + 1, centroY + 2}, tamanho / 26, Fade(BLACK, 0.2f));
        desenharIcone(b.icone, {x + larguraIcone / 2, centroY}, tamanho / 26, WHITE);
    }
    if (larguraRotulo > 0) {
        float tx = x + larguraIcone + espaco;
        texto(b.rotulo, tx + 1, centroY - tamanho * 0.56f + 2, tamanho, Fade(BLACK, 0.2f));
        texto(b.rotulo, tx, centroY - tamanho * 0.56f, tamanho, WHITE);
    }
}

// Etiqueta arredondada com informacao (fase, jogadas)
void desenharPilula(Rectangle r, const string& titulo, const string& valor) {
    DrawRectangleRounded({r.x + 2, r.y + 5, r.width, r.height}, 0.5f, 10, Fade(BLACK, 0.15f));
    DrawRectangleRounded(r, 0.5f, 10, COR_CREME);
    DrawRectangleRounded({r.x + 4, r.y + 4, r.width - 8, r.height - 8}, 0.5f, 10, Color{255, 252, 244, 255});
    float tamanho = r.height * 0.46f;
    string tudo = titulo + " " + valor;
    float x = r.x + (r.width - larguraTexto(tudo, tamanho)) / 2;
    float y = r.y + r.height / 2 - tamanho * 0.56f;
    texto(titulo + " ", x, y, tamanho, Fade(COR_TEXTO, 0.7f));
    texto(valor, x + larguraTexto(titulo + " ", tamanho), y, tamanho, COR_LARANJA);
}

// Etiqueta com moeda e quantidade
void desenharPilulaMoedas(Rectangle r, int moedas) {
    DrawRectangleRounded({r.x + 2, r.y + 5, r.width, r.height}, 0.5f, 10, Fade(BLACK, 0.15f));
    DrawRectangleRounded(r, 0.5f, 10, COR_CREME);
    DrawRectangleRounded({r.x + 4, r.y + 4, r.width - 8, r.height - 8}, 0.5f, 10, Color{255, 252, 244, 255});
    float raio = r.height * 0.32f;
    desenharMoeda({r.x + r.height * 0.55f, r.y + r.height / 2}, raio);
    float tamanho = r.height * 0.5f;
    textoCentro(to_string(moedas), r.x + r.height * 0.5f + (r.width - r.height * 0.5f) / 2, r.y + r.height / 2 - tamanho * 0.56f,
                tamanho, COR_LARANJA);
}


// ============================================================
//  VIAGEM PELO BRASIL: caminhao do Seu Ze, marcadores e postal
// ============================================================

// Caminhaozinho de feira carregado de frutas. "direcao" 1 = para a direita
void desenharCaminhao(Vector2 c, float s, float tempo, float direcao = 1) {
    float balanco = sinf(tempo * 10) * 1.2f * s;
    auto X = [&](float dx) { return c.x + dx * s * direcao; };
    float y = c.y + balanco;
    // Sombra
    DrawEllipse((int)c.x, (int)(c.y + 22 * s), 46 * s, 7 * s, Fade(BLACK, 0.25f));
    // Carroceria com caixotes e frutas
    Rectangle carroceria = {min(X(-44), X(8)), y - 14 * s, 52 * s, 26 * s};
    DrawRectangleRounded(carroceria, 0.2f, 6, Color{196, 138, 74, 255});
    DrawRectangle((int)carroceria.x, (int)(y - 4 * s), (int)carroceria.width, (int)(2 * s), Color{150, 100, 50, 255});
    desenharFruta(LARANJA, {X(-30), y - 20 * s}, 0.32f * s);
    desenharFruta(MACA, {X(-14), y - 22 * s}, 0.32f * s);
    desenharFruta(BANANA, {X(0), y - 18 * s}, 0.3f * s);
    // Cabine
    Rectangle cabine = {min(X(10), X(40)), y - 16 * s, 30 * s, 30 * s};
    DrawRectangleRounded(cabine, 0.35f, 6, Color{214, 60, 52, 255});
    Rectangle janela = {min(X(18), X(34)), y - 11 * s, 16 * s, 11 * s};
    DrawRectangleRounded(janela, 0.3f, 6, Color{200, 232, 250, 255});
    DrawCircleV({X(24), y - 5 * s}, 4 * s, Color{242, 192, 150, 255});  // Seu Ze dirigindo
    DrawRectangle((int)min(X(19), X(29)), (int)(y - 10 * s), (int)(10 * s), (int)(2.5f * s), Color{236, 196, 116, 255});
    DrawRectangle((int)min(X(38), X(42)), (int)(y + 4 * s), (int)(4 * s), (int)(4 * s), Color{255, 220, 90, 255});  // farol
    // Rodas
    for (float dx : {-30.0f, 24.0f}) {
        DrawCircleV({X(dx), c.y + 15 * s}, 8 * s, Color{50, 50, 56, 255});
        DrawCircleV({X(dx), c.y + 15 * s}, 3.5f * s, Color{190, 190, 196, 255});
    }
}

// Marcador de cidade no mapa. estado: 0 bloqueada, 1 atual, 2 concluida
void desenharMarcadorCidade(Vector2 p, int estado, float tempo) {
    float pulso = estado == 1 ? 1 + 0.12f * sinf(tempo * 5) : 1;
    float r = 17 * pulso;
    Color cor = estado == 0 ? Color{160, 152, 142, 255} : (estado == 1 ? COR_LARANJA : COR_VERDE);
    DrawCircleV(mais(p, 0, 3), r + 3, Fade(BLACK, 0.25f));
    if (estado == 1) DrawCircleV(p, r + 7, Fade(COR_OURO, 0.7f));
    DrawCircleV(p, r + 3, WHITE);
    DrawCircleV(p, r, cor);
    if (estado == 2) {
        DrawLineEx(mais(p, -7, 0), mais(p, -2, 6), 3.5f, WHITE);
        DrawLineEx(mais(p, -2, 6), mais(p, 8, -6), 3.5f, WHITE);
    } else if (estado == 0) {
        desenharCadeado(p, 0.55f, Color{240, 236, 230, 255});
    } else {
        DrawCircleV(p, r * 0.35f, WHITE);
    }
}

// Linha tracejada em curva (a rota da viagem)
void desenharRota(Vector2 a, Vector2 b, float curvatura, Color cor, float progresso = 1) {
    Vector2 meio = {(a.x + b.x) / 2 - (b.y - a.y) * curvatura, (a.y + b.y) / 2 + (b.x - a.x) * curvatura};
    const int PASSOS = 40;
    for (int i = 0; i < PASSOS; i++) {
        float t = (float)i / PASSOS;
        if (t > progresso) break;
        if (i % 2 == 1) continue;  // tracejado
        float u = 1 - t;
        Vector2 p = {u * u * a.x + 2 * u * t * meio.x + t * t * b.x, u * u * a.y + 2 * u * t * meio.y + t * t * b.y};
        DrawCircleV(p, 3.2f, cor);
    }
}

Vector2 pontoNaRota(Vector2 a, Vector2 b, float curvatura, float t) {
    Vector2 meio = {(a.x + b.x) / 2 - (b.y - a.y) * curvatura, (a.y + b.y) / 2 + (b.x - a.x) * curvatura};
    float u = 1 - t;
    return {u * u * a.x + 2 * u * t * meio.x + t * t * b.x, u * u * a.y + 2 * u * t * meio.y + t * t * b.y};
}

// Envelope (icone do postal)
void desenharEnvelope(Vector2 c, float s, Color papel, Color linha) {
    Rectangle r = {c.x - 20 * s, c.y - 14 * s, 40 * s, 28 * s};
    DrawRectangleRounded({r.x - 2, r.y - 2, r.width + 4, r.height + 4}, 0.15f, 6, linha);
    DrawRectangleRounded(r, 0.15f, 6, papel);
    DrawLineEx({r.x + 2, r.y + 2}, {c.x, c.y + 2 * s}, 2.5f * s, linha);
    DrawLineEx({r.x + r.width - 2, r.y + 2}, {c.x, c.y + 2 * s}, 2.5f * s, linha);
    DrawCircleV(mais(c, 0, 5 * s), 5 * s, Color{210, 52, 48, 255});  // lacre
}

// ============================================================
//  PARTICULAS (confete e brilhos)
// ============================================================

struct Particula {
    Vector2 pos, vel;
    Color cor;
    float vida, vidaMax, tamanho, angulo, giro;
    bool estrela;
};

vector<Particula> particulas;

void soltarConfete(Vector2 origem, int quantidade, float forca) {
    Color cores[] = {{232, 64, 64, 255}, {255, 196, 40, 255}, {62, 150, 220, 255}, {76, 176, 80, 255}, {236, 110, 180, 255}, {250, 140, 40, 255}};
    for (int i = 0; i < quantidade; i++) {
        float ang = GetRandomValue(200, 340) * DEG2RAD;
        float v = forca * GetRandomValue(50, 100) / 100.0f;
        Particula p;
        p.pos = origem;
        p.vel = {cosf(ang) * v, sinf(ang) * v};
        p.cor = cores[GetRandomValue(0, 5)];
        p.vidaMax = p.vida = GetRandomValue(90, 160) / 100.0f;
        p.tamanho = (float)GetRandomValue(10, 18);
        p.angulo = (float)GetRandomValue(0, 360);
        p.giro = (float)GetRandomValue(-400, 400);
        p.estrela = GetRandomValue(0, 4) == 0;
        particulas.push_back(p);
    }
}

void atualizarParticulas(float dt) {
    for (Particula& p : particulas) {
        p.vel.y += 900 * dt;
        p.vel.x *= 0.99f;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.angulo += p.giro * dt;
        p.vida -= dt;
    }
    particulas.erase(remove_if(particulas.begin(), particulas.end(), [](const Particula& p) { return p.vida <= 0; }),
                     particulas.end());
}

void desenharParticulas() {
    for (const Particula& p : particulas) {
        float alfa = min(1.0f, p.vida / (p.vidaMax * 0.4f));
        if (p.estrela) {
            desenharEstrela(p.pos, p.tamanho, Fade(COR_OURO, alfa));
        } else {
            DrawRectanglePro({p.pos.x, p.pos.y, p.tamanho, p.tamanho * 0.6f}, {p.tamanho / 2, p.tamanho * 0.3f},
                             p.angulo, Fade(p.cor, alfa));
        }
    }
}
