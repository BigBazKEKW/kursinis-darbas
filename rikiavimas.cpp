
#include <iostream>   // įvesties/išvesties srautai (cout, cerr)
#include <fstream>    // failų srautai (ifstream, ofstream)
#include <vector>     // std::vector konteineris
#include <chrono>     // laiko matavimas (high_resolution_clock)
#include <algorithm>  // pagalbinės funkcijos (pvz., std::swap)
#include <random>     // atsitiktinių skaičių generavimas (mt19937)
#include <iomanip>    // išvesties formatavimas (setw, fixed)
#include <string>     // std::string klasė
#include <cstdint>    // fiksuoto dydžio sveikieji tipai (uint64_t)
#include <sys/stat.h> // failo egzistavimo tikrinimui (stat)

// Naudosime std vardų sritį, kad kodas būtų trumpesnis
using namespace std;
using namespace std::chrono;

// Duomenų struktūros

// Struktūra vieno bandymo (vieno paleidimo) rezultatui saugoti
struct RunResult
{
    int dydis;               // masyvo dydis (elementų skaičius)
    string tipas;            // duomenų tipas: "atsitiktiniai", "atvirkstiniai", "surikiuoti"
    string algoritmas;       // algoritmo pavadinimas: "SelectionSort" arba "MergeSort"
    int bandymas;            // bandymo numeris (1..N)
    long long laikas_mikros; // rikiavimo trukmė mikrosekundėmis
    uint64_t palyginimai;    // palyginimų skaičius
    uint64_t sukeitimai;     // sukeitimų (selection) arba priskyrimų (merge) skaičius
};

// ===== Pagalbinės funkcijos failams =====

// Funkcija: tikrina, ar nurodytas failas egzistuoja diske
// Parametrai: kelias - failo pavadinimas
// Grąžina: true jei failas egzistuoja, false jei ne
bool failasEgzistuoja(const string &kelias)
{
    struct stat buferis;                          // struktūra failo informacijai
    return (stat(kelias.c_str(), &buferis) == 0); // stat grąžina 0 jei sėkminga
}

// Funkcija: sugeneruoja duomenis pagal tipą ir įrašo į tekstinį failą
// Parametrai: dydis - kiek elementų, tipas - duomenų tipas, failoVardas - į kurį failą rašyti
// Grąžina: nieko (void), bet sukuria failą diske
void generuotiFaila(int dydis, const string &tipas, const string &failoVardas)
{
    // Jei failas jau yra - nieko nedarome (kad būtų galima dalintis duomenimis)
    if (failasEgzistuoja(failoVardas))
    {
        cout << "  Failas " << failoVardas << " jau egzistuoja, naudojamas esamas.\n";
        return;
    }

    vector<int> duomenys;    // vektorius generuotiems skaičiams
    duomenys.reserve(dydis); // rezervuojame atminties

    if (tipas == "atsitiktiniai")
    {
        // Inicializuojame atsitiktinių skaičių generatorių su fiksuota seed'u,
        // kad rezultatai būtų atkartojami
        mt19937 gen(12345);
        uniform_int_distribution<int> dist(1, 1000000); // intervalas [1, 1000000]
        for (int i = 0; i < dydis; ++i)
            duomenys.push_back(dist(gen)); // pridedame atsitiktinį skaičių
    }
    else if (tipas == "atvirkstiniai")
    {
        // Skaičiai mažėjimo tvarka: dydis, dydis-1, ..., 1
        for (int i = dydis; i >= 1; --i)
            duomenys.push_back(i);
    }
    else if (tipas == "surikiuoti")
    {
        // Skaičiai didėjimo tvarka: 1, 2, ..., dydis
        for (int i = 1; i <= dydis; ++i)
            duomenys.push_back(i);
    }

    // Atidarome failą rašymui
    ofstream f(failoVardas);
    if (!f)
    {
        cerr << "Klaida: negaliu sukurti failo " << failoVardas << "\n";
        return;
    }
    // Rašome skaičius, atskirtus tarpais
    for (size_t i = 0; i < duomenys.size(); ++i)
    {
        f << duomenys[i];
        if (i + 1 < duomenys.size())
            f << ' ';
    }
    f << '\n';
    f.close();
}

