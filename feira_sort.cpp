// ============================================================
//  FEIRA SORT - Versao 1
//  Jogo de organizar frutas nos caixotes, feito em C++.
//  Objetivo: deixar cada caixote com um so tipo de fruta.
//  Pensado para ser calmo: sem cronometro e com "desfazer" ilimitado.
// ============================================================

#include <algorithm>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// Quantas frutas cabem em cada caixote
const int CAPACIDADE = 4;

// Quantos caixotes comecam vazios (espaco para manobrar)
const int CAIXOTES_VAZIOS = 2;

// Cada fruta tem uma letra, um nome e uma cor de terminal (codigo ANSI)
struct Fruta {
    char letra;
    string nome;
    string cor;
};

const vector<Fruta> FRUTAS = {
    {'M', "Maçã",   "\033[1;97;41m"},  // branco sobre vermelho
    {'B', "Banana", "\033[1;30;43m"},  // preto sobre amarelo
    {'U', "Uva",    "\033[1;97;45m"},  // branco sobre roxo
    {'L', "Limão",  "\033[1;30;42m"},  // preto sobre verde
};

const string RESET = "\033[0m";

// Um caixote e uma pilha de frutas: o fim do vetor (back) e o topo.
// Guardamos o numero da fruta (posicao dentro de FRUTAS).
using Caixote = vector<int>;
using Feira = vector<Caixote>;

// ------------------------------------------------------------
// Cria uma feira nova: embaralha as frutas e distribui nos caixotes
// ------------------------------------------------------------
Feira criarFeira(mt19937& gerador) {
    vector<int> todasAsFrutas;
    for (int f = 0; f < (int)FRUTAS.size(); f++) {
        for (int i = 0; i < CAPACIDADE; i++) {
            todasAsFrutas.push_back(f);
        }
    }
    shuffle(todasAsFrutas.begin(), todasAsFrutas.end(), gerador);

    Feira feira(FRUTAS.size() + CAIXOTES_VAZIOS);
    int pos = 0;
    for (int c = 0; c < (int)FRUTAS.size(); c++) {
        for (int i = 0; i < CAPACIDADE; i++) {
            feira[c].push_back(todasAsFrutas[pos++]);
        }
    }
    return feira;
}

// ------------------------------------------------------------
// Desenha os caixotes na tela
// ------------------------------------------------------------
void desenhar(const Feira& feira, int jogadas, const string& mensagem) {
    cout << "\033[2J\033[H";  // limpa a tela

    cout << "\n   🍊  FEIRA SORT  🍊\n";
    cout << "   Deixe cada caixote com um só tipo de fruta.\n\n";

    // Desenha de cima para baixo
    for (int nivel = CAPACIDADE - 1; nivel >= 0; nivel--) {
        cout << "   ";
        for (const Caixote& caixote : feira) {
            if (nivel < (int)caixote.size()) {
                const Fruta& fruta = FRUTAS[caixote[nivel]];
                cout << "|" << fruta.cor << "  " << fruta.letra << "  " << RESET << "|  ";
            } else {
                cout << "|     |  ";
            }
        }
        cout << "\n";
    }

    // Base dos caixotes e numeros
    cout << "   ";
    for (size_t i = 0; i < feira.size(); i++) cout << "+-----+  ";
    cout << "\n   ";
    for (size_t i = 0; i < feira.size(); i++) cout << "   " << i + 1 << "     ";
    cout << "\n\n";

    // Legenda
    cout << "   Frutas: ";
    for (const Fruta& fruta : FRUTAS) {
        cout << fruta.cor << " " << fruta.letra << " " << RESET << " " << fruta.nome << "   ";
    }
    cout << "\n   Jogadas: " << jogadas << "\n\n";

    if (!mensagem.empty()) cout << "   " << mensagem << "\n\n";

    cout << "   Como jogar: digite DE onde e PARA onde. Exemplo: 1 5\n";
    cout << "   Comandos:  d = desfazer   r = recomeçar   n = nova feira   s = sair\n\n";
    cout << "   Sua jogada: ";
}

