// ============================================================
//  Rikiavimo algoritmų lyginamoji analizė: Selection sort vs Merge sort
//  Autorius: sugeneruota Lovable
//  Standartas: C++17
//  Kompiliavimas: g++ -std=c++17 -Wall -Wextra -O2 rikiavimo_analize.cpp -o rikiavimo_analize
// ============================================================

#include <iostream>  // įvedimo/išvedimo srautams (cin, cout)
#include <fstream>   // failų skaitymui ir rašymui (ifstream, ofstream)
#include <vector>    // dinaminiam masyvui std::vector
#include <chrono>    // tiksliam laiko matavimui (high_resolution_clock)
#include <random>    // atsitiktinių skaičių generavimui (mt19937)
#include <iomanip>   // išvedimo formatavimui (setw, fixed, setprecision)
#include <string>    // std::string tipui
#include <algorithm> // std::swap funkcijai
#include <map>       // vidurkių grupavimui pagal raktą
#include <locale>    // norint priversti CSV naudoti taško skirtuką (klasikinis locale)

// ===== Globalūs skaitikliai =====
// Naudojami norint suskaičiuoti palyginimus ir judesius (sukeitimus/priskyrimus)
// rikiavimo metu. Prieš kiekvieną matavimą juos atstatome į 0.
static long long g_palyginimai = 0; // bendras palyginimų skaičius einamame rikiavime
static long long g_sukeitimai = 0;  // sukeitimų (selection) arba priskyrimų (merge) skaičius

// ===== Duomenų struktūra rezultatams =====
// Viena struktūra atitinka vieną atskirą bandymą (vieną algoritmo paleidimą).
struct RunResult
{
    int dydis;              // duomenų masyvo dydis (5000, 10000, 50000)
    std::string tipas;      // duomenų tipas: "atsitiktiniai", "atvirkstiniai", "surikiuoti"
    std::string algoritmas; // algoritmo pavadinimas: "SelectionSort" arba "MergeSort"
    int bandymas;           // bandymo numeris (1..N)
    long long laikas_us;    // bandymo trukmė mikrosekundėmis
    long long palyginimai;  // palyginimų skaičius
    long long sukeitimai;   // sukeitimų (selection) arba priskyrimų (merge) skaičius
};

// ============================================================
// ===== Duomenų generavimas =====
// ============================================================

// Funkcija sukuria failą su nurodyto dydžio ir tipo duomenimis,
// jei toks failas dar neegzistuoja. Jei egzistuoja – paliekamas nepakeistas,
// kad pakartotinai paleidžiant programą rezultatai būtų palyginami.
// Parametrai:
//   dydis – kiek skaičių sugeneruoti
//   tipas – "atsitiktiniai", "atvirkstiniai" arba "surikiuoti"
// Grąžina: failo pavadinimą (string).
std::string sugeneruotiDuomenuFaila(int dydis, const std::string &tipas)
{
    // Sudarome failo pavadinimą pagal duotą formatą
    std::string failoVardas = "duomenys_" + std::to_string(dydis) + "_" + tipas + ".txt";

    // Patikriname, ar failas jau egzistuoja – jei taip, nieko nedarome
    {
        std::ifstream tikrinimas(failoVardas);
        if (tikrinimas.good())
        {
            return failoVardas; // failas jau yra, naudosime esamą
        }
    }

    // Atidarome failą rašymui
    std::ofstream isvedimas(failoVardas);
    if (!isvedimas.is_open())
    {
        std::cerr << "Klaida: nepavyko sukurti failo " << failoVardas << std::endl;
        return failoVardas;
    }

    if (tipas == "atsitiktiniai")
    {
        // Atsitiktinių skaičių generatorius su fiksuota sėkla, kad rezultatas būtų atkartojamas
        std::mt19937 generatorius(12345u + static_cast<unsigned>(dydis));
        std::uniform_int_distribution<int> pasiskirstymas(1, 1000000);
        for (int i = 0; i < dydis; ++i)
        {
            isvedimas << pasiskirstymas(generatorius);       // įrašome atsitiktinį skaičių
            isvedimas << (((i + 1) % 20 == 0) ? '\n' : ' '); // formatavimas: 20 sk. per eilutę
        }
    }
    else if (tipas == "atvirkstiniai")
    {
        // Skaičiai nuo dydžio iki 1 (mažėjimo tvarka) – blogiausias atvejis selection sortui
        for (int i = dydis; i >= 1; --i)
        {
            isvedimas << i;
            isvedimas << (((dydis - i + 1) % 20 == 0) ? '\n' : ' ');
        }
    }
    else if (tipas == "surikiuoti")
    {
        // Jau didėjančia tvarka surikiuoti skaičiai nuo 1 iki dydis
        for (int i = 1; i <= dydis; ++i)
        {
            isvedimas << i;
            isvedimas << ((i % 20 == 0) ? '\n' : ' ');
        }
    }

    isvedimas.close(); // uždarome failą
    return failoVardas;
}