// Funkcija: nuskaito sveikuosius skaičius iš tekstinio failo į vektorių
// Parametrai: failoVardas - failo pavadinimas
// Grąžina: vector<int> su perskaitytais skaičiais
vector<int> skaitytiFaila(const string &failoVardas)
{
    vector<int> rez;         // grąžinamas vektorius
    ifstream f(failoVardas); // atidarome failą skaitymui
    if (!f)
    {
        cerr << "Klaida: negaliu atidaryti failo " << failoVardas << "\n";
        return rez;
    }
    int x;
    while (f >> x) // skaitome po vieną sveikąjį skaičių
        rez.push_back(x);
    return rez;
}

// ===== Rikiavimo algoritmai

// Funkcija: išrinkimo rikiavimas (Selection sort), rikiuoja didėjimo tvarka
// Parametrai: a - vektorius (modifikuojamas vietoje);
//             palyginimai - palyginimų skaitiklis (per nuorodą);
//             sukeitimai - sukeitimų skaitiklis (per nuorodą)
// Grąžina: nieko, masyvas surikiuojamas vietoje
void selectionSort(vector<int> &a, uint64_t &palyginimai, uint64_t &sukeitimai)
{
    palyginimai = 0;                    // pradinis palyginimų skaičius
    sukeitimai = 0;                     // pradinis sukeitimų skaičius
    int n = static_cast<int>(a.size()); // masyvo dydis

    // Pagrindinis ciklas: i nuo 0 iki n-2
    for (int i = 0; i < n - 1; ++i)
    {
        int minIdx = i; // mažiausio elemento indeksas pradžioje = i
        // Vidinis ciklas: ieškome mažiausio elemento likusioje dalyje
        for (int j = i + 1; j < n; ++j)
        {
            ++palyginimai;        // skaičiuojame palyginimą
            if (a[j] < a[minIdx]) // jei radome mažesnį
                minIdx = j;       // įsimename jo indeksą
        }
        // Jei mažiausias elementas nėra esamoje pozicijoje - sukeičiame
        if (minIdx != i)
        {
            swap(a[i], a[minIdx]); // sukeičiame du elementus
            ++sukeitimai;          // didiname sukeitimų skaitiklį
        }
    }
}

// Pagalbinė funkcija Merge sort'ui: sulieja du surikiuotus pomasyvius
// Parametrai: a - pagrindinis masyvas; kair, vid, des - indeksų ribos;
//             palyginimai, priskyrimai - skaitikliai per nuorodą
// PASTABA: Merge sort neturi tiesioginių sukeitimų, todėl skaičiuojame priskyrimus
static void merge(vector<int> &a, int kair, int vid, int des,
                  uint64_t &palyginimai, uint64_t &priskyrimai)
{
    int n1 = vid - kair + 1; // kairės pusės dydis
    int n2 = des - vid;      // dešinės pusės dydis

    // Sukuriame laikinus vektorius kairei ir dešinei pusėms
    vector<int> L(n1), R(n2);

    // Nukopijuojame duomenis į laikinus vektorius
    for (int i = 0; i < n1; ++i)
    {
        L[i] = a[kair + i];
        ++priskyrimai; // priskyrimas - skaičiuojame
    }
    for (int j = 0; j < n2; ++j)
    {
        R[j] = a[vid + 1 + j];
        ++priskyrimai;
    }

    int i = 0, j = 0, k = kair; // indeksai: L, R ir pagrindinio masyvo
    // Suliejame elementus atgal į a, lyginant L ir R
    while (i < n1 && j < n2)
    {
        ++palyginimai; // skaičiuojame palyginimą
        if (L[i] <= R[j])
        {
            a[k++] = L[i++]; // imame iš kairės
            ++priskyrimai;
        }
        else
        {
            a[k++] = R[j++]; // imame iš dešinės
            ++priskyrimai;
        }
    }
    // Įrašome likusius kairės pusės elementus (jei yra)
    while (i < n1)
    {
        a[k++] = L[i++];
        ++priskyrimai;
    }
    // Įrašome likusius dešinės pusės elementus (jei yra)
    while (j < n2)
    {
        a[k++] = R[j++];
        ++priskyrimai;
    }
}

