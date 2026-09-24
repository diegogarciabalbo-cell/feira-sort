// ============================================================
//  som.h - Efeitos sonoros e musica de fundo, gerados pelo
//  proprio codigo (sintese de ondas). Nenhum arquivo de audio.
// ============================================================
#pragma once

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace std;

const int TAXA = 44100;

float frequenciaDaNota(int midi) {
    return 440.0f * powf(2.0f, (midi - 69) / 12.0f);
}

// Soma uma nota "dedilhada" (tipo violao/cavaquinho) dentro do buffer
void adicionarNota(vector<float>& buffer, float inicio, float frequencia, float duracao, float volume) {
    int comeco = (int)(inicio * TAXA);
    int total = (int)(duracao * TAXA);
    for (int i = 0; i < total && comeco + i < (int)buffer.size(); i++) {
        float t = (float)i / TAXA;
        float ataque = min(1.0f, i / 300.0f);
        float queda = expf(-4.0f * t / duracao);
        float onda = sinf(2 * PI * frequencia * t) + 0.35f * sinf(4 * PI * frequencia * t) +
                     0.12f * sinf(6 * PI * frequencia * t);
        buffer[comeco + i] += onda * ataque * queda * volume;
    }
}

Sound somDoBuffer(const vector<float>& buffer) {
    vector<short> amostras(buffer.size());
    for (size_t i = 0; i < buffer.size(); i++) {
        float v = max(-1.0f, min(1.0f, buffer[i]));
        amostras[i] = (short)(v * 30000);
    }
    Wave onda = {(unsigned int)amostras.size(), TAXA, 16, 1, amostras.data()};
    return LoadSoundFromWave(onda);  // a raylib copia os dados
}

// Sequencia de notas: {midi, duracao em segundos}
Sound criarEfeito(const vector<pair<int, float>>& notas, float volume) {
    float total = 0;
    for (auto& n : notas) total += n.second;
    vector<float> buffer((size_t)((total + 0.3f) * TAXA), 0);
    float t = 0;
    for (auto& n : notas) {
        adicionarNota(buffer, t, frequenciaDaNota(n.first), n.second + 0.25f, volume);
        t += n.second;
    }
    return somDoBuffer(buffer);
}

// Musiquinha calma de feira: 8 compassos em do maior
Sound criarMusica() {
    const float TEMPO = 0.62f;  // segundos por batida
    const int COMPASSOS = 8;
    vector<float> buffer((size_t)(COMPASSOS * 4 * TEMPO * TAXA + TAXA), 0);

    // Acordes: raiz e se e menor
    int raizes[COMPASSOS] = {48, 45, 41, 43, 48, 45, 41, 43};
    bool menor[COMPASSOS] = {false, true, false, false, false, true, false, false};
    int melodia[COMPASSOS][4] = {{76, 79, 81, 79}, {76, 72, 74, 76}, {81, 79, 76, 72}, {74, 76, 74, 0},
                                 {76, 79, 84, 81}, {79, 76, 72, 76}, {74, 72, 74, 79}, {72, 0, 0, 0}};

    for (int c = 0; c < COMPASSOS; c++) {
        float inicio = c * 4 * TEMPO;
        int raiz = raizes[c];
        int terca = menor[c] ? 3 : 4;
        // Baixo nas batidas 1 e 3
        adicionarNota(buffer, inicio, frequenciaDaNota(raiz - 12), TEMPO * 2, 0.22f);
        adicionarNota(buffer, inicio + 2 * TEMPO, frequenciaDaNota(raiz - 5), TEMPO * 2, 0.18f);
        // Arpejo em colcheias
        int arpejo[4] = {raiz + 12, raiz + 12 + terca, raiz + 19, raiz + 12 + terca};
        for (int k = 0; k < 8; k++) {
            adicionarNota(buffer, inicio + k * TEMPO / 2, frequenciaDaNota(arpejo[k % 4]), TEMPO, 0.08f);
        }
        // Melodia
        for (int b = 0; b < 4; b++) {
            if (melodia[c][b] == 0) continue;
            float duracao = (c == COMPASSOS - 1) ? TEMPO * 3 : TEMPO * 1.2f;
            adicionarNota(buffer, inicio + b * TEMPO, frequenciaDaNota(melodia[c][b]), duracao, 0.2f);
        }
    }
    return somDoBuffer(buffer);
}

// ---------------- Conjunto de sons do jogo ----------------

struct Sons {
    bool disponivel = false;
    bool ligado = true;
    Sound plim[4], pegar, erro, pronto, vitoria, clique, musica;

    void carregar() {
        InitAudioDevice();
        disponivel = IsAudioDeviceReady();
        if (!disponivel) return;
        int notas[4] = {72, 74, 76, 79};  // do, re, mi, sol
        for (int i = 0; i < 4; i++) plim[i] = criarEfeito({{notas[i], 0.12f}}, 0.4f);
        pegar = criarEfeito({{67, 0.06f}}, 0.25f);
        erro = criarEfeito({{55, 0.12f}, {52, 0.18f}}, 0.3f);
        pronto = criarEfeito({{76, 0.08f}, {79, 0.08f}, {84, 0.2f}}, 0.35f);
        vitoria = criarEfeito({{72, 0.13f}, {76, 0.13f}, {79, 0.13f}, {84, 0.13f}, {79, 0.1f}, {84, 0.5f}}, 0.4f);
        clique = criarEfeito({{84, 0.03f}}, 0.15f);
        musica = criarMusica();
        SetSoundVolume(musica, 0.55f);
    }

    void tocar(Sound& s) {
        if (disponivel && ligado) PlaySound(s);
    }

    // Chamado todo quadro: recomeca a musica quando ela termina
    void atualizarMusica() {
        if (!disponivel) return;
        if (ligado && !IsSoundPlaying(musica)) PlaySound(musica);
        if (!ligado && IsSoundPlaying(musica)) StopSound(musica);
    }

    void descarregar() {
        if (!disponivel) return;
        for (Sound& s : plim) UnloadSound(s);
        for (Sound* s : {&pegar, &erro, &pronto, &vitoria, &clique, &musica}) UnloadSound(*s);
        CloseAudioDevice();
    }
};