// Nuskaito skaičius iš teksto failo į vector<int>.
// Parametras: failo vardas. Grąžina: vektorių su skaičiais.
std::vector<int> nuskaitytiDuomenis(const std::string &failoVardas)
{
    std::vector<int> duomenys;           // čia kaupsime nuskaitytus skaičius
    std::ifstream ivedimas(failoVardas); // atidarome failą skaitymui
    if (!ivedimas.is_open())
    {
        std::cerr << "Klaida: nepavyko atidaryti failo " << failoVardas << std::endl;
        return duomenys;
    }
    int skaicius;
    while (ivedimas >> skaicius)
    { // skaitome po vieną sveiką skaičių iki failo pabaigos
        duomenys.push_back(skaicius);
    }
    return duomenys;
}

// ============================================================
// ===== Rikiavimo algoritmai =====
// ============================================================

// ----- Selection sort -----
// Funkcija rikiuoja vektorių didėjimo tvarka išrinkimo metodu.
// Globaliuose skaitikliuose kaupiami palyginimų ir sukeitimų skaičiai.
// Parametras: nuoroda į vector<int>, kuris bus rikiuojamas vietoje.
void selectionSort(std::vector<int> &a)
{
    const int n = static_cast<int>(a.size()); // masyvo dydis
    // Pagrindinis ciklas: ieškome mažiausio elemento likusioje dalyje
    for (int i = 0; i < n - 1; ++i)
    {
        int minIndeksas = i; // pradžioje laikome, kad mažiausias yra i-tasis
        // Vidinis ciklas – ieškome tikrojo mažiausio elemento [i+1..n-1] intervale
        for (int j = i + 1; j < n; ++j)
        {
            ++g_palyginimai; // skaičiuojame palyginimą
            if (a[j] < a[minIndeksas])
            { // jei radome mažesnį – atnaujiname indeksą
                minIndeksas = j;
            }
        }
        // Jeigu mažiausias elementas nėra dabartinėje pozicijoje – sukeičiame
        if (minIndeksas != i)
        {
            std::swap(a[i], a[minIndeksas]); // realus sukeitimas
            ++g_sukeitimai;                  // skaičiuojame sukeitimą
        }
    }
}

// ----- Merge sort -----
// Pagalbinė funkcija – sulieja du jau surikiuotus posekius a[l..m] ir a[m+1..r].
// Kadangi merge sort tiesioginių sukeitimų neturi,
// kaip "judesius" skaičiuojame priskyrimus (elementų kopijavimus).
void merge(std::vector<int> &a, int l, int m, int r)
{
    const int n1 = m - l + 1; // kairiojo posekio ilgis
    const int n2 = r - m;     // dešiniojo posekio ilgis

    // Sukuriame laikinus vektorius su abiem posekiais
    std::vector<int> kaire(n1), desine(n2);

    // Nukopijuojame duomenis į laikinus vektorius – tai irgi priskyrimai
    for (int i = 0; i < n1; ++i)
    {
        kaire[i] = a[l + i];
        ++g_sukeitimai; // priskyrimas (kopijavimas) – traktuojame kaip "judesį"
    }
    for (int j = 0; j < n2; ++j)
    {
        desine[j] = a[m + 1 + j];
        ++g_sukeitimai;
    }

    int i = 0, j = 0, k = l; // i – kairės, j – dešinės, k – rezultato indeksas
    // Suliejame du surikiuotus posekius atgal į a[l..r]
    while (i < n1 && j < n2)
    {
        ++g_palyginimai; // palyginimas tarp dviejų posekių elementų
        if (kaire[i] <= desine[j])
        {
            a[k] = kaire[i]; // įrašome mažesnįjį iš kairės
            ++g_sukeitimai;  // priskyrimas
            ++i;
        }
        else
        {
            a[k] = desine[j]; // įrašome mažesnįjį iš dešinės
            ++g_sukeitimai;
            ++j;
        }
        ++k;
    }
    // Įkeliame likusius kairės posekio elementus (jei tokių liko)
    while (i < n1)
    {
        a[k] = kaire[i];
        ++g_sukeitimai;
        ++i;
        ++k;
    }
    // Įkeliame likusius dešinės posekio elementus (jei tokių liko)
    while (j < n2)
    {
        a[k] = desine[j];
        ++g_sukeitimai;
        ++j;
        ++k;
    }
}