// Pagalbinė rekursinė Merge sort funkcija
// Parametrai: a - masyvas; kair, des - ribos; skaitikliai per nuorodą
static void mergeSortRek(vector<int> &a, int kair, int des,
                         uint64_t &palyginimai, uint64_t &priskyrimai)
{
    if (kair < des)
    {                                      // rekursijos sąlyga
        int vid = kair + (des - kair) / 2; // vidurio indeksas (saugu nuo overflow)
        // Rekursyviai rikiuojame kairę pusę
        mergeSortRek(a, kair, vid, palyginimai, priskyrimai);
        // Rekursyviai rikiuojame dešinę pusę
        mergeSortRek(a, vid + 1, des, palyginimai, priskyrimai);
        // Suliejame abi surikiuotas puses
        merge(a, kair, vid, des, palyginimai, priskyrimai);
    }
}

// Funkcija: sąlajinio rikiavimo (Merge sort) viršutinis lygis
// Parametrai: a - vektorius (modifikuojamas vietoje);
//             palyginimai - palyginimų skaitiklis;
//             priskyrimai - priskyrimų (judesių) skaitiklis (atstoja sukeitimus)
// Grąžina: nieko, masyvas surikiuojamas vietoje
void mergeSort(vector<int> &a, uint64_t &palyginimai, uint64_t &priskyrimai)
{
    palyginimai = 0; // pradiniai skaitikliai
    priskyrimai = 0;
    if (a.size() < 2)
        return; // jau "surikiuotas"
    mergeSortRek(a, 0, static_cast<int>(a.size()) - 1, palyginimai, priskyrimai);
}

// ===== Eksperimento vykdymas =====

// Funkcija: paleidžia vieną rikiavimą, matuoja laiką ir skaitiklius
// Parametrai: pradinis - originalūs duomenys; algoritmas - "SelectionSort" arba "MergeSort";
//             palyginimai, sukeitimai - skaitikliai per nuorodą
// Grąžina: laiką mikrosekundėmis (long long)
long long paleistiRikiavima(const vector<int> &pradinis, const string &algoritmas,
                            uint64_t &palyginimai, uint64_t &sukeitimai)
{
    // Sukuriame masyvo kopiją, kad originalūs duomenys liktų nepakitę
    vector<int> kopija = pradinis;

    // Fiksuojame pradžios laiką (tik rikiavimo dalis)
    auto pradzia = high_resolution_clock::now();

    if (algoritmas == "SelectionSort")
        selectionSort(kopija, palyginimai, sukeitimai);
    else if (algoritmas == "MergeSort")
        mergeSort(kopija, palyginimai, sukeitimai);

    // Fiksuojame pabaigos laiką
    auto pabaiga = high_resolution_clock::now();

    // Apskaičiuojame trukmę mikrosekundėmis
    return duration_cast<microseconds>(pabaiga - pradzia).count();
}

// ===== Rezultatų išvedimas =====

// Funkcija: surašo visus bandymų rezultatus į CSV failą
// Parametrai: rezultatai - visų bandymų vektorius; failoVardas - išvesties failas
void rasytiRawCSV(const vector<RunResult> &rezultatai, const string &failoVardas)
{
    ofstream f(failoVardas); // atidarome failą rašymui
    // Antraštė (tik ASCII simboliai, kableliais atskirti stulpeliai)
    f << "Dydis,Tipas,Algoritmas,Bandymas,Laikas_mikros,Palyginimai,Sukeitimai\n";
    for (const auto &r : rezultatai)
    {
        f << r.dydis << ','
          << r.tipas << ','
          << r.algoritmas << ','
          << r.bandymas << ','
          << r.laikas_mikros << ','
          << r.palyginimai << ','
          << r.sukeitimai << '\n';
    }
    f.close();
}

