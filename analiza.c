#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//CONFIGURARE 
#define FISIER_CSV   "sales.csv"  // numele fisierului tau     
#define SEPARATOR    ','          // virgula                   
#define MAX_LINII    110000       // > 100 000 randuri         
#define MAX_STR      128
#define MAX_CAMP     10
#define TOP_N        5

//STRUCTURI DE DATE
typedef struct {
    char   sale_date[MAX_STR];
    char   product_id[MAX_STR];
    char   product_name[MAX_STR];
    char   product_category[MAX_STR];
    char   product_subcategory[MAX_STR];
    double unit_price;
    int    quantity_sold;
    char   sale_country[MAX_STR];
    char   sale_city[MAX_STR];
    // campuri calculate
    double venit;    // unit_price * quantity_sold 
    int    luna;     // 1-12                        
    int    an;       // ex: 2024                    
} Tranzactie;

// Structura generica pentru agregare cheie -> valoare
typedef struct {
    char   cheie[MAX_STR];
    double valoare;
    int    contor;
} Agregat;

// Structura pentru analiza orase 
typedef struct {
    char   tara[MAX_STR];
    char   oras[MAX_STR];
    double venit;
    int    contor;
} OrasVanzare;

// DATE GLOBALE
static Tranzactie *date = NULL;   // alocat dinamic - 100k inregistrari
static int         nr_tranzactii = 0;

// UTILITARE - PARSARE CSV


// Elimina ghilimele si spatii de la capete
static void curata(char *s) {
    int len = (int)strlen(s);
    while (len > 0 && (s[len-1] == '"' || s[len-1] == '\r' ||
                        s[len-1] == '\n' || s[len-1] == ' '))
        s[--len] = '\0';
    if (len > 0 && s[0] == '"') {
        memmove(s, s+1, len);
        len--;
        if (len > 0 && s[len-1] == '"') s[--len] = '\0';
    }
}

// Desparte o linie CSV in campuri; returneaza nr de campuri
static int split_csv(char *linie, char out[][MAX_STR], int max_out) {
    int idx = 0, ci = 0, in_gh = 0;
    while (*linie && idx < max_out) {
        if (*linie == '"')             { in_gh = !in_gh; }
        else if (*linie == SEPARATOR && !in_gh) {
            out[idx][ci] = '\0'; curata(out[idx]); idx++; ci = 0;
        } else if (ci < MAX_STR - 1)  { out[idx][ci++] = *linie; }
        linie++;
    }
    out[idx][ci] = '\0'; curata(out[idx]);
    return idx + 1;
}

// Extrage luna si anul din "YYYY-MM-DD"
static void extrage_data(const char *d, int *luna, int *an) {
    *luna = 0; *an = 0;
    if (strlen(d) >= 10 && d[4] == '-') {
        *an   = atoi(d);          /* primele 4 cifre */
        *luna = atoi(d + 5);      /* dupa primul '-' */
    }
    // Suport si pentru DD/MM/YYYY
    else if (strlen(d) >= 10 && (d[2] == '/' || d[2] == '-')) {
        *luna = atoi(d + 3);
        *an   = atoi(d + 6);
    }
}