// Rekursinė merge sort funkcija – rikiuoja a[l..r] intervalą.
void mergeSortRek(std::vector<int> &a, int l, int r)
{
    if (l < r)
    {
        int m = l + (r - l) / 2;   // vidurio indeksas (saugiai nuo perpildymo)
        mergeSortRek(a, l, m);     // rikiuojame kairę pusę
        mergeSortRek(a, m + 1, r); // rikiuojame dešinę pusę
        merge(a, l, m, r);         // suliejame dvi surikiuotas puses
    }
}

// Patogus įvyniojimas – iškviečia mergeSortRek visam vektoriui.
void mergeSort(std::vector<int> &a)
{
    if (!a.empty())
    {
        mergeSortRek(a, 0, static_cast<int>(a.size()) - 1);
    }
}

// ============================================================
// ===== Eksperimento vykdymas =====
// ============================================================

// Paleidžia vieną rikiavimo bandymą ir užfiksuoja rezultatą RunResult struktūroje.
// Parametrai:
//   originalas – pradinių duomenų kopija (nekeičiama)
//   algoritmas – "SelectionSort" arba "MergeSort"
//   dydis, tipas, bandymas – metainformacija įrašymui
RunResult paleistiBandyma(const std::vector<int> &originalas,
                          const std::string &algoritmas,
                          int dydis,
                          const std::string &tipas,
                          int bandymas)
{
    // Sukuriame DARBINĘ kopiją, kad originalas nesikeistų ir kiti algoritmai
    // gautų identišką pradinę būseną.
    std::vector<int> darbinis = originalas;

    // Atstatome globalius skaitiklius prieš matavimą
    g_palyginimai = 0;
    g_sukeitimai = 0;

    // Pradedame matuoti laiką tik prieš rikiavimą (be kopijavimo!)
    auto pradzia = std::chrono::high_resolution_clock::now();

    if (algoritmas == "SelectionSort")
    {
        selectionSort(darbinis); // vykdome išrinkimo rikiavimą
    }
    else
    {
        mergeSort(darbinis); // vykdome sąlajinį rikiavimą
    }

    // Sustabdome laikrodį iškart po rikiavimo
    auto pabaiga = std::chrono::high_resolution_clock::now();

    // Apskaičiuojame trukmę mikrosekundėmis
    long long trukme = std::chrono::duration_cast<std::chrono::microseconds>(pabaiga - pradzia).count();

    // Užpildome ir grąžiname rezultato struktūrą
    RunResult r;
    r.dydis = dydis;
    r.tipas = tipas;
    r.algoritmas = algoritmas;
    r.bandymas = bandymas;
    r.laikas_us = trukme;
    r.palyginimai = g_palyginimai;
    r.sukeitimai = g_sukeitimai;
    return r;
}

// ============================================================
// ===== Rezultatų išvedimas =====
// ============================================================

// Įrašo visus bandymus į CSV failą "rezultatai_raw.csv".
void irasytiRawCSV(const std::vector<RunResult> &rezultatai)
{
    std::ofstream f("rezultatai_raw.csv");
    f.imbue(std::locale::classic()); // priverčiame klasikinį locale (taškas kaip skirtukas)
    // ASCII antraštė
    f << "Dydis,Tipas,Algoritmas,Bandymas,Laikas_mikros,Palyginimai,Sukeitimai\n";
    for (const auto &r : rezultatai)
    {
        f << r.dydis << ',' << r.tipas << ',' << r.algoritmas << ','
          << r.bandymas << ',' << r.laikas_us << ','
          << r.palyginimai << ',' << r.sukeitimai << '\n';
    }
    f.close();
}