// Struktūra vidurkiams saugoti (pagal dydis+tipas+algoritmas)
struct AvgKey
{
    int dydis;
    string tipas;
    string algoritmas;
};

// Funkcija: apskaičiuoja vidurkius ir įrašo į CSV; taip pat grąžina vektorių išvesties į ekraną
struct AvgResult
{
    int dydis;
    string tipas;
    string algoritmas;
    double vidLaikas;
    double vidPalyginimai;
    double vidSukeitimai;
};

vector<AvgResult> apskaiciuotiVidurkius(const vector<RunResult> &rezultatai)
{
    vector<AvgResult> vid; // vidurkių sąrašas
    // Iteruojame per visus rezultatus ir grupuojame
    for (const auto &r : rezultatai)
    {
        // Ieškome ar jau yra įrašas su tokia kombinacija
        bool rastas = false;
        for (auto &a : vid)
        {
            if (a.dydis == r.dydis && a.tipas == r.tipas && a.algoritmas == r.algoritmas)
            {
                a.vidLaikas += r.laikas_mikros;
                a.vidPalyginimai += r.palyginimai;
                a.vidSukeitimai += r.sukeitimai;
                rastas = true;
                break;
            }
        }
        if (!rastas)
        {
            vid.push_back({r.dydis, r.tipas, r.algoritmas,
                           (double)r.laikas_mikros,
                           (double)r.palyginimai,
                           (double)r.sukeitimai});
        }
    }
    // Suskaičiuojame, kiek bandymų vienam grupavimui (priklauso nuo BANDYMU_SK)
    // Dalijame iš bandymų skaičiaus, kurį nustatysime apskaičiuodami
    // Patikslinsime: suskaičiuojame, kiek kartų kiekviena kombinacija pasitaikė
    for (auto &a : vid)
    {
        int kiek = 0;
        for (const auto &r : rezultatai)
        {
            if (a.dydis == r.dydis && a.tipas == r.tipas && a.algoritmas == r.algoritmas)
                ++kiek;
        }
        if (kiek > 0)
        {
            a.vidLaikas /= kiek;
            a.vidPalyginimai /= kiek;
            a.vidSukeitimai /= kiek;
        }
    }
    return vid;
}

// Funkcija: įrašo vidurkius į CSV failą
void rasytiAvgCSV(const vector<AvgResult> &vid, const string &failoVardas)
{
    ofstream f(failoVardas);
    f << "Dydis,Tipas,Algoritmas,VidLaikas_mikros,VidPalyginimai,VidSukeitimai\n";
    f << fixed << setprecision(2);
    for (const auto &a : vid)
    {
        f << a.dydis << ','
          << a.tipas << ','
          << a.algoritmas << ','
          << a.vidLaikas << ','
          << a.vidPalyginimai << ','
          << a.vidSukeitimai << '\n';
    }
    f.close();
}

// Funkcija: išveda vidurkių lentelę į ekraną
void spausdintiLentele(const vector<AvgResult> &vid)
{
    cout << "\n========================= REZULTATU LENTELE (vidurkiai) =========================\n";
    cout << left
         << setw(8) << "Dydis"
         << setw(18) << "Tipas"
         << setw(16) << "Algoritmas"
         << right
         << setw(16) << "Laikas (mikros)"
         << setw(18) << "Palyginimai"
         << setw(16) << "Sukeit/Prisk"
         << "\n";
    cout << string(92, '-') << "\n";
    cout << fixed << setprecision(2);
    for (const auto &a : vid)
    {
        cout << left
             << setw(8) << a.dydis
             << setw(18) << a.tipas
             << setw(16) << a.algoritmas
             << right
             << setw(16) << a.vidLaikas
             << setw(18) << a.vidPalyginimai
             << setw(16) << a.vidSukeitimai
             << "\n";
    }
    cout << string(92, '-') << "\n";
}

// ===== Pagrindinė funkcija (main) =====