// INCARCARE DATE
static int incarca_csv(const char *fisier) {
    FILE *f = fopen(fisier, "r");
    if (!f) {
        printf("\n  [EROARE] Nu pot deschide fisierul: \"%s\"\n", fisier);
        printf("  Solutii:\n");
        printf("  1. Copiaza sales.csv in folderul Debug\\ al proiectului\n");
        printf("  2. Sau schimba FISIER_CSV cu calea completa,\n");
        printf("     ex: \"C:\\\\Users\\\\NumeTau\\\\Desktop\\\\sales.csv\"\n\n");
        return 0;
    }

    // Aloca memorie pentru 110000 inregistrari
    date = (Tranzactie *)malloc(MAX_LINII * sizeof(Tranzactie));
    if (!date) { printf("  [EROARE] Memorie insuficienta.\n"); fclose(f); return 0; }

    char linie[2048];
    char camp[MAX_CAMP][MAX_STR];
    int  prima_linie = 1;

    while (fgets(linie, sizeof(linie), f) && nr_tranzactii < MAX_LINII) {
        if (prima_linie) { prima_linie = 0; continue; }  /* sari antetul */
        if (strlen(linie) < 5) continue;

        int n = split_csv(linie, camp, MAX_CAMP);
        if (n < 9) continue;

        Tranzactie *t = &date[nr_tranzactii];
        strncpy(t->sale_date,            camp[0], MAX_STR-1);
        strncpy(t->product_id,           camp[1], MAX_STR-1);
        strncpy(t->product_name,         camp[2], MAX_STR-1);
        strncpy(t->product_category,     camp[3], MAX_STR-1);
        strncpy(t->product_subcategory,  camp[4], MAX_STR-1);
        t->unit_price    = atof(camp[5]);
        t->quantity_sold = atoi(camp[6]);
        strncpy(t->sale_country, camp[7], MAX_STR-1);
        strncpy(t->sale_city,    camp[8], MAX_STR-1);

        t->venit = t->unit_price * t->quantity_sold;
        extrage_data(t->sale_date, &t->luna, &t->an);

        nr_tranzactii++;
    }

    fclose(f);
    return nr_tranzactii;
}

// AFISARE - ELEMENTE DE INTERFATA
static const char *LUNI_RO[] = {
    "", "Ian", "Feb", "Mar", "Apr", "Mai", "Iun",
    "Iul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static void linie_h(int n) {
    printf("  +");
    for (int i = 0; i < n; i++) printf("-");
    printf("+\n");
}

static void titlu_sectiune(const char *titlu) {
    int l   = (int)strlen(titlu);
    int pad = (54 - l) / 2;
    if (pad < 0) pad = 0;
    printf("\n  +========================================================+\n");
    printf("  |%*s%s%*s|\n", pad, "", titlu, 54 - pad - l, "");
    printf("  +========================================================+\n\n");
}

// Grafic bara orizontal ASCII
static void bara(double val, double max_val, int latime) {
    int n = (max_val > 0) ? (int)(val / max_val * latime) : 0;
    if (n > latime) n = latime;
    printf("[");
    for (int i = 0; i < n;      i++) printf("#");
    for (int i = n; i < latime; i++) printf(" ");
    printf("]");
}

// AGREGARE GENERICA
static int cmp_desc(const void *a, const void *b) {
    double da = ((const Agregat *)a)->valoare;
    double db = ((const Agregat *)b)->valoare;
    return (db > da) - (db < da);
}

static void agreg_adauga(Agregat *arr, int *n, int max,
                          const char *cheie, double val) {
    for (int i = 0; i < *n; i++) {
        if (strcmp(arr[i].cheie, cheie) == 0) {
            arr[i].valoare += val;
            arr[i].contor++;
            return;
        }
    }
    if (*n < max) {
        strncpy(arr[*n].cheie, cheie, MAX_STR-1);
        arr[*n].valoare = val;
        arr[*n].contor  = 1;
        (*n)++;
    }
}

/* ================================================================
   ANALIZA 1 - VENITURI LUNARE
   (raspunde la: "Care este venitul total generat in fiecare luna?")
   ================================================================ */