// Apskaičiuoja vidurkius pagal (dydis, tipas, algoritmas) ir įrašo į "rezultatai_avg.csv".
// Tuo pat metu išveda gražią lentelę į ekraną.
void apskaiciuotiIrIsvestiVidurkius(const std::vector<RunResult> &rezultatai)
{
    // Raktas: "dydis|tipas|algoritmas" -> sumos ir bandymų skaičius
    struct Suma
    {
        long long laikas = 0;
        long long palyg = 0;
        long long suk = 0;
        int n = 0;
        int dydis = 0;
        std::string tipas;
        std::string algoritmas;
    };
    std::map<std::string, Suma> sumos;

    for (const auto &r : rezultatai)
    {
        std::string raktas = std::to_string(r.dydis) + "|" + r.tipas + "|" + r.algoritmas;
        auto &s = sumos[raktas];
        s.laikas += r.laikas_us;
        s.palyg += r.palyginimai;
        s.suk += r.sukeitimai;
        s.n += 1;
        s.dydis = r.dydis;
        s.tipas = r.tipas;
        s.algoritmas = r.algoritmas;
    }

    // ----- CSV su vidurkiais -----
    std::ofstream f("rezultatai_avg.csv");
    f.imbue(std::locale::classic());
    f << "Dydis,Tipas,Algoritmas,VidLaikas_mikros,VidPalyginimai,VidSukeitimai\n";

    // ----- Ekrano lentelė -----
    std::cout << "\n===== Vidutiniai rezultatai =====\n";
    std::cout << std::left
              << std::setw(8) << "Dydis"
              << std::setw(16) << "Tipas"
              << std::setw(16) << "Algoritmas"
              << std::setw(16) << "VidLaikas(us)"
              << std::setw(18) << "VidPalyginimai"
              << std::setw(18) << "VidSukeitimai"
              << "\n";
    std::cout << std::string(92, '-') << "\n";

    std::cout.imbue(std::locale::classic());
    for (const auto &[raktas, s] : sumos)
    {
        double vidL = static_cast<double>(s.laikas) / s.n;
        double vidP = static_cast<double>(s.palyg) / s.n;
        double vidS = static_cast<double>(s.suk) / s.n;

        // CSV eilutė
        f << s.dydis << ',' << s.tipas << ',' << s.algoritmas << ','
          << std::fixed << std::setprecision(2) << vidL << ','
          << std::fixed << std::setprecision(2) << vidP << ','
          << std::fixed << std::setprecision(2) << vidS << '\n';

        // Ekrano eilutė
        std::cout << std::left
                  << std::setw(8) << s.dydis
                  << std::setw(16) << s.tipas
                  << std::setw(16) << s.algoritmas
                  << std::setw(16) << std::fixed << std::setprecision(1) << vidL
                  << std::setw(18) << std::fixed << std::setprecision(1) << vidP
                  << std::setw(18) << std::fixed << std::setprecision(1) << vidS
                  << "\n";
    }
    f.close();
}

// ============================================================
// ===== Pagrindinė programa =====
// ============================================================
int main()
{
    // Eksperimento parametrai
    const std::vector<int> dydziai = {5000, 10000, 50000}; // tiriami dydžiai
    const std::vector<std::string> tipai = {"atsitiktiniai", "atvirkstiniai", "surikiuoti"};
    const std::vector<std::string> algoritmai = {"SelectionSort", "MergeSort"};
    const int bandymuSkaicius = 5; // pakartojimų sk.

    std::vector<RunResult> visiRezultatai; // čia kaupsime VISUS bandymus
    visiRezultatai.reserve(dydziai.size() * tipai.size() * algoritmai.size() * bandymuSkaicius);

    std::cout << "Generuojami / nuskaitomi duomenu failai...\n";

    // ----- Žingsnis 1: sugeneruojame visus reikiamus duomenų failus -----
    for (int d : dydziai)
    {
        for (const auto &t : tipai)
        {
            std::string f = sugeneruotiDuomenuFaila(d, t);
            std::cout << "  Paruosta: " << f << "\n";
        }
    }

    std::cout << "\nVykdomas eksperimentas (po " << bandymuSkaicius << " bandymus kiekvienai kombinacijai)...\n";

    // ----- Žingsnis 2: vykdome eksperimentą -----
    for (int d : dydziai)
    {
        for (const auto &t : tipai)
        {
            // Nuskaitome duomenų failą tik vieną kartą šiai kombinacijai
            std::string failas = "duomenys_" + std::to_string(d) + "_" + t + ".txt";
            std::vector<int> originalas = nuskaitytiDuomenis(failas);

            for (const auto &alg : algoritmai)
            {
                for (int b = 1; b <= bandymuSkaicius; ++b)
                {
                    // Kiekvienas bandymas naudoja TUOS PAČIUS pradinius duomenis,
                    // o "paleistiBandyma" viduje sukuriama nauja darbinė kopija.
                    RunResult r = paleistiBandyma(originalas, alg, d, t, b);
                    visiRezultatai.push_back(r);

                    std::cout << "  [" << alg << "] dydis=" << d
                              << " tipas=" << t
                              << " bandymas=" << b
                              << " laikas=" << r.laikas_us << "us"
                              << " palyg=" << r.palyginimai
                              << " suk/prisk=" << r.sukeitimai
                              << "\n";
                }
            }
        }
    }

    // ----- Žingsnis 3: išvedame rezultatus -----
    irasytiRawCSV(visiRezultatai);                  // visi bandymai į CSV
    apskaiciuotiIrIsvestiVidurkius(visiRezultatai); // vidurkiai į CSV ir į ekraną

    std::cout << "\nRezultatai issaugoti i failus: rezultatai_raw.csv, rezultatai_avg.csv\n";
    std::cout << "Pastaba: MergeSort stulpelyje 'Sukeitimai' nurodytas priskyrimu (kopijavimu) skaicius,\n";
    std::cout << "         nes salajinis rikiavimas tiesiogiai elementu nesukeicia.\n";

    return 0;
}