int main()
{
    // Eksperimento parametrai
    const vector<int> dydziai = {5000, 10000, 50000};                              // testuojami dydžiai
    const vector<string> tipai = {"atsitiktiniai", "atvirkstiniai", "surikiuoti"}; // duomenų tipai
    const vector<string> algoritmai = {"SelectionSort", "MergeSort"};              // algoritmai
    const int BANDYMU_SK = 5;                                                      // pakartojimų skaičius

    // ===== Duomenų generavimas =====
    cout << "===== Duomenu generavimas =====\n";
    // Atidarome debug failą generavimo laikams fiksuoti
    ofstream dbg("debug_generavimas.txt");
    dbg << "Failas;Generavimo_laikas_mikros\n";

    for (int dydis : dydziai)
    {
        for (const string &tipas : tipai)
        {
            string failoVardas = "duomenys_" + to_string(dydis) + "_" + tipas + ".txt";
            cout << "Generuojama: " << failoVardas << "\n";

            // Matuojame generavimo (ir įrašymo) laiką
            auto t1 = high_resolution_clock::now();
            generuotiFaila(dydis, tipas, failoVardas);
            auto t2 = high_resolution_clock::now();
            long long laikas = duration_cast<microseconds>(t2 - t1).count();

            // Įrašome į debug failą
            dbg << failoVardas << ';' << laikas << '\n';
        }
    }
    dbg.close();

    // ===== Eksperimento vykdymas =====
    cout << "\n===== Eksperimento vykdymas =====\n";
    vector<RunResult> visiRezultatai; // čia kaupsime visus bandymus

    // Debug failas rikiavimo laikams (kiekvienas bandymas)
    ofstream dbgRik("debug_rikiavimas.txt");
    dbgRik << "Dydis;Tipas;Algoritmas;Bandymas;Laikas_mikros;Palyginimai;Sukeitimai\n";

    for (int dydis : dydziai)
    {
        for (const string &tipas : tipai)
        {
            // Sudarome failo vardą ir nuskaitome duomenis
            string failoVardas = "duomenys_" + to_string(dydis) + "_" + tipas + ".txt";
            vector<int> pradiniai = skaitytiFaila(failoVardas);
            if ((int)pradiniai.size() != dydis)
            {
                cerr << "Ispejimas: faile " << failoVardas
                     << " yra " << pradiniai.size() << " elementu, tikejomes " << dydis << "\n";
            }
            cout << "Testuojama: dydis=" << dydis << ", tipas=" << tipas << "\n";

            for (const string &alg : algoritmai)
            {
                for (int b = 1; b <= BANDYMU_SK; ++b)
                {
                    uint64_t palyg = 0, sukeit = 0;
                    long long laikas = paleistiRikiavima(pradiniai, alg, palyg, sukeit);

                    // Saugojame rezultatą
                    RunResult r{dydis, tipas, alg, b, laikas, palyg, sukeit};
                    visiRezultatai.push_back(r);

                    // Debug įrašas
                    dbgRik << dydis << ';' << tipas << ';' << alg << ';' << b << ';'
                           << laikas << ';' << palyg << ';' << sukeit << '\n';

                    cout << "  " << alg << " bandymas " << b
                         << ": laikas=" << laikas << " mikros, palyg=" << palyg
                         << ", sukeit/prisk=" << sukeit << "\n";
                }
            }
        }
    }
    dbgRik.close();

    // ===== Rezultatų išvedimas =====
    cout << "\n===== Rezultatu issaugojimas =====\n";
    rasytiRawCSV(visiRezultatai, "rezultatai_raw.csv");
    cout << "Sukurtas failas: rezultatai_raw.csv\n";

    vector<AvgResult> vidurkiai = apskaiciuotiVidurkius(visiRezultatai);
    rasytiAvgCSV(vidurkiai, "rezultatai_avg.csv");
    cout << "Sukurtas failas: rezultatai_avg.csv\n";

    // Išvedame lentelę ekrane
    spausdintiLentele(vidurkiai);

    cout << "\nPASTABA: MergeSort 'Sukeit/Prisk' stulpelyje rodomas PRISKYRIMU skaicius,\n"
         << "nes saljinis rikiavimas tiesiogiai elementu nesukeicia.\n";
    cout << "\nDarbas baigtas sekmingai.\n";
    return 0;
}