static void analiza_1_lunara(void) {
    titlu_sectiune("ANALIZA 1: VENITURI TOTALE PE LUNI");

    /* Venituri agregate pe fiecare an-luna */
    typedef struct { int an; int luna; double venit; int contor; } LunaAn;
    LunaAn luni[500]; int n_luni = 0;
    double total_general = 0;

    for (int i = 0; i < nr_tranzactii; i++) {
        int a = date[i].an, l = date[i].luna;
        if (a <= 0 || l < 1 || l > 12) continue;

        int gasit = 0;
        for (int j = 0; j < n_luni; j++) {
            if (luni[j].an == a && luni[j].luna == l) {
                luni[j].venit  += date[i].venit;
                luni[j].contor++;
                gasit = 1; break;
            }
        }
        if (!gasit && n_luni < 500) {
            luni[n_luni].an     = a;
            luni[n_luni].luna   = l;
            luni[n_luni].venit  = date[i].venit;
            luni[n_luni].contor = 1;
            n_luni++;
        }
        total_general += date[i].venit;
    }

    // Sortam cronologic
    for (int a = 0; a < n_luni-1; a++)
        for (int b = 0; b < n_luni-a-1; b++)
            if (luni[b].an > luni[b+1].an ||
               (luni[b].an == luni[b+1].an && luni[b].luna > luni[b+1].luna)) {
                LunaAn tmp = luni[b]; luni[b] = luni[b+1]; luni[b+1] = tmp;
            }

    double max_v = 0;
    for (int j = 0; j < n_luni; j++) if (luni[j].venit > max_v) max_v = luni[j].venit;

    printf("  %-12s | %-15s | %-10s | %-8s | Grafic\n",
           "Luna", "Venit (USD)", "Tranzactii", "% Total");
    linie_h(72);

    for (int j = 0; j < n_luni; j++) {
        double pct = (total_general > 0) ? luni[j].venit / total_general * 100.0 : 0;
        char eticheta[20];
        snprintf(eticheta, sizeof(eticheta), "%s %d", LUNI_RO[luni[j].luna], luni[j].an);
        printf("  %-12s | %15.2f | %10d | %7.1f%% | ",
               eticheta, luni[j].venit, luni[j].contor, pct);
        bara(luni[j].venit, max_v, 18);
        printf("\n");
    }

    linie_h(72);
    printf("  %-12s | %15.2f | %10d | %7.1f%% |\n",
           "TOTAL", total_general, nr_tranzactii, 100.0);

    // Luna cu venit maxim
    int idx_max = 0;
    for (int j = 1; j < n_luni; j++) if (luni[j].venit > luni[idx_max].venit) idx_max = j;
    printf("\n  >> Luna cu venit maxim: %s %d  (%.2f USD)\n",
           LUNI_RO[luni[idx_max].luna], luni[idx_max].an, luni[idx_max].venit);
}

/* ================================================================
   ANALIZA 2 - TOP 5 PRODUSE
   (raspunde la: "Care sunt primele 5 produse dupa venit?")
   ================================================================ */

static void analiza_2_top_produse(void) {
    titlu_sectiune("ANALIZA 2: TOP 5 PRODUSE DUPA VENIT");

    Agregat produse[2000]; int n = 0; double total = 0;
    for (int i = 0; i < nr_tranzactii; i++) {
        agreg_adauga(produse, &n, 2000, date[i].product_name, date[i].venit);
        total += date[i].venit;
    }
    qsort(produse, n, sizeof(Agregat), cmp_desc);

    int    top   = (n < TOP_N) ? n : TOP_N;
    double max_v = (top > 0)   ? produse[0].valoare : 1.0;

    printf("  %-4s | %-28s | %-15s | %-8s | Tranz.\n",
           "Rang", "Produs", "Venit (USD)", "% Total");
    linie_h(75);

    for (int i = 0; i < top; i++) {
        double pct = (total > 0) ? produse[i].valoare / total * 100.0 : 0;
        printf("  #%-3d | %-28s | %15.2f | %7.2f%% | %d\n",
               i+1, produse[i].cheie, produse[i].valoare, pct, produse[i].contor);
    }
    linie_h(75);

    printf("\n  Grafic comparativ:\n\n");
    for (int i = 0; i < top; i++) {
        printf("  #%d %-24.24s ", i+1, produse[i].cheie);
        bara(produse[i].valoare, max_v, 28);
        printf(" %.2f\n", produse[i].valoare);
    }
}

/* ================================================================
   ANALIZA 3 - DISTRIBUTIE PE CATEGORII
   (raspunde la: "Cum se distribuie vanzarile pe categorii?")
   ================================================================ */