// ------------------------------------------------------------
// Verifica se a jogada e permitida. Se nao for, explica o motivo.
// ------------------------------------------------------------
bool jogadaValida(const Feira& feira, int de, int para, string& motivo) {
    int total = feira.size();
    if (de < 0 || de >= total || para < 0 || para >= total) {
        motivo = "Esse caixote não existe. Use números de 1 a " + to_string(total) + ".";
        return false;
    }
    if (de == para) {
        motivo = "Escolha dois caixotes diferentes.";
        return false;
    }
    if (feira[de].empty()) {
        motivo = "O caixote " + to_string(de + 1) + " está vazio.";
        return false;
    }
    if ((int)feira[para].size() >= CAPACIDADE) {
        motivo = "O caixote " + to_string(para + 1) + " já está cheio.";
        return false;
    }
    if (!feira[para].empty() && feira[para].back() != feira[de].back()) {
        motivo = "Só pode colocar fruta em cima da mesma fruta (ou em caixote vazio).";
        return false;
    }
    return true;
}

// ------------------------------------------------------------
// Move as frutas iguais do topo, enquanto couber no destino
// ------------------------------------------------------------
void mover(Feira& feira, int de, int para) {
    int fruta = feira[de].back();
    while (!feira[de].empty() && feira[de].back() == fruta &&
           (int)feira[para].size() < CAPACIDADE) {
        feira[para].push_back(fruta);
        feira[de].pop_back();
    }
}

// ------------------------------------------------------------
// Venceu quando todo caixote esta vazio ou cheio de uma fruta so
// ------------------------------------------------------------
bool venceu(const Feira& feira) {
    for (const Caixote& caixote : feira) {
        if (caixote.empty()) continue;
        if ((int)caixote.size() != CAPACIDADE) return false;
        for (int fruta : caixote) {
            if (fruta != caixote[0]) return false;
        }
    }
    return true;
}

// ------------------------------------------------------------
// Programa principal
// ------------------------------------------------------------
int main() {
    random_device semente;
    mt19937 gerador(semente());

    Feira inicio = criarFeira(gerador);
    Feira feira = inicio;
    vector<Feira> historico;  // guarda estados anteriores para o "desfazer"
    int jogadas = 0;
    string mensagem = "Bem-vindo à feira! Sem pressa, pense com calma.";

    while (true) {
        desenhar(feira, jogadas, mensagem);
        mensagem = "";

        string linha;
        if (!getline(cin, linha)) break;  // fim da entrada

        // Comandos de uma letra
        if (linha == "s" || linha == "S") {
            cout << "\n   Até a próxima feira! 👋\n\n";
            break;
        }
        if (linha == "d" || linha == "D") {
            if (historico.empty()) {
                mensagem = "Não há jogada para desfazer.";
            } else {
                feira = historico.back();
                historico.pop_back();
                jogadas--;
                mensagem = "Jogada desfeita.";
            }
            continue;
        }
        if (linha == "r" || linha == "R") {
            feira = inicio;
            historico.clear();
            jogadas = 0;
            mensagem = "Feira recomeçada.";
            continue;
        }
        if (linha == "n" || linha == "N") {
            inicio = criarFeira(gerador);
            feira = inicio;
            historico.clear();
            jogadas = 0;
            mensagem = "Nova feira montada!";
            continue;
        }

        // Jogada normal: dois numeros
        istringstream leitor(linha);
        int de, para;
        if (!(leitor >> de >> para)) {
            mensagem = "Não entendi. Digite dois números, por exemplo: 1 5";
            continue;
        }
        de--;    // o jogador ve 1, 2, 3... mas o vetor comeca em 0
        para--;

        string motivo;
        if (!jogadaValida(feira, de, para, motivo)) {
            mensagem = "⚠ " + motivo;
            continue;
        }

        historico.push_back(feira);
        mover(feira, de, para);
        jogadas++;

        if (venceu(feira)) {
            desenhar(feira, jogadas, "🎉 Parabéns! Feira organizada em " +
                                         to_string(jogadas) + " jogadas!");
            cout << "\n   Jogar de novo? (s/n): ";
            string resposta;
            if (!getline(cin, resposta) || resposta == "n" || resposta == "N") {
                cout << "\n   Até a próxima feira! 👋\n\n";
                break;
            }
            inicio = criarFeira(gerador);
            feira = inicio;
            historico.clear();
            jogadas = 0;
            mensagem = "Nova feira montada!";
        }
    }
    return 0;
}