static void analiza_3_categorii(void) {
    titlu_sectiune("ANALIZA 3: DISTRIBUTIA PE CATEGORII");

    Agregat cat[100]; int n = 0; double total = 0;
    for (int i = 0; i < nr_tranzactii; i++) {
        agreg_adauga(cat, &n, 100, date[i].product_category, date[i].venit);
        total += date[i].venit;
    }
    qsort(cat, n, sizeof(Agregat), cmp_desc);

    printf("  %-22s | %-15s | %-8s | %-10s | Distributie\n",
           "Categorie", "Venit (USD)", "% Total", "Tranzactii");
    linie_h(80);

    for (int i = 0; i < n; i++) {
        double pct  = (total > 0) ? cat[i].valoare / total * 100.0 : 0;
        int    bare = (int)(pct / 2.5);
        printf("  %-22s | %15.2f | %7.1f%% | %10d | ",
               cat[i].cheie, cat[i].valoare, pct, cat[i].contor);
        for (int b = 0; b < bare; b++) printf("=");
        printf("\n");
    }

    linie_h(80);
    printf("  %-22s | %15.2f | %7.1f%% | %10d |\n",
           "TOTAL", total, 100.0, nr_tranzactii);
}

/* ================================================================
   ANALIZA 4 - ORASE PE TARA
   (raspunde la: "Care sunt orasele cu cele mai mari vanzari pe tara?")
   ================================================================ */

static void analiza_4_orase(void) {
    titlu_sectiune("ANALIZA 4: ORASE CU CELE MAI MARI VANZARI PE TARA");

    OrasVanzare *orase = (OrasVanzare *)malloc(5000 * sizeof(OrasVanzare));
    if (!orase) { printf("  [EROARE] Memorie.\n"); return; }
    int n_or = 0;

    for (int i = 0; i < nr_tranzactii; i++) {
        int gasit = 0;
        for (int j = 0; j < n_or; j++) {
            if (strcmp(orase[j].tara, date[i].sale_country) == 0 &&
                strcmp(orase[j].oras, date[i].sale_city) == 0) {
                orase[j].venit += date[i].venit;
                orase[j].contor++;
                gasit = 1; break;
            }
        }
        if (!gasit && n_or < 5000) {
            strncpy(orase[n_or].tara, date[i].sale_country, MAX_STR-1);
            strncpy(orase[n_or].oras, date[i].sale_city,    MAX_STR-1);
            orase[n_or].venit  = date[i].venit;
            orase[n_or].contor = 1;
            n_or++;
        }
    }

    // Tari unice 
    Agregat tari[500]; int n_t = 0;
    for (int i = 0; i < n_or; i++) {
        double v_tara = 0;
        for (int j = 0; j < n_or; j++)
            if (strcmp(orase[j].tara, orase[i].tara) == 0) v_tara += orase[j].venit;
        agreg_adauga(tari, &n_t, 500, orase[i].tara, 0);
    }
    // Recalculam totalul pe tara
    for (int t = 0; t < n_t; t++) {
        tari[t].valoare = 0;
        for (int j = 0; j < n_or; j++)
            if (strcmp(orase[j].tara, tari[t].cheie) == 0) tari[t].valoare += orase[j].venit;
    }
    qsort(tari, n_t, sizeof(Agregat), cmp_desc);

    int tari_afisate = (n_t < 10) ? n_t : 10;   // top 10 tari 

    printf("  %-22s | %-22s | %-14s | Tranz.\n",
           "Tara", "Oras (Top 3)", "Venit Oras");
    linie_h(70);

    for (int t = 0; t < tari_afisate; t++) {
        // Selectam orasele din tara 
        OrasVanzare loc[500]; int n_loc = 0;
        for (int i = 0; i < n_or; i++)
            if (strcmp(orase[i].tara, tari[t].cheie) == 0 && n_loc < 500)
                loc[n_loc++] = orase[i];

        // Sortare descrescatoare bubble sort
        for (int a = 0; a < n_loc-1; a++)
            for (int b = 0; b < n_loc-a-1; b++)
                if (loc[b].venit < loc[b+1].venit) {
                    OrasVanzare tmp = loc[b]; loc[b] = loc[b+1]; loc[b+1] = tmp;
                }

        int afisate = (n_loc < 3) ? n_loc : 3;
        for (int i = 0; i < afisate; i++) {
            printf("  %-22s | %-22s | %14.2f | %d%s\n",
                   (i == 0) ? tari[t].cheie : "",
                   loc[i].oras, loc[i].venit, loc[i].contor,
                   (i == 0) ? " <--TOP" : "");
        }
        linie_h(70);
    }

    if (n_t > 10)
        printf("  ... si alte %d tari. Mareste tari_afisate din cod pentru mai multe.\n", n_t - 10);

    free(orase);
}

/* ================================================================
   ANALIZA 5 - TENDINTE SUBCATEGORII
   (raspunde la: "Ce tendinte se observa in vanzarile lunare
                  pentru diferite subcategorii?")
   ================================================================ */

static void analiza_5_tendinte(void) {
    titlu_sectiune("ANALIZA 5: TENDINTE LUNARE PE SUBCATEGORII");

    // Subcategorii unice 
    char sub[200][MAX_STR]; int n_sub = 0;
    for (int i = 0; i < nr_tranzactii; i++) {
        int g = 0;
        for (int j = 0; j < n_sub; j++)
            if (strcmp(sub[j], date[i].product_subcategory) == 0) { g=1; break; }
        if (!g && n_sub < 200)
            strncpy(sub[n_sub++], date[i].product_subcategory, MAX_STR-1);
    }

    // Total per subcategorie pentru a alege top 5 
    double tot_sub[200]; memset(tot_sub, 0, sizeof(tot_sub));
    int    ord[200];
    for (int i = 0; i < n_sub; i++) ord[i] = i;
    for (int i = 0; i < nr_tranzactii; i++)
        for (int j = 0; j < n_sub; j++)
            if (strcmp(sub[j], date[i].product_subcategory) == 0) {
                tot_sub[j] += date[i].venit; break;
            }

    // Sortam subcategoriile dupa total desc 
    for (int a = 0; a < n_sub-1; a++)
        for (int b = 0; b < n_sub-a-1; b++)
            if (tot_sub[ord[b]] < tot_sub[ord[b+1]]) {
                int tmp = ord[b]; ord[b] = ord[b+1]; ord[b+1] = tmp;
            }

    int top_sub = (n_sub < 5) ? n_sub : 5;

    // Ani unici 
    int ani[50]; int n_ani = 0;
    for (int i = 0; i < nr_tranzactii; i++) {
        int a = date[i].an; if (a <= 0) continue;
        int g = 0;
        for (int j = 0; j < n_ani; j++) if (ani[j] == a) { g=1; break; }
        if (!g && n_ani < 50) ani[n_ani++] = a;
    }
    // Sortam anii
    for (int a = 0; a < n_ani-1; a++)
        for (int b = 0; b < n_ani-a-1; b++)
            if (ani[b] > ani[b+1]) { int t=ani[b]; ani[b]=ani[b+1]; ani[b+1]=t; }

    // Afisam tabel anual per subcategorie (mai lizibil decat lunar pentru multi ani)
    printf("  Venituri ANUALE (USD) - Top %d subcategorii:\n\n", top_sub);
    printf("  %-22s |", "Subcategorie");
    for (int a = 0; a < n_ani; a++) printf(" %-10d|", ani[a]);
    printf("      TOTAL\n  ");
    for (int k = 0; k < 23 + n_ani*12 + 12; k++) printf("-");
    printf("\n");

    for (int s = 0; s < top_sub; s++) {
        int idx = ord[s];
        printf("  %-22s |", sub[idx]);
        for (int a = 0; a < n_ani; a++) {
            double v = 0;
            for (int i = 0; i < nr_tranzactii; i++)
                if (strcmp(date[i].product_subcategory, sub[idx]) == 0 && date[i].an == ani[a])
                    v += date[i].venit;
            if (v > 0) printf(" %10.0f|", v);
            else       printf("          -|");
        }
        printf("  %10.0f\n", tot_sub[idx]);
    }

    // Tendinte: compara primul an cu ultimul an
    if (n_ani >= 2) {
        printf("\n  Tendinte (primul an vs ultimul an in date):\n\n");
        printf("  %-22s | %-12s | %-12s | %-10s | Trend\n",
               "Subcategorie",
               ani[0]        > 0 ? "Primul an" : "An 1",
               ani[n_ani-1]  > 0 ? "Ultimul an": "An N",
               "Variatie %%");
        linie_h(72);

        for (int s = 0; s < top_sub; s++) {
            int idx = ord[s];
            double v_prim = 0, v_ultim = 0;
            for (int i = 0; i < nr_tranzactii; i++) {
                if (strcmp(date[i].product_subcategory, sub[idx]) != 0) continue;
                if (date[i].an == ani[0])        v_prim  += date[i].venit;
                if (date[i].an == ani[n_ani-1])  v_ultim += date[i].venit;
            }
            double delta = (v_prim > 0) ? (v_ultim - v_prim) / v_prim * 100.0 : 0;
            printf("  %-22s | %12.0f | %12.0f | %+9.1f%% | %s\n",
                   sub[idx], v_prim, v_ultim, delta,
                   delta >  5.0 ? "IN CRESTERE ^" :
                   delta < -5.0 ? "IN SCADERE  v" : "STABIL      ~");
        }
        linie_h(72);
    }
}

/* ================================================================
   MENIU PRINCIPAL
   ================================================================ */

static void meniu(void) {
    printf("\n");
    printf("  +----------------------------------------------------------+\n");
    printf("  |        TABLOU DINAMIC - ANALIZA VANZARILOR               |\n");
    printf("  |        Fisier: %-20s                      |\n", FISIER_CSV);
    printf("  |        Tranzactii incarcate: %-6d                      |\n", nr_tranzactii);
    printf("  +----------------------------------------------------------+\n");
    printf("  |  1.  Venituri totale pe luni                             |\n");
    printf("  |  2.  Top %d produse dupa venit                            |\n", TOP_N);
    printf("  |  3.  Distributia vanzarilor pe categorii                 |\n");
    printf("  |  4.  Orase cu cele mai mari vanzari pe tara              |\n");
    printf("  |  5.  Tendinte lunare pe subcategorii                     |\n");
    printf("  |  6.  Afiseaza TOATE analizele                            |\n");
    printf("  |  0.  Iesire                                              |\n");
    printf("  +----------------------------------------------------------+\n");
    printf("  Alegerea ta: ");
}

// MAIN

int main(void) {
    printf("\n");
    printf("  ==========================================================\n");
    printf("   ANALIZA VANZARILOR - se incarca datele...\n");
    printf("  ==========================================================\n");
    printf("  Fisier: %s\n", FISIER_CSV);

    int n = incarca_csv(FISIER_CSV);
    if (n == 0) {
        printf("  Program oprit. Apasa Enter.\n");
        getchar();
        return 1;
    }
    printf("  Tranzactii incarcate cu succes: %d\n\n", n);

    int opt;
    do {
        meniu();
        if (scanf("%d", &opt) != 1) opt = -1;

        switch (opt) {
            case 1: analiza_1_lunara();        break;
            case 2: analiza_2_top_produse();   break;
            case 3: analiza_3_categorii();     break;
            case 4: analiza_4_orase();         break;
            case 5: analiza_5_tendinte();      break;
            case 6:
                analiza_1_lunara();
                analiza_2_top_produse();
                analiza_3_categorii();
                analiza_4_orase();
                analiza_5_tendinte();
                break;
            case 0:
                printf("\n  La revedere!\n\n");
                break;
            default:
                printf("\n  Optiune invalida. Incearca din nou.\n");
        }
    } while (opt != 0);

    if (date) free(date);
    return 0;
}